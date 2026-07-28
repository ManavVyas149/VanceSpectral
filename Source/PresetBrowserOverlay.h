#pragma once

#include <JuceHeader.h>
#include "PresetManager.h"
#include "SpectralUILookAndFeel.h"

class PresetBrowserOverlay : public juce::Component, public juce::ListBoxModel
{
public:
    PresetBrowserOverlay(PresetManager& manager, juce::AudioProcessorValueTreeState& apvts);
    ~PresetBrowserOverlay() override;

    void paint(juce::Graphics& g) override;
    void resized() override;

    void refreshPresetList();
    void refreshSampleList();

    // ListBoxModel interface for Presets
    int getNumRows() override;
    void paintListBoxItem(int rowNumber, juce::Graphics& g, int width, int height, bool rowIsSelected) override;
    void listBoxItemClicked(int row, const juce::MouseEvent& e) override;
    void listBoxItemDoubleClicked(int row, const juce::MouseEvent& e) override;

    std::function<void(const juce::File& presetFile, const juce::String& sampleFileName)> onPresetSelected;
    std::function<void(const juce::File& sampleFile)> onSampleSelected;
    std::function<void()> onClose;

    void setCurrentlyLoadedSampleName(const juce::String& name) { currentSampleName = name; }

private:
    PresetManager& presetManager;
    juce::AudioProcessorValueTreeState& apvts;

    // Header Components
    juce::TextButton closeButton{ "X" };

    // Column 1: Presets & Search
    juce::TextEditor searchBox;
    juce::ListBox presetListBox;

    juce::TextButton filterAllBtn{ "ALL" };
    juce::TextButton filterSynthBtn{ "SYNTH" };
    juce::TextButton filterLeadBtn{ "LEAD" };
    juce::TextButton filterBassBtn{ "BASS" };
    juce::TextButton filterPadBtn{ "PAD" };

    juce::String activeCategoryFilter = "ALL";

    // Column 2: Preset Details & Actions
    juce::Label statusLabel;
    juce::Label selectedPresetTitle;
    juce::Label selectedPresetDetails;

    juce::TextButton loadPresetBtn{ "LOAD PRESET" };
    juce::TextButton deletePresetBtn{ "DELETE PRESET" };

    juce::TextEditor saveNameInput;
    juce::TextEditor saveCategoryInput;
    juce::TextButton savePresetBtn{ "SAVE CURRENT PRESET" };

    // Column 3: Sample Storage
    juce::ListBox sampleListBox;
    juce::TextButton importSampleBtn{ "IMPORT SAMPLE FILE" };
    juce::TextButton loadSampleToEngineBtn{ "LOAD SAMPLE" };

    // Data lists
    juce::Array<PresetInfo> allPresets;
    juce::Array<PresetInfo> filteredPresets;
    juce::Array<juce::File> allSamples;

    int selectedPresetIndex = -1;
    int selectedSampleIndex = -1;

    juce::String currentSampleName;
    std::unique_ptr<juce::FileChooser> fileChooser;

    void filterPresets();
    void updateSelectedPresetDetails();
    void executeLoadSelectedPreset();

    class SampleListModel : public juce::ListBoxModel
    {
    public:
        SampleListModel(PresetBrowserOverlay& owner) : owner(owner) {}
        int getNumRows() override { return owner.allSamples.size(); }
        void paintListBoxItem(int rowNumber, juce::Graphics& g, int width, int height, bool rowIsSelected) override;
        void listBoxItemClicked(int row, const juce::MouseEvent& e) override;
        void listBoxItemDoubleClicked(int row, const juce::MouseEvent& e) override;
    private:
        PresetBrowserOverlay& owner;
    };

    SampleListModel sampleListModel{ *this };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PresetBrowserOverlay)
};
