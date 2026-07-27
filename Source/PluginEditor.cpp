#include "PluginEditor.h"
#include "PluginProcessor.h"

//==============================================================================
VancespectralAudioProcessorEditor::VancespectralAudioProcessorEditor(
    VancespectralAudioProcessor &p)
    : AudioProcessorEditor(&p), audioProcessor(p),
      ampADSRPanel(p.getAPVTS(), "AMP", "AMP_"),
      filterADSRPanel(p.getAPVTS(), "FILTER", "FILTER_") {
  spectrogram = std::make_unique<SpectrogramComponent>(audioProcessor);

  addAndMakeVisible(toolbar);
  addAndMakeVisible(presetHeader);
  addAndMakeVisible(*spectrogram);
  addAndMakeVisible(ampADSRPanel);
  addAndMakeVisible(filterADSRPanel);

  setSize(1200, 700);
}

VancespectralAudioProcessorEditor::~VancespectralAudioProcessorEditor() {}

//==============================================================================
void VancespectralAudioProcessorEditor::paint(juce::Graphics &g) {
  //==============================
  // Background
  //==============================
  g.fillAll(juce::Colour::fromRGB(28, 28, 28));

  constexpr int margin = 12;
  constexpr int gap = 10;

  auto area = getLocalBounds().reduced(margin);

  //==============================
  // RIGHT COLUMN (Playback & Effects)
  //==============================
  auto rightColumn = area.removeFromRight(320);
  area.removeFromRight(gap);

  // Playback height (230px)
  auto playbackArea = rightColumn.removeFromTop(230);
  rightColumn.removeFromTop(gap);

  // Effects is taller (takes the rest of the right column)
  auto fxArea = rightColumn;

  //==============================
  // LEFT TOOLBAR
  //==============================
  auto toolbarArea = area.removeFromLeft(42);
  area.removeFromLeft(gap);

  //==============================
  // CENTER COLUMN (Spectrogram, Top Box, ADSR Panels)
  //==============================
  auto centerColumn = area;

  // Top dark rectangle (placed at top of drag sample display: 42px height)
  auto graphArea = centerColumn.removeFromTop(42);
  centerColumn.removeFromTop(gap);

  // Compact ADSR row at bottom (180px height)
  auto adsrRowArea = centerColumn.removeFromBottom(180);
  centerColumn.removeFromBottom(gap);

  auto filterArea = adsrRowArea.removeFromRight(adsrRowArea.getWidth() / 2);
  adsrRowArea.removeFromRight(gap);

  auto ampArea = adsrRowArea;

  // Spectrogram / Drag sample display (takes remaining center height)
  auto spectrogramArea = centerColumn;

  //==============================
  // DRAW PANELS
  //==============================
  g.setColour(juce::Colour(8, 8, 8));
  g.fillRoundedRectangle(spectrogramArea.toFloat(), 10.0f);

  g.setColour(juce::Colour(35, 35, 35));
  g.fillRoundedRectangle(playbackArea.toFloat(), 10.0f);
  g.fillRoundedRectangle(ampArea.toFloat(), 10.0f);
  g.fillRoundedRectangle(filterArea.toFloat(), 10.0f);
  g.fillRoundedRectangle(fxArea.toFloat(), 10.0f);

  //==============================
  // LABELS
  //==============================
  g.setColour(juce::Colours::white);
  g.setFont(22.0f);

  auto drawTitle = [&](juce::String text, juce::Rectangle<int> bounds) {
    if (bounds.getWidth() > 36 && bounds.getHeight() > 10) {
      auto titleBounds =
          bounds.removeFromTop(juce::jmin(32, bounds.getHeight()));
      g.drawText(
          text,
          titleBounds.reduced(juce::jmin(14, titleBounds.getWidth() / 4), 0),
          juce::Justification::left);
    }
  };

  drawTitle("Spectrogram", spectrogramArea);
  drawTitle("Playback", playbackArea);
  drawTitle("Effects", fxArea);
}

void VancespectralAudioProcessorEditor::resized() {
  constexpr int margin = 12;
  constexpr int gap = 10;

  auto area = getLocalBounds().reduced(margin);

  //==============================
  // RIGHT COLUMN (Playback & Effects)
  //==============================
  auto rightColumn = area.removeFromRight(320);
  area.removeFromRight(gap);

  // Playback height (230px)
  auto playbackArea = rightColumn.removeFromTop(230);
  rightColumn.removeFromTop(gap);

  // Effects is taller (takes the rest of the right column)
  auto fxArea = rightColumn;

  //==============================
  // LEFT TOOLBAR
  //==============================
  auto toolbarArea = area.removeFromLeft(42);
  area.removeFromLeft(gap);

  //==============================
  // CENTER COLUMN (Spectrogram, Top Box, ADSR Panels)
  //==============================
  auto centerColumn = area;

  // Top dark rectangle (42px height)
  auto graphArea = centerColumn.removeFromTop(42);
  centerColumn.removeFromTop(gap);

  // Compact ADSR row at bottom (180px height)
  auto adsrRowArea = centerColumn.removeFromBottom(180);
  centerColumn.removeFromBottom(gap);

  auto filterArea = adsrRowArea.removeFromRight(adsrRowArea.getWidth() / 2);
  adsrRowArea.removeFromRight(gap);

  auto ampArea = adsrRowArea;

  // Spectrogram / Drag sample display (takes remaining center height)
  auto spectrogramArea = centerColumn;

  toolbar.setBounds(toolbarArea);
  presetHeader.setBounds(graphArea);

  if (spectrogram)
    spectrogram->setBounds(spectrogramArea);

  ampADSRPanel.setBounds(ampArea.reduced(4));
  filterADSRPanel.setBounds(filterArea.reduced(4));
}