#include "PluginEditor.h"
#include "PluginProcessor.h"
#include "BinaryFontData.h"

VancespectralAudioProcessorEditor::VancespectralAudioProcessorEditor(VancespectralAudioProcessor &p)
    : AudioProcessorEditor(&p), audioProcessor(p),
      adsrPanel(p.getAPVTS()),
      effectsPanel(p.getAPVTS(), [&p]() { return p.getHostBpm(); }) {
  
  setLookAndFeel(&spectralLookAndFeel);
  setWantsKeyboardFocus(true);

  spectrogram = std::make_unique<SpectrogramComponent>(audioProcessor);
  presetOverlay = std::make_unique<PresetBrowserOverlay>(presetManager, audioProcessor.getAPVTS(), &audioProcessor.getHistoryManager());
  presetOverlay->bindSpectrogramComponent(spectrogram.get());

  presetManager.createDefaultFactoryPresets(audioProcessor.getAPVTS());

  addAndMakeVisible(contentWrapper);

  contentWrapper.addAndMakeVisible(presetBar);
  contentWrapper.addAndMakeVisible(toolbar);
  contentWrapper.addAndMakeVisible(*spectrogram);
  contentWrapper.addAndMakeVisible(playbackControl);
  contentWrapper.addAndMakeVisible(pitchControl);
  contentWrapper.addAndMakeVisible(adsrPanel);
  contentWrapper.addAndMakeVisible(effectsPanel);

  volumeSlider.setSliderStyle(juce::Slider::LinearHorizontal);
  volumeSlider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
  volumeSlider.setName("VOLUME");
  contentWrapper.addAndMakeVisible(volumeSlider);

  contentWrapper.addAndMakeVisible(polyButton);

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

  contentWrapper.addAndMakeVisible(*presetOverlay);
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
    presetOverlay->setVisible(true);
    presetOverlay->toFront(true);
    presetOverlay->checkForExternalLibraryChangesAsync();
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
              presetBar.setBankName("User");
              audioProcessor.setCurrentPresetName(name);
              audioProcessor.setCurrentBankName("User");
              if (presetOverlay)
              {
                presetOverlay->syncActivePresetFromProcessor(name);
                presetOverlay->refreshPresetList();
              }
              double currentSr = audioProcessor.getSampleRate() > 0.0 ? audioProcessor.getSampleRate() : 44100.0;
              audioProcessor.checkpointHistoryState("State Saved: " + name, audioBuf, currentSr);
              updatePresetNavigationButtons();
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

      juce::String bank = presetManager.getBankForPreset(presetFile);
      presetBar.setBankName(bank);
      audioProcessor.setCurrentBankName(bank);

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
      updatePresetNavigationButtons();
    }
  };

  presetBar.onPrevClicked = [this, loadPresetAtomic]() {
    juce::String currentBank = presetBar.getBankName();
    auto list = presetOverlay ? presetOverlay->getNavigablePresetsForBank(currentBank) : juce::Array<PresetInfo>();
    if (list.isEmpty()) {
      updatePresetNavigationButtons();
      return;
    }

    juce::String currentName = presetBar.getCurrentPresetName();
    juce::File currentFile = presetOverlay ? presetOverlay->getActivePresetFile() : juce::File();

    int currentIndex = -1;
    for (int i = 0; i < list.size(); ++i) {
      if ((currentFile.existsAsFile() && list[i].file == currentFile) ||
          list[i].name.equalsIgnoreCase(currentName) ||
          list[i].file.getFileNameWithoutExtension().equalsIgnoreCase(currentName)) {
        currentIndex = i;
        break;
      }
    }

    int targetIndex = -1;
    if (currentIndex < 0) {
      targetIndex = 0;
    } else if (currentIndex > 0) {
      targetIndex = currentIndex - 1;
    }

    if (targetIndex >= 0 && targetIndex < list.size()) {
      loadPresetAtomic(list[targetIndex].file);
    }
    updatePresetNavigationButtons();
  };

  presetBar.onNextClicked = [this, loadPresetAtomic]() {
    juce::String currentBank = presetBar.getBankName();
    auto list = presetOverlay ? presetOverlay->getNavigablePresetsForBank(currentBank) : juce::Array<PresetInfo>();
    if (list.isEmpty()) {
      updatePresetNavigationButtons();
      return;
    }

    juce::String currentName = presetBar.getCurrentPresetName();
    juce::File currentFile = presetOverlay ? presetOverlay->getActivePresetFile() : juce::File();

    int currentIndex = -1;
    for (int i = 0; i < list.size(); ++i) {
      if ((currentFile.existsAsFile() && list[i].file == currentFile) ||
          list[i].name.equalsIgnoreCase(currentName) ||
          list[i].file.getFileNameWithoutExtension().equalsIgnoreCase(currentName)) {
        currentIndex = i;
        break;
      }
    }

    int targetIndex = -1;
    if (currentIndex < 0) {
      targetIndex = 0;
    } else if (currentIndex < list.size() - 1) {
      targetIndex = currentIndex + 1;
    }

    if (targetIndex >= 0 && targetIndex < list.size()) {
      loadPresetAtomic(list[targetIndex].file);
    }
    updatePresetNavigationButtons();
  };

  presetOverlay->onFilterOrSortChanged = [this]() {
    updatePresetNavigationButtons();
  };

  presetOverlay->onPresetSelected = [this, loadPresetAtomic](const juce::File &presetFile, const juce::String &) {
    loadPresetAtomic(presetFile);
  };

  presetOverlay->onBankSelected = [this](const juce::String &/*bankName*/) {
    updatePresetNavigationButtons();
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
    presetBar.setBankName(audioProcessor.getCurrentBankName());
    syncUIFromAPVTS();
    toolbar.setEnabled(spectrogram && spectrogram->isFileLoaded());
  }

  updatePresetNavigationButtons();

  startTimer(2000);

  setResizable(true, true);
  if (auto* c = getConstrainer())
  {
      c->setFixedAspectRatio((double)nativeWidth / (double)nativeHeight);
      c->setSizeLimits(816, 408, 1904, 952);
  }
  setResizeLimits(816, 408, 1904, 952);

  float savedScale = audioProcessor.getEditorScale();
  if (savedScale < 0.75f || savedScale > 1.75f)
      savedScale = 1.0f;

  setSize((int)std::round((float)nativeWidth * savedScale), (int)std::round((float)nativeHeight * savedScale));
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

void VancespectralAudioProcessorEditor::ContentWrapper::paint(juce::Graphics &g) {
  g.setImageResamplingQuality(juce::Graphics::highResamplingQuality);
  auto bounds = getLocalBounds().toFloat();

  // 1. Draw outer frosted-white chassis, PCB trace motif, corner screws, and 1px border
  SpectralUILookAndFeel::drawChassisBackground(g, bounds);

  // 2. Footer strip at bottom (26px height)
  auto footerArea = bounds.removeFromBottom(26.0f).reduced(16.0f, 0.0f);
  g.setColour(SpectralUILookAndFeel::dividerColour);
  g.drawHorizontalLine((int)footerArea.getY(), 16.0f, (float)getWidth() - 16.0f);

  // Branding text on left with authentic Vance logo
  float logoH = 14.0f;
  float logoW = logoH * (793.0f / 1024.0f); // 0.7744 aspect ratio
  float logoX = footerArea.getX() + 2.0f;
  float logoY = footerArea.getCentreY() - logoH * 0.5f;
  juce::Rectangle<float> logoRect(logoX, logoY, logoW, logoH);
  juce::MemoryInputStream stream(BinaryData::VanceLogo_png, (size_t)BinaryData::VanceLogo_pngSize, false);
  auto footerLogo = juce::PNGImageFormat().decodeImage(stream);
  if (footerLogo.isValid())
  {
      g.drawImageWithin(footerLogo, (int)logoRect.getX(), (int)logoRect.getY(), (int)logoRect.getWidth(), (int)logoRect.getHeight(),
                        juce::RectanglePlacement::centred | juce::RectanglePlacement::onlyReduceInSize, false);
  }
  else
  {
      g.setColour(SpectralUILookAndFeel::accentColour);
      g.fillRoundedRectangle(logoRect, 2.0f);
  }

  // Left branding text (strictly bounded so it never collides with center badge)
  g.setFont(SpectralUILookAndFeel::getJetBrainsMono(9.0f));
  g.setColour(SpectralUILookAndFeel::textMutedColour);
  int maxBrandingWidth = (int)(bounds.getWidth() * 0.5f - logoRect.getRight() - 75.0f);
  juce::Rectangle<int> brandingRect((int)logoRect.getRight() + 6, (int)footerArea.getY(),
                                    juce::jmax(50, maxBrandingWidth), (int)footerArea.getHeight());
  g.drawText("VANCESPECTRAL // PRECISION BRUTALIST INSTRUMENT // MODEL-01",
             brandingRect, juce::Justification::centredLeft, true);

  // Base Octave Readout Badge (dynamically centered in bottom status bar)
  int octaveNum = 3 + (editor.currentOctaveOffset / 12);
  juce::String octaveText = "OCTAVE: C" + juce::String(octaveNum) + " [Z/X]";
  float badgeW = 124.0f;
  float badgeH = 18.0f;
  float badgeX = (bounds.getWidth() - badgeW) * 0.5f;
  float badgeY = footerArea.getY() + (footerArea.getHeight() - badgeH) * 0.5f;
  auto octaveRect = juce::Rectangle<float>(badgeX, badgeY, badgeW, badgeH);

  g.setColour(SpectralUILookAndFeel::panelBgColour);
  g.fillRoundedRectangle(octaveRect, 3.0f);
  g.setColour(SpectralUILookAndFeel::dividerColour);
  g.drawRoundedRectangle(octaveRect, 3.0f, 1.0f);
  g.setColour(SpectralUILookAndFeel::textMainColour);
  g.setFont(SpectralUILookAndFeel::getJetBrainsMono(9.0f, true));
  g.drawText(octaveText, octaveRect.toNearestInt(), juce::Justification::centred, false);
}

void VancespectralAudioProcessorEditor::ContentWrapper::resized() {
  constexpr int margin = 12;
  constexpr int gap = 10;

  auto totalBounds = getLocalBounds();

  // Bottom Status Bar (26px height, 16px horizontal inset)
  auto footerArea = totalBounds.removeFromBottom(26).reduced(16, 0);

  // Full-size master volume horizontal fader on far right of bottom footer bar (extended to 240px width)
  int volumeWidth = 240;
  int volH = 18;
  auto volumeArea = footerArea.removeFromRight(volumeWidth);
  int volY = footerArea.getY() + (footerArea.getHeight() - volH) / 2;
  editor.volumeSlider.setBounds(volumeArea.getX(), volY, volumeArea.getWidth(), volH);

  // Main work area bounded above the footer bar
  auto area = totalBounds.reduced(margin);

  // Top Bar (Preset Navigation & Management) - 32px height
  auto topBarArea = area.removeFromTop(32);
  editor.presetBar.setBounds(topBarArea);

  // Relocate MONO/POLY toggle button to top navigation bar immediately to the left of the preset group
  auto polyArea = editor.presetBar.getPolyButtonArea().translated(editor.presetBar.getX(), editor.presetBar.getY());
  editor.polyButton.setBounds(polyArea);
  editor.polyButton.toFront(false);

  area.removeFromTop(gap);

  // Split remaining vertical height between Upper Main Surface and Lower Area
  int totalMainHeight = area.getHeight() - gap;
  int upperHeight = (int)(totalMainHeight * 0.55f);
  auto upperArea = area.removeFromTop(upperHeight);

  area.removeFromTop(gap);
  auto lowerArea = area;

  // Effects Panel on right side of lower row (330px width)
  int effectsWidth = 330;
  auto effectsArea = lowerArea.removeFromRight(effectsWidth);
  lowerArea.removeFromRight(gap);
  editor.effectsPanel.setBounds(effectsArea);

  // AMP ENV + PITCH & DRIFT + EXCITER & GLIDE takes full remaining lower row width
  editor.adsrPanel.setBounds(lowerArea);

  // Upper Area:
  // 1. Left Toolbox (36px width)
  auto toolboxArea = upperArea.removeFromLeft(36);
  editor.toolbar.setBounds(toolboxArea);
  upperArea.removeFromLeft(gap);

  // 2. Relocated PLAYBACK & PITCH panel on right side of upper row
  // Sits flush beside screen-graph, matching effectsPanel width and aligning directly above it
  auto upperControlsArea = upperArea.removeFromRight(effectsWidth);
  upperArea.removeFromRight(gap);

  // 3. Screen-Graph (Spectrogram): takes the remaining upperArea
  // Its right edge aligns exactly with adsrPanel's right edge (EXCITER & GLIDE)
  if (editor.spectrogram)
    editor.spectrogram->setBounds(upperArea);

  // PLAYBACK & PITCH stacked vertically inside upperControlsArea, matching screen-graph's height
  int playbackHeight = (int)(upperControlsArea.getHeight() * 0.64f);
  editor.playbackControl.setBounds(upperControlsArea.removeFromTop(playbackHeight));
  upperControlsArea.removeFromTop(gap);
  editor.pitchControl.setBounds(upperControlsArea);

  if (editor.presetOverlay)
    editor.presetOverlay->setBounds(getLocalBounds());
}

void VancespectralAudioProcessorEditor::paint(juce::Graphics &g) {
  g.fillAll(SpectralUILookAndFeel::bgColour);
}

void VancespectralAudioProcessorEditor::resized() {
  juce::AudioProcessorEditor::resized(); // Positions the corner resizer component if present

  float scale = (float)getWidth() / (float)nativeWidth;
  audioProcessor.setEditorScale(scale);

  contentWrapper.setBounds(0, 0, nativeWidth, nativeHeight);
  contentWrapper.setTransform(juce::AffineTransform::scale(scale));

  for (auto* child : getChildren())
  {
      if (dynamic_cast<juce::ResizableCornerComponent*>(child) != nullptr)
      {
          child->toFront(false);
          break;
      }
  }
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
  if (presetOverlay && presetOverlay->isVisible()) {
    if (presetOverlay->keyPressed(key))
      return true;
  }

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
    contentWrapper.repaint();
    return true;
  }
  if (c == 'x') {
    currentOctaveOffset = juce::jmin(36, currentOctaveOffset + 12);
    contentWrapper.repaint();
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

void VancespectralAudioProcessorEditor::updatePresetNavigationButtons() {
  juce::String currentBank = presetBar.getBankName();
  auto list = presetOverlay ? presetOverlay->getNavigablePresetsForBank(currentBank) : juce::Array<PresetInfo>();

  if (list.isEmpty()) {
    presetBar.setPrevEnabled(false);
    presetBar.setNextEnabled(false);
    return;
  }

  juce::String currentName = presetBar.getCurrentPresetName();
  juce::File currentFile = presetOverlay ? presetOverlay->getActivePresetFile() : juce::File();

  int currentIndex = -1;
  for (int i = 0; i < list.size(); ++i) {
    if ((currentFile.existsAsFile() && list[i].file == currentFile) ||
        list[i].name.equalsIgnoreCase(currentName) ||
        list[i].file.getFileNameWithoutExtension().equalsIgnoreCase(currentName)) {
      currentIndex = i;
      break;
    }
  }

  if (currentIndex < 0) {
    // Current preset not in the active filtered bank list
    presetBar.setPrevEnabled(true);
    presetBar.setNextEnabled(true);
  } else {
    presetBar.setPrevEnabled(currentIndex > 0);
    presetBar.setNextEnabled(currentIndex < list.size() - 1);
  }
}