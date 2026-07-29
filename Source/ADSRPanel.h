#pragma once

#include <JuceHeader.h>
#include "SpectralUILookAndFeel.h"

class ADSRPanel : public juce::Component
{
public:
    explicit ADSRPanel(juce::AudioProcessorValueTreeState& apvts);
    ~ADSRPanel() override;

    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    class HoverValueSlider : public juce::Slider
    {
    public:
        HoverValueSlider(const juce::String& unitSuffix = "") : unit(unitSuffix)
        {
            setSliderStyle(juce::Slider::RotaryVerticalDrag);
            setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
        }

        void mouseEnter(const juce::MouseEvent& e) override
        {
            juce::Slider::mouseEnter(e);
            updateTooltipValue();
        }

        void mouseDrag(const juce::MouseEvent& e) override
        {
            juce::Slider::mouseDrag(e);
            updateTooltipValue();
        }

    private:
        void updateTooltipValue()
        {
            double val = getValue();
            juce::String valStr;
            if (unit == "s" || unit == "ms")
            {
                if (val < 1.0)
                    valStr = juce::String((int)(val * 1000.0)) + " ms";
                else
                    valStr = juce::String(val, 2) + " s";
            }
            else if (unit == "%")
            {
                valStr = juce::String((int)(val * 100.0)) + " %";
            }
            else
            {
                valStr = juce::String(val, 2);
            }
            setTooltip(valStr);
        }

        juce::String unit;
    };

    // AMP ENV Sliders & Labels
    HoverValueSlider ampAttackSlider{ "s" };
    HoverValueSlider ampDecaySlider{ "s" };
    HoverValueSlider ampSustainSlider{ "" };
    HoverValueSlider ampReleaseSlider{ "s" };

    juce::Label ampAttackLabel;
    juce::Label ampDecayLabel;
    juce::Label ampSustainLabel;
    juce::Label ampReleaseLabel;

    // FILTER ENV Sliders & Labels
    HoverValueSlider filterAttackSlider{ "s" };
    HoverValueSlider filterDecaySlider{ "s" };
    HoverValueSlider filterSustainSlider{ "" };
    HoverValueSlider filterReleaseSlider{ "s" };

    juce::Label filterAttackLabel;
    juce::Label filterDecayLabel;
    juce::Label filterSustainLabel;
    juce::Label filterReleaseLabel;

    // PITCH & EXCITER Sliders & Labels
    HoverValueSlider pitchSlider{ "st" };
    juce::Label pitchLabel;

    HoverValueSlider exciterSlider{ "%" };
    juce::Label exciterLabel;

    using Attachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    std::unique_ptr<Attachment> ampAttackAttachment;
    std::unique_ptr<Attachment> ampDecayAttachment;
    std::unique_ptr<Attachment> ampSustainAttachment;
    std::unique_ptr<Attachment> ampReleaseAttachment;

    std::unique_ptr<Attachment> filterAttackAttachment;
    std::unique_ptr<Attachment> filterDecayAttachment;
    std::unique_ptr<Attachment> filterSustainAttachment;
    std::unique_ptr<Attachment> filterReleaseAttachment;

    std::unique_ptr<Attachment> pitchAttachment;
    std::unique_ptr<Attachment> exciterAttachment;

    juce::Rectangle<int> ampHeaderArea;
    juce::Rectangle<int> filterHeaderArea;
    juce::Rectangle<int> pitchHeaderArea;
    juce::Rectangle<int> exciterHeaderArea;
    int dividerY{ 0 };
    int exciterDividerX{ 0 };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ADSRPanel)
};
