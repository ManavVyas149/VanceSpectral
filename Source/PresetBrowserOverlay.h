#pragma once

#include <JuceHeader.h>
#include "PresetManager.h"
#include "HistoryManager.h"
#include "SpectralUILookAndFeel.h"

class PresetBrowserOverlay : public juce::Component,
                             public juce::ListBoxModel,
                             public juce::FileDragAndDropTarget
{
public:
    PresetBrowserOverlay(PresetManager& manager, juce::AudioProcessorValueTreeState& apvts, HistoryManager* historyMgr = nullptr);
    ~PresetBrowserOverlay() override;

    void paint(juce::Graphics& g) override;
    void paintOverChildren(juce::Graphics& g) override;
    void resized() override;
    void visibilityChanged() override;
    bool keyPressed(const juce::KeyPress& key) override;

    void dismissActiveDialog();
    juce::AlertWindow* getActiveAlertWindow() const { return activeAlertWindow.getComponent(); }
    void executeDeleteCurrentSelection();
    void showNewBankDialog();
    void showDeleteBankDialog(const juce::String& bankName);
    void showBankCardContextMenu(const juce::String& bankName);
    class HistoryIconButton : public juce::Button
    {
    public:
        HistoryIconButton() : juce::Button("History")
        {
            setTooltip("History snapshots");
        }

        void paintButton(juce::Graphics& g, bool isHighlighted, bool isDown) override
        {
            auto bounds = getLocalBounds().toFloat().reduced(0.5f);
            float cornerRadius = 4.0f;

            bool isToggled = getToggleState();
            juce::Colour fillColour = isToggled ? SpectralUILookAndFeel::accentColour.withAlpha(0.15f)
                                                : (isDown ? juce::Colour(0xE4, 0xE4, 0xE7)
                                                          : (isHighlighted ? juce::Colour(0xF4, 0xF4, 0xF5)
                                                                           : juce::Colours::white));
            juce::Colour borderColour = isToggled ? SpectralUILookAndFeel::accentColour
                                                  : (isHighlighted ? juce::Colour(0x8B, 0x5C, 0xF6).withAlpha(0.6f)
                                                                   : juce::Colour(0xE4, 0xE4, 0xE7));

            g.setColour(fillColour);
            g.fillRoundedRectangle(bounds, cornerRadius);
            g.setColour(borderColour);
            g.drawRoundedRectangle(bounds, cornerRadius, 1.0f);

            float cx = bounds.getCentreX();
            float cy = bounds.getCentreY();
            float r = 6.0f;

            juce::Colour iconColour = isToggled ? SpectralUILookAndFeel::accentColour
                                                : (isHighlighted ? SpectralUILookAndFeel::textMainColour
                                                                 : juce::Colour(0x71, 0x71, 0x7A));
            g.setColour(iconColour);
            g.drawEllipse(cx - r, cy - r, r * 2.0f, r * 2.0f, 1.3f);

            juce::Path hands;
            hands.startNewSubPath(cx, cy - r + 2.0f);
            hands.lineTo(cx, cy);
            hands.lineTo(cx + r - 2.5f, cy);
            g.strokePath(hands, juce::PathStrokeType(1.3f, juce::PathStrokeType::mitered, juce::PathStrokeType::square));
        }
    };

    class HistoryFlyoutComponent : public juce::Component
    {
    public:
        HistoryFlyoutComponent(PresetBrowserOverlay& owner);
        void setupChildren();
        void paint(juce::Graphics& g) override;
        void resized() override;
        void mouseDown(const juce::MouseEvent&) override {}
    private:
        PresetBrowserOverlay& owner;
        juce::Label titleLabel;
        juce::TextButton closeBtn{ juce::String::fromUTF8("\xe2\x9c\x95") };
    };

    class FlyoutBackdropComponent : public juce::Component
    {
    public:
        FlyoutBackdropComponent(std::function<void()> onDismiss) : dismissCallback(onDismiss)
        {
            setInterceptsMouseClicks(true, false);
        }
        void mouseDown(const juce::MouseEvent&) override
        {
            if (dismissCallback)
                dismissCallback();
        }
    private:
        std::function<void()> dismissCallback;
    };

    void toggleHistoryFlyout();
    void hideHistoryFlyout();
    bool isHistoryFlyoutVisible() const;
    HistoryIconButton& getHistoryButton() { return historyButton; }
    HistoryFlyoutComponent& getHistoryFlyout() { return historyFlyout; }
    int getSampleCount() const { return allSamples.size(); }
    juce::File getSampleAt(int index) const { return (index >= 0 && index < allSamples.size()) ? allSamples[index] : juce::File(); }
    void selectSampleRow(int row);

    // Drag and drop audio import
    bool isInterestedInFileDrag(const juce::StringArray& files) override;
    void filesDropped(const juce::StringArray& files, int x, int y) override;

    void refreshBankList();
    void refreshPresetList();
    void refreshSampleList();
    void refreshHistoryList();

    // ListBoxModel for Presets (Panel 1)
    int getNumRows() override;
    void paintListBoxItem(int rowNumber, juce::Graphics& g, int width, int height, bool rowIsSelected) override;
    void listBoxItemClicked(int row, const juce::MouseEvent& e) override;
    void listBoxItemDoubleClicked(int row, const juce::MouseEvent& e) override;

    std::function<void(const juce::File& presetFile, const juce::String& sampleFileName)> onPresetSelected;
    std::function<void(const juce::String& bankName)> onBankSelected;
    std::function<void(const juce::File& sampleFile)> onSampleSelected;
    std::function<void(const HistoryEntry& entry)> onHistoryEntryRestored;
    std::function<void()> onClose;
    std::function<void()> onFilterOrSortChanged;

    void setCurrentlyLoadedSampleName(const juce::String& name) { currentSampleName = name; }
    void bindSpectrogramComponent(class SpectrogramComponent* comp) { spectrogram = comp; }
    void bindHistoryManager(HistoryManager* hm) { historyManager = hm; refreshHistoryList(); }

    void syncActivePresetFromProcessor(const juce::String& loadedPresetName);
    void clearActivePresetSelection();
    juce::File getActivePresetFile() const { return activePresetFile; }

    void executeShuffleFx();
    void updateShuffleFxButtonState();
    juce::Array<PresetInfo> getNavigablePresetsForBank(const juce::String& bankName) const;
    void checkForExternalLibraryChangesAsync();

private:
    class ModernBrowserLookAndFeel : public SpectralUILookAndFeel
    {
    public:
        ModernBrowserLookAndFeel();
        ~ModernBrowserLookAndFeel() override = default;

        juce::Font getTextButtonFont(juce::TextButton& button, int buttonHeight) override;
        void drawButtonBackground(juce::Graphics& g, juce::Button& button,
                                  const juce::Colour& backgroundColour,
                                  bool shouldDrawButtonAsHighlighted,
                                  bool shouldDrawButtonAsDown) override;
        void drawButtonText(juce::Graphics& g, juce::TextButton& button,
                            bool shouldDrawButtonAsHighlighted,
                            bool shouldDrawButtonAsDown) override;
        void drawComboBox(juce::Graphics& g, int width, int height, bool isButtonDown,
                          int buttonX, int buttonY, int buttonW, int buttonH,
                          juce::ComboBox& box) override;
        juce::Font getComboBoxFont(juce::ComboBox& box) override;
        void positionComboBoxText(juce::ComboBox& box, juce::Label& label) override;
        juce::Font getLabelFont(juce::Label& label) override;
        void drawScrollbar(juce::Graphics& g, juce::ScrollBar& scrollbar,
                           int x, int y, int width, int height,
                           bool isScrollbarVertical, int thumbStartPosition,
                           int thumbSize, bool isMouseOver, bool isMouseDown) override;
    };

    ModernBrowserLookAndFeel modernLookAndFeel;

    PresetManager& presetManager;
    juce::AudioProcessorValueTreeState& apvts;
    HistoryManager* historyManager = nullptr;

    // Feature toggle: set to true to restore the 3-column Library panel
    static constexpr bool enableLibraryPanel = false;

    // Header Controls
    juce::TextButton closeButton{ juce::String::fromUTF8("\xe2\x9c\x95") }; // ✕
    HistoryIconButton historyButton;
    juce::Label libraryIndexedBadge;

    //==========================================================================
    // PANEL 1: PRESETS (Left Panel)
    //==========================================================================
    juce::Label presetsHeaderLabel;
    juce::Label presetsCountLabel;

    juce::TextEditor searchBox;
    juce::TextButton shuffleFxBtn{ juce::String::fromUTF8("\xe2\x87\x86 SHUFFLE FX") };
    juce::ComboBox sortSelector;
    juce::TextButton favoriteFilterBtn{ juce::String::fromUTF8("\xe2\x98\x85 FAVS") };

    // Category Filter Pills
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
    juce::ListBox presetListBox;

    //==========================================================================
    // PANEL 2: BANKS (Middle Panel)
    //==========================================================================
    juce::Label banksHeaderLabel;
    juce::Label banksCountLabel;
    juce::TextButton newBankBtn{ "+ NEW BANK" };
    juce::TextButton bankActionsBtn{ "ACTIONS..." };
    juce::ListBox bankListBox;

    //==========================================================================
    // PANEL 3: SAMPLES & HISTORY (Right Panel)
    //==========================================================================
    juce::Label panel3HeaderLabel;
    juce::TextButton samplesTabBtn{ "SAMPLES" };
    juce::TextButton historyTabBtn{ "HISTORY" };
    bool isHistoryViewActive = false;

    // Samples sub-panel
    juce::ComboBox sampleSortSelector;
    juce::ListBox sampleListBox;

    class DropImportZone : public juce::Component
    {
    public:
        DropImportZone(PresetBrowserOverlay& owner) : owner(owner) {}
        void paint(juce::Graphics& g) override;
        void mouseUp(const juce::MouseEvent& e) override;
    private:
        PresetBrowserOverlay& owner;
    };
    DropImportZone dropImportZone{ *this };

    // History sub-panel
    juce::ListBox historyListBox;
    juce::TextButton restoreHistoryBtn{ "RESTORE SELECTED" };
    juce::TextButton clearHistoryBtn{ "CLEAR ALL" };

    // History Flyout popover controls
    FlyoutBackdropComponent historyBackdrop{ [this]() { hideHistoryFlyout(); } };
    HistoryFlyoutComponent historyFlyout{ *this };

    //==========================================================================
    // FOOTER: Status & Actions Bar
    //==========================================================================
    juce::Label bottomStatusLabel;
    juce::TextButton revealFileBtn{ "REVEAL FILE" };
    juce::TextButton saveAsBtn{ "SAVE AS..." };
    juce::TextButton loadMainBtn{ juce::String::fromUTF8("LOAD PRESET \xe2\x9c\x93") };
    juce::TextButton deleteBtn{ "DELETE" };

    //==========================================================================
    // State Tracking
    //==========================================================================
    enum class SelectedViewType { None, Preset, Bank, Sample, History };
    SelectedViewType currentSelectionType = SelectedViewType::None;

    juce::Array<PresetInfo> allPresets;
    juce::Array<PresetInfo> filteredPresets;
    juce::StringArray allBanks;
    juce::Array<juce::File> allSamples;
    juce::Array<HistoryEntry> allHistoryEntries;

    int selectedPresetIndex = -1;
    int selectedBankIndex = 0; // 0 is "ALL BANKS"
    int selectedSampleIndex = -1;
    int selectedHistoryIndex = -1;

    juce::String activeCategoryFilter = "ALL";
    juce::String activeBankFilter = "ALL BANKS";
    bool onlyFavoritesFilter = false;
    int activeSortMode = 0; // 0: A-Z, 1: Favorites First, 2: Recently Used

    juce::File activePresetFile;
    juce::String activeBankName = "ALL BANKS";
    juce::File selectedSampleFile;
    juce::String activeLoadedPresetName;
    juce::String currentSampleName;

    juce::File lastShuffledPresetFile;
    juce::String lastShuffledBank;

    class SpectrogramComponent* spectrogram = nullptr;
    std::unique_ptr<juce::FileChooser> fileChooser;
    juce::Component::SafePointer<juce::AlertWindow> activeAlertWindow;

    void filterPresets();
    void updateBottomBar();
    void executeLoadCurrentSelection();
    void executeRestoreSelectedHistory();

    void showBankActionsMenu();
    void showRenameBankDialog(const juce::String& bankName);
    void showSavePresetDialog();
    void showRenameSampleDialog(int sampleRow);
    void promptDuplicateSampleImport(const juce::File& sourceFile, const juce::File& existingDest);

    // ListBoxModel for Banks (Panel 2)
    class BankListModel : public juce::ListBoxModel
    {
    public:
        BankListModel(PresetBrowserOverlay& owner) : owner(owner) {}
        int getNumRows() override { return owner.allBanks.size(); }
        void paintListBoxItem(int rowNumber, juce::Graphics& g, int width, int height, bool rowIsSelected) override;
        void listBoxItemClicked(int row, const juce::MouseEvent& e) override;
        void listBoxItemDoubleClicked(int row, const juce::MouseEvent& e) override;
    private:
        PresetBrowserOverlay& owner;
    };

    // ListBoxModel for Samples (Panel 3)
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

    // ListBoxModel for History (Panel 3)
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

    BankListModel bankListModel{ *this };
    SampleListModel sampleListModel{ *this };
    HistoryListModel historyListModel{ *this };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PresetBrowserOverlay)
};
