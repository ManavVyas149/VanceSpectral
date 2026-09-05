#include "PluginEditor.h"
#include "PluginProcessor.h"

VancespectralAudioProcessorEditor::VancespectralAudioProcessorEditor(VancespectralAudioProcessor &p)
    : AudioProcessorEditor(&p), audioProcessor(p),
      adsrPanel(p.getAPVTS()),
      effectsPanel(p.getAPVTS()) {
  
  setLookAndFeel(&spectralLookAndFeel);
  setWantsKeyboardFocus(true);

  spectrogram = std::make_unique<SpectrogramComponent>(audioProcessor);
  presetOverlay = std::make_unique<PresetBrowserOverlay>(presetManager, audioProcessor.getAPVTS(), &audioProcessor.getHistoryManager());
  presetOverlay->bindSpectrogramComponent(spectrogram.get());

  presetManager.createDefaultFactoryPresets(audioProcessor.getAPVTS());

  addAndMakeVisible(presetBar);
  addAndMakeVisible(toolbar);
  addAndMakeVisible(*spectrogram);
  addAndMakeVisible(playbackControl);
  addAndMakeVisible(pitchControl);
  addAndMakeVisible(adsrPanel);
  addAndMakeVisible(effectsPanel);

  volumeSlider.setSliderStyle(juce::Slider::LinearHorizontal);
  volumeSlider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
  addAndMakeVisible(volumeSlider);

  addAndMakeVisible(polyButton);

  polyButton.onClick = [this]() {
    if (auto* param = audioProcessor.getAPVTS().getParameter("POLY_MODE")) {
      bool next = !polyButton.getToggleState();
      polyButton.setToggleState(next, juce::dontSendNotification);
      param->setValueNotifyingHost(next ? 1.0f : 0.0f);
      adsrPanel.updatePolyMode(next);
    }
  };

  volumeAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
      audioProcessor.getAPVTS(), "GAIN", volumeSlider);

  addAndMakeVisible(*presetOverlay);
  presetOverlay->setVisible(false);

  auto syncUIFromAPVTS = [this]() {
    int pIdx = 0;
    bool isPoly = false;
    if (auto* param = audioProcessor.getAPVTS().getParameter("PLAYBACK_MODE")) {
      int idx = juce::jlimit(0, 4, (int)std::round(param->getValue() * 4.0f));
      playbackControl.setSelectedIndex(idx, false);
    }
    if (auto* param = audioProcessor.getAPVTS().getParameter("PITCH_MODE")) {
      pIdx = juce::jlimit(0, 1, (int)std::round(param->getValue() * 1.0f));
      pitchControl.setSelectedIndex(pIdx, false);
    }
    if (auto* param = audioProcessor.getAPVTS().getParameter("POLY_MODE")) {
      isPoly = (param->getValue() >= 0.5f);
      polyButton.setToggleState(isPoly, juce::dontSendNotification);
    }
    adsrPanel.updatePolyMode(isPoly);
  };

  syncUIFromAPVTS();

  // Sync toolbar enable state
  toolbar.setEnabled(spectrogram && spectrogram->isFileLoaded());

  spectrogram->onFileLoadedStateChanged = [this](bool loaded) {
    toolbar.setEnabled(loaded);
  };

  // Wire toolbar selection callback to spectrogram
  toolbar.onToolSelected = [this](ToolType selectedTool) {
    if (spectrogram)
      spectrogram->setActiveTool(selectedTool);
  };

  // Wire preset bar callbacks
  presetBar.onBrowseClicked = [this]() {
    presetOverlay->syncActivePresetFromProcessor(audioProcessor.getCurrentPresetName());
    presetOverlay->refreshBankList();
    presetOverlay->refreshPresetList();
    presetOverlay->refreshSampleList();
    presetOverlay->refreshHistoryList();
    presetOverlay->setVisible(true);
    presetOverlay->toFront(true);
  };

  presetBar.onShuffleFxClicked = [this]() {
    if (presetOverlay)
      presetOverlay->executeShuffleFx();
  };

  presetBar.onSaveStateClicked = [this]() {
    juce::String defaultName = "State " + juce::Time::getCurrentTime().formatted("%Y-%m-%d %H-%M");
    if (spectrogram && spectrogram->isFileLoaded())
      defaultName = spectrogram->getLoadedFile().getFileNameWithoutExtension() + " State";

    auto* dialog = new juce::AlertWindow("SAVE FULL STATE", "Enter a name for this full session snapshot (sample + settings):", juce::AlertWindow::NoIcon);
    dialog->addTextEditor("stateName", defaultName, "State Name");
    dialog->addButton("Save State", 1, juce::KeyPress(juce::KeyPress::returnKey));
    dialog->addButton("Cancel", 0, juce::KeyPress(juce::KeyPress::escapeKey));

    dialog->enterModalState(true, juce::ModalCallbackFunction::create([this, dialog](int button) {
      if (button == 1)
      {
        juce::String name = dialog->getTextEditorContents("stateName").trim();
        if (name.isNotEmpty())
        {
          auto executeSaveState = [this, name]() {
            float startReg = spectrogram ? spectrogram->getStartRegion() : 0.0f;
            float endReg = spectrogram ? spectrogram->getEndRegion() : 1.0f;
            juce::var selectionsVar = spectrogram ? spectrogram->getSelectionsAsVar() : juce::var();
            bool loopEnabled = spectrogram ? spectrogram->isLoopEnabled() : false;
            const juce::AudioBuffer<float>* audioBuf = spectrogram ? &spectrogram->getAudioBuffer() : nullptr;
            juce::String sampleFileName = spectrogram ? spectrogram->getLoadedFile().getFileName() : "";

            if (presetManager.savePreset(name, "STATES", "User", sampleFileName, audioProcessor.getAPVTS(), startReg, endReg, selectionsVar, false, loopEnabled, audioBuf, 44100.0))
            {
              presetBar.setPresetName(name);
              audioProcessor.setCurrentPresetName(name);
              if (presetOverlay)
              {
                presetOverlay->syncActivePresetFromProcessor(name);
                presetOverlay->refreshPresetList();
              }
              double currentSr = audioProcessor.getSampleRate() > 0.0 ? audioProcessor.getSampleRate() : 44100.0;
              audioProcessor.checkpointHistoryState("State Saved: " + name, audioBuf, currentSr);
            }
            else
            {
              auto* errDialog = new juce::AlertWindow("SAVE STATE FAILED", "Failed to write state file '" + name + ".vsts' to disk in User bank. Please check folder permissions.", juce::AlertWindow::WarningIcon);
              errDialog->addButton("OK", 1, juce::KeyPress(juce::KeyPress::returnKey));
              errDialog->enterModalState(true, nullptr, true);
            }
          };

          juce::String cleanName = juce::File::createLegalFileName(name);
          juce::File targetFile = presetManager.getPresetsFolder().getChildFile("User").getChildFile(cleanName + ".vsts");

          if (targetFile.existsAsFile())
          {
            auto* confirmDialog = new juce::AlertWindow("STATE ALREADY EXISTS", "A full state named '" + name + "' already exists in the User bank. Overwrite existing file on disk?", juce::AlertWindow::QuestionIcon);
            confirmDialog->addButton("Overwrite", 1, juce::KeyPress(juce::KeyPress::returnKey));
            confirmDialog->addButton("Cancel", 0, juce::KeyPress(juce::KeyPress::escapeKey));

            confirmDialog->enterModalState(true, juce::ModalCallbackFunction::create([executeSaveState](int buttonChoice) {
              if (buttonChoice == 1)
              {
                executeSaveState();
              }
            }), true);
          }
          else
          {
            executeSaveState();
          }
        }
      }
    }), true);
  };

  // Callback when user loads a sample manually (independent of preset browser)
  spectrogram->onManualSampleLoaded = [this, syncUIFromAPVTS]() {
    syncUIFromAPVTS();
    juce::String sampleName = "Custom / Unsaved";
    if (spectrogram && spectrogram->isFileLoaded())
      sampleName = spectrogram->getLoadedFile().getFileNameWithoutExtension();
    presetBar.setPresetName(sampleName);
    audioProcessor.setCurrentPresetName(sampleName);

    if (presetOverlay)
      presetOverlay->clearActivePresetSelection();

    const juce::AudioBuffer<float>* audioBuf = spectrogram ? &spectrogram->getAudioBuffer() : nullptr;
    double currentSr = audioProcessor.getSampleRate() > 0.0 ? audioProcessor.getSampleRate() : 44100.0;
    audioProcessor.checkpointHistoryState("Sample Loaded: " + sampleName, audioBuf, currentSr);
  };

  // Atomic preset load helper
  auto loadPresetAtomic = [this, syncUIFromAPVTS](const juce::File& presetFile) {
    juce::String sampleFileName;
    float startReg = 0.0f;
    float endReg = 1.0f;
    juce::var selectionsVar;
    bool loopEnabled = false;
    juce::AudioBuffer<float> loadedBuf;
    double loadedSr = 44100.0;

    if (presetManager.loadPreset(presetFile, audioProcessor.getAPVTS(), sampleFileName, startReg, endReg, selectionsVar, loopEnabled, &loadedBuf, &loadedSr)) {
      syncUIFromAPVTS();
      juce::String pName = presetFile.getFileNameWithoutExtension();
      presetBar.setPresetName(pName);
      audioProcessor.setCurrentPresetName(pName);

      if (presetOverlay)
        presetOverlay->syncActivePresetFromProcessor(pName);

      // If preset has sample audio buffer embedded, reload it into Spectrogram & DSP engine!
      if (loadedBuf.getNumSamples() > 0 && spectrogram) {
        spectrogram->loadDirectAudioBuffer(loadedBuf, loadedSr, sampleFileName, loopEnabled);
      } else if (sampleFileName.isNotEmpty() && spectrogram) {
        juce::File sampleFile = presetManager.getSamplesFolder().getChildFile(sampleFileName);
        if (sampleFile.existsAsFile())
          spectrogram->loadAudioFile(sampleFile, true); // true = isPartOfPresetLoad!
      }
      // Note: For settings-only presets (FX category), loadedBuf is empty, so we do NOT reload sample audio!

      if (spectrogram) {
        spectrogram->setLoopEnabled(loopEnabled);
        spectrogram->restorePresetSnapshot(startReg, endReg, selectionsVar);
      }

      const juce::AudioBuffer<float>* audioBuf = spectrogram ? &spectrogram->getAudioBuffer() : nullptr;
      double currentSr = audioProcessor.getSampleRate() > 0.0 ? audioProcessor.getSampleRate() : 44100.0;
      audioProcessor.checkpointHistoryState("Preset Loaded: " + pName, audioBuf, currentSr);
    }
  };

  presetBar.onPrevClicked = [this, loadPresetAtomic]() {
    auto allPresets = presetManager.getAllPresets();
    if (!allPresets.isEmpty()) {
      juce::String current = presetBar.getCurrentPresetName();
      int index = 0;
      for (int i = 0; i < allPresets.size(); ++i) {
        if (allPresets[i].name.equalsIgnoreCase(current)) {
          index = (i - 1 + allPresets.size()) % allPresets.size();
          break;
        }
      }
      loadPresetAtomic(allPresets[index].file);
    }
  };

  presetBar.onNextClicked = [this, loadPresetAtomic]() {
    auto allPresets = presetManager.getAllPresets();
    if (!allPresets.isEmpty()) {
      juce::String current = presetBar.getCurrentPresetName();
      int index = 0;
      for (int i = 0; i < allPresets.size(); ++i) {
        if (allPresets[i].name.equalsIgnoreCase(current)) {
          index = (i + 1) % allPresets.size();
          break;
        }
      }
      loadPresetAtomic(allPresets[index].file);
    }
  };

  presetOverlay->onPresetSelected = [this, loadPresetAtomic](const juce::File &presetFile, const juce::String &) {
    loadPresetAtomic(presetFile);
  };

  presetOverlay->onSampleSelected = [this](const juce::File &sampleFile) {
    if (sampleFile.existsAsFile() && spectrogram) {
      spectrogram->loadAudioFile(sampleFile, false); // false = manual sample load resets settings
      const juce::AudioBuffer<float>* audioBuf = spectrogram ? &spectrogram->getAudioBuffer() : nullptr;
      double currentSr = audioProcessor.getSampleRate() > 0.0 ? audioProcessor.getSampleRate() : 44100.0;
      audioProcessor.checkpointHistoryState("Sample Loaded: " + sampleFile.getFileNameWithoutExtension(), audioBuf, currentSr);
    }
  };

  presetOverlay->onHistoryEntryRestored = [this, syncUIFromAPVTS](const HistoryEntry& entry) {
    syncUIFromAPVTS();
    presetBar.setPresetName("History: " + entry.label);
  };

  // Fixed APVTS parameter sync for Playback & Pitch mode segmented controls
  playbackControl.onSelectionChanged = [this](int index) {
    if (auto* param = audioProcessor.getAPVTS().getParameter("PLAYBACK_MODE")) {
      float normVal = juce::jlimit(0.0f, 1.0f, (float)index / 4.0f);
      param->setValueNotifyingHost(normVal);
    }
    if (index == 4) {
      triggerRandomConfigurationReroll();
    }
  };

  playbackControl.onOptionClicked = [this](int index, bool isReclick) {
    if (index == 4 && isReclick) {
      triggerRandomConfigurationReroll();
    }
  };

  pitchControl.onSelectionChanged = [this](int index) {
    juce::ignoreUnused(index);
    if (auto* param = audioProcessor.getAPVTS().getParameter("PITCH_MODE")) {
      float normVal = juce::jlimit(0.0f, 1.0f, (float)index / 1.0f);
      param->setValueNotifyingHost(normVal);
    }
  };

  // Auto-load initial default factory preset on initial startup; otherwise restore UI display state from processor
  if (!audioProcessor.isPluginInitialized()) {
    auto allPresets = presetManager.getAllPresets();
    if (!allPresets.isEmpty()) {
      loadPresetAtomic(allPresets[0].file);
    }
    audioProcessor.setPluginInitialized(true);
  } else {
    if (spectrogram) {
      spectrogram->restoreFromProcessorState();
    }
    presetBar.setPresetName(audioProcessor.getCurrentPresetName());
    syncUIFromAPVTS();
    toolbar.setEnabled(spectrogram && spectrogram->isFileLoaded());
  }

  startTimer(2000);
  setSize(1280, 640);
}

VancespectralAudioProcessorEditor::~VancespectralAudioProcessorEditor() {
  stopTimer();
  setLookAndFeel(nullptr);
}

void VancespectralAudioProcessorEditor::timerCallback() {
  juce::int64 now = juce::Time::currentTimeMillis();
  if (lastAutoCheckpointTimeMs == 0)
    lastAutoCheckpointTimeMs = now;

  if (now - lastAutoCheckpointTimeMs > 120000) { // Every 2 minutes
    if (spectrogram && spectrogram->isFileLoaded()) {
      const juce::AudioBuffer<float>* audioBuf = &spectrogram->getAudioBuffer();
      double currentSr = audioProcessor.getSampleRate() > 0.0 ? audioProcessor.getSampleRate() : 44100.0;
      if (audioProcessor.checkpointHistoryState("Auto Snapshot", audioBuf, currentSr)) {
        lastAutoCheckpointTimeMs = now;
        if (presetOverlay)
          presetOverlay->refreshHistoryList();
      }
    }
  }
}

void VancespectralAudioProcessorEditor::paint(juce::Graphics &g) {
  auto bounds = getLocalBounds().toFloat();

  // Background: Warm off-white / bone (#F0ECE1)
  g.fillAll(SpectralUILookAndFeel::bgColour);

  // Footer strip at bottom
  auto footerArea = bounds.removeFromBottom(24.0f);
  g.setColour(SpectralUILookAndFeel::dividerColour);
  g.drawHorizontalLine((int)footerArea.getY(), 0.0f, (float)getWidth());

  // Restrained branding: small logo mark + wordmark
  g.setFont(SpectralUILookAndFeel::getGeometricFont(10.0f, true));
  g.setColour(SpectralUILookAndFeel::textMutedColour);

  // Logo mark (small thin square + dot)
  float logoX = footerArea.getX() + 16.0f;
  float logoY = footerArea.getCentreY() - 4.0f;
  g.drawRect(logoX, logoY, 8.0f, 8.0f, 1.0f);
  g.fillEllipse(logoX + 2.5f, logoY + 2.5f, 3.0f, 3.0f);

  g.drawText("VANCE SPECTRAL - SPECTRAL FREQUENCY SAMPLER",
             footerArea.reduced(32.0f, 0.0f).toNearestInt(), juce::Justification::left, true);

  // Draw Base Octave Readout Badge in bottom status bar (matching FL Studio / Ableton)
  int octaveNum = 3 + (currentOctaveOffset / 12);
  juce::String octaveText = "OCTAVE: C" + juce::String(octaveNum) + " [Z/X]";
  auto octaveRect = footerArea.toNearestInt().withTrimmedLeft(410).withWidth(110).reduced(0, 3);
  g.setColour(juce::Colour(0x22, 0x22, 0x2E));
  g.fillRoundedRectangle(octaveRect.toFloat(), 3.0f);
  g.setColour(SpectralUILookAndFeel::accentColour);
  g.setFont(SpectralUILookAndFeel::getMonospaceFont(9.5f));
  g.drawText(octaveText, octaveRect, juce::Justification::centred, false);
}

void VancespectralAudioProcessorEditor::resized() {
  constexpr int margin = 16;
  constexpr int gap = 12;

  auto area = getLocalBounds().reduced(margin);
  auto footerArea = area.removeFromBottom(24); // Reserve for footer strip

  // Compact master volume slider on far right of bottom footer bar (opposite branding text)
  int volumeWidth = 180;
  auto volumeArea = footerArea.removeFromRight(volumeWidth);
  volumeSlider.setBounds(volumeArea);

  footerArea.removeFromRight(8);

  auto polyArea = footerArea.removeFromRight(64).reduced(0, 2);
  polyButton.setBounds(polyArea);

  // Top Bar (Preset Browser) - 36px height
  auto topBarArea = area.removeFromTop(36);
  presetBar.setBounds(topBarArea);

  area.removeFromTop(gap);

  // Upper main surface: ~58% of remaining height
  int upperHeight = (int)(area.getHeight() * 0.58f);
  auto upperArea = area.removeFromTop(upperHeight);

  area.removeFromTop(gap);

  // Left Toolbox (40px width)
  auto toolboxArea = upperArea.removeFromLeft(40);
  toolbar.setBounds(toolboxArea);

  upperArea.removeFromLeft(gap);

  // Spectrogram Graph takes remaining upper area
  if (spectrogram)
    spectrogram->setBounds(upperArea);

  // Lower Area: Segmented Controls + ADSR Panel + Effects Panel
  auto lowerArea = area;

  // Segmented controls column (Playback & Pitch) on left (300px width)
  auto controlsArea = lowerArea.removeFromLeft(300);
  lowerArea.removeFromLeft(gap);

  int controlHeight = (controlsArea.getHeight() - gap) / 2;
  playbackControl.setBounds(controlsArea.removeFromTop(controlHeight));
  controlsArea.removeFromTop(gap);
  pitchControl.setBounds(controlsArea);

  // Effects Panel positioned to the right of AMP ENV / PITCH / EXCITER
  int effectsWidth = juce::jmin(460, (int)(lowerArea.getWidth() * 0.46f));
  auto effectsArea = lowerArea.removeFromRight(effectsWidth);
  lowerArea.removeFromRight(gap);

  effectsPanel.setBounds(effectsArea);

  // Unified Envelope + Performance section taking remaining lower width
  adsrPanel.setBounds(lowerArea);

  if (presetOverlay)
    presetOverlay->setBounds(getLocalBounds());
}

int VancespectralAudioProcessorEditor::getQwertySemitone(juce::juce_wchar c) {
  c = juce::CharacterFunctions::toLowerCase(c);
  switch (c) {
    // Lower Octave Row (Bottom letter row)
    case 'a': return 0;   // C
    case 'w': return 1;   // C#
    case 's': return 2;   // D
    case 'e': return 3;   // D#
    case 'd': return 4;   // E
    case 'f': return 5;   // F
    case 't': return 6;   // F#
    case 'g': return 7;   // G
    case 'y': return 8;   // G#
    case 'h': return 9;   // A
    case 'u': return 10;  // A#
    case 'j': return 11;  // B

    // Upper Octave Row (Continues seamlessly into next row)
    case 'k': return 12;  // C (next octave up)
    case 'o': return 13;  // C#
    case 'l': return 14;  // D
    case 'p': return 15;  // D#
    case ';': return 16;  // E
    case '\'': return 17; // F
    default: return -1;
  }
}

bool VancespectralAudioProcessorEditor::keyPressed(const juce::KeyPress &key) {
  if (!hasKeyboardFocus(true))
    return false;

  if (key == juce::KeyPress::spaceKey) {
    if (audioProcessor.isPlaying())
      audioProcessor.stopSample();
    else
      audioProcessor.playSample();
    return true;
  }

  auto c = juce::CharacterFunctions::toLowerCase(key.getTextCharacter());

  if (c == 'z') {
    currentOctaveOffset = juce::jmax(-36, currentOctaveOffset - 12);
    repaint();
    return true;
  }
  if (c == 'x') {
    currentOctaveOffset = juce::jmin(36, currentOctaveOffset + 12);
    repaint();
    return true;
  }

  int semitones = getQwertySemitone(c);
  if (semitones >= 0) {
    int note = juce::jlimit(0, 127, 60 + currentOctaveOffset + semitones);
    int code = key.getKeyCode();
    if (activeQwertyNoteKeys.find(code) == activeQwertyNoteKeys.end()) {
      activeQwertyNoteKeys[code] = note;
      audioProcessor.getSampleEngine().noteOn(note);
    }
    return true;
  }

  return juce::AudioProcessorEditor::keyPressed(key);
}

bool VancespectralAudioProcessorEditor::keyStateChanged(bool isKeyDown) {
  if (!isKeyDown) {
    std::vector<int> releasedKeys;
    for (const auto& [code, note] : activeQwertyNoteKeys) {
      if (!juce::KeyPress::isKeyCurrentlyDown(code)) {
        releasedKeys.push_back(code);
      }
    }
    for (int code : releasedKeys) {
      auto it = activeQwertyNoteKeys.find(code);
      if (it != activeQwertyNoteKeys.end()) {
        audioProcessor.getSampleEngine().noteOff(it->second);
        activeQwertyNoteKeys.erase(it);
      }
    }
  }
  return false;
}

void VancespectralAudioProcessorEditor::triggerRandomConfigurationReroll() {
  audioProcessor.rerollRandomDirection();

  if (auto* exciterParam = audioProcessor.getAPVTS().getParameter("EXCITER")) {
    float randExciter = 0.15f + juce::Random::getSystemRandom().nextFloat() * 0.70f;
    exciterParam->setValueNotifyingHost(randExciter);
  }

  if (spectrogram) {
    spectrogram->generateRandomSelections();
  }
}