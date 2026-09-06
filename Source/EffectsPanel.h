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
#include <memory>

class EffectsPanel : public juce::Component,
                     public juce::AudioProcessorValueTreeState::Listener,
                     public juce::Timer
{
public:
    explicit EffectsPanel(juce::AudioProcessorValueTreeState& apvts,
                          std::function<double()> bpmProvider = nullptr);
    ~EffectsPanel() override;

    void paint(juce::Graphics& g) override;
    void resized() override;
    void timerCallback() override;

    // APVTS Listener callback for instant preset / shuffle updates
    void parameterChanged(const juce::String& parameterID, float newValue) override;

    // Note division utilities for Delay
    static const juce::StringArray& getNoteDivisionNames();
    static double getNoteDivisionFactor(int index);

    //==============================================================================
    // Config Struct for Effect Zones
    //==============================================================================
    struct EffectZoneConfig
    {
        int index;
        juce::String name;             // "DRIVE", "CHORUS", "PHASER", "DELAY", "GATE"
        juce::Colour lcdColor;         // Theme color tuned for dark vintage LCD
        juce::String enableParamID;    // e.g. "FX_DRIVE_ENABLE"
        juce::String mixParamID;       // e.g. "FX_DRIVE_AMOUNT"
        juce::String secParam1ID;      // e.g. "FX_DRIVE_TONE"
        juce::String secParam2ID;      // e.g. "FX_DELAY_FEEDBACK" (for Delay)
    };

    //==============================================================================
    // Single Zone Component inside the continuous LCD
    //==============================================================================
    class EffectZoneComponent : public juce::Component
    {
    public:
        EffectZoneComponent(const EffectZoneConfig& config,
                            juce::AudioProcessorValueTreeState& apvts,
                            std::function<double()> bpmProvider,
                            std::function<void(int)> onSelectRequest,
                            std::function<void(int, const juce::String&)> onParameterAdjusted);
        ~EffectZoneComponent() override = default;

        void paint(juce::Graphics& g) override;
        void resized() override;

        void mouseDown(const juce::MouseEvent& e) override;
        void mouseDrag(const juce::MouseEvent& e) override;
        void mouseUp(const juce::MouseEvent& e) override;
        void mouseDoubleClick(const juce::MouseEvent& e) override;
        void mouseMove(const juce::MouseEvent& e) override;
        void mouseExit(const juce::MouseEvent& e) override;

        void setSelected(bool selected);
        bool isSelected() const { return isExpanded; }

        void updateReadout(const juce::String& text);
        void tickAnimation();

        int getZoneIndex() const { return config.index; }
        const EffectZoneConfig& getConfig() const { return config; }

        // External APVTS state sync
        void syncFromAPVTS();

    private:
        // Layout regions
        juce::Rectangle<float> getHeaderBounds() const;
        juce::Rectangle<float> getMixBarBounds() const;
        juce::Rectangle<float> getSecondaryControlsBounds() const;
        juce::Rectangle<float> getReadoutBounds() const;

        // Custom mini-knob / arc widget data
        struct MiniKnob {
            juce::Rectangle<float> bounds;
            juce::String label;
            juce::String paramID;
            bool isDragging = false;
            float dragStartY = 0.0f;
            float dragStartVal = 0.0f;
        };

        // Delay specific widgets hit areas
        juce::Rectangle<float> getBypassToggleBounds() const;
        juce::Rectangle<float> getDelaySyncToggleBounds() const;
        juce::Rectangle<float> getDelayDivisionBounds() const;

        // Helpers
        float getNormalizedParamValue(const juce::String& paramID) const;
        void setNormalizedParamValue(const juce::String& paramID, float normVal);
        bool isEffectEnabled() const;
        void syncDelayTimeFromDivision();

        EffectZoneConfig config;
        juce::AudioProcessorValueTreeState& apvts;
        std::function<double()> bpmProvider;
        std::function<void(int)> onSelectRequest;
        std::function<void(int, const juce::String&)> onParameterAdjusted;

        bool isExpanded = false;
        float expansionProgress = 0.0f; // 0.0 (collapsed) to 1.0 (expanded)

        // Hover & Drag state
        bool isHeaderHovered = false;
        bool isMixBarHovered = false;
        bool isDraggingMixBar = false;
        float mixBarDragStartVal = 0.0f;

        // Secondary controls
        MiniKnob secondaryKnob1; // Tone / Rate / Delay Time
        MiniKnob secondaryKnob2; // Delay Feedback

        // Delay tempo sync state
        bool delayIsSynced = true;
        int delayDivisionIdx = 2; // 1/8 default

        // Live Readout state
        juce::String readoutText;
        float readoutAlpha = 0.0f;
        int readoutHoldFrames = 0;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(EffectZoneComponent)
    };

private:
    void setupZones();

    juce::AudioProcessorValueTreeState& apvts;
    std::function<double()> bpmProvider;

    std::vector<std::unique_ptr<EffectZoneComponent>> zones;
    int selectedZoneIndex = -1; // -1 means all collapsed

    const std::vector<juce::String> watchedParamIDs{
        "FX_DRIVE_ENABLE", "FX_DRIVE_AMOUNT", "FX_DRIVE_TONE",
        "FX_CHORUS_ENABLE", "FX_CHORUS_AMOUNT", "FX_CHORUS_RATE",
        "FX_PHASER_ENABLE", "FX_PHASER_AMOUNT", "FX_PHASER_RATE",
        "FX_DELAY_ENABLE", "FX_DELAY_AMOUNT", "FX_DELAY_TIME", "FX_DELAY_FEEDBACK",
        "FX_SIDECHAIN_ENABLE", "FX_SIDECHAIN_MIX", "FX_SIDECHAIN_RATE"
    };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(EffectsPanel)
};
