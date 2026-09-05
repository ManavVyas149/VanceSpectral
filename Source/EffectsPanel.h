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
#include <vector>
#include <functional>

class EffectsPanel : public juce::Component
{
public:
    explicit EffectsPanel(juce::AudioProcessorValueTreeState& apvts);
    ~EffectsPanel() override;

    void paint(juce::Graphics& g) override;
    void resized() override;

    class EffectModuleComponent : public juce::Component
    {
    public:
        EffectModuleComponent(const juce::String& name,
                              const std::vector<juce::Colour>& segmentColours,
                              juce::Colour glowColour);
        ~EffectModuleComponent() override = default;

        void paint(juce::Graphics& g) override;
        void mouseDown(const juce::MouseEvent& e) override;
        void mouseDrag(const juce::MouseEvent& e) override;
        void mouseUp(const juce::MouseEvent& e) override;
        void mouseEnter(const juce::MouseEvent& e) override;
        void mouseExit(const juce::MouseEvent& e) override;
        void mouseWheelMove(const juce::MouseEvent& e, const juce::MouseWheelDetails& wheel) override;

        // Underlying standard JUCE controls for APVTS attachments
        juce::TextButton toggleButton;
        juce::Slider primarySlider;
        juce::Slider secondarySlider;

        std::function<void(EffectModuleComponent*)> onHoverChanged;
        std::function<void(float)> onValueChanged;

        bool isEffectEnabled() const { return toggleButton.getToggleState(); }
        float getAmount() const { return (float)primarySlider.getValue(); }
        const juce::String& getEffectName() const { return effectName; }

    private:
        juce::String effectName;
        std::vector<juce::Colour> colors;
        juce::Colour glow;
        bool isHovered = false;
        float dragStartValue = 0.0f;
        juce::Point<float> mouseDownPos;
        bool wasDragged = false;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(EffectModuleComponent)
    };

private:
    void updateDisplayedPercentage();
    void drawDigitalDisplay(juce::Graphics& g, juce::Rectangle<float> area);

    // 5 Effect Modules
    EffectModuleComponent driveModule;
    EffectModuleComponent phaserModule;
    EffectModuleComponent delayModule;
    EffectModuleComponent chorusModule;
    EffectModuleComponent gateModule;

    // Attachments
    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;

    std::unique_ptr<ButtonAttachment> driveToggleAttachment;
    std::unique_ptr<SliderAttachment> driveAmountAttachment;
    std::unique_ptr<SliderAttachment> driveToneAttachment;

    std::unique_ptr<ButtonAttachment> phaserToggleAttachment;
    std::unique_ptr<SliderAttachment> phaserAmountAttachment;
    std::unique_ptr<SliderAttachment> phaserRateAttachment;

    std::unique_ptr<ButtonAttachment> delayToggleAttachment;
    std::unique_ptr<SliderAttachment> delayAmountAttachment;
    std::unique_ptr<SliderAttachment> delayTimeAttachment;

    std::unique_ptr<ButtonAttachment> chorusToggleAttachment;
    std::unique_ptr<SliderAttachment> chorusAmountAttachment;
    std::unique_ptr<SliderAttachment> chorusRateAttachment;

    std::unique_ptr<ButtonAttachment> sidechainToggleAttachment;
    std::unique_ptr<SliderAttachment> sidechainMixAttachment;
    std::unique_ptr<SliderAttachment> sidechainRateAttachment;

    // State for Digital Display
    int displayedPercentage = 33;
    EffectModuleComponent* focusedModule = nullptr;

    // Coordinates
    float dividerX = 0.0f;
    juce::Rectangle<float> digitalDisplayArea;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(EffectsPanel)
};
