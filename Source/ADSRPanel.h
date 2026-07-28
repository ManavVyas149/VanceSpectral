#pragma once

#include <JuceHeader.h>
#include "SpectralUILookAndFeel.h"

class ADSRPanel : public juce::Component
{
public:
    ADSRPanel(juce::AudioProcessorValueTreeState& apvts, const juce::String& panelTitle, const juce::String& paramPrefix);
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
            else
            {
                valStr = juce::String(val, 2);
            }
            setTooltip(valStr);
        }

        juce::String unit;
    };

    juce::String title;

    HoverValueSlider attackSlider{ "s" };
    HoverValueSlider decaySlider{ "s" };
    HoverValueSlider sustainSlider{ "" };
    HoverValueSlider releaseSlider{ "s" };

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
