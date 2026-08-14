#pragma once

#include <JuceHeader.h>
#include "PresetManager.h"
#include "SpectralUILookAndFeel.h"

#include "HistoryManager.h"

class PresetBrowserOverlay : public juce::Component, public juce::ListBoxModel
{
public:
    PresetBrowserOverlay(PresetManager& manager, juce::AudioProcessorValueTreeState& apvts, HistoryManager* historyMgr = nullptr);
    ~PresetBrowserOverlay() override;

    void paint(juce::Graphics& g) override;
    void resized() override;

    void refreshBankList();
    void refreshPresetList();
    void refreshSampleList();
    void refreshHistoryList();

    // ListBoxModel interface for Presets
    int getNumRows() override;
    void paintListBoxItem(int rowNumber, juce::Graphics& g, int width, int height, bool rowIsSelected) override;
    void listBoxItemClicked(int row, const juce::MouseEvent& e) override;
    void listBoxItemDoubleClicked(int row, const juce::MouseEvent& e) override;

    std::function<void(const juce::File& presetFile, const juce::String& sampleFileName)> onPresetSelected;
    std::function<void(const juce::File& sampleFile)> onSampleSelected;
    std::function<void(const HistoryEntry& entry)> onHistoryEntryRestored;
    std::function<void()> onClose;

    void setCurrentlyLoadedSampleName(const juce::String& name) { currentSampleName = name; }
    void bindSpectrogramComponent(class SpectrogramComponent* comp) { spectrogram = comp; }
    void bindHistoryManager(HistoryManager* hm) { historyManager = hm; refreshHistoryList(); }
    void syncActivePresetFromProcessor(const juce::String& loadedPresetName);
    void clearActivePresetSelection();
    juce::File getActivePresetFile() const { return activePresetFile; }

private:
    PresetManager& presetManager;
    juce::AudioProcessorValueTreeState& apvts;
    HistoryManager* historyManager = nullptr;

    // Header Components
    juce::TextButton closeButton{ "X" };

    // Column 1: Presets & Bank & Search
    juce::ComboBox bankSelector;
    juce::TextButton bankActionsBtn{ "BANK..." };

    juce::TextEditor searchBox;
    juce::ComboBox sortSelector;
    juce::TextButton favoriteFilterBtn;

    juce::ListBox presetListBox;

    juce::TextButton filterAllBtn{ "ALL" };
    juce::TextButton filterSynthBtn{ "SYNTH" };
    juce::TextButton filterLeadBtn{ "LEAD" };
    juce::TextButton filterBassBtn{ "BASS" };
    juce::TextButton filterPadBtn{ "PAD" };
    juce::TextButton filterFxBtn{ "FX" };
    juce::TextButton filterStatesBtn{ "STATES" };

    class CategoryBarContainer : public juce::Component
    {
    public:
        CategoryBarContainer(PresetBrowserOverlay& owner) : owner(owner) {}
        void resized() override;
        void mouseWheelMove(const juce::MouseEvent& e, const juce::MouseWheelDetails& wheel) override;
    private:
        PresetBrowserOverlay& owner;
    };

    CategoryBarContainer categoryContainer{ *this };
    juce::Viewport categoryViewport;

    juce::String activeCategoryFilter = "ALL";
    juce::String activeBankFilter = "ALL BANKS";
    bool onlyFavoritesFilter = false;
    int activeSortMode = 0; // 0: A-Z, 1: Favorites First, 2: Recently Used

    // Column 2: Preset Details & Actions
    juce::Label statusLabel;
    juce::Label selectedPresetTitle;
    juce::Label selectedPresetDetails;

    juce::TextButton loadPresetBtn{ "LOAD PRESET" };
    juce::TextButton deletePresetBtn{ "DELETE PRESET" };

    juce::TextEditor saveNameInput;
    juce::ComboBox saveCategoryInput;
    juce::ComboBox saveBankSelector;
    juce::TextButton savePresetBtn{ "SAVE CURRENT PRESET" };

    bool confirmDeletePresetPending = false;

    // Column 3: Sample Storage & Edit History
    juce::TextButton sampleStorageTabBtn{ "SAMPLES" };
    juce::TextButton historyTabBtn{ "EDIT HISTORY (10)" };
    int activeSampleStorageTab = 0; // 0 = Samples, 1 = History

    juce::ListBox sampleListBox;
    juce::TextButton importSampleBtn{ "IMPORT SAMPLE FILE" };
    juce::TextButton loadSampleToEngineBtn{ "LOAD SAMPLE" };
    juce::TextButton renameSampleBtn{ "RENAME SAMPLE" };
    juce::TextButton deleteSampleBtn{ "DELETE SAMPLE" };
    bool confirmDeleteSamplePending = false;

    juce::ListBox historyListBox;
    juce::TextButton restoreHistoryBtn{ "RESTORE SELECTED STATE" };
    juce::TextButton clearHistoryBtn{ "CLEAR HISTORY" };

    // Data lists
    juce::Array<PresetInfo> allPresets;
    juce::Array<PresetInfo> filteredPresets;
    juce::Array<juce::File> allSamples;
    juce::Array<HistoryEntry> allHistoryEntries;

    int selectedPresetIndex = -1;
    int selectedSampleIndex = -1;
    int selectedHistoryIndex = -1;

    juce::File activePresetFile;
    juce::String activeLoadedPresetName;
    juce::String currentSampleName;
    class SpectrogramComponent* spectrogram = nullptr;
    std::unique_ptr<juce::FileChooser> fileChooser;

    void filterPresets();
    void updateSelectedPresetDetails();
    void executeLoadSelectedPreset();
    void executeRestoreSelectedHistory();
    void showBankActionsMenu();
    void showRenameSampleDialog(int sampleRow);

    class SampleListModel : public juce::ListBoxModel
    {
    public:
        SampleListModel(PresetBrowserOverlay& owner) : owner(owner) {}
        int getNumRows() override { return owner.allSamples.size(); }
        void paintListBoxItem(int rowNumber, juce::Graphics& g, int width, int height, bool rowIsSelected) override;
        void listBoxItemClicked(int row, const juce::MouseEvent& e) override;
        void listBoxItemDoubleClicked(int row, const juce::MouseEvent& e) override;
        juce::var getDragSourceDescription(const juce::SparseSet<int>& selectedRows) override;
    private:
        PresetBrowserOverlay& owner;
    };

    class HistoryListModel : public juce::ListBoxModel
    {
    public:
        HistoryListModel(PresetBrowserOverlay& owner) : owner(owner) {}
        int getNumRows() override { return owner.allHistoryEntries.size(); }
        void paintListBoxItem(int rowNumber, juce::Graphics& g, int width, int height, bool rowIsSelected) override;
        void listBoxItemClicked(int row, const juce::MouseEvent& e) override;
        void listBoxItemDoubleClicked(int row, const juce::MouseEvent& e) override;
    private:
        PresetBrowserOverlay& owner;
    };

    SampleListModel sampleListModel{ *this };
    HistoryListModel historyListModel{ *this };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PresetBrowserOverlay)
};
