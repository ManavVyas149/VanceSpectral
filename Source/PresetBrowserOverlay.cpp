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
    searchBox.setTextToShowWhenEmpty("Search presets...", SpectralUILookAndFeel::textMutedColour);
    searchBox.setColour(juce::TextEditor::backgroundColourId, juce::Colour(0x16, 0x16, 0x1C));
    searchBox.setColour(juce::TextEditor::textColourId, SpectralUILookAndFeel::bgColour);
    searchBox.setColour(juce::TextEditor::outlineColourId, juce::Colour(0x2A, 0x2A, 0x34));
    searchBox.setColour(juce::TextEditor::focusedOutlineColourId, SpectralUILookAndFeel::accentColour);
    searchBox.onTextChange = [this]() { filterPresets(); };

    // Filter Buttons
    auto setupFilterBtn = [this](juce::TextButton& btn, const juce::String& cat) {
        addAndMakeVisible(btn);
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
    presetListBox.setColour(juce::ListBox::backgroundColourId, juce::Colour(0x12, 0x12, 0x17));
    presetListBox.setColour(juce::ListBox::outlineColourId, juce::Colour(0x2A, 0x2A, 0x35));
    presetListBox.setRowHeight(38);

    // Selected Preset Header / Status Labels
    addAndMakeVisible(selectedPresetTitle);
    selectedPresetTitle.setText("No Preset Selected", juce::dontSendNotification);
    selectedPresetTitle.setFont(SpectralUILookAndFeel::getGeometricFont(15.0f, true));
    selectedPresetTitle.setColour(juce::Label::textColourId, SpectralUILookAndFeel::accentColour);

    addAndMakeVisible(statusLabel);
    statusLabel.setText("Double-click a preset to load", juce::dontSendNotification);
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
            if (presetManager.deletePreset(preset.file))
            {
                statusLabel.setText("Deleted preset: " + preset.name, juce::dontSendNotification);
                refreshPresetList();
            }
        }
    };

    // Save Preset Inputs & Button
    addAndMakeVisible(saveNameInput);
    saveNameInput.setTextToShowWhenEmpty("New Preset Name (e.g. My Lead)", SpectralUILookAndFeel::textMutedColour);
    saveNameInput.setColour(juce::TextEditor::backgroundColourId, juce::Colour(0x16, 0x16, 0x1C));
    saveNameInput.setColour(juce::TextEditor::textColourId, SpectralUILookAndFeel::bgColour);
    saveNameInput.setColour(juce::TextEditor::outlineColourId, juce::Colour(0x2A, 0x2A, 0x34));
    saveNameInput.setColour(juce::TextEditor::focusedOutlineColourId, SpectralUILookAndFeel::accentColour);

    addAndMakeVisible(saveCategoryInput);
    saveCategoryInput.setTextToShowWhenEmpty("Category (e.g. Synth, Lead, Bass)", SpectralUILookAndFeel::textMutedColour);
    saveCategoryInput.setColour(juce::TextEditor::backgroundColourId, juce::Colour(0x16, 0x16, 0x1C));
    saveCategoryInput.setColour(juce::TextEditor::textColourId, SpectralUILookAndFeel::bgColour);
    saveCategoryInput.setColour(juce::TextEditor::outlineColourId, juce::Colour(0x2A, 0x2A, 0x34));
    saveCategoryInput.setColour(juce::TextEditor::focusedOutlineColourId, SpectralUILookAndFeel::accentColour);

    addAndMakeVisible(savePresetBtn);
    savePresetBtn.setColour(juce::TextButton::buttonColourId, juce::Colour(0x2B, 0x8A, 0x5A)); // Emerald accent
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
    sampleListBox.setColour(juce::ListBox::backgroundColourId, juce::Colour(0x12, 0x12, 0x17));
    sampleListBox.setColour(juce::ListBox::outlineColourId, juce::Colour(0x2A, 0x2A, 0x35));
    sampleListBox.setRowHeight(34);

    // Sample Action Buttons
    addAndMakeVisible(importSampleBtn);
    importSampleBtn.setColour(juce::TextButton::buttonColourId, juce::Colour(0x24, 0x24, 0x2E));
    importSampleBtn.setColour(juce::TextButton::textColourOffId, SpectralUILookAndFeel::accentColour);
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

    // Name
    g.setFont(SpectralUILookAndFeel::getGeometricFont(13.0f, true));
    g.setColour(rowIsSelected ? juce::Colours::white : juce::Colour(0xE0, 0xDC, 0xD0));
    g.drawText(preset.name, 12, 0, width - 110, height, juce::Justification::centredLeft);

    // Category Badge Color
    juce::Colour catBadgeCol(0x7E, 0x7B, 0x75);
    juce::String catUp = preset.category.toUpperCase();
    if (catUp == "SYNTH") catBadgeCol = SpectralUILookAndFeel::accentColour; // Amber
    else if (catUp == "LEAD") catBadgeCol = juce::Colour(0x38, 0xBD, 0xF8); // Sky Cyan
    else if (catUp == "BASS") catBadgeCol = juce::Colour(0x2D, 0xD4, 0xBF); // Emerald
    else if (catUp == "PAD") catBadgeCol = juce::Colour(0xA8, 0x55, 0xF7); // Purple
    else if (catUp == "FX") catBadgeCol = juce::Colour(0xF4, 0x3F, 0x5E);  // Rose

    g.setFont(SpectralUILookAndFeel::getMonospaceFont(9.5f));
    auto badgeArea = juce::Rectangle<float>((float)width - 85.0f, (float)height * 0.5f - 9.0f, 75.0f, 18.0f);
    g.setColour(catBadgeCol.withAlpha(0.15f));
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
