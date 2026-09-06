#pragma once

#include <JuceHeader.h>
#include "PresetManager.h"
#include "SpectralUILookAndFeel.h"

#include "HistoryManager.h"

// Simple two-level directory browser: ROOT FOLDER -> BANK (immediate subfolder) -> presets
// (files inside that bank folder). See CLAUDE.md "Presets / History" for the on-disk model.
class PresetBrowserOverlay : public juce::Component, public juce::ListBoxModel
{
public:
    PresetBrowserOverlay(PresetManager& manager, juce::AudioProcessorValueTreeState& apvts, HistoryManager* historyMgr = nullptr);
    ~PresetBrowserOverlay() override;

    void paint(juce::Graphics& g) override;
    void resized() override;
    void visibilityChanged() override;
    bool keyPressed(const juce::KeyPress& key) override;

    void refreshBankList();
    void refreshPresetList();
    void refreshSampleList();
    void refreshHistoryList();

    // ListBoxModel interface for the bank list (primary navigation)
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

    // Reachable from the toolbar/preset bar independently of this overlay's own UI.
    void executeShuffleFx();

private:
    enum class ViewMode { Banks, PresetsInBank, Samples, History };

    PresetManager& presetManager;
    juce::AudioProcessorValueTreeState& apvts;
    HistoryManager* historyManager = nullptr;

    ViewMode currentView = ViewMode::Banks;

    // Header
    juce::TextButton closeButton{ "X" };
    juce::Label rootPathLabel;
    juce::TextButton chooseRootBtn{ "Choose Folder..." };
    juce::TextButton samplesToggleBtn{ "SAMPLES" };
    juce::TextButton historyToggleBtn{ "HISTORY" };
    juce::Label statusLabel;

    // Bank list (top level)
    juce::ListBox bankListBox;
    juce::TextButton manageBanksBtn{ "Manage" };
    juce::Label emptyStateLabel;
    juce::StringArray currentBanks;

    // Preset list (inside a bank, one level down)
    juce::TextButton backToBanksBtn{ "< Back to Banks" };
    juce::Label currentBankLabel;
    juce::ListBox presetListBox;
    juce::Label selectedPresetTitle;
    juce::TextButton loadPresetBtn{ "LOAD PRESET" };
    juce::TextButton deletePresetBtn{ "DELETE PRESET" };
    bool confirmDeletePresetPending = false;

    // Compact secondary "save current preset" action (saves into the open bank)
    juce::TextEditor saveNameInput;
    juce::TextButton savePresetBtn{ "SAVE CURRENT PRESET" };

    juce::String currentBankName;
    juce::Array<PresetInfo> currentBankPresets;
    juce::Array<PresetInfo> allPresets; // all presets under the root, used by Shuffle FX only
    int selectedPresetIndex = -1;

    juce::File lastShuffledPresetFile;

    // Sample Storage & Edit History (secondary panels, off the primary bank tree)
    juce::ListBox sampleListBox;
    juce::TextButton importSampleBtn{ "IMPORT SAMPLE FILE" };
    juce::TextButton loadSampleToEngineBtn{ "LOAD SAMPLE" };
    juce::TextButton renameSampleBtn{ "RENAME SAMPLE" };
    juce::TextButton deleteSampleBtn{ "DELETE SAMPLE" };
    bool confirmDeleteSamplePending = false;

    juce::ListBox historyListBox;
    juce::TextButton restoreHistoryBtn{ "RESTORE SELECTED STATE" };
    juce::TextButton clearHistoryBtn{ "CLEAR HISTORY" };

    juce::Array<juce::File> allSamples;
    juce::Array<HistoryEntry> allHistoryEntries;
    int selectedSampleIndex = -1;
    int selectedHistoryIndex = -1;

    juce::File activePresetFile;
    juce::String activeLoadedPresetName;
    juce::String currentSampleName;
    class SpectrogramComponent* spectrogram = nullptr;
    std::unique_ptr<juce::FileChooser> fileChooser;

    void showView(ViewMode mode);
    void openBank(const juce::String& bankName);
    void updateSelectedPresetDetails();
    void executeLoadSelectedPreset();
    void executeRestoreSelectedHistory();
    void showBankActionsMenu();
    void showRenameSampleDialog(int sampleRow);
    void updateRootPathLabel();
    void updateEmptyStateVisibility();

    class PresetListModel : public juce::ListBoxModel
    {
    public:
        PresetListModel(PresetBrowserOverlay& owner) : owner(owner) {}
        int getNumRows() override { return owner.currentBankPresets.size(); }
        void paintListBoxItem(int rowNumber, juce::Graphics& g, int width, int height, bool rowIsSelected) override;
        void listBoxItemClicked(int row, const juce::MouseEvent& e) override;
        void listBoxItemDoubleClicked(int row, const juce::MouseEvent& e) override;
    private:
        PresetBrowserOverlay& owner;
    };

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

    PresetListModel presetListModel{ *this };
    SampleListModel sampleListModel{ *this };
    HistoryListModel historyListModel{ *this };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PresetBrowserOverlay)
};
