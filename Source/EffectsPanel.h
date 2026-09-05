/*
  ==============================================================================

    EffectsPanel.h
    Created: 6 Sep 2026
    Author:  Manav Vyas / VanceSpectral Team

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "SpectralUILookAndFeel.h"

class EffectsPanel : public juce::Component
{
public:
    explicit EffectsPanel(juce::AudioProcessorValueTreeState& apvts);
    ~EffectsPanel() override;

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
            if (unit == "ms" || unit == "s")
            {
                if (val < 1.0 && unit == "s")
                    valStr = juce::String((int)(val * 1000.0)) + " ms";
                else if (unit == "ms")
                    valStr = juce::String((int)val) + " ms";
                else
                    valStr = juce::String(val, 2) + " s";
            }
            else if (unit == "%")
            {
                valStr = juce::String((int)(val * 100.0)) + " %";
            }
            else if (unit == "Hz")
            {
                valStr = juce::String(val, 2) + " Hz";
            }
            else
            {
                valStr = juce::String(val, 2);
            }
            setTooltip(valStr);
        }

        juce::String unit;
    };

    // 1. GATE
    juce::TextButton gateToggle{ "GATE" };
    HoverValueSlider gateAmountSlider{ "%" };
    juce::Label gateAmountLabel;

    // 2. CHORUS
    juce::TextButton chorusToggle{ "CHORUS" };
    HoverValueSlider chorusAmountSlider{ "%" };
    juce::Label chorusAmountLabel;
    HoverValueSlider chorusRateSlider{ "Hz" };
    juce::Label chorusRateLabel;

    // 3. PHASER
    juce::TextButton phaserToggle{ "PHASER" };
    HoverValueSlider phaserAmountSlider{ "%" };
    juce::Label phaserAmountLabel;
    HoverValueSlider phaserRateSlider{ "Hz" };
    juce::Label phaserRateLabel;

    // 4. DELAY
    juce::TextButton delayToggle{ "DELAY" };
    HoverValueSlider delayAmountSlider{ "%" };
    juce::Label delayAmountLabel;
    HoverValueSlider delayTimeSlider{ "ms" };
    juce::Label delayTimeLabel;

    // 5. DRIVE
    juce::TextButton driveToggle{ "DRIVE" };
    HoverValueSlider driveAmountSlider{ "%" };
    juce::Label driveAmountLabel;
    HoverValueSlider driveToneSlider{ "%" };
    juce::Label driveToneLabel;

    // Attachments
    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;

    std::unique_ptr<ButtonAttachment> gateToggleAttachment;
    std::unique_ptr<SliderAttachment> gateAmountAttachment;

    std::unique_ptr<ButtonAttachment> chorusToggleAttachment;
    std::unique_ptr<SliderAttachment> chorusAmountAttachment;
    std::unique_ptr<SliderAttachment> chorusRateAttachment;

    std::unique_ptr<ButtonAttachment> phaserToggleAttachment;
    std::unique_ptr<SliderAttachment> phaserAmountAttachment;
    std::unique_ptr<SliderAttachment> phaserRateAttachment;

    std::unique_ptr<ButtonAttachment> delayToggleAttachment;
    std::unique_ptr<SliderAttachment> delayAmountAttachment;
    std::unique_ptr<SliderAttachment> delayTimeAttachment;

    std::unique_ptr<ButtonAttachment> driveToggleAttachment;
    std::unique_ptr<SliderAttachment> driveAmountAttachment;
    std::unique_ptr<SliderAttachment> driveToneAttachment;

    // Layout coordinates
    juce::Rectangle<int> headerArea;
    int dividerXs[4]{ 0, 0, 0, 0 };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(EffectsPanel)
};
