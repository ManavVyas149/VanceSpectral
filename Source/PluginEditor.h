#pragma once

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

    SegmentedControlComponent playbackControl{ "playback", { "Forward", "Backward", "Forw-Backw", "Back-Forw", "Random" } };
    SegmentedControlComponent pitchControl{ "pitch", { "Stretch", "Resample" } };

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

            juce::Colour bg = active ? SpectralUILookAndFeel::accentColour
                                     : (isHighlighted ? juce::Colour(0x2C, 0x2E, 0x3A)
                                                      : juce::Colour(0x20, 0x22, 0x2A));

            g.setColour(bg);
            g.fillRoundedRectangle(bounds, 3.0f);

            juce::Colour borderCol = active ? SpectralUILookAndFeel::accentBright
                                            : SpectralUILookAndFeel::dividerColour;
            g.setColour(borderCol);
            g.drawRoundedRectangle(bounds, 3.0f, 1.0f);

            g.setFont(SpectralUILookAndFeel::getMonospaceFont(9.5f));
            g.setColour(active ? juce::Colours::black : SpectralUILookAndFeel::textMutedColour);
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

#include <map>

    int currentOctaveOffset = 0;
    std::map<int, int> activeQwertyNoteKeys;
    static int getQwertySemitone(juce::juce_wchar c);
    juce::int64 lastAutoCheckpointTimeMs = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(VancespectralAudioProcessorEditor)
};
