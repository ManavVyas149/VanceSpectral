#pragma once

#include <JuceHeader.h>

class ADSRPanelLookAndFeel : public juce::LookAndFeel_V4
{
public:
    ADSRPanelLookAndFeel();
    void drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height,
                          float sliderPos, float rotaryStartAngle, float rotaryEndAngle,
                          juce::Slider& slider) override;
};

class ADSRPanel : public juce::Component
{
public:
    ADSRPanel(juce::AudioProcessorValueTreeState& apvts, const juce::String& panelTitle, const juce::String& paramPrefix);
    ~ADSRPanel() override;

    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    juce::String title;
    ADSRPanelLookAndFeel customLookAndFeel;

    juce::Slider attackSlider;
    juce::Slider decaySlider;
    juce::Slider sustainSlider;
    juce::Slider releaseSlider;

    juce::Label attackLabel;
    juce::Label decayLabel;
    juce::Label sustainLabel;
    juce::Label releaseLabel;

    using Attachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    std::unique_ptr<Attachment> attackAttachment;
    std::unique_ptr<Attachment> decayAttachment;
    std::unique_ptr<Attachment> sustainAttachment;
    std::unique_ptr<Attachment> releaseAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ADSRPanel)
};
