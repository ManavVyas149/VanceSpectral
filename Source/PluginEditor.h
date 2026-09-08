#pragma once

#include <map>
#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "SpectralUILookAndFeel.h"
#include "PresetBarComponent.h"
#include "SpectrogramComponent.h"
#include "ToolbarComponent.h"
#include "SegmentedControlComponent.h"
#include "ADSRPanel.h"
#include "EffectsPanel.h"
#include "PresetManager.h"
#include "PresetBrowserOverlay.h"

class VancespectralAudioProcessorEditor : public juce::AudioProcessorEditor,
                                          public juce::DragAndDropContainer,
                                          public juce::Timer
{
public:
    static constexpr int nativeWidth = 1088;
    static constexpr int nativeHeight = 544;

    class ContentWrapper : public juce::Component
    {
    public:
        ContentWrapper(VancespectralAudioProcessorEditor& owner) : editor(owner) {}
        void paint(juce::Graphics& g) override;
        void resized() override;

    private:
        VancespectralAudioProcessorEditor& editor;
    };

    VancespectralAudioProcessorEditor(VancespectralAudioProcessor&);
    ~VancespectralAudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;
    bool keyPressed(const juce::KeyPress& key) override;
    bool keyStateChanged(bool isKeyDown) override;
    void timerCallback() override;

    void triggerRandomConfigurationReroll();

private:
    VancespectralAudioProcessor& audioProcessor;
    SpectralUILookAndFeel spectralLookAndFeel;

    PresetBarComponent presetBar;
    ToolbarComponent toolbar;
    std::unique_ptr<SpectrogramComponent> spectrogram;

    SegmentedControlComponent playbackControl{ "playback", { "Forward", "Backward", "Forward-Backward", "Backward-Forward", "Random" }, SegmentedControlComponent::LayoutMode::Vertical };
    SegmentedControlComponent pitchControl{ "pitch", { "Stretch", "Resample" }, SegmentedControlComponent::LayoutMode::Horizontal };

    ADSRPanel adsrPanel;
    EffectsPanel effectsPanel;

    juce::Slider volumeSlider;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> volumeAttachment;

    class PolyButton : public juce::Button
    {
    public:
        PolyButton() : juce::Button("POLY") {}

        void paintButton(juce::Graphics& g, bool isHighlighted, bool isDown) override
        {
            juce::ignoreUnused(isDown);
            auto bounds = getLocalBounds().toFloat().reduced(1.0f);
            bool active = getToggleState();

            juce::Colour bg = active ? SpectralUILookAndFeel::accentColour.withAlpha(0.20f)
                                     : (isHighlighted ? juce::Colour(0xEE, 0xF0, 0xF8)
                                                      : SpectralUILookAndFeel::panelBgColour);

            g.setColour(bg);
            g.fillRoundedRectangle(bounds, 3.0f);

            juce::Colour borderCol = active ? SpectralUILookAndFeel::accentColour
                                            : SpectralUILookAndFeel::dividerColour;
            g.setColour(borderCol);
            g.drawRoundedRectangle(bounds, 3.0f, 1.0f);

            g.setFont(SpectralUILookAndFeel::getJetBrainsMono(10.5f, true));
            g.setColour(active ? SpectralUILookAndFeel::accentColour : SpectralUILookAndFeel::textMainColour);
            g.drawText(active ? "POLY" : "MONO", bounds.toNearestInt(), juce::Justification::centred, false);
        }
    };

    PolyButton polyButton;

    PresetManager presetManager;
    std::unique_ptr<PresetBrowserOverlay> presetOverlay;

    using ParameterAttachment = juce::AudioProcessorValueTreeState::ComboBoxAttachment;
    // Attachments for segmented control parameter sync
    std::unique_ptr<juce::ParameterAttachment> playbackAttachment;
    std::unique_ptr<juce::ParameterAttachment> pitchAttachment;

    ContentWrapper contentWrapper{*this};

    int currentOctaveOffset = 0;
    std::map<int, int> activeQwertyNoteKeys;
    static int getQwertySemitone(juce::juce_wchar c);
    juce::int64 lastAutoCheckpointTimeMs = 0;

    void updatePresetNavigationButtons();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(VancespectralAudioProcessorEditor)
};
