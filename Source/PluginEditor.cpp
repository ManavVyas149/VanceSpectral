#include "PluginEditor.h"
#include "PluginProcessor.h"

VancespectralAudioProcessorEditor::VancespectralAudioProcessorEditor(VancespectralAudioProcessor &p)
    : AudioProcessorEditor(&p), audioProcessor(p),
      ampADSRPanel(p.getAPVTS(), "AMP ENV", "AMP_"),
      filterADSRPanel(p.getAPVTS(), "FILTER ENV", "FILTER_") {
  
  setLookAndFeel(&spectralLookAndFeel);
  setWantsKeyboardFocus(true);

  spectrogram = std::make_unique<SpectrogramComponent>(audioProcessor);
  presetOverlay = std::make_unique<PresetBrowserOverlay>(presetManager, audioProcessor.getAPVTS());

  presetManager.createDefaultFactoryPresets(audioProcessor.getAPVTS());

  addAndMakeVisible(presetBar);
  addAndMakeVisible(toolbar);
  addAndMakeVisible(*spectrogram);
  addAndMakeVisible(playbackControl);
  addAndMakeVisible(pitchControl);
  addAndMakeVisible(ampADSRPanel);
  addAndMakeVisible(filterADSRPanel);

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

  presetBar.onPrevClicked = [this]() {
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
      auto selected = allPresets[index];
      presetBar.setPresetName(selected.name);
      juce::String sampleFile;
      presetManager.loadPreset(selected.file, audioProcessor.getAPVTS(), sampleFile);
      if (sampleFile.isNotEmpty() && spectrogram) {
        auto file = presetManager.getSamplesFolder().getChildFile(sampleFile);
        if (file.existsAsFile())
          spectrogram->loadAudioFile(file);
      }
    }
  };

  presetBar.onNextClicked = [this]() {
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
      auto selected = allPresets[index];
      presetBar.setPresetName(selected.name);
      juce::String sampleFile;
      presetManager.loadPreset(selected.file, audioProcessor.getAPVTS(), sampleFile);
      if (sampleFile.isNotEmpty() && spectrogram) {
        auto file = presetManager.getSamplesFolder().getChildFile(sampleFile);
        if (file.existsAsFile())
          spectrogram->loadAudioFile(file);
      }
    }
  };

  presetOverlay->onPresetSelected = [this](const juce::File &presetFile, const juce::String &sampleFileName) {
    juce::var parsed = juce::JSON::parse(presetFile.loadFileAsString());
    if (parsed.isObject()) {
      presetBar.setPresetName(parsed.getProperty("name", presetFile.getFileNameWithoutExtension()).toString());
    }
    if (sampleFileName.isNotEmpty()) {
      auto sampleFile = presetManager.getSamplesFolder().getChildFile(sampleFileName);
      if (sampleFile.existsAsFile() && spectrogram) {
        spectrogram->loadAudioFile(sampleFile);
      }
    }
  };

  presetOverlay->onSampleSelected = [this](const juce::File &sampleFile) {
    if (sampleFile.existsAsFile() && spectrogram) {
      spectrogram->loadAudioFile(sampleFile);
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

  // Auto-load initial default factory preset on startup
  auto allPresets = presetManager.getAllPresets();
  if (!allPresets.isEmpty()) {
    auto defaultPreset = allPresets[0];
    presetBar.setPresetName(defaultPreset.name);
    juce::String sampleFile;
    presetManager.loadPreset(defaultPreset.file, audioProcessor.getAPVTS(), sampleFile);
    if (sampleFile.isNotEmpty() && spectrogram) {
      auto file = presetManager.getSamplesFolder().getChildFile(sampleFile);
      if (file.existsAsFile())
        spectrogram->loadAudioFile(file);
    }
  }

  setSize(1040, 640);
}

VancespectralAudioProcessorEditor::~VancespectralAudioProcessorEditor() {
  setLookAndFeel(nullptr);
}

bool VancespectralAudioProcessorEditor::keyPressed(const juce::KeyPress &key) {
  if (key == juce::KeyPress::spaceKey) {
    if (audioProcessor.isPlaying())
      audioProcessor.stopSample();
    else
      audioProcessor.playSample();
    return true;
  }
  return AudioProcessorEditor::keyPressed(key);
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
}

void VancespectralAudioProcessorEditor::resized() {
  constexpr int margin = 16;
  constexpr int gap = 12;

  auto area = getLocalBounds().reduced(margin);
  area.removeFromBottom(24); // Reserve for footer strip

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

  // Envelope section (Amp Env + Filter Env) taking remaining lower width
  int envWidth = (lowerArea.getWidth() - gap) / 2;
  ampADSRPanel.setBounds(lowerArea.removeFromLeft(envWidth));
  lowerArea.removeFromLeft(gap);
  filterADSRPanel.setBounds(lowerArea);

  if (presetOverlay)
    presetOverlay->setBounds(getLocalBounds());
}