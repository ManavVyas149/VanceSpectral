#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "SpectralUILookAndFeel.h"
#include "PresetBarComponent.h"
#include "SpectrogramComponent.h"
#include "ToolbarComponent.h"
#include "SegmentedControlComponent.h"
#include "ADSRPanel.h"
#include "PresetManager.h"
#include "PresetBrowserOverlay.h"

class VancespectralAudioProcessorEditor : public juce::AudioProcessorEditor,
                                          public juce::DragAndDropContainer
{
public:
    VancespectralAudioProcessorEditor(VancespectralAudioProcessor&);
    ~VancespectralAudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;
    bool keyPressed(const juce::KeyPress& key) override;
    bool keyStateChanged(bool isKeyDown) override;

private:
    VancespectralAudioProcessor& audioProcessor;
    SpectralUILookAndFeel spectralLookAndFeel;

    PresetBarComponent presetBar;
    ToolbarComponent toolbar;
    std::unique_ptr<SpectrogramComponent> spectrogram;

    SegmentedControlComponent playbackControl{ "playback", { "Forward", "Backward", "Forw-Backw", "Back-Forw", "Random" } };
    SegmentedControlComponent pitchControl{ "pitch", { "Stretch", "Resample", "Axial" } };

    ADSRPanel adsrPanel;

    juce::Slider volumeSlider;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> volumeAttachment;

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

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(VancespectralAudioProcessorEditor)
};
