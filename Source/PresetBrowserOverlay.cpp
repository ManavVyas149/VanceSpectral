#include "PresetBrowserOverlay.h"

PresetBrowserOverlay::PresetBrowserOverlay(PresetManager& manager, juce::AudioProcessorValueTreeState& state)
    : presetManager(manager), apvts(state)
{
    // Close Button
    addAndMakeVisible(closeButton);
    closeButton.setColour(juce::TextButton::buttonColourId, juce::Colour(200, 45, 55));
    closeButton.setColour(juce::TextButton::textColourOffId, juce::Colours::white);
    closeButton.onClick = [this]() {
        if (onClose) onClose();
        setVisible(false);
    };

    // Search Box
    addAndMakeVisible(searchBox);
    searchBox.setTextToShowWhenEmpty("Search presets...", juce::Colour(150, 155, 170));
    searchBox.setColour(juce::TextEditor::backgroundColourId, juce::Colour(15, 16, 20));
    searchBox.setColour(juce::TextEditor::textColourId, juce::Colours::white);
    searchBox.setColour(juce::TextEditor::outlineColourId, juce::Colour(50, 55, 68));
    searchBox.setColour(juce::TextEditor::focusedOutlineColourId, juce::Colour(0, 200, 255));
    searchBox.onTextChange = [this]() { filterPresets(); };

    // Filter Buttons
    auto setupFilterBtn = [this](juce::TextButton& btn, const juce::String& cat) {
        addAndMakeVisible(btn);
        btn.setColour(juce::TextButton::buttonColourId, activeCategoryFilter == cat ? juce::Colour(0, 180, 255) : juce::Colour(30, 32, 42));
        btn.setColour(juce::TextButton::textColourOffId, activeCategoryFilter == cat ? juce::Colours::white : juce::Colour(170, 175, 190));
        btn.onClick = [this, &btn, cat]() {
            activeCategoryFilter = cat;
            filterAllBtn.setColour(juce::TextButton::buttonColourId, activeCategoryFilter == "ALL" ? juce::Colour(0, 180, 255) : juce::Colour(30, 32, 42));
            filterSynthBtn.setColour(juce::TextButton::buttonColourId, activeCategoryFilter == "SYNTH" ? juce::Colour(0, 180, 255) : juce::Colour(30, 32, 42));
            filterLeadBtn.setColour(juce::TextButton::buttonColourId, activeCategoryFilter == "LEAD" ? juce::Colour(0, 180, 255) : juce::Colour(30, 32, 42));
            filterBassBtn.setColour(juce::TextButton::buttonColourId, activeCategoryFilter == "BASS" ? juce::Colour(0, 180, 255) : juce::Colour(30, 32, 42));
            filterPadBtn.setColour(juce::TextButton::buttonColourId, activeCategoryFilter == "PAD" ? juce::Colour(0, 180, 255) : juce::Colour(30, 32, 42));
            filterPresets();
        };
    };

    setupFilterBtn(filterAllBtn, "ALL");
    setupFilterBtn(filterSynthBtn, "SYNTH");
    setupFilterBtn(filterLeadBtn, "LEAD");
    setupFilterBtn(filterBassBtn, "BASS");
    setupFilterBtn(filterPadBtn, "PAD");

    // Presets ListBox
    addAndMakeVisible(presetListBox);
    presetListBox.setModel(this);
    presetListBox.setColour(juce::ListBox::backgroundColourId, juce::Colour(14, 15, 18));
    presetListBox.setColour(juce::ListBox::outlineColourId, juce::Colour(40, 44, 55));
    presetListBox.setRowHeight(38);

    // Selected Preset Header / Status Labels
    addAndMakeVisible(selectedPresetTitle);
    selectedPresetTitle.setText("No Preset Selected", juce::dontSendNotification);
    selectedPresetTitle.setFont(juce::FontOptions(17.0f, juce::Font::bold));
    selectedPresetTitle.setColour(juce::Label::textColourId, juce::Colour(0, 220, 255));

    addAndMakeVisible(statusLabel);
    statusLabel.setText("Double-click a preset to load", juce::dontSendNotification);
    statusLabel.setFont(juce::FontOptions(12.0f));
    statusLabel.setColour(juce::Label::textColourId, juce::Colour(150, 155, 170));

    // Load & Delete Buttons
    addAndMakeVisible(loadPresetBtn);
    loadPresetBtn.setColour(juce::TextButton::buttonColourId, juce::Colour(0, 150, 230));
    loadPresetBtn.setColour(juce::TextButton::textColourOffId, juce::Colours::white);
    loadPresetBtn.onClick = [this]() { executeLoadSelectedPreset(); };

    addAndMakeVisible(deletePresetBtn);
    deletePresetBtn.setColour(juce::TextButton::buttonColourId, juce::Colour(170, 35, 45));
    deletePresetBtn.setColour(juce::TextButton::textColourOffId, juce::Colours::white);
    deletePresetBtn.onClick = [this]() {
        int r = presetListBox.getSelectedRow();
        if (r >= 0 && r < filteredPresets.size())
        {
            auto preset = filteredPresets[r];
            if (presetManager.deletePreset(preset.file))
            {
                statusLabel.setText("Deleted preset: " + preset.name, juce::dontSendNotification);
                refreshPresetList();
            }
        }
    };

    // Save Preset Inputs & Button
    addAndMakeVisible(saveNameInput);
    saveNameInput.setTextToShowWhenEmpty("New Preset Name (e.g. My Lead)", juce::Colour(140, 145, 160));
    saveNameInput.setColour(juce::TextEditor::backgroundColourId, juce::Colour(15, 16, 20));
    saveNameInput.setColour(juce::TextEditor::textColourId, juce::Colours::white);
    saveNameInput.setColour(juce::TextEditor::outlineColourId, juce::Colour(50, 55, 68));
    saveNameInput.setColour(juce::TextEditor::focusedOutlineColourId, juce::Colour(0, 220, 150));

    addAndMakeVisible(saveCategoryInput);
    saveCategoryInput.setTextToShowWhenEmpty("Category (e.g. Synth, Lead, Bass)", juce::Colour(140, 145, 160));
    saveCategoryInput.setColour(juce::TextEditor::backgroundColourId, juce::Colour(15, 16, 20));
    saveCategoryInput.setColour(juce::TextEditor::textColourId, juce::Colours::white);
    saveCategoryInput.setColour(juce::TextEditor::outlineColourId, juce::Colour(50, 55, 68));
    saveCategoryInput.setColour(juce::TextEditor::focusedOutlineColourId, juce::Colour(0, 220, 150));

    addAndMakeVisible(savePresetBtn);
    savePresetBtn.setColour(juce::TextButton::buttonColourId, juce::Colour(35, 165, 90));
    savePresetBtn.setColour(juce::TextButton::textColourOffId, juce::Colours::white);
    savePresetBtn.onClick = [this]() {
        juce::String name = saveNameInput.getText().trim();
        juce::String cat = saveCategoryInput.getText().trim();

        if (presetManager.savePreset(name, cat, currentSampleName, apvts))
        {
            saveNameInput.clear();
            saveCategoryInput.clear();
            statusLabel.setText("Preset Saved Successfully!", juce::dontSendNotification);
            refreshPresetList();
        }
    };

    // Sample ListBox
    addAndMakeVisible(sampleListBox);
    sampleListBox.setModel(&sampleListModel);
    sampleListBox.setColour(juce::ListBox::backgroundColourId, juce::Colour(14, 15, 18));
    sampleListBox.setColour(juce::ListBox::outlineColourId, juce::Colour(40, 44, 55));
    sampleListBox.setRowHeight(34);

    // Sample Action Buttons
    addAndMakeVisible(importSampleBtn);
    importSampleBtn.setColour(juce::TextButton::buttonColourId, juce::Colour(110, 55, 190));
    importSampleBtn.setColour(juce::TextButton::textColourOffId, juce::Colours::white);
    importSampleBtn.onClick = [this]() {
        fileChooser = std::make_unique<juce::FileChooser>(
            "Select Audio Sample to Store in Library",
            juce::File::getSpecialLocation(juce::File::userHomeDirectory),
            "*.wav;*.mp3;*.flac;*.aiff;*.ogg;*.m4a");

        auto chooserFlags = juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles;
        fileChooser->launchAsync(chooserFlags, [this](const juce::FileChooser& fc) {
            auto result = fc.getResult();
            if (result.existsAsFile())
            {
                auto imported = presetManager.importSample(result);
                refreshSampleList();
                if (imported.existsAsFile())
                {
                    currentSampleName = imported.getFileName();
                    if (onSampleSelected)
                        onSampleSelected(imported);
                    statusLabel.setText("Sample Imported: " + imported.getFileName(), juce::dontSendNotification);
                }
            }
        });
    };

    addAndMakeVisible(loadSampleToEngineBtn);
    loadSampleToEngineBtn.setColour(juce::TextButton::buttonColourId, juce::Colour(0, 180, 160));
    loadSampleToEngineBtn.setColour(juce::TextButton::textColourOffId, juce::Colours::white);
    loadSampleToEngineBtn.onClick = [this]() {
        int r = sampleListBox.getSelectedRow();
        if (r >= 0 && r < allSamples.size())
        {
            auto sampleFile = allSamples[r];
            currentSampleName = sampleFile.getFileName();
            if (onSampleSelected)
                onSampleSelected(sampleFile);
            statusLabel.setText("Loaded Sample: " + sampleFile.getFileName(), juce::dontSendNotification);
        }
    };

    refreshPresetList();
    refreshSampleList();
}

PresetBrowserOverlay::~PresetBrowserOverlay()
{
    presetListBox.setModel(nullptr);
    sampleListBox.setModel(nullptr);
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

void PresetBrowserOverlay::filterPresets()
{
    filteredPresets.clear();
    juce::String query = searchBox.getText().trim().toLowerCase();

    for (const auto& p : allPresets)
    {
        bool matchesCat = (activeCategoryFilter == "ALL" || p.category.equalsIgnoreCase(activeCategoryFilter));
        bool matchesQuery = (query.isEmpty() || p.name.toLowerCase().contains(query) || p.category.toLowerCase().contains(query));

        if (matchesCat && matchesQuery)
        {
            filteredPresets.add(p);
        }
    }

    presetListBox.updateContent();
    if (!filteredPresets.isEmpty())
    {
        presetListBox.selectRow(0);
        selectedPresetIndex = 0;
    }
    else
    {
        presetListBox.deselectAllRows();
        selectedPresetIndex = -1;
    }
    presetListBox.repaint();
    updateSelectedPresetDetails();
}

void PresetBrowserOverlay::updateSelectedPresetDetails()
{
    int r = presetListBox.getSelectedRow();
    if (r >= 0 && r < filteredPresets.size())
    {
        auto preset = filteredPresets[r];
        selectedPresetTitle.setText(preset.name, juce::dontSendNotification);
        statusLabel.setText("Category: " + preset.category + (preset.sampleFileName.isNotEmpty() ? " | Sample: " + preset.sampleFileName : ""), juce::dontSendNotification);
    }
    else
    {
        selectedPresetTitle.setText("No Preset Selected", juce::dontSendNotification);
        statusLabel.setText("Select a preset from the list", juce::dontSendNotification);
    }
}

void PresetBrowserOverlay::executeLoadSelectedPreset()
{
    int r = presetListBox.getSelectedRow();
    if (r >= 0 && r < filteredPresets.size())
    {
        auto preset = filteredPresets[r];
        juce::String sampleFile;
        if (presetManager.loadPreset(preset.file, apvts, sampleFile))
        {
            if (onPresetSelected)
                onPresetSelected(preset.file, sampleFile);

            statusLabel.setText("Loaded Preset: " + preset.name, juce::dontSendNotification);
            setVisible(false);
        }
    }
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

    if (rowIsSelected)
    {
        g.setColour(juce::Colour(0, 160, 230).withAlpha(0.28f));
        g.fillRoundedRectangle(2, 2, (float)width - 4, (float)height - 4, 4.0f);
        g.setColour(juce::Colour(0, 200, 255));
        g.drawRoundedRectangle(2, 2, (float)width - 4, (float)height - 4, 4.0f, 1.2f);
    }
    else
    {
        g.setColour(rowNumber % 2 == 0 ? juce::Colour(18, 19, 24) : juce::Colour(14, 15, 18));
        g.fillRect(0, 0, width, height);
    }

    // Name
    g.setFont(juce::FontOptions(14.0f, juce::Font::bold));
    g.setColour(rowIsSelected ? juce::Colours::white : juce::Colour(210, 215, 225));
    g.drawText(preset.name, 12, 0, width - 110, height, juce::Justification::centredLeft);

    // Category Pill Color
    juce::Colour catBadgeCol(100, 110, 130);
    juce::String catUp = preset.category.toUpperCase();
    if (catUp == "SYNTH") catBadgeCol = juce::Colour(0, 190, 240);
    else if (catUp == "LEAD") catBadgeCol = juce::Colour(240, 170, 0);
    else if (catUp == "BASS") catBadgeCol = juce::Colour(220, 50, 150);
    else if (catUp == "PAD") catBadgeCol = juce::Colour(140, 70, 240);
    else if (catUp == "FX") catBadgeCol = juce::Colour(230, 60, 60);

    g.setFont(juce::FontOptions(10.0f, juce::Font::bold));
    auto badgeArea = juce::Rectangle<float>((float)width - 85.0f, (float)height * 0.5f - 9.0f, 75.0f, 18.0f);
    g.setColour(catBadgeCol.withAlpha(0.2f));
    g.fillRoundedRectangle(badgeArea, 4.0f);
    g.setColour(catBadgeCol);
    g.drawRoundedRectangle(badgeArea, 4.0f, 1.0f);
    g.drawText(catUp, badgeArea, juce::Justification::centred, false);
}

void PresetBrowserOverlay::listBoxItemClicked(int row, const juce::MouseEvent&)
{
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
        g.setColour(juce::Colour(110, 55, 190).withAlpha(0.3f));
        g.fillRoundedRectangle(2, 2, (float)width - 4, (float)height - 4, 4.0f);
        g.setColour(juce::Colour(170, 90, 255));
        g.drawRoundedRectangle(2, 2, (float)width - 4, (float)height - 4, 4.0f, 1.2f);
    }
    else
    {
        g.setColour(rowNumber % 2 == 0 ? juce::Colour(18, 19, 24) : juce::Colour(14, 15, 18));
        g.fillRect(0, 0, width, height);
    }

    g.setFont(juce::FontOptions(13.0f));
    g.setColour(rowIsSelected ? juce::Colours::white : juce::Colour(200, 205, 215));
    g.drawText(sample.getFileName(), 10, 0, width - 20, height, juce::Justification::centredLeft);
}

void PresetBrowserOverlay::SampleListModel::listBoxItemClicked(int row, const juce::MouseEvent&)
{
    owner.selectedSampleIndex = row;
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
        owner.statusLabel.setText("Loaded Sample: " + sampleFile.getFileName(), juce::dontSendNotification);
    }
}

void PresetBrowserOverlay::paint(juce::Graphics& g)
{
    // Full screen dark translucent overlay backdrop
    g.fillAll(juce::Colour(10, 11, 15).withAlpha(0.95f));

    // Main Modal Box
    auto mainBounds = getLocalBounds().reduced(20).toFloat();
    g.setColour(juce::Colour(18, 20, 26));
    g.fillRoundedRectangle(mainBounds, 14.0f);

    g.setColour(juce::Colour(45, 50, 65));
    g.drawRoundedRectangle(mainBounds, 14.0f, 1.5f);

    // Title Bar Area (Header)
    auto headerArea = mainBounds.removeFromTop(50.0f);
    g.setColour(juce::Colour(25, 28, 36));
    g.fillRoundedRectangle(headerArea.reduced(2.0f, 2.0f), 12.0f);

    g.setFont(juce::FontOptions("Consolas", 18.0f, juce::Font::bold));
    g.setColour(juce::Colours::white);
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
        g.setColour(juce::Colour(22, 24, 32));
        g.fillRoundedRectangle(cardBounds, 10.0f);

        g.setColour(juce::Colour(42, 46, 60));
        g.drawRoundedRectangle(cardBounds, 10.0f, 1.2f);

        // Header strip inside card
        auto titleStrip = cardBounds.removeFromTop(36.0f);
        g.setColour(juce::Colour(28, 30, 40));
        g.fillRoundedRectangle(titleStrip.reduced(1.0f, 1.0f), 9.0f);

        g.setFont(juce::FontOptions(13.0f, juce::Font::bold));
        g.setColour(juce::Colour(0, 200, 255));
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
    searchBox.setBounds(col1.removeFromTop(32));
    col1.removeFromTop(8);

    auto filterRow = col1.removeFromTop(26);
    int btnW = filterRow.getWidth() / 5;
    filterAllBtn.setBounds(filterRow.removeFromLeft(btnW));
    filterSynthBtn.setBounds(filterRow.removeFromLeft(btnW));
    filterLeadBtn.setBounds(filterRow.removeFromLeft(btnW));
    filterBassBtn.setBounds(filterRow.removeFromLeft(btnW));
    filterPadBtn.setBounds(filterRow);
    col1.removeFromTop(8);

    presetListBox.setBounds(col1);

    // Column 2: Selected Preset Details & Save New
    col2.removeFromTop(32); // Title strip offset

    selectedPresetTitle.setBounds(col2.removeFromTop(28));
    statusLabel.setBounds(col2.removeFromTop(22));
    col2.removeFromTop(10);

    loadPresetBtn.setBounds(col2.removeFromTop(36));
    col2.removeFromTop(8);
    deletePresetBtn.setBounds(col2.removeFromTop(34));

    col2.removeFromTop(24); // Divider gap

    // Save Section
    saveNameInput.setBounds(col2.removeFromTop(32));
    col2.removeFromTop(8);
    saveCategoryInput.setBounds(col2.removeFromTop(32));
    col2.removeFromTop(12);
    savePresetBtn.setBounds(col2.removeFromTop(38));

    // Column 3: Sample Storage
    col3.removeFromTop(32); // Title strip offset

    sampleListBox.setBounds(col3.removeFromTop(col3.getHeight() - 48));
    col3.removeFromTop(10);
    
    auto sampleBtnRow = col3.removeFromTop(36);
    int halfW = sampleBtnRow.getWidth() / 2 - 4;
    importSampleBtn.setBounds(sampleBtnRow.removeFromLeft(halfW));
    loadSampleToEngineBtn.setBounds(sampleBtnRow.removeFromRight(halfW));
}
