#include "ADSRPanel.h"

ADSRPanel::ADSRPanel(juce::AudioProcessorValueTreeState &apvts) {
  auto setupSlider = [this](juce::Slider &slider, juce::Label &label, const juce::String &name) {
    addAndMakeVisible(slider);

    label.setText(name, juce::dontSendNotification);
    label.setFont(SpectralUILookAndFeel::getSpaceGrotesk(9.5f, false));
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

  // Coated frosted glass card panel background
  SpectralUILookAndFeel::drawPanelCard(g, bounds);

  // Section title styling
  g.setFont(SpectralUILookAndFeel::getJetBrainsMono(11.8f, true));

  auto drawSectionHeader = [&](const juce::String& title, juce::Rectangle<int> headerArea) {
    if (!headerArea.isEmpty()) {
      float dotY = (float)headerArea.getCentreY();
      g.setColour(SpectralUILookAndFeel::accentColour.withAlpha(0.75f));
      g.fillEllipse((float)headerArea.getX(), dotY - 2.5f, 5.0f, 5.0f);

      g.setColour(SpectralUILookAndFeel::textMainColour);
      g.drawText(title, headerArea.withTrimmedLeft(9), juce::Justification::left, true);
    }
  };

  // AMP ENV section label
  drawSectionHeader("AMP ENV", ampHeaderArea);

  // Vertical divider line separating AMP ENV section from PITCH & DRIFT section
  if (pitchDividerX > 0) {
    g.setColour(SpectralUILookAndFeel::dividerColour);
    g.drawVerticalLine(pitchDividerX, 8.0f, (float)getHeight() - 8.0f);
  }

  // PITCH & DRIFT section label
  drawSectionHeader("PITCH & DRIFT", pitchHeaderArea);

  // Vertical divider line separating PITCH & DRIFT section from EXCITER & GLIDE section
  if (exciterDividerX > 0) {
    g.setColour(SpectralUILookAndFeel::dividerColour);
    g.drawVerticalLine(exciterDividerX, 8.0f, (float)getHeight() - 8.0f);
  }

  // EXCITER & GLIDE section label
  drawSectionHeader("EXCITER & GLIDE", exciterHeaderArea);
}

void ADSRPanel::resized() {
  auto area = getLocalBounds().reduced(10, 6);
  if (area.getWidth() <= 0 || area.getHeight() <= 0)
    return;

  auto setupColumn = [](juce::Rectangle<int> colArea, juce::Label &label, juce::Slider &slider) {
    if (colArea.getWidth() <= 0 || colArea.getHeight() <= 0)
      return;

    auto labelHeight = 14;
    label.setBounds(colArea.removeFromBottom(labelHeight));
    slider.setBounds(colArea.reduced(2, 2));
  };

  // Rightmost section: EXCITER & GLIDE (2 knobs)
  int exciterWidth = juce::jmin(145, (int)(area.getWidth() * 0.24f));
  auto exciterSectionArea = area.removeFromRight(exciterWidth);
  exciterDividerX = exciterSectionArea.getX() - 6;
  area.removeFromRight(12);

  exciterHeaderArea = exciterSectionArea.removeFromTop(18);
  int eColWidth = exciterSectionArea.getWidth() / 2;
  setupColumn(exciterSectionArea.removeFromLeft(eColWidth), exciterLabel, exciterSlider);
  setupColumn(exciterSectionArea, glideLabel, glideSlider);

  // Middle section: PITCH & DRIFT (2 knobs)
  int pitchWidth = juce::jmin(145, (int)(area.getWidth() * 0.32f));
  auto pitchSectionArea = area.removeFromRight(pitchWidth);
  pitchDividerX = pitchSectionArea.getX() - 6;
  area.removeFromRight(12);

  pitchHeaderArea = pitchSectionArea.removeFromTop(18);
  int pColWidth = pitchSectionArea.getWidth() / 2;
  setupColumn(pitchSectionArea.removeFromLeft(pColWidth), pitchLabel, pitchSlider);
  setupColumn(pitchSectionArea, driftLabel, driftSlider);

  // Left section: AMP ENV (spans across full left side, 4 knobs)
  auto ampRowArea = area;
  ampHeaderArea = ampRowArea.removeFromTop(18);

  int knobWidth1 = ampRowArea.getWidth() / 4;
  setupColumn(ampRowArea.removeFromLeft(knobWidth1), ampAttackLabel, ampAttackSlider);
  setupColumn(ampRowArea.removeFromLeft(knobWidth1), ampDecayLabel, ampDecaySlider);
  setupColumn(ampRowArea.removeFromLeft(knobWidth1), ampSustainLabel, ampSustainSlider);
  setupColumn(ampRowArea, ampReleaseLabel, ampReleaseSlider);
}
