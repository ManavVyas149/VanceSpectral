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

  // Setup FILTER ENV controls
  setupSlider(filterAttackSlider, filterAttackLabel, "Attack");
  setupSlider(filterDecaySlider, filterDecayLabel, "Decay");
  setupSlider(filterSustainSlider, filterSustainLabel, "Sustain");
  setupSlider(filterReleaseSlider, filterReleaseLabel, "Release");

  // Setup PITCH & EXCITER controls
  setupSlider(pitchSlider, pitchLabel, "0 st");
  setupSlider(exciterSlider, exciterLabel, "Exciter");

  pitchSlider.onValueChange = [this]() {
    int st = (int)pitchSlider.getValue();
    juce::String valStr = (st > 0 ? "+" : "") + juce::String(st) + " st";
    pitchLabel.setText(valStr, juce::dontSendNotification);
  };

  // Attachments
  ampAttackAttachment = std::make_unique<Attachment>(apvts, "AMP_ATTACK", ampAttackSlider);
  ampDecayAttachment = std::make_unique<Attachment>(apvts, "AMP_DECAY", ampDecaySlider);
  ampSustainAttachment = std::make_unique<Attachment>(apvts, "AMP_SUSTAIN", ampSustainSlider);
  ampReleaseAttachment = std::make_unique<Attachment>(apvts, "AMP_RELEASE", ampReleaseSlider);

  filterAttackAttachment = std::make_unique<Attachment>(apvts, "FILTER_ATTACK", filterAttackSlider);
  filterDecayAttachment = std::make_unique<Attachment>(apvts, "FILTER_DECAY", filterDecaySlider);
  filterSustainAttachment = std::make_unique<Attachment>(apvts, "FILTER_SUSTAIN", filterSustainSlider);
  filterReleaseAttachment = std::make_unique<Attachment>(apvts, "FILTER_RELEASE", filterReleaseSlider);

  pitchAttachment = std::make_unique<Attachment>(apvts, "PITCH_SEMITONES", pitchSlider);
  exciterAttachment = std::make_unique<Attachment>(apvts, "EXCITER", exciterSlider);
}

ADSRPanel::~ADSRPanel() {
  ampAttackAttachment.reset();
  ampDecayAttachment.reset();
  ampSustainAttachment.reset();
  ampReleaseAttachment.reset();

  filterAttackAttachment.reset();
  filterDecayAttachment.reset();
  filterSustainAttachment.reset();
  filterReleaseAttachment.reset();

  pitchAttachment.reset();
  exciterAttachment.reset();
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

  // Subtle horizontal divider line between rows
  if (dividerY > 0) {
    g.setColour(SpectralUILookAndFeel::dividerColour);
    g.drawHorizontalLine(dividerY, 12.0f, (float)getWidth() - 12.0f);
  }

  // FILTER ENV section label
  if (!filterHeaderArea.isEmpty()) {
    g.setColour(SpectralUILookAndFeel::textMutedColour);
    g.drawText("FILTER ENV", filterHeaderArea, juce::Justification::left, true);
  }

  // Vertical divider line separating ADSR section from EXCITER section
  if (exciterDividerX > 0) {
    g.setColour(SpectralUILookAndFeel::dividerColour);
    g.drawVerticalLine(exciterDividerX, 12.0f, (float)getHeight() - 12.0f);
  }

  // PITCH section label
  if (!pitchHeaderArea.isEmpty()) {
    g.setColour(SpectralUILookAndFeel::textMutedColour);
    g.drawText("PITCH", pitchHeaderArea, juce::Justification::left, true);
  }

  // EXCITER section label
  if (!exciterHeaderArea.isEmpty()) {
    g.setColour(SpectralUILookAndFeel::textMutedColour);
    g.drawText("EXCITER", exciterHeaderArea, juce::Justification::left, true);
  }
}

void ADSRPanel::resized() {
  auto area = getLocalBounds().reduced(12, 8);
  if (area.getWidth() <= 0 || area.getHeight() <= 0)
    return;

  // Rightmost column for PITCH & EXCITER knobs (~88px width)
  int rightWidth = juce::jmin(88, area.getWidth() / 5);
  auto rightSectionArea = area.removeFromRight(rightWidth);

  // Vertical divider line position
  exciterDividerX = rightSectionArea.getX() - 6;
  area.removeFromRight(12);

  int totalH = area.getHeight();
  int spacing = 10;
  int rowH = (totalH - spacing) / 2;

  // Upper section: AMP ENV
  auto ampRowArea = area.removeFromTop(rowH);
  ampHeaderArea = ampRowArea.removeFromTop(16);

  int knobWidth1 = ampRowArea.getWidth() / 4;
  auto setupColumn = [](juce::Rectangle<int> colArea, juce::Label &label, juce::Slider &slider) {
    if (colArea.getWidth() <= 0 || colArea.getHeight() <= 0)
      return;

    auto labelHeight = 14;
    label.setBounds(colArea.removeFromBottom(labelHeight));
    slider.setBounds(colArea.reduced(2, 2));
  };

  setupColumn(ampRowArea.removeFromLeft(knobWidth1), ampAttackLabel, ampAttackSlider);
  setupColumn(ampRowArea.removeFromLeft(knobWidth1), ampDecayLabel, ampDecaySlider);
  setupColumn(ampRowArea.removeFromLeft(knobWidth1), ampSustainLabel, ampSustainSlider);
  setupColumn(ampRowArea, ampReleaseLabel, ampReleaseSlider);

  // Upper right: PITCH knob
  auto pitchSectionArea = rightSectionArea.removeFromTop(rowH);
  pitchHeaderArea = pitchSectionArea.removeFromTop(16);
  setupColumn(pitchSectionArea, pitchLabel, pitchSlider);

  // Horizontal divider position between AMP ENV & FILTER ENV
  dividerY = area.getY() + spacing / 2;
  area.removeFromTop(spacing);
  rightSectionArea.removeFromTop(spacing);

  // Lower section: FILTER ENV
  auto filterRowArea = area;
  filterHeaderArea = filterRowArea.removeFromTop(16);

  int knobWidth2 = filterRowArea.getWidth() / 4;
  setupColumn(filterRowArea.removeFromLeft(knobWidth2), filterAttackLabel, filterAttackSlider);
  setupColumn(filterRowArea.removeFromLeft(knobWidth2), filterDecayLabel, filterDecaySlider);
  setupColumn(filterRowArea.removeFromLeft(knobWidth2), filterSustainLabel, filterSustainSlider);
  setupColumn(filterRowArea, filterReleaseLabel, filterReleaseSlider);

  // Lower right: EXCITER knob
  auto exciterSectionArea = rightSectionArea;
  exciterHeaderArea = exciterSectionArea.removeFromTop(16);
  setupColumn(exciterSectionArea, exciterLabel, exciterSlider);
}
