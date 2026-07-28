#include "ADSRPanel.h"

ADSRPanel::ADSRPanel(juce::AudioProcessorValueTreeState &apvts,
                     const juce::String &panelTitle,
                     const juce::String &paramPrefix)
    : title(panelTitle) {
  auto setupSlider = [this](juce::Slider &slider, juce::Label &label, const juce::String &name) {
    addAndMakeVisible(slider);

    label.setText(name, juce::dontSendNotification);
    label.setFont(SpectralUILookAndFeel::getGeometricFont(11.0f, false));
    label.setJustificationType(juce::Justification::centred);
    label.setColour(juce::Label::textColourId, SpectralUILookAndFeel::textMutedColour);
    addAndMakeVisible(label);
  };

  setupSlider(attackSlider, attackLabel, "Attack");
  setupSlider(decaySlider, decayLabel, "Decay");
  setupSlider(sustainSlider, sustainLabel, "Sustain");
  setupSlider(releaseSlider, releaseLabel, "Release");

  attackAttachment = std::make_unique<Attachment>(apvts, paramPrefix + "ATTACK", attackSlider);
  decayAttachment = std::make_unique<Attachment>(apvts, paramPrefix + "DECAY", decaySlider);
  sustainAttachment = std::make_unique<Attachment>(apvts, paramPrefix + "SUSTAIN", sustainSlider);
  releaseAttachment = std::make_unique<Attachment>(apvts, paramPrefix + "RELEASE", releaseSlider);
}

ADSRPanel::~ADSRPanel() {
  attackAttachment.reset();
  decayAttachment.reset();
  sustainAttachment.reset();
  releaseAttachment.reset();
}

void ADSRPanel::paint(juce::Graphics &g) {
  auto bounds = getLocalBounds().toFloat();

  // Matte panel background
  g.setColour(SpectralUILookAndFeel::panelBgColour);
  g.fillRoundedRectangle(bounds, 6.0f);

  // Hairline border
  g.setColour(SpectralUILookAndFeel::dividerColour);
  g.drawRoundedRectangle(bounds, 6.0f, 1.0f);

  // Section title top-left
  g.setFont(SpectralUILookAndFeel::getGeometricFont(10.0f, true));
  g.setColour(SpectralUILookAndFeel::textMutedColour);
  g.drawText(title.toUpperCase(), bounds.removeFromTop(20.0f).reduced(10.0f, 0.0f).toNearestInt(), juce::Justification::left, true);
}

void ADSRPanel::resized() {
  auto area = getLocalBounds().reduced(8, 4);
  if (area.getWidth() <= 0 || area.getHeight() <= 0)
    return;

  area.removeFromTop(18); // Reserve for panel header label

  int knobWidth = area.getWidth() / 4;

  auto setupColumn = [](juce::Rectangle<int> colArea, juce::Label &label, juce::Slider &slider) {
    if (colArea.getWidth() <= 0 || colArea.getHeight() <= 0)
      return;

    auto labelHeight = 14;
    label.setBounds(colArea.removeFromBottom(labelHeight));
    slider.setBounds(colArea.reduced(2, 2));
  };

  setupColumn(area.removeFromLeft(knobWidth), attackLabel, attackSlider);
  setupColumn(area.removeFromLeft(knobWidth), decayLabel, decaySlider);
  setupColumn(area.removeFromLeft(knobWidth), sustainLabel, sustainSlider);
  setupColumn(area, releaseLabel, releaseSlider);
}
