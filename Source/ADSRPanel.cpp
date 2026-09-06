#include "ADSRPanel.h"

ADSRPanel::ADSRPanel(juce::AudioProcessorValueTreeState &apvts) {
  auto setupSlider = [this](juce::Slider &slider, juce::Label &label, const juce::String &name) {
    addAndMakeVisible(slider);

    label.setText(name, juce::dontSendNotification);
    label.setFont(SpectralUILookAndFeel::getGeometricFont(11.0f, false));
    label.setJustificationType(juce::Justification::centred);
    label.setColour(juce::Label::textColourId, SpectralUILookAndFeel::textMutedColour);
    addAndMakeVisible(label);
  };

  // Setup AMP ENV controls
  setupSlider(ampAttackSlider, ampAttackLabel, "Attack");
  setupSlider(ampDecaySlider, ampDecayLabel, "Decay");
  setupSlider(ampSustainSlider, ampSustainLabel, "Sustain");
  setupSlider(ampReleaseSlider, ampReleaseLabel, "Release");

  // Setup PITCH, GLIDE & EXCITER controls
  setupSlider(pitchSlider, pitchLabel, "0 st");
  setupSlider(glideSlider, glideLabel, "0 ms");
  setupSlider(exciterSlider, exciterLabel, "Exciter");
  setupSlider(driftSlider, driftLabel, "Drift");

  // Attachments
  ampAttackAttachment = std::make_unique<Attachment>(apvts, "AMP_ATTACK", ampAttackSlider);
  ampDecayAttachment = std::make_unique<Attachment>(apvts, "AMP_DECAY", ampDecaySlider);
  ampSustainAttachment = std::make_unique<Attachment>(apvts, "AMP_SUSTAIN", ampSustainSlider);
  ampReleaseAttachment = std::make_unique<Attachment>(apvts, "AMP_RELEASE", ampReleaseSlider);

  pitchAttachment = std::make_unique<Attachment>(apvts, "PITCH_SEMITONES", pitchSlider);
  driftAttachment = std::make_unique<Attachment>(apvts, "TIMBRE_DRIFT", driftSlider);
  glideAttachment = std::make_unique<Attachment>(apvts, "GLIDE", glideSlider);
  exciterAttachment = std::make_unique<Attachment>(apvts, "EXCITER", exciterSlider);
}

ADSRPanel::~ADSRPanel() {
  ampAttackAttachment.reset();
  ampDecayAttachment.reset();
  ampSustainAttachment.reset();
  ampReleaseAttachment.reset();

  pitchAttachment.reset();
  driftAttachment.reset();
  glideAttachment.reset();
  exciterAttachment.reset();
}

void ADSRPanel::updatePolyMode(bool isPoly) {
  driftSlider.setEnabled(isPoly);
}

void ADSRPanel::paint(juce::Graphics &g) {
  auto bounds = getLocalBounds().toFloat();

  // Matte panel background
  g.setColour(SpectralUILookAndFeel::panelBgColour);
  g.fillRoundedRectangle(bounds, 6.0f);

  // Hairline border
  g.setColour(SpectralUILookAndFeel::dividerColour);
  g.drawRoundedRectangle(bounds, 6.0f, 1.0f);

  // Section title styling
  g.setFont(SpectralUILookAndFeel::getGeometricFont(10.0f, true));
  g.setColour(SpectralUILookAndFeel::textMutedColour);

  // AMP ENV section label
  if (!ampHeaderArea.isEmpty()) {
    g.setColour(SpectralUILookAndFeel::textMutedColour);
    g.drawText("AMP ENV", ampHeaderArea, juce::Justification::left, true);
  }

  // Vertical divider line separating AMP ENV section from PITCH & DRIFT section
  if (pitchDividerX > 0) {
    g.setColour(SpectralUILookAndFeel::dividerColour);
    g.drawVerticalLine(pitchDividerX, 12.0f, (float)getHeight() - 12.0f);
  }

  // PITCH & DRIFT section label
  if (!pitchHeaderArea.isEmpty()) {
    g.setColour(SpectralUILookAndFeel::textMutedColour);
    g.drawText("PITCH & DRIFT", pitchHeaderArea, juce::Justification::left, true);
  }

  // Vertical divider line separating PITCH & DRIFT section from EXCITER & GLIDE section
  if (exciterDividerX > 0) {
    g.setColour(SpectralUILookAndFeel::dividerColour);
    g.drawVerticalLine(exciterDividerX, 12.0f, (float)getHeight() - 12.0f);
  }

  // EXCITER & GLIDE section label
  if (!exciterHeaderArea.isEmpty()) {
    g.setColour(SpectralUILookAndFeel::textMutedColour);
    g.drawText("EXCITER & GLIDE", exciterHeaderArea, juce::Justification::left, true);
  }
}

void ADSRPanel::resized() {
  auto area = getLocalBounds().reduced(12, 8);
  if (area.getWidth() <= 0 || area.getHeight() <= 0)
    return;

  auto setupColumn = [](juce::Rectangle<int> colArea, juce::Label &label, juce::Slider &slider) {
    if (colArea.getWidth() <= 0 || colArea.getHeight() <= 0)
      return;

    auto labelHeight = 14;
    label.setBounds(colArea.removeFromBottom(labelHeight));
    slider.setBounds(colArea.reduced(2, 2));
  };

  // Rightmost section: EXCITER & GLIDE
  int exciterWidth = juce::jmin(150, area.getWidth() / 4);
  auto exciterSectionArea = area.removeFromRight(exciterWidth);
  exciterDividerX = exciterSectionArea.getX() - 6;
  area.removeFromRight(12);

  exciterHeaderArea = exciterSectionArea.removeFromTop(16);
  int eColWidth = exciterSectionArea.getWidth() / 2;
  setupColumn(exciterSectionArea.removeFromLeft(eColWidth), exciterLabel, exciterSlider);
  setupColumn(exciterSectionArea, glideLabel, glideSlider);

  // Middle section: PITCH & DRIFT
  int pitchWidth = juce::jmin(150, area.getWidth() / 3);
  auto pitchSectionArea = area.removeFromRight(pitchWidth);
  pitchDividerX = pitchSectionArea.getX() - 6;
  area.removeFromRight(12);

  pitchHeaderArea = pitchSectionArea.removeFromTop(16);
  int pColWidth = pitchSectionArea.getWidth() / 2;
  setupColumn(pitchSectionArea.removeFromLeft(pColWidth), pitchLabel, pitchSlider);
  setupColumn(pitchSectionArea, driftLabel, driftSlider);

  // Left section: AMP ENV (spans across full left side)
  auto ampRowArea = area;
  ampHeaderArea = ampRowArea.removeFromTop(16);

  int knobWidth1 = ampRowArea.getWidth() / 4;
  setupColumn(ampRowArea.removeFromLeft(knobWidth1), ampAttackLabel, ampAttackSlider);
  setupColumn(ampRowArea.removeFromLeft(knobWidth1), ampDecayLabel, ampDecaySlider);
  setupColumn(ampRowArea.removeFromLeft(knobWidth1), ampSustainLabel, ampSustainSlider);
  setupColumn(ampRowArea, ampReleaseLabel, ampReleaseSlider);
}
