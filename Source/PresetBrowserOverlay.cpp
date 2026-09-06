#include "PresetBrowserOverlay.h"
#include "SpectrogramComponent.h"

PresetBrowserOverlay::PresetBrowserOverlay(PresetManager& manager, juce::AudioProcessorValueTreeState& state, HistoryManager* historyMgr)
    : presetManager(manager), apvts(state), historyManager(historyMgr)
{
    setWantsKeyboardFocus(true);

    // Close Button
    addAndMakeVisible(closeButton);
    closeButton.setColour(juce::TextButton::buttonColourId, juce::Colour(200, 45, 55));
    closeButton.setColour(juce::TextButton::textColourOffId, juce::Colours::white);
    closeButton.onClick = [this]() {
        if (onClose) onClose();
        setVisible(false);
    };

    // Root folder row: current path + chooser + secondary Samples/History toggles
    addAndMakeVisible(rootPathLabel);
    rootPathLabel.setFont(SpectralUILookAndFeel::getMonospaceFont(11.5f));
    rootPathLabel.setColour(juce::Label::textColourId, SpectralUILookAndFeel::textMutedColour);
    rootPathLabel.setMinimumHorizontalScale(1.0f);

    addAndMakeVisible(chooseRootBtn);
    chooseRootBtn.setColour(juce::TextButton::buttonColourId, juce::Colour(0x24, 0x24, 0x30));
    chooseRootBtn.setColour(juce::TextButton::textColourOffId, SpectralUILookAndFeel::accentColour);
    chooseRootBtn.onClick = [this]() {
        if (fileChooser != nullptr)
            return;

        fileChooser = std::make_unique<juce::FileChooser>(
            "Choose Presets Folder",
            presetManager.getPresetsRoot());

        auto chooserFlags = juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectDirectories;
        fileChooser->launchAsync(chooserFlags, [this](const juce::FileChooser& fc) {
            auto result = fc.getResult();
            if (result == juce::File())
                return;

            bool ok = presetManager.setPresetsRoot(result);
            currentBankName.clear();
            updateRootPathLabel();
            refreshBankList();
            refreshPresetList();
            showView(ViewMode::Banks);
            statusLabel.setText(ok ? "Presets folder changed."
                                   : "That folder is missing — showing the default presets folder instead.",
                                 juce::dontSendNotification);
        });
    };

    addAndMakeVisible(samplesToggleBtn);
    samplesToggleBtn.onClick = [this]() {
        if (currentView == ViewMode::Samples)
        {
            showView(ViewMode::Banks);
        }
        else
        {
            refreshSampleList();
            showView(ViewMode::Samples);
        }
    };

    addAndMakeVisible(historyToggleBtn);
    historyToggleBtn.onClick = [this]() {
        if (currentView == ViewMode::History)
        {
            showView(ViewMode::Banks);
        }
        else
        {
            refreshHistoryList();
            showView(ViewMode::History);
        }
    };

    // Status line
    addAndMakeVisible(statusLabel);
    statusLabel.setText("Click a bank to see its presets.", juce::dontSendNotification);
    statusLabel.setFont(SpectralUILookAndFeel::getGeometricFont(11.5f, false));
    statusLabel.setColour(juce::Label::textColourId, SpectralUILookAndFeel::textMutedColour);

    // Empty-state message (shared by the bank list and preset list views)
    addAndMakeVisible(emptyStateLabel);
    emptyStateLabel.setJustificationType(juce::Justification::centred);
    emptyStateLabel.setFont(SpectralUILookAndFeel::getGeometricFont(13.0f, false));
    emptyStateLabel.setColour(juce::Label::textColourId, SpectralUILookAndFeel::textMutedColour);
    emptyStateLabel.setInterceptsMouseClicks(false, false);
    emptyStateLabel.setVisible(false);

    // Bank list (top level)
    addAndMakeVisible(bankListBox);
    bankListBox.setModel(this);
    bankListBox.setColour(juce::ListBox::backgroundColourId, juce::Colour(0x12, 0x12, 0x17));
    bankListBox.setColour(juce::ListBox::outlineColourId, juce::Colour(0x2A, 0x2A, 0x35));
    bankListBox.setRowHeight(46);

    addAndMakeVisible(manageBanksBtn);
    manageBanksBtn.setColour(juce::TextButton::buttonColourId, juce::Colour(0x24, 0x24, 0x30));
    manageBanksBtn.setColour(juce::TextButton::textColourOffId, SpectralUILookAndFeel::textMutedColour);
    manageBanksBtn.onClick = [this]() { showBankActionsMenu(); };

    // Preset list (inside a bank)
    addAndMakeVisible(backToBanksBtn);
    backToBanksBtn.setColour(juce::TextButton::buttonColourId, juce::Colour(0x24, 0x24, 0x30));
    backToBanksBtn.setColour(juce::TextButton::textColourOffId, SpectralUILookAndFeel::accentColour);
    backToBanksBtn.onClick = [this]() { showView(ViewMode::Banks); };

    addAndMakeVisible(currentBankLabel);
    currentBankLabel.setFont(SpectralUILookAndFeel::getGeometricFont(14.0f, true));
    currentBankLabel.setColour(juce::Label::textColourId, SpectralUILookAndFeel::accentColour);
    currentBankLabel.setJustificationType(juce::Justification::centredRight);

    addAndMakeVisible(presetListBox);
    presetListBox.setModel(&presetListModel);
    presetListBox.setColour(juce::ListBox::backgroundColourId, juce::Colour(0x12, 0x12, 0x17));
    presetListBox.setColour(juce::ListBox::outlineColourId, juce::Colour(0x2A, 0x2A, 0x35));
    presetListBox.setRowHeight(40);

    addAndMakeVisible(selectedPresetTitle);
    selectedPresetTitle.setText("No preset selected", juce::dontSendNotification);
    selectedPresetTitle.setFont(SpectralUILookAndFeel::getGeometricFont(14.5f, true));
    selectedPresetTitle.setColour(juce::Label::textColourId, SpectralUILookAndFeel::accentColour);

    addAndMakeVisible(loadPresetBtn);
    loadPresetBtn.setColour(juce::TextButton::buttonColourId, SpectralUILookAndFeel::accentColour);
    loadPresetBtn.setColour(juce::TextButton::textColourOffId, juce::Colours::black);
    loadPresetBtn.onClick = [this]() { executeLoadSelectedPreset(); };

    addAndMakeVisible(deletePresetBtn);
    deletePresetBtn.setColour(juce::TextButton::buttonColourId, juce::Colour(200, 45, 55));
    deletePresetBtn.setColour(juce::TextButton::textColourOffId, juce::Colours::white);
    deletePresetBtn.onClick = [this]() {
        int r = presetListBox.getSelectedRow();
        if (r < 0 || r >= currentBankPresets.size())
            return;

        auto preset = currentBankPresets[r];
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
    };

    // Compact secondary "save current preset" action (saves into the currently open bank)
    addAndMakeVisible(saveNameInput);
    saveNameInput.setTextToShowWhenEmpty("New preset name...", SpectralUILookAndFeel::textMutedColour);
    saveNameInput.setColour(juce::TextEditor::backgroundColourId, juce::Colour(0x16, 0x16, 0x1C));
    saveNameInput.setColour(juce::TextEditor::textColourId, juce::Colours::white);
    saveNameInput.setColour(juce::TextEditor::outlineColourId, juce::Colour(0x2A, 0x2A, 0x34));
    saveNameInput.setColour(juce::TextEditor::focusedOutlineColourId, SpectralUILookAndFeel::accentColour);

    addAndMakeVisible(savePresetBtn);
    savePresetBtn.setColour(juce::TextButton::buttonColourId, juce::Colour(0x2B, 0x8A, 0x5A));
    savePresetBtn.setColour(juce::TextButton::textColourOffId, juce::Colours::white);
    savePresetBtn.onClick = [this]() {
        juce::String name = saveNameInput.getText().trim();
        juce::String bank = currentBankName;

        if (name.isEmpty())
        {
            statusLabel.setText("Enter a preset name before saving.", juce::dontSendNotification);
            return;
        }

        if (bank.equalsIgnoreCase("Factory"))
        {
            statusLabel.setText("Factory bank is read-only. Open or create a different bank to save into.", juce::dontSendNotification);
            return;
        }

        auto executeSave = [this, name, bank]() {
            float startReg = spectrogram ? spectrogram->getStartRegion() : 0.0f;
            float endReg = spectrogram ? spectrogram->getEndRegion() : 1.0f;
            juce::var selectionsVar = spectrogram ? spectrogram->getSelectionsAsVar() : juce::var();
            bool loopEnabled = spectrogram ? spectrogram->isLoopEnabled() : false;

            juce::String sampleFileToSave = currentSampleName;
            if (sampleFileToSave.isEmpty() && spectrogram && spectrogram->isFileLoaded())
                sampleFileToSave = spectrogram->getLoadedFile().getFileName();

            // Pass nullptr for sampleAudioBuffer: settings only, no base64 audio bundled.
            if (presetManager.savePreset(name, "FX", bank, sampleFileToSave, apvts, startReg, endReg, selectionsVar, false, loopEnabled, nullptr))
            {
                activeLoadedPresetName = name;
                saveNameInput.clear();
                statusLabel.setText("Saved '" + name + "' to bank '" + bank + "'.", juce::dontSendNotification);
                if (historyManager != nullptr)
                {
                    historyManager->pushHistoryState("Preset Saved: " + name, sampleFileToSave, apvts, startReg, endReg, loopEnabled, selectionsVar, juce::AudioBuffer<float>(), 44100.0);
                    refreshHistoryList();
                }
                refreshPresetList();
            }
            else
            {
                auto* errDialog = new juce::AlertWindow("Save Failed", "Failed to write preset file '" + name + ".vsfx' to disk in bank '" + bank + "'. Please check folder permissions.", juce::AlertWindow::WarningIcon);
                errDialog->addButton("OK", 1, juce::KeyPress(juce::KeyPress::returnKey));
                errDialog->enterModalState(true, nullptr, true);
                statusLabel.setText("Error: failed to write preset file to disk!", juce::dontSendNotification);
            }
        };

        juce::String cleanBank = juce::File::createLegalFileName(bank);
        juce::String cleanName = juce::File::createLegalFileName(name);
        juce::File targetFile = presetManager.getPresetsRoot().getChildFile(cleanBank).getChildFile(cleanName + ".vsfx");

        if (targetFile.existsAsFile())
        {
            auto* confirmDialog = new juce::AlertWindow("Preset Already Exists", "A preset named '" + name + "' already exists in bank '" + bank + "'. Overwrite existing file on disk?", juce::AlertWindow::QuestionIcon);
            confirmDialog->addButton("Overwrite", 1, juce::KeyPress(juce::KeyPress::returnKey));
            confirmDialog->addButton("Cancel", 0, juce::KeyPress(juce::KeyPress::escapeKey));

            confirmDialog->enterModalState(true, juce::ModalCallbackFunction::create([executeSave](int button) {
                if (button == 1)
                    executeSave();
            }), true);
        }
        else
        {
            executeSave();
        }
    };

    // Sample Library panel (secondary, off the primary bank tree)
    addAndMakeVisible(sampleListBox);
    sampleListBox.setModel(&sampleListModel);
    sampleListBox.setColour(juce::ListBox::backgroundColourId, juce::Colour(0x12, 0x12, 0x17));
    sampleListBox.setColour(juce::ListBox::outlineColourId, juce::Colour(0x2A, 0x2A, 0x35));
    sampleListBox.setRowHeight(34);

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
                    auto* dialog = new juce::AlertWindow(
                        "Duplicate Sample Detected",
                        "A sample named \"" + existingDuplicate.getFileName() + "\" already exists in your library. Replace it?",
                        juce::AlertWindow::QuestionIcon);

                    dialog->addButton("Replace", 1, juce::KeyPress(juce::KeyPress::returnKey));
                    dialog->addButton("Cancel", 0, juce::KeyPress(juce::KeyPress::escapeKey));

                    dialog->enterModalState(true, juce::ModalCallbackFunction::create([this, result, existingDuplicate](int button) {
                        if (button == 1)
                        {
                            auto imported = presetManager.importSample(result, true, existingDuplicate);
                            refreshSampleList();
                            if (imported.existsAsFile())
                            {
                                currentSampleName = imported.getFileName();
                                if (onSampleSelected)
                                    onSampleSelected(imported);
                                statusLabel.setText("Sample Replaced: " + imported.getFileName(), juce::dontSendNotification);
                            }
                        }
                        else
                        {
                            statusLabel.setText("Import cancelled. Original sample kept.", juce::dontSendNotification);
                        }
                    }), true);
                }
                else
                {
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
            showRenameSampleDialog(r);
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

    // Edit History panel (secondary, off the primary bank tree)
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

    updateRootPathLabel();
    refreshBankList();
    refreshPresetList();
    refreshSampleList();
    refreshHistoryList();
    showView(ViewMode::Banks);
}

PresetBrowserOverlay::~PresetBrowserOverlay()
{
    bankListBox.setModel(nullptr);
    presetListBox.setModel(nullptr);
    sampleListBox.setModel(nullptr);
    historyListBox.setModel(nullptr);
}

void PresetBrowserOverlay::visibilityChanged()
{
    // isVisible() is true as soon as the flag is set (including during the
    // editor constructor via addAndMakeVisible). JUCE only allows grabKeyboardFocus
    // when the component is actually on-screen.
    if (isShowing())
        grabKeyboardFocus();
}

bool PresetBrowserOverlay::keyPressed(const juce::KeyPress& key)
{
    if (key == juce::KeyPress::escapeKey)
    {
        if (onClose) onClose();
        setVisible(false);
        return true;
    }

    if (key == juce::KeyPress::returnKey)
    {
        if (currentView == ViewMode::PresetsInBank)
        {
            executeLoadSelectedPreset();
            return true;
        }

        if (currentView == ViewMode::Banks)
        {
            int r = bankListBox.getSelectedRow();
            if (r >= 0 && r < currentBanks.size())
            {
                openBank(currentBanks[r]);
                return true;
            }
        }
    }

    return false;
}

void PresetBrowserOverlay::updateRootPathLabel()
{
    rootPathLabel.setText(presetManager.getPresetsRoot().getFullPathName(), juce::dontSendNotification);
}

void PresetBrowserOverlay::updateEmptyStateVisibility()
{
    if (currentView == ViewMode::Banks)
    {
        bool empty = currentBanks.isEmpty();
        emptyStateLabel.setText("No banks in this folder", juce::dontSendNotification);
        emptyStateLabel.setVisible(empty);
        bankListBox.setVisible(!empty);
    }
    else if (currentView == ViewMode::PresetsInBank)
    {
        bool empty = currentBankPresets.isEmpty();
        emptyStateLabel.setText("This bank has no presets", juce::dontSendNotification);
        emptyStateLabel.setVisible(empty);
        presetListBox.setVisible(!empty);
    }
    else
    {
        emptyStateLabel.setVisible(false);
    }
}

void PresetBrowserOverlay::showView(ViewMode mode)
{
    currentView = mode;

    bool banks = (mode == ViewMode::Banks);
    bool presets = (mode == ViewMode::PresetsInBank);
    bool samples = (mode == ViewMode::Samples);
    bool history = (mode == ViewMode::History);

    bankListBox.setVisible(banks);
    manageBanksBtn.setVisible(banks);

    backToBanksBtn.setVisible(presets);
    currentBankLabel.setVisible(presets);
    presetListBox.setVisible(presets);
    selectedPresetTitle.setVisible(presets);
    loadPresetBtn.setVisible(presets);
    deletePresetBtn.setVisible(presets);
    saveNameInput.setVisible(presets);
    savePresetBtn.setVisible(presets);

    sampleListBox.setVisible(samples);
    importSampleBtn.setVisible(samples);
    loadSampleToEngineBtn.setVisible(samples);
    renameSampleBtn.setVisible(samples);
    deleteSampleBtn.setVisible(samples);

    historyListBox.setVisible(history);
    restoreHistoryBtn.setVisible(history);
    clearHistoryBtn.setVisible(history);

    auto setToggleColour = [](juce::TextButton& b, bool active) {
        b.setColour(juce::TextButton::buttonColourId, active ? SpectralUILookAndFeel::accentColour : juce::Colour(0x22, 0x22, 0x2A));
        b.setColour(juce::TextButton::textColourOffId, active ? juce::Colours::black : SpectralUILookAndFeel::textMutedColour);
    };
    setToggleColour(samplesToggleBtn, samples);
    setToggleColour(historyToggleBtn, history);

    updateEmptyStateVisibility();
    resized();
    repaint();
}

void PresetBrowserOverlay::openBank(const juce::String& bankName)
{
    currentBankName = bankName;
    currentBankLabel.setText(bankName.toUpperCase(), juce::dontSendNotification);
    refreshPresetList();
    showView(ViewMode::PresetsInBank);
}

void PresetBrowserOverlay::refreshBankList()
{
    currentBanks = presetManager.getAllBanks();
    bankListBox.updateContent();
    bankListBox.repaint();
    updateRootPathLabel();
    updateEmptyStateVisibility();
}

void PresetBrowserOverlay::refreshPresetList()
{
    allPresets = presetManager.getAllPresets();

    currentBankPresets.clear();
    if (currentBankName.isNotEmpty())
        currentBankPresets = presetManager.getPresetsInBank(currentBankName);

    std::sort(currentBankPresets.begin(), currentBankPresets.end(), [](const PresetInfo& a, const PresetInfo& b) {
        return a.name.compareIgnoreCase(b.name) < 0;
    });

    presetListBox.updateContent();

    int activeIndex = -1;
    if (activeLoadedPresetName.isNotEmpty())
    {
        for (int i = 0; i < currentBankPresets.size(); ++i)
        {
            if (currentBankPresets[i].name.equalsIgnoreCase(activeLoadedPresetName) ||
               (activePresetFile.existsAsFile() && currentBankPresets[i].file == activePresetFile))
            {
                activeIndex = i;
                activePresetFile = currentBankPresets[i].file;
                break;
            }
        }
    }

    if (activeIndex >= 0)
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
    updateEmptyStateVisibility();
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

void PresetBrowserOverlay::executeShuffleFx()
{
    juce::Array<PresetInfo> candidates;
    for (const auto& p : allPresets)
    {
        bool isFx = !p.category.equalsIgnoreCase("STATES") && !p.file.hasFileExtension(".vsts");
        if (isFx)
            candidates.add(p);
    }

    if (candidates.isEmpty())
    {
        statusLabel.setText("No FX presets found.", juce::dontSendNotification);
        return;
    }

    PresetInfo chosenPreset;
    if (candidates.size() == 1)
    {
        chosenPreset = candidates[0];
    }
    else
    {
        juce::Array<PresetInfo> pool;
        for (const auto& p : candidates)
        {
            if (lastShuffledPresetFile.existsAsFile() && p.file == lastShuffledPresetFile)
                continue;
            pool.add(p);
        }

        if (pool.isEmpty())
            pool = candidates;

        int randIdx = juce::Random::getSystemRandom().nextInt(pool.size());
        chosenPreset = pool[randIdx];
    }

    lastShuffledPresetFile = chosenPreset.file;
    activePresetFile = chosenPreset.file;
    activeLoadedPresetName = chosenPreset.name;

    juce::String sampleFile;
    float startReg = 0.0f;
    float endReg = 1.0f;
    juce::var selectionsVar;
    bool loopEnabled = false;
    juce::AudioBuffer<float> loadedBuf;
    double loadedSr = 44100.0;

    if (presetManager.loadPreset(chosenPreset.file, apvts, sampleFile, startReg, endReg, selectionsVar, loopEnabled, &loadedBuf, &loadedSr))
    {
        if (spectrogram != nullptr)
        {
            spectrogram->setLoopEnabled(loopEnabled);
            spectrogram->restorePresetSnapshot(startReg, endReg, selectionsVar);
        }

        if (onPresetSelected)
            onPresetSelected(chosenPreset.file, sampleFile);

        if (historyManager != nullptr)
        {
            historyManager->pushHistoryState("Shuffle FX: " + chosenPreset.name, currentSampleName, apvts, startReg, endReg, loopEnabled, selectionsVar, juce::AudioBuffer<float>(), 44100.0);
            refreshHistoryList();
        }

        statusLabel.setText("Shuffled FX preset: " + chosenPreset.name + " (bank: " + chosenPreset.bank + ")", juce::dontSendNotification);

        if (currentView == ViewMode::PresetsInBank && currentBankName.equalsIgnoreCase(chosenPreset.bank))
            refreshPresetList();
    }
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
    if (r >= 0 && r < currentBankPresets.size())
    {
        auto preset = currentBankPresets[r];
        selectedPresetTitle.setText(preset.name, juce::dontSendNotification);
        saveNameInput.setText(preset.name, juce::dontSendNotification);

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
    }
    else
    {
        selectedPresetTitle.setText("No preset selected", juce::dontSendNotification);
        deletePresetBtn.setEnabled(false);
        deletePresetBtn.setAlpha(0.4f);
    }
}

void PresetBrowserOverlay::refreshHistoryList()
{
    if (historyManager != nullptr)
        allHistoryEntries = historyManager->getHistoryEntries();
    else
        allHistoryEntries.clear();

    historyToggleBtn.setButtonText("HISTORY (" + juce::String(allHistoryEntries.size()) + ")");
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
    if (r < 0 || r >= currentBankPresets.size())
        return;

    auto preset = currentBankPresets[r];
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

void PresetBrowserOverlay::showBankActionsMenu()
{
    juce::PopupMenu renameMenu;
    juce::PopupMenu deleteMenu;
    for (int i = 0; i < currentBanks.size(); ++i)
    {
        auto name = currentBanks[i];
        if (!name.equalsIgnoreCase("Factory"))
            renameMenu.addItem(1000 + i, name);
        if (!name.equalsIgnoreCase("Factory") && !name.equalsIgnoreCase("User"))
            deleteMenu.addItem(2000 + i, name);
    }

    juce::PopupMenu menu;
    menu.addItem(1, "Create New Bank...");
    menu.addSubMenu("Rename Bank...", renameMenu, renameMenu.getNumItems() > 0);
    menu.addSubMenu("Delete Bank...", deleteMenu, deleteMenu.getNumItems() > 0);
    menu.addSeparator();
    menu.addItem(3, "Import Bank Package (.zip / folder)...");

    menu.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(manageBanksBtn), [this](int result) {
        if (result == 1) // Create New Bank
        {
            auto* dialog = new juce::AlertWindow("Create New Bank", "Enter a name for the new bank (creates a subfolder in the current root):", juce::AlertWindow::NoIcon);
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
                        statusLabel.setText("Created bank: " + name, juce::dontSendNotification);
                    }
                }
            }), true);
        }
        else if (result == 3) // Import Bank Package
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
                        refreshSampleList();
                        statusLabel.setText("Imported Bank Package: " + resultFile.getFileName(), juce::dontSendNotification);
                    }
                }
            });
        }
        else if (result >= 1000 && result < 2000) // Rename Bank
        {
            int idx = result - 1000;
            if (idx < 0 || idx >= currentBanks.size())
                return;

            juce::String oldName = currentBanks[idx];
            auto* dialog = new juce::AlertWindow("Rename Bank", "Enter new name for bank '" + oldName + "':", juce::AlertWindow::NoIcon);
            dialog->addTextEditor("bankName", oldName, "New Bank Name");
            dialog->addButton("Rename", 1, juce::KeyPress(juce::KeyPress::returnKey));
            dialog->addButton("Cancel", 0, juce::KeyPress(juce::KeyPress::escapeKey));

            dialog->enterModalState(true, juce::ModalCallbackFunction::create([this, dialog, oldName](int button) {
                if (button == 1)
                {
                    juce::String newName = dialog->getTextEditorContents("bankName").trim();
                    if (presetManager.renameBank(oldName, newName))
                    {
                        if (currentBankName.equalsIgnoreCase(oldName))
                            currentBankName = newName;
                        refreshBankList();
                        statusLabel.setText("Renamed bank to: " + newName, juce::dontSendNotification);
                    }
                }
            }), true);
        }
        else if (result >= 2000) // Delete Bank
        {
            int idx = result - 2000;
            if (idx < 0 || idx >= currentBanks.size())
                return;

            juce::String bankToDelete = currentBanks[idx];
            auto* dialog = new juce::AlertWindow("Delete Bank", "Are you sure you want to delete bank '" + bankToDelete + "' and all presets inside it?", juce::AlertWindow::WarningIcon);
            dialog->addButton("Delete", 1);
            dialog->addButton("Cancel", 0, juce::KeyPress(juce::KeyPress::escapeKey));

            dialog->enterModalState(true, juce::ModalCallbackFunction::create([this, dialog, bankToDelete](int button) {
                if (button == 1)
                {
                    if (presetManager.deleteBank(bankToDelete))
                    {
                        statusLabel.setText("Deleted Bank: " + bankToDelete, juce::dontSendNotification);
                        refreshBankList();
                    }
                }
            }), true);
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

// ---- Bank list ListBoxModel (this class) ----

int PresetBrowserOverlay::getNumRows()
{
    return currentBanks.size();
}

void PresetBrowserOverlay::paintListBoxItem(int rowNumber, juce::Graphics& g, int width, int height, bool rowIsSelected)
{
    if (rowNumber < 0 || rowNumber >= currentBanks.size())
        return;

    auto bankName = currentBanks[rowNumber];

    if (rowIsSelected)
    {
        g.setColour(SpectralUILookAndFeel::accentColour.withAlpha(0.18f));
        g.fillRoundedRectangle(2.0f, 2.0f, (float)width - 4.0f, (float)height - 4.0f, 5.0f);
        g.setColour(SpectralUILookAndFeel::accentColour);
        g.drawRoundedRectangle(2.0f, 2.0f, (float)width - 4.0f, (float)height - 4.0f, 5.0f, 1.3f);
    }
    else
    {
        g.setColour(rowNumber % 2 == 0 ? juce::Colour(0x16, 0x16, 0x1C) : juce::Colour(0x1A, 0x1A, 0x22));
        g.fillRoundedRectangle(2.0f, 2.0f, (float)width - 4.0f, (float)height - 4.0f, 5.0f);
    }

    g.setFont(SpectralUILookAndFeel::getGeometricFont(15.0f, true));
    g.setColour(rowIsSelected ? juce::Colours::white : juce::Colour(0xE0, 0xDC, 0xD0));
    g.drawText(bankName, 20, 0, width - 60, height, juce::Justification::centredLeft);

    g.setFont(SpectralUILookAndFeel::getGeometricFont(18.0f, true));
    g.setColour(SpectralUILookAndFeel::accentColour.withAlpha(0.8f));
    g.drawText(">", width - 34, 0, 24, height, juce::Justification::centred);
}

void PresetBrowserOverlay::listBoxItemClicked(int row, const juce::MouseEvent&)
{
    if (row < 0 || row >= currentBanks.size())
        return;

    openBank(currentBanks[row]);
}

void PresetBrowserOverlay::listBoxItemDoubleClicked(int row, const juce::MouseEvent& e)
{
    listBoxItemClicked(row, e);
}

// ---- Preset-in-bank ListBoxModel ----

void PresetBrowserOverlay::PresetListModel::paintListBoxItem(int rowNumber, juce::Graphics& g, int width, int height, bool rowIsSelected)
{
    if (rowNumber < 0 || rowNumber >= owner.currentBankPresets.size())
        return;

    auto preset = owner.currentBankPresets[rowNumber];
    bool isCurrentlyLoaded = (owner.activePresetFile.existsAsFile() && preset.file == owner.activePresetFile)
                          || (owner.activeLoadedPresetName.isNotEmpty() && preset.name.equalsIgnoreCase(owner.activeLoadedPresetName));

    if (rowIsSelected && isCurrentlyLoaded)
    {
        g.setColour(SpectralUILookAndFeel::accentColour.withAlpha(0.22f));
        g.fillRoundedRectangle(2, 2, (float)width - 4, (float)height - 4, 4.0f);
        g.setColour(SpectralUILookAndFeel::accentColour);
        g.drawRoundedRectangle(2, 2, (float)width - 4, (float)height - 4, 4.0f, 1.5f);
    }
    else if (isCurrentlyLoaded)
    {
        g.setColour(SpectralUILookAndFeel::accentColour.withAlpha(0.12f));
        g.fillRoundedRectangle(2, 2, (float)width - 4, (float)height - 4, 4.0f);
        g.setColour(SpectralUILookAndFeel::accentColour.withAlpha(0.85f));
        g.drawRoundedRectangle(2, 2, (float)width - 4, (float)height - 4, 4.0f, 1.2f);
    }
    else if (rowIsSelected)
    {
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

    g.setFont(SpectralUILookAndFeel::getGeometricFont(14.0f, true));
    g.setColour(preset.isFavorite ? juce::Colour(0xFF, 0xC1, 0x07) : juce::Colour(0x4A, 0x4A, 0x56));
    g.drawText(preset.isFavorite ? juce::String::fromUTF8("\xe2\x98\x85") : juce::String::fromUTF8("\xe2\x98\x86"), 8, 0, 24, height, juce::Justification::centred);

    g.setFont(SpectralUILookAndFeel::getGeometricFont(13.5f, true));
    g.setColour((rowIsSelected || isCurrentlyLoaded) ? juce::Colours::white : juce::Colour(0xE0, 0xDC, 0xD0));
    g.drawText(preset.name, 36, 0, width - 50, height, juce::Justification::centredLeft);
}

void PresetBrowserOverlay::PresetListModel::listBoxItemClicked(int row, const juce::MouseEvent& e)
{
    if (row < 0 || row >= owner.currentBankPresets.size())
        return;

    auto preset = owner.currentBankPresets[row];

    if (e.x < 34) // favorite star click
    {
        owner.presetManager.toggleFavorite(preset.file);
        owner.refreshPresetList();
        return;
    }

    owner.selectedPresetIndex = row;
    owner.updateSelectedPresetDetails();
}

void PresetBrowserOverlay::PresetListModel::listBoxItemDoubleClicked(int row, const juce::MouseEvent&)
{
    owner.selectedPresetIndex = row;
    owner.executeLoadSelectedPreset();
}

// ---- Sample library ListBoxModel ----

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

// ---- Edit history ListBoxModel ----

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
    g.fillAll(juce::Colour(0x0A, 0x0A, 0x0E).withAlpha(0.95f));

    auto mainBounds = getLocalBounds().reduced(20).toFloat();
    g.setColour(juce::Colour(0x14, 0x14, 0x1A));
    g.fillRoundedRectangle(mainBounds, 12.0f);

    g.setColour(juce::Colour(0x2A, 0x2A, 0x36));
    g.drawRoundedRectangle(mainBounds, 12.0f, 1.5f);

    auto headerArea = mainBounds.removeFromTop(50.0f);
    g.setColour(juce::Colour(0x1B, 0x1B, 0x24));
    g.fillRoundedRectangle(headerArea.reduced(2.0f, 2.0f), 10.0f);

    g.setFont(SpectralUILookAndFeel::getMonospaceFont(15.0f));
    g.setColour(SpectralUILookAndFeel::accentColour);
    g.drawText("PRESET BROWSER", headerArea.reduced(20.0f, 0.0f), juce::Justification::left, false);

    auto contentBounds = getLocalBounds().reduced(32).toFloat();
    contentBounds.removeFromTop(50.0f + 34.0f + 6.0f + 22.0f + 10.0f);
    g.setColour(juce::Colour(0x18, 0x18, 0x20));
    g.fillRoundedRectangle(contentBounds, 8.0f);
    g.setColour(juce::Colour(0x2C, 0x2C, 0x3A));
    g.drawRoundedRectangle(contentBounds, 8.0f, 1.2f);
}

void PresetBrowserOverlay::resized()
{
    auto area = getLocalBounds().reduced(32);

    closeButton.setBounds(getWidth() - 65, 28, 34, 28);

    area.removeFromTop(50); // header bar height

    // Root folder row
    auto rootRow = area.removeFromTop(34);
    chooseRootBtn.setBounds(rootRow.removeFromRight(140));
    rootRow.removeFromRight(8);
    historyToggleBtn.setBounds(rootRow.removeFromRight(100));
    rootRow.removeFromRight(6);
    samplesToggleBtn.setBounds(rootRow.removeFromRight(90));
    rootRow.removeFromRight(10);
    rootPathLabel.setBounds(rootRow);
    area.removeFromTop(6);

    // Status row
    statusLabel.setBounds(area.removeFromTop(22));
    area.removeFromTop(10);

    auto content = area.reduced(12, 10);
    emptyStateLabel.setBounds(content);

    if (currentView == ViewMode::Banks)
    {
        auto topRow = content.removeFromTop(30);
        manageBanksBtn.setBounds(topRow.removeFromRight(80));
        content.removeFromTop(8);
        bankListBox.setBounds(content);
        emptyStateLabel.setBounds(content);
    }
    else if (currentView == ViewMode::PresetsInBank)
    {
        auto topRow = content.removeFromTop(30);
        backToBanksBtn.setBounds(topRow.removeFromLeft(150));
        currentBankLabel.setBounds(topRow);
        content.removeFromTop(8);

        auto rightPanel = content.removeFromRight(260);
        content.removeFromRight(16);

        presetListBox.setBounds(content);
        emptyStateLabel.setBounds(content);

        selectedPresetTitle.setBounds(rightPanel.removeFromTop(28));
        rightPanel.removeFromTop(10);
        loadPresetBtn.setBounds(rightPanel.removeFromTop(38));
        rightPanel.removeFromTop(8);
        deletePresetBtn.setBounds(rightPanel.removeFromTop(34));
        rightPanel.removeFromTop(24);

        saveNameInput.setBounds(rightPanel.removeFromTop(32));
        rightPanel.removeFromTop(8);
        savePresetBtn.setBounds(rightPanel.removeFromTop(38));
    }
    else if (currentView == ViewMode::Samples)
    {
        sampleListBox.setBounds(content.removeFromTop(content.getHeight() - 84));
        content.removeFromTop(10);

        auto row1 = content.removeFromTop(34);
        int halfW = row1.getWidth() / 2 - 4;
        importSampleBtn.setBounds(row1.removeFromLeft(halfW));
        loadSampleToEngineBtn.setBounds(row1.removeFromRight(halfW));
        content.removeFromTop(6);

        auto row2 = content.removeFromTop(32);
        renameSampleBtn.setBounds(row2.removeFromLeft(halfW));
        deleteSampleBtn.setBounds(row2.removeFromRight(halfW));
    }
    else if (currentView == ViewMode::History)
    {
        historyListBox.setBounds(content.removeFromTop(content.getHeight() - 44));
        content.removeFromTop(10);

        auto row = content.removeFromTop(34);
        int restW = (row.getWidth() * 2) / 3 - 4;
        restoreHistoryBtn.setBounds(row.removeFromLeft(restW));
        clearHistoryBtn.setBounds(row);
    }
}
