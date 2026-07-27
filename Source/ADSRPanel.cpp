#include "ADSRPanel.h"

//==============================================================================
ADSRPanelLookAndFeel::ADSRPanelLookAndFeel() {}

void ADSRPanelLookAndFeel::drawRotarySlider(
    juce::Graphics &g, int x, int y, int width, int height, float sliderPos,
    float rotaryStartAngle, float rotaryEndAngle, juce::Slider &slider) {
  juce::ignoreUnused(slider);

  if (width <= 0 || height <= 0)
    return;

  auto radius = (float)juce::jmin(width / 2, height / 2) - 5.0f;
  if (radius <= 2.0f)
    return;

  auto centreX = (float)x + (float)width * 0.5f;
  auto centreY = (float)y + (float)height * 0.5f;
  auto angle =
      rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);

  auto outlineColour = juce::Colour(55, 58, 68);
  auto fillGradient = juce::ColourGradient(
      juce::Colour(0, 220, 255), static_cast<float>(x), static_cast<float>(y),
      juce::Colour(160, 80, 255), static_cast<float>(x + width),
      static_cast<float>(y + height), false);

  // Draw background track arc
  juce::Path backgroundTrack;
  backgroundTrack.addCentredArc(centreX, centreY, radius, radius, 0.0f,
                                rotaryStartAngle, rotaryEndAngle, true);
  g.setColour(outlineColour);
  g.strokePath(backgroundTrack,
               juce::PathStrokeType(3.5f, juce::PathStrokeType::curved,
                                    juce::PathStrokeType::rounded));

  // Draw active value arc track
  if (sliderPos > 0.0f) {
    juce::Path valueTrack;
    valueTrack.addCentredArc(centreX, centreY, radius, radius, 0.0f,
                             rotaryStartAngle, angle, true);
    g.setGradientFill(fillGradient);
    g.strokePath(valueTrack,
                 juce::PathStrokeType(3.5f, juce::PathStrokeType::curved,
                                      juce::PathStrokeType::rounded));
  }

  // Medium Knob body
  auto knobRadius = juce::jmax(2.0f, radius - 5.0f);
  juce::ColourGradient knobGradient(
      juce::Colour(48, 52, 62), centreX, centreY - knobRadius,
      juce::Colour(24, 26, 32), centreX, centreY + knobRadius, false);
  g.setGradientFill(knobGradient);
  g.fillEllipse(centreX - knobRadius, centreY - knobRadius, knobRadius * 2.0f,
                knobRadius * 2.0f);

  g.setColour(juce::Colour(75, 80, 95));
  g.drawEllipse(centreX - knobRadius, centreY - knobRadius, knobRadius * 2.0f,
                knobRadius * 2.0f, 1.5f);

  // Draw pointer line
  auto pointerLength = juce::jmax(1.0f, knobRadius * 0.72f);
  auto pointerThickness = 3.0f;
  juce::Path p;
  p.addRoundedRectangle(-pointerThickness * 0.5f, -pointerLength,
                        pointerThickness, pointerLength, 1.2f);
  p.applyTransform(
      juce::AffineTransform::rotation(angle).translated(centreX, centreY));
  g.setColour(juce::Colour(240, 245, 255));
  g.fillPath(p);
}

//==============================================================================
ADSRPanel::ADSRPanel(juce::AudioProcessorValueTreeState &apvts,
                     const juce::String &panelTitle,
                     const juce::String &paramPrefix)
    : title(panelTitle) {
  auto setupSlider = [this](juce::Slider &slider, juce::Label &label,
                            const juce::String &name) {
    slider.setSliderStyle(juce::Slider::SliderStyle::RotaryVerticalDrag);
    slider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    slider.setLookAndFeel(&customLookAndFeel);
    addAndMakeVisible(slider);

    label.setText(name, juce::dontSendNotification);
    label.setFont(juce::FontOptions(13.0f, juce::Font::FontStyleFlags::bold));
    label.setJustificationType(juce::Justification::centred);
    label.setColour(juce::Label::textColourId, juce::Colour(180, 185, 200));
    addAndMakeVisible(label);
  };

  setupSlider(attackSlider, attackLabel, "Attack");
  setupSlider(decaySlider, decayLabel, "Decay");
  setupSlider(sustainSlider, sustainLabel, "Sustain");
  setupSlider(releaseSlider, releaseLabel, "Release");

  attackAttachment =
      std::make_unique<Attachment>(apvts, paramPrefix + "ATTACK", attackSlider);
  decayAttachment =
      std::make_unique<Attachment>(apvts, paramPrefix + "DECAY", decaySlider);
  sustainAttachment = std::make_unique<Attachment>(
      apvts, paramPrefix + "SUSTAIN", sustainSlider);
  releaseAttachment = std::make_unique<Attachment>(
      apvts, paramPrefix + "RELEASE", releaseSlider);
}

ADSRPanel::~ADSRPanel() {
  attackAttachment.reset();
  decayAttachment.reset();
  sustainAttachment.reset();
  releaseAttachment.reset();

  attackSlider.setLookAndFeel(nullptr);
  decaySlider.setLookAndFeel(nullptr);
  sustainSlider.setLookAndFeel(nullptr);
  releaseSlider.setLookAndFeel(nullptr);
}

void ADSRPanel::paint(juce::Graphics &g) {
  auto titleArea = getLocalBounds().removeFromTop(26);
  if (titleArea.getWidth() > 10 && titleArea.getHeight() > 5) {
    g.setColour(juce::Colours::white);
    g.setFont(juce::FontOptions(16.0f, juce::Font::FontStyleFlags::bold));
    auto marginX = juce::jmin(12, titleArea.getWidth() / 4);
    g.drawText(title, titleArea.reduced(marginX, 0), juce::Justification::left);
  }
}

void ADSRPanel::resized() {
  auto area = getLocalBounds();
  if (area.getWidth() <= 0 || area.getHeight() <= 0)
    return;

  area.removeFromTop(26); // Compact header space

  auto knobWidth = area.getWidth() / 4;

  auto setupColumn = [](juce::Rectangle<int> colArea, juce::Label &label,
                        juce::Slider &slider) {
    if (colArea.getWidth() <= 0 || colArea.getHeight() <= 0)
      return;

    // Position label right under the knob with zero bottom gap
    auto labelHeight = juce::jmin(16, colArea.getHeight());
    label.setBounds(colArea.removeFromBottom(labelHeight));

    slider.setBounds(colArea.reduced(1, 1));
  };

  setupColumn(area.removeFromLeft(knobWidth), attackLabel, attackSlider);
  setupColumn(area.removeFromLeft(knobWidth), decayLabel, decaySlider);
  setupColumn(area.removeFromLeft(knobWidth), sustainLabel, sustainSlider);
  setupColumn(area, releaseLabel, releaseSlider);
}
