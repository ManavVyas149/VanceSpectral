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

  // Setup PITCH, GLIDE & EXCITER controls
  setupSlider(pitchSlider, pitchLabel, "0 st");
  setupSlider(glideSlider, glideLabel, "0 ms");
  setupSlider(exciterSlider, exciterLabel, "Exciter");
  setupSlider(driftSlider, driftLabel, "Drift");

  // Setup TIMBRE vertical slider and disclosure button
  timbreSlider.setSliderStyle(juce::Slider::LinearVertical);
  timbreSlider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
  addAndMakeVisible(timbreSlider);

  timbreLabel.setText("0 st", juce::dontSendNotification);
  timbreLabel.setFont(SpectralUILookAndFeel::getGeometricFont(11.0f, false));
  timbreLabel.setJustificationType(juce::Justification::centred);
  timbreLabel.setColour(juce::Label::textColourId, SpectralUILookAndFeel::textMutedColour);
  addAndMakeVisible(timbreLabel);

  addAndMakeVisible(timbreDisclosureButton);

  if (auto* param = apvts.getParameter("TIMBRE_LINK")) {
    timbreExpanded = (param->getValue() < 0.5f);
  }
  timbreDisclosureButton.setToggleState(timbreExpanded, juce::dontSendNotification);

  timbreDisclosureButton.onClick = [this, &apvts]() {
    timbreExpanded = !timbreExpanded;
    timbreDisclosureButton.setToggleState(timbreExpanded, juce::dontSendNotification);
    if (auto* param = apvts.getParameter("TIMBRE_LINK")) {
      param->setValueNotifyingHost(timbreExpanded ? 0.0f : 1.0f);
    }
    resized();
    repaint();
  };

  pitchSlider.onValueChange = [this]() {
    int st = (int)pitchSlider.getValue();
    juce::String valStr = (st > 0 ? "+" : "") + juce::String(st) + " st";
    pitchLabel.setText(valStr, juce::dontSendNotification);
  };

  timbreSlider.onValueChange = [this, &apvts]() {
    int st = (int)std::round(timbreSlider.getValue());
    juce::String valStr = (st > 0 ? "+" : "") + juce::String(st) + " st";
    timbreLabel.setText(valStr, juce::dontSendNotification);
    if (auto* param = apvts.getParameter("TIMBRE_LINK")) {
      if (param->getValue() >= 0.5f) {
        param->setValueNotifyingHost(0.0f);
        timbreExpanded = true;
        timbreDisclosureButton.setToggleState(true, juce::dontSendNotification);
        resized();
        repaint();
      }
    }
  };

  glideSlider.onValueChange = [this]() {
    double val = glideSlider.getValue();
    juce::String valStr;
    if (val < 1.0)
      valStr = juce::String((int)(val * 1000.0)) + " ms";
    else
      valStr = juce::String(val, 2) + " s";
    glideLabel.setText(valStr, juce::dontSendNotification);
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
  timbreAttachment = std::make_unique<Attachment>(apvts, "TIMBRE_SEMITONES", timbreSlider);
  driftAttachment = std::make_unique<Attachment>(apvts, "TIMBRE_DRIFT", driftSlider);
  glideAttachment = std::make_unique<Attachment>(apvts, "GLIDE", glideSlider);
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
  timbreAttachment.reset();
  timbreLinkAttachment.reset();
  driftAttachment.reset();
  glideAttachment.reset();
  exciterAttachment.reset();
}

void ADSRPanel::updateTimbreEnabledState(int pitchModeIndex, bool isPoly) {
  bool enabled = (pitchModeIndex != 2); // Disable in Axial mode
  timbreSlider.setEnabled(enabled);
  timbreDisclosureButton.setEnabled(enabled);
  driftSlider.setEnabled(enabled && isPoly);
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

  // Vertical divider line separating ADSR section from PITCH & EXCITER section
  if (exciterDividerX > 0) {
    g.setColour(SpectralUILookAndFeel::dividerColour);
    g.drawVerticalLine(exciterDividerX, 12.0f, (float)getHeight() - 12.0f);
  }

  // PITCH & DRIFT section label
  if (!pitchHeaderArea.isEmpty()) {
    g.setColour(SpectralUILookAndFeel::textMutedColour);
    g.drawText("PITCH & DRIFT", pitchHeaderArea, juce::Justification::left, true);
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

  // Rightmost column for PITCH, DRIFT, EXCITER & GLIDE controls
  int rightWidth = timbreExpanded ? juce::jmin(185, area.getWidth() / 3) : juce::jmin(140, area.getWidth() / 4);
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

  // Upper right: PITCH & DRIFT (plus TIMBRE if expanded)
  auto pitchSectionArea = rightSectionArea.removeFromTop(rowH);
  pitchHeaderArea = pitchSectionArea.removeFromTop(16);

  if (timbreExpanded) {
    timbreSlider.setVisible(true);
    timbreLabel.setVisible(true);

    int pColWidth = pitchSectionArea.getWidth() / 3;
    auto col1 = pitchSectionArea.removeFromLeft(pColWidth);
    timbreDisclosureButton.setBounds(col1.getRight() - 14, col1.getY() + 1, 13, 13);
    setupColumn(col1, pitchLabel, pitchSlider);

    auto col2 = pitchSectionArea.removeFromLeft(pColWidth);
    setupColumn(col2, timbreLabel, timbreSlider);

    setupColumn(pitchSectionArea, driftLabel, driftSlider);
  } else {
    timbreSlider.setVisible(false);
    timbreLabel.setVisible(false);

    int pColWidth = pitchSectionArea.getWidth() / 2;
    auto col1 = pitchSectionArea.removeFromLeft(pColWidth);
    timbreDisclosureButton.setBounds(col1.getRight() - 14, col1.getY() + 1, 13, 13);
    setupColumn(col1, pitchLabel, pitchSlider);

    setupColumn(pitchSectionArea, driftLabel, driftSlider);
  }

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

  // Lower right: EXCITER & GLIDE knobs
  auto exciterSectionArea = rightSectionArea;
  exciterHeaderArea = exciterSectionArea.removeFromTop(16);

  int eColWidth = exciterSectionArea.getWidth() / 2;
  setupColumn(exciterSectionArea.removeFromLeft(eColWidth), exciterLabel, exciterSlider);
  setupColumn(exciterSectionArea, glideLabel, glideSlider);
}
