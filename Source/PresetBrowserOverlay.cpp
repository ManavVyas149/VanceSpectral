#include "PresetBrowserOverlay.h"
#include "SpectrogramComponent.h"

PresetBrowserOverlay::PresetBrowserOverlay(PresetManager& manager, juce::AudioProcessorValueTreeState& state, HistoryManager* historyMgr)
    : presetManager(manager), apvts(state), historyManager(historyMgr)
{
    // Close Button
    addAndMakeVisible(closeButton);
    closeButton.setColour(juce::TextButton::buttonColourId, juce::Colour(200, 45, 55));
    closeButton.setColour(juce::TextButton::textColourOffId, juce::Colours::white);
    closeButton.onClick = [this]() {
        if (onClose) onClose();
        setVisible(false);
    };

    // Bank Selector & Actions Button
    addAndMakeVisible(bankSelector);
    bankSelector.setColour(juce::ComboBox::backgroundColourId, juce::Colour(0x18, 0x18, 0x22));
    bankSelector.setColour(juce::ComboBox::textColourId, SpectralUILookAndFeel::accentColour);
    bankSelector.setColour(juce::ComboBox::outlineColourId, juce::Colour(0x2A, 0x2A, 0x36));
    bankSelector.onChange = [this]() {
        juce::String selected = bankSelector.getText();
        if (selected.isNotEmpty())
        {
            activeBankFilter = selected;
            filterPresets();
        }
    };

    addAndMakeVisible(bankActionsBtn);
    bankActionsBtn.setColour(juce::TextButton::buttonColourId, juce::Colour(0x24, 0x24, 0x30));
    bankActionsBtn.setColour(juce::TextButton::textColourOffId, SpectralUILookAndFeel::accentColour);
    bankActionsBtn.onClick = [this]() { showBankActionsMenu(); };

    // Search Box
    addAndMakeVisible(searchBox);
    searchBox.setTextToShowWhenEmpty("Search presets...", SpectralUILookAndFeel::textMutedColour);
    searchBox.setColour(juce::TextEditor::backgroundColourId, juce::Colour(0x16, 0x16, 0x1C));
    searchBox.setColour(juce::TextEditor::textColourId, juce::Colours::white);
    searchBox.setColour(juce::TextEditor::outlineColourId, juce::Colour(0x2A, 0x2A, 0x34));
    searchBox.setColour(juce::TextEditor::focusedOutlineColourId, SpectralUILookAndFeel::accentColour);
    searchBox.onTextChange = [this]() { filterPresets(); };

    // Sort Selector
    addAndMakeVisible(sortSelector);
    sortSelector.addItem("Sort: A to Z", 1);
    sortSelector.addItem("Sort: Favorites First", 2);
    sortSelector.addItem("Sort: Recently Used", 3);
    sortSelector.setSelectedId(1, juce::dontSendNotification);
    sortSelector.setColour(juce::ComboBox::backgroundColourId, juce::Colour(0x18, 0x18, 0x22));
    sortSelector.setColour(juce::ComboBox::textColourId, juce::Colour(0xD0, 0xCC, 0xC0));
    sortSelector.setColour(juce::ComboBox::outlineColourId, juce::Colour(0x2A, 0x2A, 0x36));
    sortSelector.onChange = [this]() {
        activeSortMode = sortSelector.getSelectedItemIndex();
        filterPresets();
    };

    // Favorites Filter Button
    addAndMakeVisible(favoriteFilterBtn);
    favoriteFilterBtn.setButtonText(juce::String::fromUTF8("\xe2\x98\x85 FAVS"));
    favoriteFilterBtn.setColour(juce::TextButton::buttonColourId, juce::Colour(0x22, 0x22, 0x2A));
    favoriteFilterBtn.setColour(juce::TextButton::textColourOffId, juce::Colour(0xA0, 0x9E, 0x96));
    favoriteFilterBtn.onClick = [this]() {
        onlyFavoritesFilter = !onlyFavoritesFilter;
        favoriteFilterBtn.setColour(juce::TextButton::buttonColourId, onlyFavoritesFilter ? SpectralUILookAndFeel::accentColour : juce::Colour(0x22, 0x22, 0x2A));
        favoriteFilterBtn.setColour(juce::TextButton::textColourOffId, onlyFavoritesFilter ? juce::Colours::black : juce::Colour(0xA0, 0x9E, 0x96));
        filterPresets();
    };

    // Filter Buttons (ALL, SYNTH, LEAD, BASS, PAD, FX, STATES) in Scrollable Viewport
    auto setupFilterBtn = [this](juce::TextButton& btn, const juce::String& cat) {
        categoryContainer.addAndMakeVisible(btn);
        bool isActive = (activeCategoryFilter == cat);
        btn.setColour(juce::TextButton::buttonColourId, isActive ? SpectralUILookAndFeel::accentColour : juce::Colour(0x22, 0x22, 0x2A));
        btn.setColour(juce::TextButton::textColourOffId, isActive ? juce::Colours::black : juce::Colour(0xA0, 0x9E, 0x96));
        btn.onClick = [this, cat]() {
            activeCategoryFilter = cat;
            auto updateCatBtn = [this](juce::TextButton& b, const juce::String& c) {
                bool active = (activeCategoryFilter == c);
                b.setColour(juce::TextButton::buttonColourId, active ? SpectralUILookAndFeel::accentColour : juce::Colour(0x22, 0x22, 0x2A));
                b.setColour(juce::TextButton::textColourOffId, active ? juce::Colours::black : juce::Colour(0xA0, 0x9E, 0x96));
            };
            updateCatBtn(filterAllBtn, "ALL");
            updateCatBtn(filterSynthBtn, "SYNTH");
            updateCatBtn(filterLeadBtn, "LEAD");
            updateCatBtn(filterBassBtn, "BASS");
            updateCatBtn(filterPadBtn, "PAD");
            updateCatBtn(filterFxBtn, "FX");
            updateCatBtn(filterStatesBtn, "STATES");
            filterPresets();
        };
    };

    setupFilterBtn(filterAllBtn, "ALL");
    setupFilterBtn(filterSynthBtn, "SYNTH");
    setupFilterBtn(filterLeadBtn, "LEAD");
    setupFilterBtn(filterBassBtn, "BASS");
    setupFilterBtn(filterPadBtn, "PAD");
    setupFilterBtn(filterFxBtn, "FX");
    setupFilterBtn(filterStatesBtn, "STATES");

    categoryViewport.setScrollBarsShown(false, false, false, false);
    categoryViewport.setViewedComponent(&categoryContainer, false);
    addAndMakeVisible(categoryViewport);

    // Presets ListBox
    addAndMakeVisible(presetListBox);
    presetListBox.setModel(this);
    presetListBox.setColour(juce::ListBox::backgroundColourId, juce::Colour(0x12, 0x12, 0x17));
    presetListBox.setColour(juce::ListBox::outlineColourId, juce::Colour(0x2A, 0x2A, 0x35));
    presetListBox.setRowHeight(40);

    // Selected Preset Header / Status Labels
    addAndMakeVisible(selectedPresetTitle);
    selectedPresetTitle.setText("No Preset Selected", juce::dontSendNotification);
    selectedPresetTitle.setFont(SpectralUILookAndFeel::getGeometricFont(15.0f, true));
    selectedPresetTitle.setColour(juce::Label::textColourId, SpectralUILookAndFeel::accentColour);

    addAndMakeVisible(statusLabel);
    statusLabel.setText("Click a preset to inspect or load", juce::dontSendNotification);
    statusLabel.setFont(SpectralUILookAndFeel::getGeometricFont(11.5f, false));
    statusLabel.setColour(juce::Label::textColourId, SpectralUILookAndFeel::textMutedColour);

    // Load & Delete Buttons
    addAndMakeVisible(loadPresetBtn);
    loadPresetBtn.setColour(juce::TextButton::buttonColourId, SpectralUILookAndFeel::accentColour);
    loadPresetBtn.setColour(juce::TextButton::textColourOffId, juce::Colours::black);
    loadPresetBtn.onClick = [this]() { executeLoadSelectedPreset(); };

    addAndMakeVisible(deletePresetBtn);
    deletePresetBtn.setColour(juce::TextButton::buttonColourId, juce::Colour(200, 45, 55));
    deletePresetBtn.setColour(juce::TextButton::textColourOffId, juce::Colours::white);
    deletePresetBtn.onClick = [this]() {
        int r = presetListBox.getSelectedRow();
        if (r >= 0 && r < filteredPresets.size())
        {
            auto preset = filteredPresets[r];
            if (preset.isFactory || preset.bank.equalsIgnoreCase("Factory"))
            {
                statusLabel.setText("Factory presets cannot be deleted!", juce::dontSendNotification);
                return;
            }

            if (!confirmDeletePresetPending)
            {
                confirmDeletePresetPending = true;
                deletePresetBtn.setButtonText("CONFIRM DELETE?");
                statusLabel.setText("Click DELETE PRESET again to confirm deletion of '" + preset.name + "'", juce::dontSendNotification);
                return;
            }

            confirmDeletePresetPending = false;
            deletePresetBtn.setButtonText("DELETE PRESET");
            if (presetManager.deletePreset(preset.file))
            {
                statusLabel.setText("Deleted preset: " + preset.name, juce::dontSendNotification);
                refreshPresetList();
            }
        }
    };

    // Save Section Inputs
    addAndMakeVisible(saveNameInput);
    saveNameInput.setTextToShowWhenEmpty("New Preset Name (e.g. My Lead)", SpectralUILookAndFeel::textMutedColour);
    saveNameInput.setColour(juce::TextEditor::backgroundColourId, juce::Colour(0x16, 0x16, 0x1C));
    saveNameInput.setColour(juce::TextEditor::textColourId, juce::Colours::white);
    saveNameInput.setColour(juce::TextEditor::outlineColourId, juce::Colour(0x2A, 0x2A, 0x34));
    saveNameInput.setColour(juce::TextEditor::focusedOutlineColourId, SpectralUILookAndFeel::accentColour);

    addAndMakeVisible(saveCategoryInput);
    saveCategoryInput.setEditableText(true);
    saveCategoryInput.addItem("Synth", 1);
    saveCategoryInput.addItem("Lead", 2);
    saveCategoryInput.addItem("Bass", 3);
    saveCategoryInput.addItem("Pad", 4);
    saveCategoryInput.addItem("FX", 5);
    saveCategoryInput.addItem("States", 6);
    saveCategoryInput.setSelectedId(5, juce::dontSendNotification);
    saveCategoryInput.setColour(juce::ComboBox::backgroundColourId, juce::Colour(0x16, 0x16, 0x1C));
    saveCategoryInput.setColour(juce::ComboBox::textColourId, juce::Colours::white);
    saveCategoryInput.setColour(juce::ComboBox::arrowColourId, SpectralUILookAndFeel::accentColour);
    saveCategoryInput.setColour(juce::ComboBox::outlineColourId, juce::Colour(0x2A, 0x2A, 0x34));

    addAndMakeVisible(saveBankSelector);
    saveBankSelector.setEditableText(true);
    saveBankSelector.setColour(juce::ComboBox::backgroundColourId, juce::Colour(0x16, 0x16, 0x1C));
    saveBankSelector.setColour(juce::ComboBox::textColourId, juce::Colours::white);
    saveBankSelector.setColour(juce::ComboBox::arrowColourId, SpectralUILookAndFeel::accentColour);
    saveBankSelector.setColour(juce::ComboBox::outlineColourId, juce::Colour(0x2A, 0x2A, 0x34));

    addAndMakeVisible(savePresetBtn);
    savePresetBtn.setColour(juce::TextButton::buttonColourId, juce::Colour(0x2B, 0x8A, 0x5A)); // Emerald accent
    savePresetBtn.setColour(juce::TextButton::textColourOffId, juce::Colours::white);
    savePresetBtn.onClick = [this]() {
        juce::String name = saveNameInput.getText().trim();
        juce::String cat = saveCategoryInput.getText().trim();
        juce::String bank = saveBankSelector.getText().trim();

        if (name.isEmpty())
        {
            statusLabel.setText("Validation Error: Preset name cannot be empty!", juce::dontSendNotification);
            return;
        }

        if (bank.equalsIgnoreCase("Factory"))
        {
            statusLabel.setText("Validation Error: Factory bank is read-only! Please save to 'User' or a custom bank.", juce::dontSendNotification);
            return;
        }

        if (cat.isEmpty())
            cat = "FX";

        if (bank.isEmpty())
            bank = "User";

        auto executeSave = [this, name, cat, bank]() {
            float startReg = spectrogram ? spectrogram->getStartRegion() : 0.0f;
            float endReg = spectrogram ? spectrogram->getEndRegion() : 1.0f;
            juce::var selectionsVar = spectrogram ? spectrogram->getSelectionsAsVar() : juce::var();
            bool loopEnabled = spectrogram ? spectrogram->isLoopEnabled() : false;

            juce::String sampleFileToSave = currentSampleName;
            if (sampleFileToSave.isEmpty() && spectrogram && spectrogram->isFileLoaded())
                sampleFileToSave = spectrogram->getLoadedFile().getFileName();

            // Pass nullptr for sampleAudioBuffer: 'SAVE CURRENT PRESET' saves settings ONLY (no base64 audio bundled)
            if (presetManager.savePreset(name, cat, bank, sampleFileToSave, apvts, startReg, endReg, selectionsVar, false, loopEnabled, nullptr))
            {
                activeLoadedPresetName = name;
                saveNameInput.clear();
                statusLabel.setText("FX Preset '" + name + "' Saved to Bank '" + bank + "' Successfully!", juce::dontSendNotification);
                if (historyManager != nullptr)
                {
                    historyManager->pushHistoryState("Preset Saved: " + name, sampleFileToSave, apvts, startReg, endReg, loopEnabled, selectionsVar, juce::AudioBuffer<float>(), 44100.0);
                    refreshHistoryList();
                }
                refreshBankList();
                refreshPresetList();
            }
            else
            {
                auto* errDialog = new juce::AlertWindow("SAVE FAILED", "Failed to write preset file '" + name + ".vsfx' to disk in bank '" + bank + "'. Please check folder permissions.", juce::AlertWindow::WarningIcon);
                errDialog->addButton("OK", 1, juce::KeyPress(juce::KeyPress::returnKey));
                errDialog->enterModalState(true, nullptr, true);
                statusLabel.setText("Error: Failed to write preset file to disk!", juce::dontSendNotification);
            }
        };

        juce::String cleanBank = juce::File::createLegalFileName(bank);
        juce::String cleanName = juce::File::createLegalFileName(name);
        juce::File targetFile = presetManager.getPresetsFolder().getChildFile(cleanBank).getChildFile(cleanName + ".vsfx");

        if (targetFile.existsAsFile())
        {
            auto* confirmDialog = new juce::AlertWindow("PRESET ALREADY EXISTS", "A preset named '" + name + "' already exists in bank '" + bank + "'. Overwrite existing file on disk?", juce::AlertWindow::QuestionIcon);
            confirmDialog->addButton("Overwrite", 1, juce::KeyPress(juce::KeyPress::returnKey));
            confirmDialog->addButton("Cancel", 0, juce::KeyPress(juce::KeyPress::escapeKey));

            confirmDialog->enterModalState(true, juce::ModalCallbackFunction::create([executeSave](int button) {
                if (button == 1)
                {
                    executeSave();
                }
            }), true);
        }
        else
        {
            executeSave();
        }
    };

    // Column 3 Tabs (SAMPLES | EDIT HISTORY)
    auto updateTabButtons = [this]() {
        bool isSampleTab = (activeSampleStorageTab == 0);
        sampleStorageTabBtn.setColour(juce::TextButton::buttonColourId, isSampleTab ? SpectralUILookAndFeel::accentColour : juce::Colour(0x22, 0x22, 0x2A));
        sampleStorageTabBtn.setColour(juce::TextButton::textColourOffId, isSampleTab ? juce::Colours::black : juce::Colour(0xA0, 0x9E, 0x96));

        historyTabBtn.setColour(juce::TextButton::buttonColourId, !isSampleTab ? SpectralUILookAndFeel::accentColour : juce::Colour(0x22, 0x22, 0x2A));
        historyTabBtn.setColour(juce::TextButton::textColourOffId, !isSampleTab ? juce::Colours::black : juce::Colour(0xA0, 0x9E, 0x96));

        sampleListBox.setVisible(isSampleTab);
        importSampleBtn.setVisible(isSampleTab);
        loadSampleToEngineBtn.setVisible(isSampleTab);
        renameSampleBtn.setVisible(isSampleTab);
        deleteSampleBtn.setVisible(isSampleTab);

        historyListBox.setVisible(!isSampleTab);
        restoreHistoryBtn.setVisible(!isSampleTab);
        clearHistoryBtn.setVisible(!isSampleTab);
    };

    addAndMakeVisible(sampleStorageTabBtn);
    sampleStorageTabBtn.onClick = [this, updateTabButtons]() {
        activeSampleStorageTab = 0;
        updateTabButtons();
        resized();
    };

    addAndMakeVisible(historyTabBtn);
    historyTabBtn.onClick = [this, updateTabButtons]() {
        activeSampleStorageTab = 1;
        refreshHistoryList();
        updateTabButtons();
        resized();
    };

    // Sample ListBox
    addAndMakeVisible(sampleListBox);
    sampleListBox.setModel(&sampleListModel);
    sampleListBox.setColour(juce::ListBox::backgroundColourId, juce::Colour(0x12, 0x12, 0x17));
    sampleListBox.setColour(juce::ListBox::outlineColourId, juce::Colour(0x2A, 0x2A, 0x35));
    sampleListBox.setRowHeight(34);

    // Sample Action Buttons
    addAndMakeVisible(importSampleBtn);
    importSampleBtn.setColour(juce::TextButton::buttonColourId, juce::Colour(0x24, 0x24, 0x2E));
    importSampleBtn.setColour(juce::TextButton::textColourOffId, SpectralUILookAndFeel::accentColour);
    importSampleBtn.onClick = [this]() {
        if (fileChooser != nullptr)
            return;

        fileChooser = std::make_unique<juce::FileChooser>(
            "Select Audio Sample to Store in Library",
            juce::File::getSpecialLocation(juce::File::userHomeDirectory),
            "*.wav;*.mp3;*.flac;*.aiff;*.ogg;*.m4a");

        auto chooserFlags = juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles;
        fileChooser->launchAsync(chooserFlags, [this](const juce::FileChooser& fc) {
            auto result = fc.getResult();
            if (result.existsAsFile())
            {
                juce::File existingDuplicate = presetManager.findMatchingSample(result);

                if (existingDuplicate.existsAsFile())
                {
                    // Duplicate detected! Show confirmation dialog box
                    auto* dialog = new juce::AlertWindow(
                        "Duplicate Sample Detected",
                        "A sample named \"" + existingDuplicate.getFileName() + "\" already exists in your library. Replace it?",
                        juce::AlertWindow::QuestionIcon);

                    dialog->addButton("Replace", 1, juce::KeyPress(juce::KeyPress::returnKey));
                    dialog->addButton("Cancel", 0, juce::KeyPress(juce::KeyPress::escapeKey));

                    dialog->enterModalState(true, juce::ModalCallbackFunction::create([this, result, existingDuplicate](int button) {
                        if (button == 1) // Replace chosen
                        {
                            auto imported = presetManager.importSample(result, true, existingDuplicate); // true = overwrite in place, replace existingDuplicate
                            refreshSampleList();
                            if (imported.existsAsFile())
                            {
                                currentSampleName = imported.getFileName();
                                if (onSampleSelected)
                                    onSampleSelected(imported);
                                statusLabel.setText("Sample Replaced: " + imported.getFileName(), juce::dontSendNotification);
                            }
                        }
                        else // Cancel or dismissed
                        {
                            statusLabel.setText("Import cancelled. Original sample kept.", juce::dontSendNotification);
                        }
                    }), true);
                }
                else
                {
                    // Non-matching sample: import normally
                    auto imported = presetManager.importSample(result, false);
                    refreshSampleList();
                    if (imported.existsAsFile())
                    {
                        currentSampleName = imported.getFileName();
                        if (onSampleSelected)
                            onSampleSelected(imported);
                        statusLabel.setText("Sample Imported: " + imported.getFileName(), juce::dontSendNotification);
                    }
                }
            }
        });
    };

    addAndMakeVisible(loadSampleToEngineBtn);
    loadSampleToEngineBtn.setColour(juce::TextButton::buttonColourId, juce::Colour(0x24, 0x24, 0x2E));
    loadSampleToEngineBtn.setColour(juce::TextButton::textColourOffId, SpectralUILookAndFeel::accentColour);
    loadSampleToEngineBtn.onClick = [this]() {
        int r = sampleListBox.getSelectedRow();
        if (r >= 0 && r < allSamples.size())
        {
            auto sampleFile = allSamples[r];
            currentSampleName = sampleFile.getFileName();
            if (onSampleSelected)
                onSampleSelected(sampleFile);
            statusLabel.setText("Loaded Sample into Engine: " + sampleFile.getFileName(), juce::dontSendNotification);
        }
    };

    addAndMakeVisible(renameSampleBtn);
    renameSampleBtn.setColour(juce::TextButton::buttonColourId, juce::Colour(0x24, 0x24, 0x2E));
    renameSampleBtn.setColour(juce::TextButton::textColourOffId, juce::Colour(0x38, 0xBD, 0xF8));
    renameSampleBtn.onClick = [this]() {
        int r = sampleListBox.getSelectedRow();
        if (r >= 0 && r < allSamples.size())
        {
            showRenameSampleDialog(r);
        }
    };

    addAndMakeVisible(deleteSampleBtn);
    deleteSampleBtn.setColour(juce::TextButton::buttonColourId, juce::Colour(200, 45, 55));
    deleteSampleBtn.setColour(juce::TextButton::textColourOffId, juce::Colours::white);
    deleteSampleBtn.onClick = [this]() {
        int r = sampleListBox.getSelectedRow();
        if (r >= 0 && r < allSamples.size())
        {
            auto sampleFile = allSamples[r];
            if (!confirmDeleteSamplePending)
            {
                confirmDeleteSamplePending = true;
                deleteSampleBtn.setButtonText("CONFIRM?");
                statusLabel.setText("Click DELETE SAMPLE again to delete '" + sampleFile.getFileName() + "'", juce::dontSendNotification);
                return;
            }

            confirmDeleteSamplePending = false;
            deleteSampleBtn.setButtonText("DELETE SAMPLE");
            if (presetManager.deleteSample(sampleFile))
            {
                statusLabel.setText("Deleted sample: " + sampleFile.getFileName(), juce::dontSendNotification);
                refreshSampleList();
            }
        }
    };

    // History ListBox & Action Buttons
    addAndMakeVisible(historyListBox);
    historyListBox.setModel(&historyListModel);
    historyListBox.setColour(juce::ListBox::backgroundColourId, juce::Colour(0x12, 0x12, 0x17));
    historyListBox.setColour(juce::ListBox::outlineColourId, juce::Colour(0x2A, 0x2A, 0x35));
    historyListBox.setRowHeight(42);

    addAndMakeVisible(restoreHistoryBtn);
    restoreHistoryBtn.setColour(juce::TextButton::buttonColourId, juce::Colour(0x2B, 0x8A, 0x5A));
    restoreHistoryBtn.setColour(juce::TextButton::textColourOffId, juce::Colours::white);
    restoreHistoryBtn.onClick = [this]() { executeRestoreSelectedHistory(); };

    addAndMakeVisible(clearHistoryBtn);
    clearHistoryBtn.setColour(juce::TextButton::buttonColourId, juce::Colour(200, 45, 55));
    clearHistoryBtn.setColour(juce::TextButton::textColourOffId, juce::Colours::white);
    clearHistoryBtn.onClick = [this]() {
        if (historyManager != nullptr)
        {
            historyManager->clearHistory();
            refreshHistoryList();
            statusLabel.setText("Cleared edit history stack.", juce::dontSendNotification);
        }
    };

    updateTabButtons();

    refreshBankList();
    refreshPresetList();
    refreshSampleList();
    refreshHistoryList();
}

PresetBrowserOverlay::~PresetBrowserOverlay()
{
    presetListBox.setModel(nullptr);
    sampleListBox.setModel(nullptr);
    historyListBox.setModel(nullptr);
}

void PresetBrowserOverlay::refreshBankList()
{
    juce::String currentBankSel = bankSelector.getText();
    bankSelector.clear();
    saveBankSelector.clear();

    auto banks = presetManager.getAllBanks();
    bankSelector.addItem("ALL BANKS", 1);

    int id = 2;
    for (const auto& b : banks)
    {
        bankSelector.addItem(b, id);
        saveBankSelector.addItem(b, id - 1);
        id++;
    }

    if (currentBankSel.isNotEmpty())
    {
        for (int i = 0; i < bankSelector.getNumItems(); ++i)
        {
            if (bankSelector.getItemText(i).equalsIgnoreCase(currentBankSel))
            {
                bankSelector.setSelectedItemIndex(i, juce::dontSendNotification);
                break;
            }
        }
    }

    if (bankSelector.getSelectedId() <= 0)
    {
        bankSelector.setSelectedId(1, juce::dontSendNotification);
        activeBankFilter = "ALL BANKS";
    }

    if (saveBankSelector.getSelectedId() <= 0 && saveBankSelector.getNumItems() > 0)
    {
        saveBankSelector.setSelectedId(1, juce::dontSendNotification);
    }
}

void PresetBrowserOverlay::refreshPresetList()
{
    allPresets = presetManager.getAllPresets();
    filterPresets();
}

void PresetBrowserOverlay::refreshSampleList()
{
    allSamples = presetManager.getAllSamples();
    sampleListBox.updateContent();
    sampleListBox.repaint();
}

void PresetBrowserOverlay::syncActivePresetFromProcessor(const juce::String& loadedPresetName)
{
    activeLoadedPresetName = loadedPresetName;
    activePresetFile = juce::File();

    if (activeLoadedPresetName.isNotEmpty())
    {
        for (const auto& p : allPresets)
        {
            if (p.name.equalsIgnoreCase(activeLoadedPresetName))
            {
                activePresetFile = p.file;
                break;
            }
        }
    }
}

void PresetBrowserOverlay::filterPresets()
{
    filteredPresets.clear();
    juce::String query = searchBox.getText().trim().toLowerCase();

    for (const auto& p : allPresets)
    {
        bool matchesBank = (activeBankFilter == "ALL BANKS" || p.bank.equalsIgnoreCase(activeBankFilter));
        bool matchesCat = (activeCategoryFilter == "ALL" || p.category.equalsIgnoreCase(activeCategoryFilter));
        bool matchesFav = (!onlyFavoritesFilter || p.isFavorite);
        bool matchesQuery = (query.isEmpty() ||
                             p.name.toLowerCase().contains(query) ||
                             p.category.toLowerCase().contains(query) ||
                             p.bank.toLowerCase().contains(query));

        if (matchesBank && matchesCat && matchesFav && matchesQuery)
        {
            filteredPresets.add(p);
        }
    }

    // Sort presets
    if (activeSortMode == 0) // A to Z
    {
        std::sort(filteredPresets.begin(), filteredPresets.end(), [](const PresetInfo& a, const PresetInfo& b) {
            return a.name.toLowerCase() < b.name.toLowerCase();
        });
    }
    else if (activeSortMode == 1) // Favorites First
    {
        std::sort(filteredPresets.begin(), filteredPresets.end(), [](const PresetInfo& a, const PresetInfo& b) {
            if (a.isFavorite != b.isFavorite)
                return a.isFavorite > b.isFavorite;
            return a.name.toLowerCase() < b.name.toLowerCase();
        });
    }
    else if (activeSortMode == 2) // Recently Used
    {
        std::sort(filteredPresets.begin(), filteredPresets.end(), [](const PresetInfo& a, const PresetInfo& b) {
            return a.lastUsed > b.lastUsed;
        });
    }

    presetListBox.updateContent();

    int activeIndex = -1;
    if (activeLoadedPresetName.isNotEmpty())
    {
        for (int i = 0; i < filteredPresets.size(); ++i)
        {
            if (filteredPresets[i].name.equalsIgnoreCase(activeLoadedPresetName) ||
               (activePresetFile.existsAsFile() && filteredPresets[i].file == activePresetFile))
            {
                activeIndex = i;
                activePresetFile = filteredPresets[i].file;
                break;
            }
        }
    }

    if (activeIndex >= 0 && activeIndex < filteredPresets.size())
    {
        presetListBox.selectRow(activeIndex);
        selectedPresetIndex = activeIndex;
    }
    else
    {
        presetListBox.deselectAllRows();
        selectedPresetIndex = -1;
    }

    presetListBox.repaint();
    updateSelectedPresetDetails();
}

void PresetBrowserOverlay::clearActivePresetSelection()
{
    activePresetFile = juce::File();
    activeLoadedPresetName = "Custom / Unsaved";
    presetListBox.deselectAllRows();
    selectedPresetIndex = -1;
    updateSelectedPresetDetails();
    repaint();
}

void PresetBrowserOverlay::updateSelectedPresetDetails()
{
    confirmDeletePresetPending = false;
    deletePresetBtn.setButtonText("DELETE PRESET");

    int r = presetListBox.getSelectedRow();
    if (r >= 0 && r < filteredPresets.size())
    {
        auto preset = filteredPresets[r];
        selectedPresetTitle.setText(preset.name, juce::dontSendNotification);
        statusLabel.setText("Bank: " + preset.bank + " | Category: " + preset.category +
                           (preset.sampleFileName.isNotEmpty() ? " | Sample: " + preset.sampleFileName : ""),
                           juce::dontSendNotification);

        if (preset.isFactory || preset.bank.equalsIgnoreCase("Factory"))
        {
            deletePresetBtn.setEnabled(false);
            deletePresetBtn.setAlpha(0.4f);
        }
        else
        {
            deletePresetBtn.setEnabled(true);
            deletePresetBtn.setAlpha(1.0f);
        }

        // Auto-populate Save fields for easy editing
        saveNameInput.setText(preset.name, juce::dontSendNotification);
        saveCategoryInput.setText(preset.category, juce::dontSendNotification);
        saveBankSelector.setText(preset.bank, juce::dontSendNotification);
    }
    else
    {
        selectedPresetTitle.setText("No Preset Selected", juce::dontSendNotification);
        statusLabel.setText("Select a preset from the list", juce::dontSendNotification);
        deletePresetBtn.setEnabled(false);
        deletePresetBtn.setAlpha(0.4f);
    }
}

void PresetBrowserOverlay::refreshHistoryList()
{
    if (historyManager != nullptr)
    {
        allHistoryEntries = historyManager->getHistoryEntries();
    }
    else
    {
        allHistoryEntries.clear();
    }
    historyTabBtn.setButtonText("EDIT HISTORY (" + juce::String(allHistoryEntries.size()) + ")");
    historyListBox.updateContent();
    historyListBox.repaint();
}

void PresetBrowserOverlay::executeRestoreSelectedHistory()
{
    if (historyManager == nullptr)
        return;

    int r = historyListBox.getSelectedRow();
    if (r >= 0 && r < allHistoryEntries.size())
    {
        auto entry = allHistoryEntries[r];
        juce::String sampleFile;
        float startReg = 0.0f;
        float endReg = 1.0f;
        juce::var selectionsVar;
        bool loopEnabled = false;
        juce::AudioBuffer<float> audioBuf;
        double sr = 44100.0;

        if (historyManager->restoreHistoryEntry(entry, apvts, sampleFile, startReg, endReg, selectionsVar, loopEnabled, audioBuf, sr))
        {
            if (spectrogram != nullptr && audioBuf.getNumSamples() > 0)
            {
                spectrogram->loadDirectAudioBuffer(audioBuf, sr, sampleFile, loopEnabled);
                spectrogram->restorePresetSnapshot(startReg, endReg, selectionsVar);
            }
            if (onHistoryEntryRestored)
                onHistoryEntryRestored(entry);

            statusLabel.setText("Restored State from History: " + entry.label + " (" + entry.formattedTime + ")", juce::dontSendNotification);
            setVisible(false);
        }
    }
}

void PresetBrowserOverlay::executeLoadSelectedPreset()
{
    int r = presetListBox.getSelectedRow();
    if (r >= 0 && r < filteredPresets.size())
    {
        auto preset = filteredPresets[r];
        activePresetFile = preset.file;
        activeLoadedPresetName = preset.name;
        juce::String sampleFile;
        float startReg = 0.0f;
        float endReg = 1.0f;
        juce::var selectionsVar;
        bool loopEnabled = false;
        juce::AudioBuffer<float> loadedBuf;
        double loadedSr = 44100.0;

        if (presetManager.loadPreset(preset.file, apvts, sampleFile, startReg, endReg, selectionsVar, loopEnabled, &loadedBuf, &loadedSr))
        {
            if (spectrogram != nullptr && loadedBuf.getNumSamples() > 0)
            {
                spectrogram->loadDirectAudioBuffer(loadedBuf, loadedSr, sampleFile, loopEnabled);
                spectrogram->restorePresetSnapshot(startReg, endReg, selectionsVar);
            }
            if (onPresetSelected)
                onPresetSelected(preset.file, sampleFile);

            if (historyManager != nullptr)
            {
                historyManager->pushHistoryState("Preset Loaded: " + preset.name, sampleFile, apvts, startReg, endReg, loopEnabled, selectionsVar, loadedBuf, loadedSr);
                refreshHistoryList();
            }

            statusLabel.setText("Loaded Preset: " + preset.name, juce::dontSendNotification);
            setVisible(false);
        }
    }
}

void PresetBrowserOverlay::showBankActionsMenu()
{
    juce::PopupMenu menu;
    menu.addItem(1, "Create New Bank...");
    menu.addItem(2, "Rename Selected Bank...", activeBankFilter != "ALL BANKS" && !activeBankFilter.equalsIgnoreCase("Factory"));
    menu.addItem(3, "Delete Selected Bank", activeBankFilter != "ALL BANKS" && !activeBankFilter.equalsIgnoreCase("Factory") && !activeBankFilter.equalsIgnoreCase("User"));
    menu.addSeparator();
    menu.addItem(4, "Import Bank Package (.zip / folder)...");

    menu.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(bankActionsBtn), [this](int result) {
        if (result == 1) // Create New Bank
        {
            auto* dialog = new juce::AlertWindow("Create New Preset Bank", "Enter a name for the new Bank:", juce::AlertWindow::NoIcon);
            dialog->addTextEditor("bankName", "", "Bank Name");
            dialog->addButton("Create", 1, juce::KeyPress(juce::KeyPress::returnKey));
            dialog->addButton("Cancel", 0, juce::KeyPress(juce::KeyPress::escapeKey));

            dialog->enterModalState(true, juce::ModalCallbackFunction::create([this, dialog](int button) {
                if (button == 1)
                {
                    juce::String name = dialog->getTextEditorContents("bankName").trim();
                    if (presetManager.createBank(name))
                    {
                        refreshBankList();
                        activeBankFilter = name;
                        for (int i = 0; i < bankSelector.getNumItems(); ++i)
                        {
                            if (bankSelector.getItemText(i).equalsIgnoreCase(name))
                            {
                                bankSelector.setSelectedItemIndex(i, juce::dontSendNotification);
                                break;
                            }
                        }
                        filterPresets();
                        statusLabel.setText("Created Bank: " + name, juce::dontSendNotification);
                    }
                }
            }), true);
        }
        else if (result == 2) // Rename Selected Bank
        {
            auto* dialog = new juce::AlertWindow("Rename Bank", "Enter new name for Bank '" + activeBankFilter + "':", juce::AlertWindow::NoIcon);
            dialog->addTextEditor("bankName", activeBankFilter, "New Bank Name");
            dialog->addButton("Rename", 1, juce::KeyPress(juce::KeyPress::returnKey));
            dialog->addButton("Cancel", 0, juce::KeyPress(juce::KeyPress::escapeKey));

            dialog->enterModalState(true, juce::ModalCallbackFunction::create([this, dialog](int button) {
                if (button == 1)
                {
                    juce::String newName = dialog->getTextEditorContents("bankName").trim();
                    if (presetManager.renameBank(activeBankFilter, newName))
                    {
                        activeBankFilter = newName;
                        refreshBankList();
                        refreshPresetList();
                        statusLabel.setText("Renamed Bank to: " + newName, juce::dontSendNotification);
                    }
                }
            }), true);
        }
        else if (result == 3) // Delete Selected Bank
        {
            auto* dialog = new juce::AlertWindow("Delete Bank", "Are you sure you want to delete Bank '" + activeBankFilter + "' and all presets inside it?", juce::AlertWindow::WarningIcon);
            dialog->addButton("Delete", 1);
            dialog->addButton("Cancel", 0, juce::KeyPress(juce::KeyPress::escapeKey));

            dialog->enterModalState(true, juce::ModalCallbackFunction::create([this, dialog](int button) {
                if (button == 1)
                {
                    if (presetManager.deleteBank(activeBankFilter))
                    {
                        statusLabel.setText("Deleted Bank: " + activeBankFilter, juce::dontSendNotification);
                        activeBankFilter = "ALL BANKS";
                        refreshBankList();
                        refreshPresetList();
                    }
                }
            }), true);
        }
        else if (result == 4) // Import Bank Package
        {
            fileChooser = std::make_unique<juce::FileChooser>(
                "Select Preset Bank Package (.zip or Folder)",
                juce::File::getSpecialLocation(juce::File::userHomeDirectory),
                "*.zip");

            auto chooserFlags = juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles | juce::FileBrowserComponent::canSelectDirectories;
            fileChooser->launchAsync(chooserFlags, [this](const juce::FileChooser& fc) {
                auto resultFile = fc.getResult();
                if (resultFile.exists())
                {
                    if (presetManager.importBankPackage(resultFile))
                    {
                        refreshBankList();
                        refreshPresetList();
                        refreshSampleList();
                        statusLabel.setText("Imported Bank Package: " + resultFile.getFileName(), juce::dontSendNotification);
                    }
                }
            });
        }
    });
}

void PresetBrowserOverlay::showRenameSampleDialog(int sampleRow)
{
    if (sampleRow < 0 || sampleRow >= allSamples.size())
        return;

    auto sampleFile = allSamples[sampleRow];
    auto* dialog = new juce::AlertWindow("Rename Sample", "Enter new filename for sample:", juce::AlertWindow::NoIcon);
    dialog->addTextEditor("sampleName", sampleFile.getFileNameWithoutExtension(), "New Sample Name");
    dialog->addButton("Rename", 1, juce::KeyPress(juce::KeyPress::returnKey));
    dialog->addButton("Cancel", 0, juce::KeyPress(juce::KeyPress::escapeKey));

    dialog->enterModalState(true, juce::ModalCallbackFunction::create([this, sampleFile, dialog](int button) {
        if (button == 1)
        {
            juce::String newName = dialog->getTextEditorContents("sampleName").trim();
            if (presetManager.renameSample(sampleFile, newName))
            {
                refreshSampleList();
                statusLabel.setText("Renamed sample successfully!", juce::dontSendNotification);
            }
        }
    }), true);
}

int PresetBrowserOverlay::getNumRows()
{
    return filteredPresets.size();
}

void PresetBrowserOverlay::paintListBoxItem(int rowNumber, juce::Graphics& g, int width, int height, bool rowIsSelected)
{
    if (rowNumber < 0 || rowNumber >= filteredPresets.size())
        return;

    auto preset = filteredPresets[rowNumber];
    bool isCurrentlyLoaded = (activePresetFile.existsAsFile() && preset.file == activePresetFile)
                          || (activeLoadedPresetName.isNotEmpty() && preset.name.equalsIgnoreCase(activeLoadedPresetName));

    if (rowIsSelected && isCurrentlyLoaded)
    {
        // Currently Loaded in Engine AND Selected (Full Orange Active Highlight)
        g.setColour(SpectralUILookAndFeel::accentColour.withAlpha(0.22f));
        g.fillRoundedRectangle(2, 2, (float)width - 4, (float)height - 4, 4.0f);
        g.setColour(SpectralUILookAndFeel::accentColour);
        g.drawRoundedRectangle(2, 2, (float)width - 4, (float)height - 4, 4.0f, 1.5f);
    }
    else if (isCurrentlyLoaded)
    {
        // Loaded in Engine, but user is inspecting another preset in Column 2 (Amber Outline)
        g.setColour(SpectralUILookAndFeel::accentColour.withAlpha(0.12f));
        g.fillRoundedRectangle(2, 2, (float)width - 4, (float)height - 4, 4.0f);
        g.setColour(SpectralUILookAndFeel::accentColour.withAlpha(0.85f));
        g.drawRoundedRectangle(2, 2, (float)width - 4, (float)height - 4, 4.0f, 1.2f);
    }
    else if (rowIsSelected)
    {
        // Inspecting preset in Column 2 (Browsing selection fill - Slate/Dark Cyan)
        g.setColour(juce::Colour(0x28, 0x28, 0x36));
        g.fillRoundedRectangle(2, 2, (float)width - 4, (float)height - 4, 4.0f);
        g.setColour(juce::Colour(0x4A, 0x4A, 0x5D));
        g.drawRoundedRectangle(2, 2, (float)width - 4, (float)height - 4, 4.0f, 1.0f);
    }
    else
    {
        g.setColour(rowNumber % 2 == 0 ? juce::Colour(0x14, 0x14, 0x1A) : juce::Colour(0x18, 0x18, 0x20));
        g.fillRect(0, 0, width, height);
    }

    // Favorite Star (x: 8 to 28)
    g.setFont(SpectralUILookAndFeel::getGeometricFont(14.0f, true));
    g.setColour(preset.isFavorite ? juce::Colour(0xFF, 0xC1, 0x07) : juce::Colour(0x4A, 0x4A, 0x56));
    g.drawText(preset.isFavorite ? juce::String::fromUTF8("\xe2\x98\x85") : juce::String::fromUTF8("\xe2\x98\x86"), 8, 0, 20, height, juce::Justification::centred);

    // Preset Name & Bank Subtext
    g.setFont(SpectralUILookAndFeel::getGeometricFont(13.0f, true));
    g.setColour((rowIsSelected || isCurrentlyLoaded) ? juce::Colours::white : juce::Colour(0xE0, 0xDC, 0xD0));
    g.drawText(preset.name, 32, 2, width - 125, height / 2, juce::Justification::bottomLeft);

    g.setFont(SpectralUILookAndFeel::getMonospaceFont(9.5f));
    g.setColour(SpectralUILookAndFeel::textMutedColour);
    g.drawText("[" + preset.bank + "]", 32, height / 2, width - 125, height / 2 - 2, juce::Justification::topLeft);

    // Category Tag Badge Color Pill
    juce::Colour catBadgeCol(0x7E, 0x7B, 0x75);
    juce::String catUp = preset.category.toUpperCase();
    if (catUp == "SYNTH") catBadgeCol = SpectralUILookAndFeel::accentColour; // Amber
    else if (catUp == "LEAD") catBadgeCol = juce::Colour(0x38, 0xBD, 0xF8); // Sky Cyan
    else if (catUp == "BASS") catBadgeCol = juce::Colour(0x2D, 0xD4, 0xBF); // Emerald
    else if (catUp == "PAD") catBadgeCol = juce::Colour(0xA8, 0x55, 0xF7); // Purple
    else if (catUp == "FX") catBadgeCol = juce::Colour(0xF4, 0x3F, 0x5E);  // Rose
    else if (catUp == "STATES") catBadgeCol = juce::Colour(0x3B, 0x82, 0xF6); // Vibrant Blue

    g.setFont(SpectralUILookAndFeel::getMonospaceFont(9.5f));
    auto badgeArea = juce::Rectangle<float>((float)width - 85.0f, (float)height * 0.5f - 9.0f, 75.0f, 18.0f);
    g.setColour(catBadgeCol.withAlpha(0.15f));
    g.fillRoundedRectangle(badgeArea, 4.0f);
    g.setColour(catBadgeCol);
    g.drawRoundedRectangle(badgeArea, 4.0f, 1.0f);
    g.drawText(catUp, badgeArea, juce::Justification::centred, false);
}

void PresetBrowserOverlay::listBoxItemClicked(int row, const juce::MouseEvent& e)
{
    if (row < 0 || row >= filteredPresets.size())
        return;

    auto preset = filteredPresets[row];

    // Star icon click (x < 30)
    if (e.x < 30)
    {
        presetManager.toggleFavorite(preset.file);
        refreshPresetList();
        return;
    }

    // Tag badge pill click (x > width - 90)
    if (e.x > presetListBox.getWidth() - 90)
    {
        activeCategoryFilter = preset.category.toUpperCase();
        auto updateCatBtn = [this](juce::TextButton& b, const juce::String& c) {
            bool active = (activeCategoryFilter == c);
            b.setColour(juce::TextButton::buttonColourId, active ? SpectralUILookAndFeel::accentColour : juce::Colour(0x22, 0x22, 0x2A));
            b.setColour(juce::TextButton::textColourOffId, active ? juce::Colours::black : juce::Colour(0xA0, 0x9E, 0x96));
        };
        updateCatBtn(filterAllBtn, "ALL");
        updateCatBtn(filterSynthBtn, "SYNTH");
        updateCatBtn(filterLeadBtn, "LEAD");
        updateCatBtn(filterBassBtn, "BASS");
        updateCatBtn(filterPadBtn, "PAD");
        updateCatBtn(filterFxBtn, "FX");
        updateCatBtn(filterStatesBtn, "STATES");
        filterPresets();
        return;
    }

    selectedPresetIndex = row;
    updateSelectedPresetDetails();
}

void PresetBrowserOverlay::listBoxItemDoubleClicked(int row, const juce::MouseEvent&)
{
    selectedPresetIndex = row;
    executeLoadSelectedPreset();
}

void PresetBrowserOverlay::SampleListModel::paintListBoxItem(int rowNumber, juce::Graphics& g, int width, int height, bool rowIsSelected)
{
    if (rowNumber < 0 || rowNumber >= owner.allSamples.size())
        return;

    auto sample = owner.allSamples[rowNumber];

    if (rowIsSelected)
    {
        g.setColour(SpectralUILookAndFeel::accentColour.withAlpha(0.18f));
        g.fillRoundedRectangle(2, 2, (float)width - 4, (float)height - 4, 4.0f);
        g.setColour(SpectralUILookAndFeel::accentColour);
        g.drawRoundedRectangle(2, 2, (float)width - 4, (float)height - 4, 4.0f, 1.2f);
    }
    else
    {
        g.setColour(rowNumber % 2 == 0 ? juce::Colour(0x14, 0x14, 0x1A) : juce::Colour(0x18, 0x18, 0x20));
        g.fillRect(0, 0, width, height);
    }

    g.setFont(SpectralUILookAndFeel::getGeometricFont(12.0f, false));
    g.setColour(rowIsSelected ? juce::Colours::white : juce::Colour(0xD0, 0xCC, 0xC0));
    g.drawText(sample.getFileName(), 10, 0, width - 20, height, juce::Justification::centredLeft);
}

void PresetBrowserOverlay::SampleListModel::listBoxItemClicked(int row, const juce::MouseEvent&)
{
    owner.selectedSampleIndex = row;
    owner.confirmDeleteSamplePending = false;
    owner.deleteSampleBtn.setButtonText("DELETE SAMPLE");
}

void PresetBrowserOverlay::SampleListModel::listBoxItemDoubleClicked(int row, const juce::MouseEvent&)
{
    owner.selectedSampleIndex = row;
    if (row >= 0 && row < owner.allSamples.size())
    {
        auto sampleFile = owner.allSamples[row];
        owner.currentSampleName = sampleFile.getFileName();
        if (owner.onSampleSelected)
            owner.onSampleSelected(sampleFile);
        owner.statusLabel.setText("Loaded Sample into Engine: " + sampleFile.getFileName(), juce::dontSendNotification);
    }
}

juce::var PresetBrowserOverlay::SampleListModel::getDragSourceDescription(const juce::SparseSet<int>& selectedRows)
{
    if (selectedRows.size() > 0)
    {
        int row = selectedRows[0];
        if (row >= 0 && row < owner.allSamples.size())
        {
            auto sampleFile = owner.allSamples[row];
            if (sampleFile.existsAsFile())
            {
                if (auto* dragContainer = juce::DragAndDropContainer::findParentDragContainerFor(&owner.sampleListBox))
                {
                    juce::StringArray files;
                    files.add(sampleFile.getFullPathName());
                    dragContainer->performExternalDragDropOfFiles(files, false);
                }
            }
        }
    }
    return {};
}

void PresetBrowserOverlay::HistoryListModel::paintListBoxItem(int rowNumber, juce::Graphics& g, int width, int height, bool rowIsSelected)
{
    if (rowNumber < 0 || rowNumber >= owner.allHistoryEntries.size())
        return;

    auto entry = owner.allHistoryEntries[rowNumber];

    if (rowIsSelected)
    {
        g.setColour(SpectralUILookAndFeel::accentColour.withAlpha(0.18f));
        g.fillRoundedRectangle(2, 2, (float)width - 4, (float)height - 4, 4.0f);
        g.setColour(SpectralUILookAndFeel::accentColour);
        g.drawRoundedRectangle(2, 2, (float)width - 4, (float)height - 4, 4.0f, 1.2f);
    }
    else
    {
        g.setColour(rowNumber % 2 == 0 ? juce::Colour(0x14, 0x14, 0x1A) : juce::Colour(0x18, 0x18, 0x20));
        g.fillRect(0, 0, width, height);
    }

    g.setFont(SpectralUILookAndFeel::getGeometricFont(12.0f, true));
    g.setColour(rowIsSelected ? juce::Colours::white : juce::Colour(0xE0, 0xDC, 0xD0));
    g.drawText(entry.label, 10, 3, width - 20, height / 2, juce::Justification::bottomLeft);

    g.setFont(SpectralUILookAndFeel::getMonospaceFont(9.5f));
    g.setColour(SpectralUILookAndFeel::accentColour);
    g.drawText(entry.formattedTime + (entry.sampleFileName.isNotEmpty() ? " | " + entry.sampleFileName : ""), 10, height / 2, width - 20, height / 2 - 3, juce::Justification::topLeft);
}

void PresetBrowserOverlay::HistoryListModel::listBoxItemClicked(int row, const juce::MouseEvent&)
{
    owner.selectedHistoryIndex = row;
}

void PresetBrowserOverlay::HistoryListModel::listBoxItemDoubleClicked(int row, const juce::MouseEvent&)
{
    owner.selectedHistoryIndex = row;
    owner.executeRestoreSelectedHistory();
}

void PresetBrowserOverlay::paint(juce::Graphics& g)
{
    // Full screen dark translucent overlay backdrop
    g.fillAll(juce::Colour(0x0A, 0x0A, 0x0E).withAlpha(0.95f));

    // Main Modal Box
    auto mainBounds = getLocalBounds().reduced(20).toFloat();
    g.setColour(juce::Colour(0x14, 0x14, 0x1A));
    g.fillRoundedRectangle(mainBounds, 12.0f);

    g.setColour(juce::Colour(0x2A, 0x2A, 0x36));
    g.drawRoundedRectangle(mainBounds, 12.0f, 1.5f);

    // Title Bar Area (Header)
    auto headerArea = mainBounds.removeFromTop(50.0f);
    g.setColour(juce::Colour(0x1B, 0x1B, 0x24));
    g.fillRoundedRectangle(headerArea.reduced(2.0f, 2.0f), 10.0f);

    g.setFont(SpectralUILookAndFeel::getMonospaceFont(15.0f));
    g.setColour(SpectralUILookAndFeel::accentColour);
    g.drawText("PRESET BROWSER & SAMPLE LIBRARY", headerArea.reduced(20.0f, 0.0f), juce::Justification::left, false);

    // Sub-Card Background Panels
    auto area = getLocalBounds().reduced(32);
    area.removeFromTop(45); // Space below header

    int gap = 16;
    int colW = (area.getWidth() - (gap * 2)) / 3;

    auto col1Bounds = area.removeFromLeft(colW).toFloat();
    area.removeFromLeft(gap);
    auto col2Bounds = area.removeFromLeft(colW).toFloat();
    area.removeFromLeft(gap);
    auto col3Bounds = area.toFloat();

    auto drawCardPanel = [&](juce::Rectangle<float> cardBounds, const juce::String& cardTitle) {
        g.setColour(juce::Colour(0x18, 0x18, 0x20));
        g.fillRoundedRectangle(cardBounds, 8.0f);

        g.setColour(juce::Colour(0x2C, 0x2C, 0x3A));
        g.drawRoundedRectangle(cardBounds, 8.0f, 1.2f);

        // Header strip inside card
        auto titleStrip = cardBounds.removeFromTop(36.0f);
        g.setColour(juce::Colour(0x20, 0x20, 0x2C));
        g.fillRoundedRectangle(titleStrip.reduced(1.0f, 1.0f), 7.0f);

        g.setFont(SpectralUILookAndFeel::getGeometricFont(12.0f, true));
        g.setColour(SpectralUILookAndFeel::accentColour);
        g.drawText(cardTitle, titleStrip.reduced(14.0f, 0.0f), juce::Justification::left, false);
    };

    drawCardPanel(col1Bounds, "PRESETS LIBRARY");
    drawCardPanel(col2Bounds, "PRESET ACTIONS & SAVE");
    drawCardPanel(col3Bounds, "SAMPLE STORAGE");
}

void PresetBrowserOverlay::resized()
{
    auto area = getLocalBounds().reduced(32);
    
    // Top Bar Close Button (X)
    closeButton.setBounds(getWidth() - 65, 28, 34, 28);

    area.removeFromTop(50); // Header height

    int gap = 16;
    int colW = (area.getWidth() - (gap * 2)) / 3;

    auto col1 = area.removeFromLeft(colW).reduced(12, 10);
    area.removeFromLeft(gap);

    auto col2 = area.removeFromLeft(colW).reduced(12, 10);
    area.removeFromLeft(gap);

    auto col3 = area.reduced(12, 10);

    // Column 1: Presets Library
    col1.removeFromTop(32); // Title strip offset

    // Bank Selector Row
    auto bankRow = col1.removeFromTop(30);
    bankSelector.setBounds(bankRow.removeFromLeft(bankRow.getWidth() - 80));
    bankActionsBtn.setBounds(bankRow.removeFromRight(74));
    col1.removeFromTop(8);

    // Search & Sort & Favs Row
    auto searchRow = col1.removeFromTop(30);
    int favW = 68;
    int sortW = 120;
    searchBox.setBounds(searchRow.removeFromLeft(searchRow.getWidth() - (sortW + favW + 8)));
    sortSelector.setBounds(searchRow.removeFromLeft(sortW));
    favoriteFilterBtn.setBounds(searchRow.removeFromRight(favW));
    col1.removeFromTop(8);

    // Category Filter Row (Scrollable Viewport with 7 tabs)
    auto filterRow = col1.removeFromTop(26);
    categoryViewport.setBounds(filterRow);
    categoryContainer.setBounds(0, 0, 440, filterRow.getHeight());
    col1.removeFromTop(8);

    presetListBox.setBounds(col1);

    // Column 2: Selected Preset Details & Save New
    col2.removeFromTop(32); // Title strip offset

    selectedPresetTitle.setBounds(col2.removeFromTop(28));
    statusLabel.setBounds(col2.removeFromTop(24));
    col2.removeFromTop(10);

    loadPresetBtn.setBounds(col2.removeFromTop(36));
    col2.removeFromTop(8);
    deletePresetBtn.setBounds(col2.removeFromTop(34));

    col2.removeFromTop(20); // Divider gap

    // Save Section
    saveNameInput.setBounds(col2.removeFromTop(32));
    col2.removeFromTop(8);
    saveCategoryInput.setBounds(col2.removeFromTop(32));
    col2.removeFromTop(8);
    saveBankSelector.setBounds(col2.removeFromTop(32));
    col2.removeFromTop(12);
    savePresetBtn.setBounds(col2.removeFromTop(38));

    // Column 3: Sample Storage & Edit History
    col3.removeFromTop(32); // Title strip offset

    auto tabRow = col3.removeFromTop(28);
    int halfTabW = tabRow.getWidth() / 2 - 2;
    sampleStorageTabBtn.setBounds(tabRow.removeFromLeft(halfTabW));
    historyTabBtn.setBounds(tabRow.removeFromRight(halfTabW));
    col3.removeFromTop(8);

    if (activeSampleStorageTab == 0)
    {
        sampleListBox.setBounds(col3.removeFromTop(col3.getHeight() - 84));
        col3.removeFromTop(10);
        
        auto sampleBtnRow1 = col3.removeFromTop(34);
        int halfW = sampleBtnRow1.getWidth() / 2 - 4;
        importSampleBtn.setBounds(sampleBtnRow1.removeFromLeft(halfW));
        loadSampleToEngineBtn.setBounds(sampleBtnRow1.removeFromRight(halfW));
        col3.removeFromTop(6);

        auto sampleBtnRow2 = col3.removeFromTop(32);
        renameSampleBtn.setBounds(sampleBtnRow2.removeFromLeft(halfW));
        deleteSampleBtn.setBounds(sampleBtnRow2.removeFromRight(halfW));
    }
    else
    {
        historyListBox.setBounds(col3.removeFromTop(col3.getHeight() - 44));
        col3.removeFromTop(10);

        auto historyBtnRow = col3.removeFromTop(34);
        int restW = (historyBtnRow.getWidth() * 2) / 3 - 4;
        restoreHistoryBtn.setBounds(historyBtnRow.removeFromLeft(restW));
        clearHistoryBtn.setBounds(historyBtnRow.removeFromRight(historyBtnRow.getWidth()));
    }
}

void PresetBrowserOverlay::CategoryBarContainer::resized()
{
    int btnW = 56;
    int gap = 4;
    owner.filterAllBtn.setBounds(0, 0, btnW, getHeight());
    owner.filterSynthBtn.setBounds((btnW + gap) * 1, 0, btnW + 8, getHeight());
    owner.filterLeadBtn.setBounds((btnW + gap) * 2 + 8, 0, btnW, getHeight());
    owner.filterBassBtn.setBounds((btnW + gap) * 3 + 8, 0, btnW, getHeight());
    owner.filterPadBtn.setBounds((btnW + gap) * 4 + 8, 0, btnW, getHeight());
    owner.filterFxBtn.setBounds((btnW + gap) * 5 + 8, 0, btnW - 6, getHeight());
    owner.filterStatesBtn.setBounds((btnW + gap) * 6 + 2, 0, btnW + 16, getHeight());
}

void PresetBrowserOverlay::CategoryBarContainer::mouseWheelMove(const juce::MouseEvent&, const juce::MouseWheelDetails& wheel)
{
    int newX = owner.categoryViewport.getViewPositionX() - (int)(wheel.deltaY * 80.0f);
    int maxX = juce::jmax(0, getWidth() - owner.categoryViewport.getWidth());
    owner.categoryViewport.setViewPosition(juce::jlimit(0, maxX, newX), 0);
}

