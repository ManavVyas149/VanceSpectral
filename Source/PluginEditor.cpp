#include "PluginEditor.h"
#include "PluginProcessor.h"

VancespectralAudioProcessorEditor::VancespectralAudioProcessorEditor(VancespectralAudioProcessor &p)
    : AudioProcessorEditor(&p), audioProcessor(p),
      adsrPanel(p.getAPVTS()) {
  
  setLookAndFeel(&spectralLookAndFeel);
  setWantsKeyboardFocus(true);

  spectrogram = std::make_unique<SpectrogramComponent>(audioProcessor);
  presetOverlay = std::make_unique<PresetBrowserOverlay>(presetManager, audioProcessor.getAPVTS());
  presetOverlay->bindSpectrogramComponent(spectrogram.get());

  presetManager.createDefaultFactoryPresets(audioProcessor.getAPVTS());

  addAndMakeVisible(presetBar);
  addAndMakeVisible(toolbar);
  addAndMakeVisible(*spectrogram);
  addAndMakeVisible(playbackControl);
  addAndMakeVisible(pitchControl);
  addAndMakeVisible(adsrPanel);

  volumeSlider.setSliderStyle(juce::Slider::LinearHorizontal);
  volumeSlider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
  addAndMakeVisible(volumeSlider);

  volumeAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
      audioProcessor.getAPVTS(), "GAIN", volumeSlider);

  addAndMakeVisible(*presetOverlay);
  presetOverlay->setVisible(false);

  // Sync segmented controls from APVTS initial values
  if (auto* param = audioProcessor.getAPVTS().getParameter("PLAYBACK_MODE")) {
    int idx = juce::jlimit(0, 4, (int)std::round(param->getValue() * 4.0f));
    playbackControl.setSelectedIndex(idx, false);
  }

  if (auto* param = audioProcessor.getAPVTS().getParameter("PITCH_MODE")) {
    int idx = juce::jlimit(0, 2, (int)std::round(param->getValue() * 2.0f));
    pitchControl.setSelectedIndex(idx, false);
  }

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
    presetOverlay->refreshPresetList();
    presetOverlay->refreshSampleList();
    presetOverlay->setVisible(true);
    presetOverlay->toFront(true);
  };

  auto syncUIFromAPVTS = [this]() {
    if (auto* param = audioProcessor.getAPVTS().getParameter("PLAYBACK_MODE")) {
      int idx = juce::jlimit(0, 4, (int)std::round(param->getValue() * 4.0f));
      playbackControl.setSelectedIndex(idx, false);
    }
    if (auto* param = audioProcessor.getAPVTS().getParameter("PITCH_MODE")) {
      int idx = juce::jlimit(0, 2, (int)std::round(param->getValue() * 2.0f));
      pitchControl.setSelectedIndex(idx, false);
    }
  };

  // Callback when user loads a sample manually (independent of preset browser)
  spectrogram->onManualSampleLoaded = [this, syncUIFromAPVTS]() {
    syncUIFromAPVTS();
    if (spectrogram && spectrogram->isFileLoaded())
      presetBar.setPresetName(spectrogram->getLoadedFile().getFileNameWithoutExtension());
    else
      presetBar.setPresetName("Custom / Unsaved");

    if (presetOverlay)
      presetOverlay->clearActivePresetSelection();
  };

  // Atomic preset load helper
  auto loadPresetAtomic = [this, syncUIFromAPVTS](const juce::File& presetFile) {
    juce::String sampleFileName;
    float startReg = 0.0f;
    float endReg = 1.0f;
    juce::var selectionsVar;

    if (presetManager.loadPreset(presetFile, audioProcessor.getAPVTS(), sampleFileName, startReg, endReg, selectionsVar)) {
      juce::var parsed = juce::JSON::parse(presetFile.loadFileAsString());
      if (parsed.isObject()) {
        presetBar.setPresetName(parsed.getProperty("name", presetFile.getFileNameWithoutExtension()).toString());
      }
      syncUIFromAPVTS();

      if (sampleFileName.isNotEmpty() && spectrogram) {
        auto sampleFile = presetManager.getSamplesFolder().getChildFile(sampleFileName);
        if (sampleFile.existsAsFile()) {
          spectrogram->loadAudioFile(sampleFile, true); // true = isPartOfPresetLoad!
        }
      }

      if (spectrogram) {
        spectrogram->restorePresetSnapshot(startReg, endReg, selectionsVar);
      }
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
    }
  };

  // Fixed APVTS parameter sync for Playback & Pitch mode segmented controls
  playbackControl.onSelectionChanged = [this](int index) {
    if (auto* param = audioProcessor.getAPVTS().getParameter("PLAYBACK_MODE")) {
      float normVal = juce::jlimit(0.0f, 1.0f, (float)index / 4.0f);
      param->setValueNotifyingHost(normVal);
    }
  };

  pitchControl.onSelectionChanged = [this](int index) {
    if (auto* param = audioProcessor.getAPVTS().getParameter("PITCH_MODE")) {
      float normVal = juce::jlimit(0.0f, 1.0f, (float)index / 2.0f);
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

  setSize(1040, 640);
}

VancespectralAudioProcessorEditor::~VancespectralAudioProcessorEditor() {
  setLookAndFeel(nullptr);
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
  int volumeWidth = 200;
  auto volumeArea = footerArea.removeFromRight(volumeWidth);
  volumeSlider.setBounds(volumeArea);

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

  // Lower Area: Segmented Controls + Envelopes
  auto lowerArea = area;

  // Segmented controls column (Playback & Pitch) on left
  auto controlsArea = lowerArea.removeFromLeft(300);
  lowerArea.removeFromLeft(gap);

  int controlHeight = (controlsArea.getHeight() - gap) / 2;
  playbackControl.setBounds(controlsArea.removeFromTop(controlHeight));
  controlsArea.removeFromTop(gap);
  pitchControl.setBounds(controlsArea);

  // Unified Envelope + Exciter section taking ~60% of remaining lower width
  int envWidth = (int)(lowerArea.getWidth() * 0.60f);
  adsrPanel.setBounds(lowerArea.removeFromLeft(envWidth));

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