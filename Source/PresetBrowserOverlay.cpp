#include "PresetBrowserOverlay.h"
#include "SpectrogramComponent.h"

//==============================================================================
// ModernBrowserLookAndFeel Implementation
//==============================================================================
PresetBrowserOverlay::ModernBrowserLookAndFeel::ModernBrowserLookAndFeel()
{
    setColour(juce::ScrollBar::thumbColourId, juce::Colour(0xC4, 0xB5, 0xFD).withAlpha(0.6f));
    setColour(juce::ScrollBar::backgroundColourId, juce::Colours::transparentBlack);
    setColour(juce::ComboBox::backgroundColourId, juce::Colour(0xF4, 0xF4, 0xF5));
    setColour(juce::ComboBox::outlineColourId, juce::Colour(0xE4, 0xE4, 0xE7));
    setColour(juce::ComboBox::textColourId, juce::Colour(0x18, 0x18, 0x1B));
    setColour(juce::PopupMenu::backgroundColourId, juce::Colour(0xFA, 0xFA, 0xFA));
    setColour(juce::PopupMenu::textColourId, juce::Colour(0x18, 0x18, 0x1B));
    setColour(juce::PopupMenu::highlightedBackgroundColourId, juce::Colour(0x8B, 0x5C, 0xF6).withAlpha(0.12f));
    setColour(juce::PopupMenu::highlightedTextColourId, juce::Colour(0x18, 0x18, 0x1B));
}

juce::Font PresetBrowserOverlay::ModernBrowserLookAndFeel::getTextButtonFont(juce::TextButton&, int)
{
    return SpectralUILookAndFeel::getSpaceGrotesk(10.0f, true);
}

void PresetBrowserOverlay::ModernBrowserLookAndFeel::drawButtonBackground(
    juce::Graphics& g, juce::Button& button,
    const juce::Colour& backgroundColour,
    bool shouldDrawButtonAsHighlighted,
    bool shouldDrawButtonAsDown)
{
    juce::ignoreUnused(backgroundColour);
    auto bounds = button.getLocalBounds().toFloat().reduced(0.5f);
    float cornerRadius = 4.0f;

    bool isPrimary = button.getName() == "PRIMARY";
    bool isDanger  = button.getName() == "DANGER";
    bool isPill    = button.getClickingTogglesState();
    bool isToggled = button.getToggleState();

    juce::Colour fillColour;
    juce::Colour borderColour;

    if (isPrimary)
    {
        fillColour = shouldDrawButtonAsDown ? juce::Colour(0x7C, 0x3A, 0xED)
                   : shouldDrawButtonAsHighlighted ? juce::Colour(0x90, 0x61, 0xF9)
                   : SpectralUILookAndFeel::accentColour;
        borderColour = fillColour.darker(0.1f);
    }
    else if (isDanger)
    {
        fillColour = shouldDrawButtonAsDown ? juce::Colour(0xDC, 0x26, 0x26)
                   : shouldDrawButtonAsHighlighted ? juce::Colour(0xEF, 0x44, 0x44).withAlpha(0.15f)
                   : juce::Colour(0xFE, 0xF2, 0xF2);
        borderColour = shouldDrawButtonAsDown ? juce::Colour(0xDC, 0x26, 0x26) : juce::Colour(0xFE, 0xCA, 0xCA);
    }
    else if (isPill)
    {
        if (isToggled)
        {
            fillColour = juce::Colour(0x18, 0x18, 0x1B);
            borderColour = juce::Colour(0x18, 0x18, 0x1B);
        }
        else
        {
            fillColour = shouldDrawButtonAsHighlighted ? juce::Colour(0xF4, 0xF4, 0xF5) : juce::Colours::white;
            borderColour = shouldDrawButtonAsHighlighted ? juce::Colour(0xA1, 0xA1, 0xAA) : juce::Colour(0xE4, 0xE4, 0xE7);
        }
    }
    else
    {
        fillColour = shouldDrawButtonAsDown ? juce::Colour(0xE4, 0xE4, 0xE7)
                   : shouldDrawButtonAsHighlighted ? juce::Colour(0xF4, 0xF4, 0xF5)
                   : juce::Colours::white;
        borderColour = shouldDrawButtonAsHighlighted ? juce::Colour(0x8B, 0x5C, 0xF6).withAlpha(0.6f) : juce::Colour(0xE4, 0xE4, 0xE7);
    }

    if (!button.isEnabled())
    {
        fillColour = juce::Colour(0xF4, 0xF4, 0xF5);
        borderColour = juce::Colour(0xE4, 0xE4, 0xE7);
    }

    g.setColour(fillColour);
    g.fillRoundedRectangle(bounds, cornerRadius);

    g.setColour(borderColour);
    g.drawRoundedRectangle(bounds, cornerRadius, 1.0f);
}

void PresetBrowserOverlay::ModernBrowserLookAndFeel::drawButtonText(
    juce::Graphics& g, juce::TextButton& button,
    bool shouldDrawButtonAsHighlighted,
    bool shouldDrawButtonAsDown)
{
    juce::ignoreUnused(shouldDrawButtonAsHighlighted, shouldDrawButtonAsDown);
    bool isPrimary = button.getName() == "PRIMARY";
    bool isDanger  = button.getName() == "DANGER";
    bool isPill    = button.getClickingTogglesState();
    bool isToggled = button.getToggleState();

    juce::Colour textCol;
    if (!button.isEnabled())
        textCol = juce::Colour(0xA1, 0xA1, 0xAA);
    else if (isPrimary)
        textCol = juce::Colours::white;
    else if (isDanger)
        textCol = shouldDrawButtonAsDown ? juce::Colours::white : juce::Colour(0xDC, 0x26, 0x26);
    else if (isPill && isToggled)
        textCol = juce::Colours::white;
    else
        textCol = juce::Colour(0x27, 0x27, 0x2A);

    g.setColour(textCol);
    g.setFont(getTextButtonFont(button, button.getHeight()));
    g.drawText(button.getButtonText(), button.getLocalBounds(), juce::Justification::centred, false);
}

void PresetBrowserOverlay::ModernBrowserLookAndFeel::drawComboBox(
    juce::Graphics& g, int width, int height, bool isButtonDown,
    int, int, int, int, juce::ComboBox& box)
{
    juce::ignoreUnused(isButtonDown);
    auto bounds = juce::Rectangle<float>(0.5f, 0.5f, (float)width - 1.0f, (float)height - 1.0f);

    g.setColour(juce::Colours::white);
    g.fillRoundedRectangle(bounds, 4.0f);

    g.setColour(box.hasKeyboardFocus(true) ? SpectralUILookAndFeel::accentColour : juce::Colour(0xE4, 0xE4, 0xE7));
    g.drawRoundedRectangle(bounds, 4.0f, 1.0f);

    // Chevron down arrow
    float arrowX = (float)width - 14.0f;
    float arrowY = (float)height * 0.5f - 2.0f;
    juce::Path p;
    p.startNewSubPath(arrowX - 4.0f, arrowY);
    p.lineTo(arrowX, arrowY + 4.0f);
    p.lineTo(arrowX + 4.0f, arrowY);
    g.setColour(juce::Colour(0x71, 0x71, 0x7A));
    g.strokePath(p, juce::PathStrokeType(1.2f));
}

juce::Font PresetBrowserOverlay::ModernBrowserLookAndFeel::getComboBoxFont(juce::ComboBox&)
{
    return SpectralUILookAndFeel::getSpaceGrotesk(10.0f, false);
}

void PresetBrowserOverlay::ModernBrowserLookAndFeel::positionComboBoxText(juce::ComboBox& box, juce::Label& label)
{
    label.setBounds(6, 0, box.getWidth() - 22, box.getHeight());
    label.setFont(getComboBoxFont(box));
    label.setColour(juce::Label::textColourId, juce::Colour(0x18, 0x18, 0x1B));
    label.setJustificationType(juce::Justification::centredLeft);
}

juce::Font PresetBrowserOverlay::ModernBrowserLookAndFeel::getLabelFont(juce::Label&)
{
    return SpectralUILookAndFeel::getSpaceGrotesk(10.5f, false);
}

void PresetBrowserOverlay::ModernBrowserLookAndFeel::drawScrollbar(
    juce::Graphics& g, juce::ScrollBar& scrollbar,
    int x, int y, int width, int height,
    bool isScrollbarVertical, int thumbStartPosition,
    int thumbSize, bool isMouseOver, bool isMouseDown)
{
    juce::ignoreUnused(scrollbar);
    if (thumbSize <= 0) return;

    juce::Colour thumbCol = juce::Colour(0xC4, 0xB5, 0xFD).withAlpha(0.70f);
    if (isMouseDown) thumbCol = juce::Colour(0x8B, 0x5C, 0xF6);
    else if (isMouseOver) thumbCol = juce::Colour(0xA7, 0x8B, 0xFA);

    g.setColour(thumbCol);
    if (isScrollbarVertical)
    {
        float w = 3.5f;
        float rx = (float)x + ((float)width - w) * 0.5f;
        g.fillRoundedRectangle(rx, (float)thumbStartPosition, w, (float)thumbSize, w * 0.5f);
    }
    else
    {
        float h = 3.5f;
        float ry = (float)y + ((float)height - h) * 0.5f;
        g.fillRoundedRectangle((float)thumbStartPosition, ry, (float)thumbSize, h, h * 0.5f);
    }
}

//==============================================================================
// CategoryBarContainer
//==============================================================================
void PresetBrowserOverlay::CategoryBarContainer::resized()
{
    auto area = getLocalBounds();
    juce::TextButton* buttons[] = {
        &owner.filterAllBtn, &owner.filterSynthBtn, &owner.filterLeadBtn,
        &owner.filterBassBtn, &owner.filterPadBtn, &owner.filterFxBtn,
        &owner.filterStatesBtn
    };

    int x = 0;
    int gap = 4;
    int h = area.getHeight();

    for (auto* btn : buttons)
    {
        int textW = juce::GlyphArrangement::getStringWidthInt(SpectralUILookAndFeel::getSpaceGrotesk(9.5f, true), btn->getButtonText());
        int w = textW + 16;
        btn->setBounds(x, 0, w, h);
        x += w + gap;
    }
}

void PresetBrowserOverlay::CategoryBarContainer::mouseWheelMove(const juce::MouseEvent&, const juce::MouseWheelDetails& wheel)
{
    owner.categoryViewport.setViewPosition(
        juce::jlimit(0,
                     juce::jmax(0, getWidth() - owner.categoryViewport.getWidth()),
                     owner.categoryViewport.getViewPositionX() - (int)(wheel.deltaX * 40.0f - wheel.deltaY * 40.0f)),
        0);
}

//==============================================================================
// DropImportZone
//==============================================================================
void PresetBrowserOverlay::DropImportZone::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat().reduced(1.0f);
    g.setColour(juce::Colour(0xF4, 0xF4, 0xF5));
    g.fillRoundedRectangle(bounds, 4.0f);

    float dashLengths[2] = { 4.0f, 3.0f };
    g.setColour(juce::Colour(0xD4, 0xD4, 0xD8));
    g.drawRoundedRectangle(bounds, 4.0f, 1.0f);
    g.drawDashedLine(juce::Line<float>(bounds.getX(), bounds.getY(), bounds.getRight(), bounds.getY()), dashLengths, 2);

    g.setFont(SpectralUILookAndFeel::getSpaceGrotesk(9.5f, false));
    g.setColour(juce::Colour(0x71, 0x71, 0x7A));
    g.drawText(juce::String::fromUTF8("+ DROP AUDIO FILE OR CLICK TO IMPORT"), bounds, juce::Justification::centred, false);
}

void PresetBrowserOverlay::DropImportZone::mouseUp(const juce::MouseEvent&)
{
    owner.fileChooser = std::make_unique<juce::FileChooser>(
        "Select Audio Sample to Import",
        juce::File::getSpecialLocation(juce::File::userMusicDirectory),
        "*.wav;*.mp3;*.flac;*.aiff;*.ogg;*.m4a");

    auto chooserFlags = juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles;
    owner.fileChooser->launchAsync(chooserFlags, [this](const juce::FileChooser& fc) {
        auto file = fc.getResult();
        if (file.existsAsFile())
        {
            auto existingDest = owner.presetManager.findMatchingSample(file);
            if (existingDest.existsAsFile())
                owner.promptDuplicateSampleImport(file, existingDest);
            else
            {
                owner.presetManager.importSample(file, false);
                owner.refreshSampleList();
            }
        }
    });
}
//==============================================================================
// HistoryFlyoutComponent Implementation
//==============================================================================
PresetBrowserOverlay::HistoryFlyoutComponent::HistoryFlyoutComponent(PresetBrowserOverlay& o)
    : owner(o)
{
    titleLabel.setText("HISTORY SNAPSHOTS", juce::dontSendNotification);
    titleLabel.setFont(SpectralUILookAndFeel::getSpaceGrotesk(10.5f, true));
    titleLabel.setColour(juce::Label::textColourId, juce::Colour(0x18, 0x18, 0x1B));
    addAndMakeVisible(titleLabel);

    closeBtn.onClick = [this]() { owner.hideHistoryFlyout(); };
    addAndMakeVisible(closeBtn);
}

void PresetBrowserOverlay::HistoryFlyoutComponent::setupChildren()
{
    addAndMakeVisible(owner.historyListBox);
    addAndMakeVisible(owner.restoreHistoryBtn);
    addAndMakeVisible(owner.clearHistoryBtn);
}

void PresetBrowserOverlay::HistoryFlyoutComponent::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat().reduced(1.0f);
    float corner = 6.0f;

    // Card background
    g.setColour(juce::Colours::white);
    g.fillRoundedRectangle(bounds, corner);

    // Chassis Border
    g.setColour(juce::Colour(0xE4, 0xE4, 0xE7));
    g.drawRoundedRectangle(bounds, corner, 1.0f);

    // Header divider line
    g.drawHorizontalLine(34, bounds.getX(), bounds.getRight());
}

void PresetBrowserOverlay::HistoryFlyoutComponent::resized()
{
    auto area = getLocalBounds().reduced(12, 8);
    auto header = area.removeFromTop(24);
    titleLabel.setBounds(header.removeFromLeft(180));
    closeBtn.setBounds(header.removeFromRight(22));

    area.removeFromTop(10);
    auto bot = area.removeFromBottom(28);
    int halfW = (bot.getWidth() - 6) / 2;
    owner.restoreHistoryBtn.setBounds(bot.removeFromLeft(halfW));
    bot.removeFromLeft(6);
    owner.clearHistoryBtn.setBounds(bot);

    area.removeFromBottom(8);
    owner.historyListBox.setBounds(area);
}

bool PresetBrowserOverlay::isHistoryFlyoutVisible() const { return historyFlyout.isVisible(); }

void PresetBrowserOverlay::toggleHistoryFlyout()
{
    if (historyFlyout.isVisible())
    {
        hideHistoryFlyout();
    }
    else
    {
        refreshHistoryList();
        historyBackdrop.setBounds(getLocalBounds());
        historyBackdrop.setVisible(true);
        historyBackdrop.toFront(true);
        historyFlyout.setVisible(true);
        historyFlyout.toFront(true);
        historyButton.setToggleState(true, juce::dontSendNotification);
        historyButton.toFront(true);
    }
}

void PresetBrowserOverlay::hideHistoryFlyout()
{
    historyFlyout.setVisible(false);
    historyBackdrop.setVisible(false);
    historyButton.setToggleState(false, juce::dontSendNotification);
}

//==============================================================================
// PresetBrowserOverlay Constructor / Destructor
//==============================================================================
PresetBrowserOverlay::PresetBrowserOverlay(PresetManager& manager, juce::AudioProcessorValueTreeState& state, HistoryManager* historyMgr)
    : presetManager(manager), apvts(state), historyManager(historyMgr)
{
    setLookAndFeel(&modernLookAndFeel);
    setWantsKeyboardFocus(true);

    // Header
    addAndMakeVisible(closeButton);
    closeButton.onClick = [this]() {
        if (onClose) onClose();
        else setVisible(false);
    };

    addAndMakeVisible(libraryIndexedBadge);
    libraryIndexedBadge.setFont(SpectralUILookAndFeel::getSpaceGrotesk(9.5f, false));
    libraryIndexedBadge.setColour(juce::Label::textColourId, juce::Colour(0x71, 0x71, 0x7A));
    libraryIndexedBadge.setColour(juce::Label::backgroundColourId, juce::Colour(0xF4, 0xF4, 0xF5));
    libraryIndexedBadge.setJustificationType(juce::Justification::centred);

    //==========================================================================
    // PANEL 1: PRESETS (Left)
    //==========================================================================
    addAndMakeVisible(presetsHeaderLabel);
    presetsHeaderLabel.setText("01 / PRESETS", juce::dontSendNotification);
    presetsHeaderLabel.setFont(SpectralUILookAndFeel::getSpaceGrotesk(10.5f, true));
    presetsHeaderLabel.setColour(juce::Label::textColourId, juce::Colour(0x18, 0x18, 0x1B));

    addAndMakeVisible(presetsCountLabel);
    presetsCountLabel.setFont(SpectralUILookAndFeel::getSpaceGrotesk(9.5f, false));
    presetsCountLabel.setColour(juce::Label::textColourId, juce::Colour(0x71, 0x71, 0x7A));
    presetsCountLabel.setJustificationType(juce::Justification::centredRight);

    addAndMakeVisible(searchBox);
    searchBox.setFont(SpectralUILookAndFeel::getSpaceGrotesk(10.5f, false));
    searchBox.setTextToShowWhenEmpty("Search presets by name or tag", juce::Colour(0xA1, 0xA1, 0xAA));
    searchBox.setColour(juce::TextEditor::backgroundColourId, juce::Colour(0xF4, 0xF4, 0xF5));
    searchBox.setColour(juce::TextEditor::outlineColourId, juce::Colour(0xE4, 0xE4, 0xE7));
    searchBox.setColour(juce::TextEditor::focusedOutlineColourId, SpectralUILookAndFeel::accentColour);
    searchBox.setColour(juce::TextEditor::textColourId, juce::Colour(0x18, 0x18, 0x1B));
    searchBox.onTextChange = [this]() { filterPresets(); };

    // Category Filter Pills
    categoryViewport.setViewedComponent(&categoryContainer, false);
    categoryViewport.setScrollBarsShown(false, false);
    addAndMakeVisible(categoryViewport);

    juce::TextButton* catBtns[] = {
        &filterAllBtn, &filterSynthBtn, &filterLeadBtn,
        &filterBassBtn, &filterPadBtn, &filterFxBtn,
        &filterStatesBtn
    };
    for (auto* btn : catBtns)
    {
        btn->setClickingTogglesState(true);
        btn->setRadioGroupId(1001);
        categoryContainer.addAndMakeVisible(btn);
    }
    filterAllBtn.setToggleState(true, juce::dontSendNotification);

    auto handleCategoryClick = [this](const juce::String& cat) {
        activeCategoryFilter = cat;
        filterPresets();
    };
    filterAllBtn.onClick    = [handleCategoryClick]() { handleCategoryClick("ALL"); };
    filterSynthBtn.onClick  = [handleCategoryClick]() { handleCategoryClick("SYNTH"); };
    filterLeadBtn.onClick   = [handleCategoryClick]() { handleCategoryClick("LEAD"); };
    filterBassBtn.onClick   = [handleCategoryClick]() { handleCategoryClick("BASS"); };
    filterPadBtn.onClick    = [handleCategoryClick]() { handleCategoryClick("PAD"); };
    filterFxBtn.onClick     = [handleCategoryClick]() { handleCategoryClick("FX"); };
    filterStatesBtn.onClick = [handleCategoryClick]() { handleCategoryClick("STATES"); };

    addAndMakeVisible(shuffleFxBtn);
    shuffleFxBtn.onClick = [this]() { executeShuffleFx(); };

    addAndMakeVisible(sortSelector);
    sortSelector.addItem("SORT: A TO Z", 1);
    sortSelector.addItem("SORT: FAVORITES", 2);
    sortSelector.addItem("SORT: RECENT", 3);
    sortSelector.setSelectedId(1, juce::dontSendNotification);
    sortSelector.onChange = [this]() {
        activeSortMode = sortSelector.getSelectedId() - 1;
        filterPresets();
    };

    addAndMakeVisible(favoriteFilterBtn);
    favoriteFilterBtn.setClickingTogglesState(true);
    favoriteFilterBtn.onClick = [this]() {
        onlyFavoritesFilter = favoriteFilterBtn.getToggleState();
        filterPresets();
    };

    presetListBox.setModel(this);
    presetListBox.setRowHeight(32);
    presetListBox.setColour(juce::ListBox::backgroundColourId, juce::Colours::transparentBlack);
    presetListBox.setColour(juce::ListBox::outlineColourId, juce::Colours::transparentBlack);
    addAndMakeVisible(presetListBox);

    //==========================================================================
    // PANEL 2: BANKS (Middle)
    //==========================================================================
    addAndMakeVisible(banksHeaderLabel);
    banksHeaderLabel.setText("02 / BANKS", juce::dontSendNotification);
    banksHeaderLabel.setFont(SpectralUILookAndFeel::getSpaceGrotesk(10.5f, true));
    banksHeaderLabel.setColour(juce::Label::textColourId, juce::Colour(0x18, 0x18, 0x1B));

    addAndMakeVisible(banksCountLabel);
    banksCountLabel.setFont(SpectralUILookAndFeel::getSpaceGrotesk(9.5f, false));
    banksCountLabel.setColour(juce::Label::textColourId, juce::Colour(0x71, 0x71, 0x7A));
    banksCountLabel.setJustificationType(juce::Justification::centredRight);

    addAndMakeVisible(newBankBtn);
    newBankBtn.onClick = [this]() { showNewBankDialog(); };

    addAndMakeVisible(bankActionsBtn);
    bankActionsBtn.onClick = [this]() { showBankActionsMenu(); };

    bankListBox.setModel(&bankListModel);
    bankListBox.setRowHeight(48);
    bankListBox.setColour(juce::ListBox::backgroundColourId, juce::Colours::transparentBlack);
    bankListBox.setColour(juce::ListBox::outlineColourId, juce::Colours::transparentBlack);
    addAndMakeVisible(bankListBox);

    //==========================================================================
    // HISTORY CONTROLS SETUP (Shared between Library panel and History popover)
    //==========================================================================
    historyListBox.setModel(&historyListModel);
    historyListBox.setRowHeight(32);
    historyListBox.setColour(juce::ListBox::backgroundColourId, juce::Colours::transparentBlack);
    historyListBox.setColour(juce::ListBox::outlineColourId, juce::Colours::transparentBlack);

    restoreHistoryBtn.onClick = [this]() { executeRestoreSelectedHistory(); };

    clearHistoryBtn.setName("DANGER");
    clearHistoryBtn.setButtonText("CLEAR ALL");
    clearHistoryBtn.onClick = [this]() {
        if (historyManager)
        {
            dismissActiveDialog();

            auto* win = new juce::AlertWindow("CLEAR HISTORY", "Clear all edit snapshots permanently?", juce::AlertWindow::QuestionIcon);
            win->addButton("Clear All", 1);
            win->addButton("Cancel", 0);
            activeAlertWindow = win;
            win->enterModalState(true, juce::ModalCallbackFunction::create([this, win](int res) {
                if (activeAlertWindow == win)
                    activeAlertWindow = nullptr;

                if (res == 1 && historyManager)
                {
                    historyManager->clearHistory();
                    selectedHistoryIndex = -1;
                    historyListBox.deselectAllRows();
                    refreshHistoryList();
                    currentSelectionType = SelectedViewType::None;
                    updateBottomBar();
                }
            }), true);
        }
    };

    //==========================================================================
    // PANEL 3 / HISTORY FLYOUT INITIALIZATION
    //==========================================================================
    if constexpr (enableLibraryPanel)
    {
        addAndMakeVisible(panel3HeaderLabel);
        panel3HeaderLabel.setText("03 / LIBRARY", juce::dontSendNotification);
        panel3HeaderLabel.setFont(SpectralUILookAndFeel::getSpaceGrotesk(10.5f, true));
        panel3HeaderLabel.setColour(juce::Label::textColourId, juce::Colour(0x18, 0x18, 0x1B));

        addAndMakeVisible(samplesTabBtn);
        samplesTabBtn.setClickingTogglesState(true);
        samplesTabBtn.setRadioGroupId(1002);
        samplesTabBtn.setToggleState(true, juce::dontSendNotification);
        samplesTabBtn.onClick = [this]() {
            isHistoryViewActive = false;
            sampleListBox.setVisible(true);
            sampleSortSelector.setVisible(true);
            dropImportZone.setVisible(true);
            historyListBox.setVisible(false);
            restoreHistoryBtn.setVisible(false);
            clearHistoryBtn.setVisible(false);
            currentSelectionType = (selectedSampleIndex >= 0 && selectedSampleIndex < allSamples.size()) ? SelectedViewType::Sample : SelectedViewType::None;
            updateBottomBar();
            resized();
        };

        addAndMakeVisible(historyTabBtn);
        historyTabBtn.setClickingTogglesState(true);
        historyTabBtn.setRadioGroupId(1002);
        historyTabBtn.onClick = [this]() {
            isHistoryViewActive = true;
            sampleListBox.setVisible(false);
            sampleSortSelector.setVisible(false);
            dropImportZone.setVisible(false);
            historyListBox.setVisible(true);
            restoreHistoryBtn.setVisible(true);
            clearHistoryBtn.setVisible(true);
            refreshHistoryList();
            currentSelectionType = (selectedHistoryIndex >= 0 && selectedHistoryIndex < allHistoryEntries.size()) ? SelectedViewType::History : SelectedViewType::None;
            updateBottomBar();
            resized();
        };

        addAndMakeVisible(sampleSortSelector);
        sampleSortSelector.addItem("SORT: RELEVANCE", 1);
        sampleSortSelector.addItem("SORT: NAME A-Z", 2);
        sampleSortSelector.setSelectedId(1, juce::dontSendNotification);
        sampleSortSelector.onChange = [this]() { refreshSampleList(); };

        sampleListBox.setModel(&sampleListModel);
        sampleListBox.setRowHeight(32);
        sampleListBox.setColour(juce::ListBox::backgroundColourId, juce::Colours::transparentBlack);
        sampleListBox.setColour(juce::ListBox::outlineColourId, juce::Colours::transparentBlack);
        addAndMakeVisible(sampleListBox);

        addAndMakeVisible(dropImportZone);
        addChildComponent(historyListBox);
        addChildComponent(restoreHistoryBtn);
        addChildComponent(clearHistoryBtn);
    }
    else
    {
        addChildComponent(historyBackdrop);
        historyBackdrop.setVisible(false);

        historyFlyout.setupChildren();
        addChildComponent(historyFlyout);
        historyFlyout.setVisible(false);

        addAndMakeVisible(historyButton);
        historyButton.onClick = [this]() { toggleHistoryFlyout(); };
    }

    //==========================================================================
    // FOOTER STRIP
    //==========================================================================
    addAndMakeVisible(bottomStatusLabel);
    bottomStatusLabel.setFont(SpectralUILookAndFeel::getSpaceGrotesk(10.0f, false));
    bottomStatusLabel.setColour(juce::Label::textColourId, juce::Colour(0x71, 0x71, 0x7A));
    bottomStatusLabel.setJustificationType(juce::Justification::centredLeft);

    addAndMakeVisible(revealFileBtn);
    revealFileBtn.onClick = [this]() {
        if (currentSelectionType == SelectedViewType::Preset && activePresetFile.existsAsFile())
            activePresetFile.revealToUser();
        else if (currentSelectionType == SelectedViewType::Sample && selectedSampleFile.existsAsFile())
            selectedSampleFile.revealToUser();
        else if (currentSelectionType == SelectedViewType::Bank)
        {
            auto dir = presetManager.getAllPresets();
            for (const auto& p : dir)
            {
                if (p.bank.equalsIgnoreCase(activeBankName) && p.file.existsAsFile())
                {
                    p.file.getParentDirectory().revealToUser();
                    return;
                }
            }
        }
    };

    addAndMakeVisible(saveAsBtn);
    saveAsBtn.onClick = [this]() { showSavePresetDialog(); };

    addAndMakeVisible(loadMainBtn);
    loadMainBtn.setName("PRIMARY");
    loadMainBtn.onClick = [this]() { executeLoadCurrentSelection(); };

    addAndMakeVisible(deleteBtn);
    deleteBtn.setName("DANGER");
    deleteBtn.onClick = [this]() { executeDeleteCurrentSelection(); };

    // Initial Load & Populate
    refreshBankList();
    refreshPresetList();
    refreshSampleList();
    refreshHistoryList();
    updateShuffleFxButtonState();

    // Default select first preset if available
    if (filteredPresets.size() > 0)
    {
        selectedPresetIndex = 0;
        activePresetFile = filteredPresets[0].file;
        currentSelectionType = SelectedViewType::Preset;
        presetListBox.selectRow(0);
    }

    updateBottomBar();
}

PresetBrowserOverlay::~PresetBrowserOverlay()
{
    hideHistoryFlyout();
    dismissActiveDialog();
    setLookAndFeel(nullptr);
}

void PresetBrowserOverlay::visibilityChanged()
{
    if (!isVisible())
    {
        hideHistoryFlyout();
        dismissActiveDialog();
    }
}

void PresetBrowserOverlay::dismissActiveDialog()
{
    if (activeAlertWindow != nullptr)
    {
        activeAlertWindow->exitModalState(0);
        activeAlertWindow = nullptr;
    }
}

//==============================================================================
// Refresh Methods
//==============================================================================
void PresetBrowserOverlay::refreshBankList()
{
    presetManager.invalidateBanksCache();
    allBanks.clear();
    allBanks.add("ALL BANKS");

    auto managerBanks = presetManager.getAllBanks();
    for (const auto& b : managerBanks)
    {
        if (!allBanks.contains(b, true))
            allBanks.add(b);
    }

    banksCountLabel.setText(juce::String::formatted("%d BANKS", allBanks.size() - 1), juce::dontSendNotification);
    bankListBox.updateContent();
    bankListBox.repaint();

    // Re-validate selected bank
    int bankIdx = allBanks.indexOf(activeBankFilter, true);
    if (bankIdx >= 0)
    {
        selectedBankIndex = bankIdx;
        bankListBox.selectRow(selectedBankIndex);
    }
    else
    {
        selectedBankIndex = 0;
        activeBankFilter = "ALL BANKS";
        bankListBox.selectRow(0);
    }
}

void PresetBrowserOverlay::refreshPresetList()
{
    presetManager.invalidatePresetsCache();
    allPresets = presetManager.getAllPresets();
    filterPresets();
    libraryIndexedBadge.setText(juce::String::fromUTF8("   LIBRARY INDEXED \xc2\xb7 ") + juce::String(allPresets.size() + allSamples.size()) + " ITEMS", juce::dontSendNotification);
    updateShuffleFxButtonState();
}

void PresetBrowserOverlay::refreshSampleList()
{
    presetManager.invalidateSamplesCache();
    allSamples = presetManager.getAllSamples();
    if (sampleSortSelector.getSelectedId() == 2)
    {
        std::sort(allSamples.begin(), allSamples.end(), [](const juce::File& a, const juce::File& b) {
            return a.getFileName().compareIgnoreCase(b.getFileName()) < 0;
        });
    }

    sampleListBox.updateContent();
    sampleListBox.repaint();
    libraryIndexedBadge.setText(juce::String::fromUTF8("   LIBRARY INDEXED \xc2\xb7 ") + juce::String(allPresets.size() + allSamples.size()) + " ITEMS", juce::dontSendNotification);
    samplesTabBtn.setButtonText(juce::String::formatted("SAMPLES (%d)", allSamples.size()));
}

void PresetBrowserOverlay::refreshHistoryList()
{
    if (historyManager)
    {
        historyManager->invalidateHistoryCache();
        allHistoryEntries = historyManager->getHistoryEntries();
    }
    else
    {
        allHistoryEntries.clear();
    }

    historyListBox.updateContent();
    historyListBox.repaint();
    historyTabBtn.setButtonText(juce::String::formatted("HISTORY (%d)", allHistoryEntries.size()));
}

void PresetBrowserOverlay::checkForExternalLibraryChangesAsync()
{
    juce::Thread::launch([this]() {
        if (presetManager.checkForExternalChanges())
        {
            juce::MessageManager::callAsync([safeThis = juce::Component::SafePointer<PresetBrowserOverlay>(this)]() {
                if (safeThis != nullptr)
                {
                    safeThis->refreshBankList();
                    safeThis->refreshPresetList();
                    safeThis->refreshSampleList();
                }
            });
        }
    });
}

//==============================================================================
// Filter Logic
//==============================================================================
void PresetBrowserOverlay::filterPresets()
{
    juce::String query = searchBox.getText().trim().toLowerCase();
    filteredPresets.clear();

    for (const auto& p : allPresets)
    {
        if (onlyFavoritesFilter && !p.isFavorite)
            continue;

        if (activeCategoryFilter != "ALL")
        {
            if (activeCategoryFilter == "STATES")
            {
                if (!p.file.getFileExtension().equalsIgnoreCase(".vsts") && !p.category.equalsIgnoreCase("STATES"))
                    continue;
            }
            else if (activeCategoryFilter == "FX")
            {
                if (!p.category.equalsIgnoreCase("FX"))
                    continue;
            }
            else if (!p.category.equalsIgnoreCase(activeCategoryFilter))
                continue;
        }

        if (activeBankFilter != "ALL BANKS")
        {
            if (!p.bank.equalsIgnoreCase(activeBankFilter))
                continue;
        }

        if (query.isNotEmpty())
        {
            bool nameMatch = p.name.toLowerCase().contains(query);
            bool catMatch  = p.category.toLowerCase().contains(query);
            bool bankMatch = p.bank.toLowerCase().contains(query);
            if (!nameMatch && !catMatch && !bankMatch)
                continue;
        }

        filteredPresets.add(p);
    }

    if (activeSortMode == 0) // A-Z
    {
        std::sort(filteredPresets.begin(), filteredPresets.end(), [](const PresetInfo& a, const PresetInfo& b) {
            return a.name.compareIgnoreCase(b.name) < 0;
        });
    }
    else if (activeSortMode == 1) // Favorites First
    {
        std::sort(filteredPresets.begin(), filteredPresets.end(), [](const PresetInfo& a, const PresetInfo& b) {
            if (a.isFavorite != b.isFavorite)
                return a.isFavorite > b.isFavorite;
            return a.name.compareIgnoreCase(b.name) < 0;
        });
    }
    else if (activeSortMode == 2) // Recent
    {
        std::sort(filteredPresets.begin(), filteredPresets.end(), [](const PresetInfo& a, const PresetInfo& b) {
            return a.lastUsed > b.lastUsed;
        });
    }

    presetsCountLabel.setText(juce::String::formatted("%d RESULTS", filteredPresets.size()), juce::dontSendNotification);
    presetListBox.updateContent();
    presetListBox.repaint();

    // Validate selectedPresetIndex
    if (selectedPresetIndex >= 0 && selectedPresetIndex < filteredPresets.size())
    {
        activePresetFile = filteredPresets[selectedPresetIndex].file;
        presetListBox.selectRow(selectedPresetIndex);
    }
    else if (filteredPresets.size() > 0 && currentSelectionType == SelectedViewType::Preset)
    {
        selectedPresetIndex = 0;
        activePresetFile = filteredPresets[0].file;
        presetListBox.selectRow(0);
    }
    else if (filteredPresets.isEmpty() && currentSelectionType == SelectedViewType::Preset)
    {
        clearActivePresetSelection();
    }

    updateBottomBar();
}

void PresetBrowserOverlay::clearActivePresetSelection()
{
    selectedPresetIndex = -1;
    activePresetFile = juce::File();
    activeLoadedPresetName = "";
    presetListBox.deselectAllRows();
    if (currentSelectionType == SelectedViewType::Preset)
        currentSelectionType = SelectedViewType::None;
    updateBottomBar();
}

void PresetBrowserOverlay::syncActivePresetFromProcessor(const juce::String& loadedPresetName)
{
    activeLoadedPresetName = loadedPresetName;
    for (int i = 0; i < filteredPresets.size(); ++i)
    {
        if (filteredPresets[i].name.equalsIgnoreCase(loadedPresetName))
        {
            selectedPresetIndex = i;
            activePresetFile = filteredPresets[i].file;
            currentSelectionType = SelectedViewType::Preset;
            presetListBox.selectRow(i);
            break;
        }
    }
    if (selectedPresetIndex < 0 || !activePresetFile.existsAsFile())
    {
        for (const auto& p : allPresets)
        {
            if (p.name.equalsIgnoreCase(loadedPresetName))
            {
                activeBankFilter = p.bank;
                activeBankName = p.bank;
                filterPresets();
                for (int i = 0; i < filteredPresets.size(); ++i)
                {
                    if (filteredPresets[i].name.equalsIgnoreCase(loadedPresetName))
                    {
                        selectedPresetIndex = i;
                        activePresetFile = filteredPresets[i].file;
                        currentSelectionType = SelectedViewType::Preset;
                        presetListBox.selectRow(i);
                        break;
                    }
                }
                break;
            }
        }
    }
    updateBottomBar();
}

void PresetBrowserOverlay::updateShuffleFxButtonState()
{
    int fxCount = 0;
    for (const auto& p : allPresets)
    {
        if (p.file.getFileExtension().equalsIgnoreCase(".vsfx") || p.category.equalsIgnoreCase("FX"))
            fxCount++;
    }
    shuffleFxBtn.setEnabled(fxCount > 1);
}

void PresetBrowserOverlay::executeShuffleFx()
{
    juce::Array<PresetInfo> fxPresets;
    for (const auto& p : allPresets)
    {
        if (p.file.getFileExtension().equalsIgnoreCase(".vsfx") || p.category.equalsIgnoreCase("FX"))
            fxPresets.add(p);
    }

    if (fxPresets.size() < 2) return;

    juce::Random rng;
    int idx = rng.nextInt(fxPresets.size());
    if (fxPresets[idx].file == lastShuffledPresetFile && fxPresets.size() > 1)
        idx = (idx + 1) % fxPresets.size();

    auto chosen = fxPresets[idx];
    lastShuffledPresetFile = chosen.file;
    lastShuffledBank = chosen.bank;

    if (onPresetSelected)
        onPresetSelected(chosen.file, chosen.sampleFileName);

    syncActivePresetFromProcessor(chosen.name);
}

juce::Array<PresetInfo> PresetBrowserOverlay::getNavigablePresetsForBank(const juce::String& bankName) const
{
    juce::Array<PresetInfo> list;
    for (const auto& p : filteredPresets)
    {
        if (bankName.equalsIgnoreCase("ALL BANKS") || p.bank.equalsIgnoreCase(bankName))
            list.add(p);
    }
    if (list.isEmpty() && !bankName.equalsIgnoreCase("ALL BANKS"))
    {
        for (const auto& p : allPresets)
        {
            if (p.bank.equalsIgnoreCase(bankName))
            {
                if (onlyFavoritesFilter && !p.isFavorite) continue;
                if (activeCategoryFilter != "ALL" && !p.category.equalsIgnoreCase(activeCategoryFilter)) continue;
                list.add(p);
            }
        }
    }
    return list;
}

//==============================================================================
// Bottom Bar & Context Status
//==============================================================================
void PresetBrowserOverlay::updateBottomBar()
{
    if (currentSelectionType == SelectedViewType::Preset && activePresetFile.existsAsFile())
    {
        juce::String name = activePresetFile.getFileNameWithoutExtension();
        juce::String bank = "USER";
        for (const auto& p : allPresets)
        {
            if (p.file == activePresetFile) { bank = p.bank; break; }
        }

        bottomStatusLabel.setText(juce::String::fromUTF8("\xe2\x97\x8f PRESET: ") + name.toUpperCase() + " [" + bank.toUpperCase() + juce::String::fromUTF8("] \xc2\xb7 DOUBLE-CLICK OR ENTER TO LOAD"), juce::dontSendNotification);
        revealFileBtn.setVisible(true);
        loadMainBtn.setButtonText(juce::String::fromUTF8("LOAD PRESET \xe2\x9c\x93"));
        loadMainBtn.setEnabled(true);

        bool canDel = presetManager.isPresetDeletable(activePresetFile);
        deleteBtn.setVisible(true);
        deleteBtn.setEnabled(canDel);
        deleteBtn.setButtonText(canDel ? "DELETE PRESET" : "FACTORY LOCKED");
    }
    else if (currentSelectionType == SelectedViewType::Bank)
    {
        int count = presetManager.getPresetCountForBank(activeBankName);
        bottomStatusLabel.setText(juce::String::fromUTF8("\xe2\x97\x8f BANK: ") + activeBankName.toUpperCase() + juce::String::fromUTF8(" \xc2\xb7 ") + juce::String(count) + " PRESETS AVAILABLE", juce::dontSendNotification);
        revealFileBtn.setVisible(!activeBankName.equalsIgnoreCase("ALL BANKS"));
        loadMainBtn.setEnabled(false);

        bool canDel = !activeBankName.equalsIgnoreCase("ALL BANKS") && !activeBankName.equalsIgnoreCase("Factory") && !activeBankName.equalsIgnoreCase("User");
        deleteBtn.setVisible(true);
        deleteBtn.setEnabled(canDel);
        deleteBtn.setButtonText(canDel ? "DELETE BANK" : "PROTECTED BANK");
    }
    else if (currentSelectionType == SelectedViewType::Sample && selectedSampleFile.existsAsFile())
    {
        bottomStatusLabel.setText(juce::String::fromUTF8("\xe2\x97\x8f SAMPLE: ") + selectedSampleFile.getFileName().toUpperCase() + juce::String::fromUTF8(" \xc2\xb7 DOUBLE-CLICK TO LOAD INTO ENGINE"), juce::dontSendNotification);
        revealFileBtn.setVisible(true);
        loadMainBtn.setButtonText(juce::String::fromUTF8("LOAD SAMPLE \xe2\x9c\x93"));
        loadMainBtn.setEnabled(true);

        deleteBtn.setVisible(true);
        deleteBtn.setEnabled(true);
        deleteBtn.setButtonText("DELETE SAMPLE");
    }
    else if (currentSelectionType == SelectedViewType::History && selectedHistoryIndex >= 0 && selectedHistoryIndex < allHistoryEntries.size())
    {
        const auto& h = allHistoryEntries[selectedHistoryIndex];
        bottomStatusLabel.setText(juce::String::fromUTF8("\xe2\x97\x8f SNAPSHOT: ") + h.label.toUpperCase() + " (" + h.formattedTime + ")", juce::dontSendNotification);
        revealFileBtn.setVisible(false);
        loadMainBtn.setButtonText(juce::String::fromUTF8("RESTORE SNAPSHOT"));
        loadMainBtn.setEnabled(true);

        deleteBtn.setVisible(true);
        deleteBtn.setEnabled(true);
        deleteBtn.setButtonText("DELETE SNAPSHOT");
    }
    else
    {
        bottomStatusLabel.setText(juce::String::fromUTF8("\xe2\x97\x8f NO ITEM SELECTED \xc2\xb7 SELECT A PRESET, BANK OR SAMPLE"), juce::dontSendNotification);
        revealFileBtn.setVisible(false);
        loadMainBtn.setEnabled(false);
        deleteBtn.setVisible(false);
    }
}

//==============================================================================
// Execution Commands
//==============================================================================
void PresetBrowserOverlay::executeLoadCurrentSelection()
{
    if (currentSelectionType == SelectedViewType::Preset && activePresetFile.existsAsFile())
    {
        juce::String sampleFile;
        for (const auto& p : allPresets)
        {
            if (p.file == activePresetFile) { sampleFile = p.sampleFileName; break; }
        }

        if (onPresetSelected)
            onPresetSelected(activePresetFile, sampleFile);

        syncActivePresetFromProcessor(activePresetFile.getFileNameWithoutExtension());
    }
    else if (currentSelectionType == SelectedViewType::Sample && selectedSampleFile.existsAsFile())
    {
        if (onSampleSelected)
            onSampleSelected(selectedSampleFile);
    }
    else if (currentSelectionType == SelectedViewType::History)
    {
        executeRestoreSelectedHistory();
    }
}

void PresetBrowserOverlay::executeRestoreSelectedHistory()
{
    if (historyManager && selectedHistoryIndex >= 0 && selectedHistoryIndex < allHistoryEntries.size())
    {
        const auto& entry = allHistoryEntries[selectedHistoryIndex];
        if (onHistoryEntryRestored)
            onHistoryEntryRestored(entry);
    }
}

void PresetBrowserOverlay::selectSampleRow(int row)
{
    if (row >= 0 && row < allSamples.size())
    {
        selectedSampleIndex = row;
        selectedSampleFile = allSamples[row];
        currentSelectionType = SelectedViewType::Sample;
        selectedPresetIndex = -1;
        presetListBox.deselectAllRows();
        sampleListBox.selectRow(row);
        updateBottomBar();
    }
}

void PresetBrowserOverlay::executeDeleteCurrentSelection()
{
    if (currentSelectionType == SelectedViewType::Preset && activePresetFile.existsAsFile())
    {
        if (!presetManager.isPresetDeletable(activePresetFile))
            return;

        dismissActiveDialog();

        auto* win = new juce::AlertWindow("DELETE PRESET", "Delete preset '" + activePresetFile.getFileNameWithoutExtension() + "' permanently?", juce::AlertWindow::QuestionIcon);
        win->addButton("Delete", 1);
        win->addButton("Cancel", 0);
        activeAlertWindow = win;
        auto fileToDelete = activePresetFile;

        win->enterModalState(true, juce::ModalCallbackFunction::create([this, win, fileToDelete](int res) {
            if (activeAlertWindow == win)
                activeAlertWindow = nullptr;

            if (res == 1)
            {
                presetManager.deletePreset(fileToDelete);
                clearActivePresetSelection();
                refreshPresetList();
            }
        }), true);
    }
    else if (currentSelectionType == SelectedViewType::Bank)
    {
        if (activeBankName.equalsIgnoreCase("ALL BANKS") || activeBankName.equalsIgnoreCase("Factory") || activeBankName.equalsIgnoreCase("User"))
            return;

        showDeleteBankDialog(activeBankName);
    }
    else if (currentSelectionType == SelectedViewType::Sample && selectedSampleFile.existsAsFile())
    {
        dismissActiveDialog();

        auto* win = new juce::AlertWindow("DELETE SAMPLE", "Delete sample '" + selectedSampleFile.getFileName() + "' permanently?", juce::AlertWindow::QuestionIcon);
        win->addButton("Delete", 1);
        win->addButton("Cancel", 0);
        activeAlertWindow = win;
        auto fileToDelete = selectedSampleFile;

        win->enterModalState(true, juce::ModalCallbackFunction::create([this, win, fileToDelete](int res) {
            if (activeAlertWindow == win)
                activeAlertWindow = nullptr;

            if (res == 1)
            {
                presetManager.deleteSample(fileToDelete);
                selectedSampleIndex = -1;
                selectedSampleFile = juce::File();
                sampleListBox.deselectAllRows();
                refreshSampleList();
                currentSelectionType = SelectedViewType::None;
                updateBottomBar();
            }
        }), true);
    }
    else if (currentSelectionType == SelectedViewType::History && selectedHistoryIndex >= 0 && selectedHistoryIndex < allHistoryEntries.size())
    {
        const auto& entry = allHistoryEntries[selectedHistoryIndex];
        dismissActiveDialog();

        auto* win = new juce::AlertWindow("DELETE SNAPSHOT", "Delete history snapshot '" + entry.label + "' permanently?", juce::AlertWindow::QuestionIcon);
        win->addButton("Delete", 1);
        win->addButton("Cancel", 0);
        activeAlertWindow = win;

        win->enterModalState(true, juce::ModalCallbackFunction::create([this, win, entry](int res) {
            if (activeAlertWindow == win)
                activeAlertWindow = nullptr;

            if (res == 1 && historyManager)
            {
                historyManager->deleteHistoryFile(entry.snapshotFile);
                selectedHistoryIndex = -1;
                historyListBox.deselectAllRows();
                refreshHistoryList();
                currentSelectionType = SelectedViewType::None;
                updateBottomBar();
            }
        }), true);
    }
}

//==============================================================================
// Bank Dialogs
//==============================================================================
void PresetBrowserOverlay::showNewBankDialog()
{
    dismissActiveDialog();

    auto* dialog = new juce::AlertWindow("NEW PRESET BANK", "Enter a name for the new bank:", juce::AlertWindow::NoIcon);
    dialog->addTextEditor("bankName", "", "Bank Name");
    dialog->addButton("Create", 1);
    dialog->addButton("Cancel", 0);
    activeAlertWindow = dialog;

    dialog->enterModalState(true, juce::ModalCallbackFunction::create([this, dialog](int res) {
        if (activeAlertWindow == dialog)
            activeAlertWindow = nullptr;

        if (res == 1)
        {
            juce::String name = dialog->getTextEditorContents("bankName").trim();
            if (name.isNotEmpty())
            {
                if (presetManager.createBank(name))
                {
                    refreshBankList();
                    activeBankFilter = name;
                    activeBankName = name;
                    int idx = allBanks.indexOf(name, true);
                    if (idx >= 0)
                    {
                        selectedBankIndex = idx;
                        bankListBox.selectRow(idx);
                    }
                    filterPresets();
                    currentSelectionType = SelectedViewType::Bank;
                    updateBottomBar();
                }
            }
        }
    }), true);
}

void PresetBrowserOverlay::showBankActionsMenu()
{
    juce::PopupMenu menu;
    menu.addItem("Import Bank Folder / Archive...", [this]() {
        fileChooser = std::make_unique<juce::FileChooser>(
            "Select Bank Folder or Package",
            juce::File::getSpecialLocation(juce::File::userDocumentsDirectory),
            "*");
        auto chooserFlags = juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectDirectories | juce::FileBrowserComponent::canSelectFiles;
        fileChooser->launchAsync(chooserFlags, [this](const juce::FileChooser& fc) {
            auto res = fc.getResult();
            if (res.exists())
            {
                presetManager.importBankPackage(res);
                refreshBankList();
                refreshPresetList();
            }
        });
    });

    menu.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(&bankActionsBtn));
}

void PresetBrowserOverlay::showDeleteBankDialog(const juce::String& bankToDelete)
{
    if (bankToDelete.equalsIgnoreCase("ALL BANKS") || bankToDelete.equalsIgnoreCase("Factory") || bankToDelete.equalsIgnoreCase("User"))
        return;

    dismissActiveDialog();

    auto* dialog = new juce::AlertWindow("DELETE BANK",
        "Delete bank '" + bankToDelete + "' and all its presets permanently? This cannot be undone.",
        juce::AlertWindow::QuestionIcon);
    dialog->addButton("Delete", 1);
    dialog->addButton("Cancel", 0);
    activeAlertWindow = dialog;

    dialog->enterModalState(true, juce::ModalCallbackFunction::create([this, bankToDelete, dialog](int res) {
        if (activeAlertWindow == dialog)
            activeAlertWindow = nullptr;

        if (res == 1)
        {
            presetManager.deleteBank(bankToDelete);
            if (activeBankFilter.equalsIgnoreCase(bankToDelete))
            {
                activeBankFilter = "ALL BANKS";
                activeBankName = "ALL BANKS";
                selectedBankIndex = 0;
            }
            refreshBankList();
            filterPresets();
            if (currentSelectionType == SelectedViewType::Bank && activeBankName.equalsIgnoreCase(bankToDelete))
                currentSelectionType = SelectedViewType::None;
            updateBottomBar();
        }
    }), true);
}

void PresetBrowserOverlay::showBankCardContextMenu(const juce::String& bankName)
{
    if (bankName.equalsIgnoreCase("ALL BANKS"))
        return;

    juce::PopupMenu menu;
    bool isFactory = bankName.equalsIgnoreCase("Factory");
    bool isUser = bankName.equalsIgnoreCase("User");

    if (isFactory)
    {
        menu.addItem(1, "Rename Bank (Factory Protected)", false, false);
        menu.addItem(2, "Delete Bank (Factory Protected)", false, false);
    }
    else if (isUser)
    {
        menu.addItem(1, "Rename Bank (Default Protected)", false, false);
        menu.addItem(2, "Delete Bank (Default Protected)", false, false);
    }
    else
    {
        menu.addItem("Rename", [this, bankName]() {
            showRenameBankDialog(bankName);
        });

        menu.addItem("Delete", [this, bankName]() {
            showDeleteBankDialog(bankName);
        });
    }

    menu.showMenuAsync(juce::PopupMenu::Options());
}

void PresetBrowserOverlay::showRenameBankDialog(const juce::String& bankName)
{
    dismissActiveDialog();

    auto* dialog = new juce::AlertWindow("RENAME BANK", "Enter new name for bank '" + bankName + "':", juce::AlertWindow::NoIcon);
    dialog->addTextEditor("newBankName", bankName, "Bank Name");
    dialog->addButton("Rename", 1);
    dialog->addButton("Cancel", 0);
    activeAlertWindow = dialog;

    dialog->enterModalState(true, juce::ModalCallbackFunction::create([this, bankName, dialog](int res) {
        if (activeAlertWindow == dialog)
            activeAlertWindow = nullptr;

        if (res == 1)
        {
            juce::String newName = dialog->getTextEditorContents("newBankName").trim();
            if (newName.isNotEmpty() && !newName.equalsIgnoreCase(bankName))
            {
                if (presetManager.renameBank(bankName, newName))
                {
                    refreshBankList();
                    activeBankFilter = newName;
                    activeBankName = newName;
                    int idx = allBanks.indexOf(newName, true);
                    if (idx >= 0)
                    {
                        selectedBankIndex = idx;
                        bankListBox.selectRow(idx);
                    }
                    filterPresets();
                    updateBottomBar();
                }
            }
        }
    }), true);
}

void PresetBrowserOverlay::showSavePresetDialog()
{
    dismissActiveDialog();

    juce::String defaultName = activePresetFile.existsAsFile() ? activePresetFile.getFileNameWithoutExtension() : "New Preset";
    auto* dialog = new juce::AlertWindow("SAVE PRESET", "Save current instrument state as a preset:", juce::AlertWindow::NoIcon);
    dialog->addTextEditor("presetName", defaultName, "Preset Name");

    juce::StringArray cats = { "Synth", "Lead", "Bass", "Pad", "FX", "States" };
    dialog->addComboBox("category", cats, "Category");
    dialog->getComboBoxComponent("category")->setSelectedItemIndex(0);

    auto banks = presetManager.getAllBanks();
    dialog->addComboBox("bank", banks, "Bank");
    dialog->getComboBoxComponent("bank")->setText(!activeBankName.equalsIgnoreCase("ALL BANKS") ? activeBankName : "User");

    dialog->addButton("Save Preset", 1);
    dialog->addButton("Cancel", 0);
    activeAlertWindow = dialog;

    dialog->enterModalState(true, juce::ModalCallbackFunction::create([this, dialog](int res) {
        if (activeAlertWindow == dialog)
            activeAlertWindow = nullptr;

        if (res == 1)
        {
            juce::String name = dialog->getTextEditorContents("presetName").trim();
            juce::String cat  = dialog->getComboBoxComponent("category")->getText();
            juce::String bank = dialog->getComboBoxComponent("bank")->getText();

            if (name.isNotEmpty())
            {
                juce::String sampleFile = currentSampleName;
                float startR = 0.0f, endR = 1.0f;
                juce::var selVar;
                bool loopEn = false;
                const juce::AudioBuffer<float>* buf = nullptr;
                double sr = 44100.0;

                if (spectrogram)
                {
                    startR = spectrogram->getStartRegion();
                    endR = spectrogram->getEndRegion();
                    selVar = spectrogram->getSelectionsAsVar();
                    loopEn = spectrogram->isLoopEnabled();
                    if (spectrogram->isFileLoaded())
                    {
                        buf = &spectrogram->getAudioBuffer();
                        sr = 44100.0;
                    }
                }

                presetManager.savePreset(name, cat, bank, sampleFile, apvts, startR, endR, selVar, false, loopEn, buf, sr);
                refreshPresetList();
                syncActivePresetFromProcessor(name);
            }
        }
    }), true);
}

void PresetBrowserOverlay::showRenameSampleDialog(int sampleRow)
{
    if (sampleRow < 0 || sampleRow >= allSamples.size()) return;
    auto file = allSamples[sampleRow];

    dismissActiveDialog();

    auto* dialog = new juce::AlertWindow("RENAME SAMPLE", "Enter new name for sample:", juce::AlertWindow::NoIcon);
    dialog->addTextEditor("sampleName", file.getFileNameWithoutExtension(), "Sample Name");
    dialog->addButton("Rename", 1);
    dialog->addButton("Cancel", 0);
    activeAlertWindow = dialog;

    dialog->enterModalState(true, juce::ModalCallbackFunction::create([this, file, dialog](int res) {
        if (activeAlertWindow == dialog)
            activeAlertWindow = nullptr;

        if (res == 1)
        {
            juce::String newName = dialog->getTextEditorContents("sampleName").trim();
            if (newName.isNotEmpty())
            {
                presetManager.renameSample(file, newName);
                refreshSampleList();
            }
        }
    }), true);
}

void PresetBrowserOverlay::promptDuplicateSampleImport(const juce::File& sourceFile, const juce::File& existingDest)
{
    dismissActiveDialog();

    auto* dialog = new juce::AlertWindow("DUPLICATE SAMPLE DETECTED",
        "A sample with identical name or content already exists:\n" + existingDest.getFileName() + "\n\nChoose an action:",
        juce::AlertWindow::QuestionIcon);

    dialog->addButton("Keep Existing", 1);
    dialog->addButton("Overwrite", 2);
    dialog->addButton("Cancel", 0);
    activeAlertWindow = dialog;

    dialog->enterModalState(true, juce::ModalCallbackFunction::create([this, dialog, sourceFile, existingDest](int res) {
        if (activeAlertWindow == dialog)
            activeAlertWindow = nullptr;

        if (res == 2)
        {
            presetManager.importSample(sourceFile, true, existingDest);
            refreshSampleList();
        }
    }), true);
}

//==============================================================================
// Preset ListBoxModel (Panel 1)
//==============================================================================
int PresetBrowserOverlay::getNumRows()
{
    return filteredPresets.size();
}

void PresetBrowserOverlay::paintListBoxItem(int rowNumber, juce::Graphics& g, int width, int height, bool rowIsSelected)
{
    if (rowNumber < 0 || rowNumber >= filteredPresets.size())
        return;

    const auto& p = filteredPresets[rowNumber];

    juce::Rectangle<int> bounds(0, 0, width, height);

    if (rowIsSelected)
    {
        g.setColour(SpectralUILookAndFeel::accentColour.withAlpha(0.12f));
        g.fillRect(bounds);
        g.setColour(SpectralUILookAndFeel::accentColour);
        g.fillRect(0, 0, 3, height);
    }
    else if (rowNumber % 2 == 1)
    {
        g.setColour(juce::Colour(0xFA, 0xFA, 0xFA));
        g.fillRect(bounds);
    }

    // Favorite Star (clickable icon)
    float starX = 8.0f;
    float starY = ((float)height - 14.0f) * 0.5f;
    if (p.isFavorite)
    {
        g.setFont(SpectralUILookAndFeel::getSpaceGrotesk(11.0f, true));
        g.setColour(juce::Colour(0xF5, 0x9E, 0x0B)); // Amber
        g.drawText(juce::String::fromUTF8("\xe2\x98\x85"), (int)starX, (int)starY, 14, 14, juce::Justification::centred, false);
    }
    else
    {
        g.setFont(SpectralUILookAndFeel::getSpaceGrotesk(11.0f, false));
        g.setColour(juce::Colour(0xD4, 0xD4, 0xD8));
        g.drawText(juce::String::fromUTF8("\xe2\x98\x86"), (int)starX, (int)starY, 14, 14, juce::Justification::centred, false);
    }

    // Preset Name
    g.setFont(SpectralUILookAndFeel::getSpaceGrotesk(10.5f, rowIsSelected));
    g.setColour(rowIsSelected ? juce::Colour(0x18, 0x18, 0x1B) : juce::Colour(0x27, 0x27, 0x2A));
    int nameW = width - 130;
    g.drawText(p.name, 26, 2, nameW, height - 4, juce::Justification::centredLeft, true);

    // Category Badge
    g.setFont(SpectralUILookAndFeel::getSpaceGrotesk(8.5f, true));
    juce::Colour catCol = p.category.equalsIgnoreCase("STATES") ? juce::Colour(0x8B, 0x5C, 0xF6) : juce::Colour(0x71, 0x71, 0x7A);
    g.setColour(catCol);
    g.drawText(p.category.toUpperCase(), width - 100, 0, 45, height, juce::Justification::centred, false);

    // Bank
    juce::String bankCol = p.bank.equalsIgnoreCase("Factory") ? juce::String::fromUTF8("\xe2\x80\x94") : p.bank.toUpperCase();
    g.setFont(SpectralUILookAndFeel::getSpaceGrotesk(9.0f, false));
    g.setColour(juce::Colour(0xA1, 0xA1, 0xAA));
    g.drawText(bankCol, width - 50, 0, 46, height, juce::Justification::centredRight, true);
}

void PresetBrowserOverlay::listBoxItemClicked(int row, const juce::MouseEvent& e)
{
    if (row >= 0 && row < filteredPresets.size())
    {
        // Check if clicking star
        if (e.x >= 6 && e.x <= 24)
        {
            presetManager.toggleFavorite(filteredPresets[row].file);
            refreshPresetList();
            return;
        }

        if (e.mods.isPopupMenu())
        {
            juce::PopupMenu menu;
            menu.addItem("Load Preset", [this, row]() {
                selectedPresetIndex = row;
                activePresetFile = filteredPresets[row].file;
                executeLoadCurrentSelection();
            });
            menu.addItem("Reveal in Explorer", [this, row]() {
                filteredPresets[row].file.revealToUser();
            });
            if (presetManager.isPresetDeletable(filteredPresets[row].file))
            {
                menu.addSeparator();
                menu.addItem("Delete Preset...", [this, row]() {
                    selectedPresetIndex = row;
                    activePresetFile = filteredPresets[row].file;
                    currentSelectionType = SelectedViewType::Preset;
                    presetListBox.selectRow(row);
                    updateBottomBar();
                    executeDeleteCurrentSelection();
                });
            }
            menu.showMenuAsync(juce::PopupMenu::Options());
            return;
        }

        selectedPresetIndex = row;
        activePresetFile = filteredPresets[row].file;
        currentSelectionType = SelectedViewType::Preset;

        // Deselect other panels
        selectedSampleIndex = -1;
        sampleListBox.deselectAllRows();
        selectedHistoryIndex = -1;
        historyListBox.deselectAllRows();

        updateBottomBar();
    }
}

void PresetBrowserOverlay::listBoxItemDoubleClicked(int row, const juce::MouseEvent&)
{
    if (row >= 0 && row < filteredPresets.size())
    {
        selectedPresetIndex = row;
        activePresetFile = filteredPresets[row].file;
        currentSelectionType = SelectedViewType::Preset;
        executeLoadCurrentSelection();
    }
}

//==============================================================================
// Bank ListBoxModel (Panel 2)
//==============================================================================
void PresetBrowserOverlay::BankListModel::paintListBoxItem(int rowNumber, juce::Graphics& g, int width, int height, bool rowIsSelected)
{
    if (rowNumber < 0 || rowNumber >= owner.allBanks.size())
        return;

    juce::String bankName = owner.allBanks[rowNumber];
    bool isAll = bankName.equalsIgnoreCase("ALL BANKS");
    bool isFactory = bankName.equalsIgnoreCase("Factory");
    int count = owner.presetManager.getPresetCountForBank(bankName);

    auto cardBounds = juce::Rectangle<float>(4.0f, 2.0f, (float)width - 8.0f, (float)height - 4.0f);
    float corner = 5.0f;

    if (rowIsSelected)
    {
        g.setColour(SpectralUILookAndFeel::accentColour.withAlpha(0.08f));
        g.fillRoundedRectangle(cardBounds, corner);
        g.setColour(SpectralUILookAndFeel::accentColour);
        g.drawRoundedRectangle(cardBounds, corner, 1.2f);
    }
    else
    {
        g.setColour(juce::Colour(0xFA, 0xFA, 0xFA));
        g.fillRoundedRectangle(cardBounds, corner);
        g.setColour(juce::Colour(0xE4, 0xE4, 0xE7));
        g.drawRoundedRectangle(cardBounds, corner, 1.0f);
    }

    // 3-dot ('⋮') overflow button on each individual bank card (aligned consistently on all cards)
    if (!isAll)
    {
        float dotBtnW = 24.0f;
        float dotBtnH = 24.0f;
        float dotBtnX = cardBounds.getRight() - 28.0f;
        float dotBtnY = cardBounds.getY() + (cardBounds.getHeight() - dotBtnH) * 0.5f;
        juce::Rectangle<float> dotRect(dotBtnX, dotBtnY, dotBtnW, dotBtnH);

        float dotX = dotRect.getCentreX();
        float dotY = dotRect.getCentreY();
        g.setColour(SpectralUILookAndFeel::textMutedColour);
        g.fillEllipse(dotX - 1.2f, dotY - 5.0f, 2.4f, 2.4f);
        g.fillEllipse(dotX - 1.2f, dotY,        2.4f, 2.4f);
        g.fillEllipse(dotX - 1.2f, dotY + 5.0f, 2.4f, 2.4f);
    }

    // Badge on right (positioned to the left of 3-dot overflow button)
    float badgeRight = !isAll ? (cardBounds.getRight() - 32.0f) : (cardBounds.getRight() - 8.0f);
    if (isFactory)
    {
        g.setFont(SpectralUILookAndFeel::getSpaceGrotesk(8.0f, true));
        g.setColour(juce::Colour(0x8B, 0x5C, 0xF6));
        g.drawText("FACTORY", (int)badgeRight - 48, (int)cardBounds.getY() + 6, 48, 16, juce::Justification::centredRight, false);
    }
    else if (!isAll)
    {
        g.setFont(SpectralUILookAndFeel::getSpaceGrotesk(8.0f, false));
        g.setColour(juce::Colour(0xA1, 0xA1, 0xAA));
        g.drawText("BANK", (int)badgeRight - 48, (int)cardBounds.getY() + 6, 48, 16, juce::Justification::centredRight, false);
    }

    // Title
    int maxTextW = (int)badgeRight - 54 - ((int)cardBounds.getX() + 10);
    g.setFont(SpectralUILookAndFeel::getSpaceGrotesk(10.5f, rowIsSelected));
    g.setColour(rowIsSelected ? juce::Colour(0x18, 0x18, 0x1B) : juce::Colour(0x27, 0x27, 0x2A));
    g.drawText(bankName, (int)cardBounds.getX() + 10, (int)cardBounds.getY() + 4, maxTextW, 20, juce::Justification::centredLeft, true);

    // Subtitle / Preset count
    g.setFont(SpectralUILookAndFeel::getSpaceGrotesk(9.0f, false));
    g.setColour(juce::Colour(0x71, 0x71, 0x7A));
    juce::String subText = isAll ? (juce::String(count) + juce::String::fromUTF8(" Presets \xc2\xb7 All Items")) : (juce::String(count) + " Presets");
    g.drawText(subText, (int)cardBounds.getX() + 10, (int)cardBounds.getY() + 24, maxTextW, 16, juce::Justification::centredLeft, true);
}

void PresetBrowserOverlay::BankListModel::listBoxItemClicked(int row, const juce::MouseEvent& e)
{
    if (row >= 0 && row < owner.allBanks.size())
    {
        juce::String bankName = owner.allBanks[row];
        bool isAll = bankName.equalsIgnoreCase("ALL BANKS");

        auto cardBounds = juce::Rectangle<float>(4.0f, 2.0f, (float)owner.bankListBox.getWidth() - 8.0f, 44.0f);
        juce::Rectangle<int> dotBtnHitArea((int)(cardBounds.getRight() - 32.0f), (int)cardBounds.getY(), 32, (int)cardBounds.getHeight());

        // 3-dot overflow button clicked
        if (!isAll && dotBtnHitArea.contains(e.x, e.y))
        {
            owner.showBankCardContextMenu(bankName);
            return;
        }

        // Right-click context menu on bank card
        if (e.mods.isPopupMenu())
        {
            if (!isAll)
            {
                owner.showBankCardContextMenu(bankName);
            }
            return;
        }

        owner.selectedBankIndex = row;
        owner.activeBankFilter = bankName;
        owner.activeBankName = bankName;
        owner.currentSelectionType = SelectedViewType::Bank;

        owner.filterPresets();
        owner.updateBottomBar();

        if (owner.onBankSelected)
            owner.onBankSelected(bankName);
    }
}

void PresetBrowserOverlay::BankListModel::listBoxItemDoubleClicked(int row, const juce::MouseEvent&)
{
    if (row >= 0 && row < owner.allBanks.size())
    {
        owner.selectedBankIndex = row;
        owner.activeBankFilter = owner.allBanks[row];
        owner.activeBankName = owner.allBanks[row];
        owner.filterPresets();
        owner.updateBottomBar();
    }
}

//==============================================================================
// Sample ListBoxModel (Panel 3)
//==============================================================================
void PresetBrowserOverlay::SampleListModel::paintListBoxItem(int rowNumber, juce::Graphics& g, int width, int height, bool rowIsSelected)
{
    if (rowNumber < 0 || rowNumber >= owner.allSamples.size())
        return;

    const auto& file = owner.allSamples[rowNumber];
    juce::Rectangle<int> bounds(0, 0, width, height);

    if (rowIsSelected)
    {
        g.setColour(SpectralUILookAndFeel::accentColour.withAlpha(0.12f));
        g.fillRect(bounds);
        g.setColour(SpectralUILookAndFeel::accentColour);
        g.fillRect(0, 0, 3, height);
    }
    else if (rowNumber % 2 == 1)
    {
        g.setColour(juce::Colour(0xFA, 0xFA, 0xFA));
        g.fillRect(bounds);
    }

    // Audio Play icon
    g.setColour(rowIsSelected ? SpectralUILookAndFeel::accentColour : juce::Colour(0xA1, 0xA1, 0xAA));
    juce::Path tri;
    float tx = bounds.getX() + 8.0f;
    float ty = bounds.getCentreY() - 4.0f;
    tri.startNewSubPath(tx, ty);
    tri.lineTo(tx + 5.0f, ty + 4.0f);
    tri.lineTo(tx, ty + 8.0f);
    tri.closeSubPath();
    g.fillPath(tri);

    // Sample filename
    g.setFont(SpectralUILookAndFeel::getSpaceGrotesk(10.0f, rowIsSelected));
    g.setColour(rowIsSelected ? juce::Colour(0x18, 0x18, 0x1B) : juce::Colour(0x27, 0x27, 0x2A));
    int fileW = width - 85;
    g.drawText(file.getFileName(), 22, 2, fileW, height - 4, juce::Justification::centredLeft, true);

    // Format badge (derived purely from extension, ZERO disk I/O in paint!)
    juce::String ext = file.getFileExtension().replace(".", "").toUpperCase();
    if (ext.isEmpty()) ext = "WAV";
    g.setFont(SpectralUILookAndFeel::getSpaceGrotesk(8.5f, true));
    g.setColour(juce::Colour(0x71, 0x71, 0x7A));
    g.drawText(ext, width - 60, 0, 52, height, juce::Justification::centredRight, false);
}

void PresetBrowserOverlay::SampleListModel::listBoxItemClicked(int row, const juce::MouseEvent& e)
{
    if (row >= 0 && row < owner.allSamples.size())
    {
        if (e.mods.isPopupMenu())
        {
            juce::PopupMenu menu;
            menu.addItem("Load Sample into Engine", [this, row]() {
                if (owner.onSampleSelected) owner.onSampleSelected(owner.allSamples[row]);
            });
            menu.addItem("Reveal in Explorer", [this, row]() {
                owner.allSamples[row].revealToUser();
            });
            menu.addItem("Rename Sample...", [this, row]() {
                owner.showRenameSampleDialog(row);
            });
            menu.addSeparator();
            menu.addItem("Delete Sample...", [this, row]() {
                owner.selectedSampleIndex = row;
                owner.selectedSampleFile = owner.allSamples[row];
                owner.currentSelectionType = SelectedViewType::Sample;
                owner.sampleListBox.selectRow(row);
                owner.updateBottomBar();
                owner.executeDeleteCurrentSelection();
            });
            menu.showMenuAsync(juce::PopupMenu::Options());
            return;
        }

        owner.selectedSampleIndex = row;
        owner.selectedSampleFile = owner.allSamples[row];
        owner.currentSelectionType = SelectedViewType::Sample;

        // Deselect preset list
        owner.selectedPresetIndex = -1;
        owner.presetListBox.deselectAllRows();

        owner.updateBottomBar();
    }
}

void PresetBrowserOverlay::SampleListModel::listBoxItemDoubleClicked(int row, const juce::MouseEvent&)
{
    if (row >= 0 && row < owner.allSamples.size())
    {
        owner.selectedSampleIndex = row;
        owner.selectedSampleFile = owner.allSamples[row];
        owner.currentSelectionType = SelectedViewType::Sample;
        owner.executeLoadCurrentSelection();
    }
}

juce::var PresetBrowserOverlay::SampleListModel::getDragSourceDescription(const juce::SparseSet<int>& selectedRows)
{
    if (!selectedRows.isEmpty())
    {
        int row = selectedRows[0];
        if (row >= 0 && row < owner.allSamples.size())
            return owner.allSamples[row].getFullPathName();
    }
    return {};
}

//==============================================================================
// History ListBoxModel (Panel 3)
//==============================================================================
void PresetBrowserOverlay::HistoryListModel::paintListBoxItem(int rowNumber, juce::Graphics& g, int width, int height, bool rowIsSelected)
{
    if (rowNumber < 0 || rowNumber >= owner.allHistoryEntries.size())
        return;

    const auto& entry = owner.allHistoryEntries[rowNumber];
    juce::Rectangle<int> bounds(0, 0, width, height);

    if (rowIsSelected)
    {
        g.setColour(SpectralUILookAndFeel::accentColour.withAlpha(0.12f));
        g.fillRect(bounds);
        g.setColour(SpectralUILookAndFeel::accentColour);
        g.fillRect(0, 0, 3, height);
    }
    else if (rowNumber % 2 == 1)
    {
        g.setColour(juce::Colour(0xFA, 0xFA, 0xFA));
        g.fillRect(bounds);
    }

    g.setFont(SpectralUILookAndFeel::getSpaceGrotesk(10.0f, rowIsSelected));
    g.setColour(rowIsSelected ? juce::Colour(0x18, 0x18, 0x1B) : juce::Colour(0x27, 0x27, 0x2A));
    g.drawText(entry.label, 12, 2, width - 110, height - 4, juce::Justification::centredLeft, true);

    g.setFont(SpectralUILookAndFeel::getSpaceGrotesk(9.0f, false));
    g.setColour(juce::Colour(0x71, 0x71, 0x7A));
    g.drawText(entry.formattedTime, width - 100, 0, 92, height, juce::Justification::centredRight, false);
}

void PresetBrowserOverlay::HistoryListModel::listBoxItemClicked(int row, const juce::MouseEvent& e)
{
    if (row >= 0 && row < owner.allHistoryEntries.size())
    {
        if (e.mods.isPopupMenu())
        {
            juce::PopupMenu menu;
            menu.addItem("Restore this Snapshot", [this, row]() {
                owner.selectedHistoryIndex = row;
                owner.executeRestoreSelectedHistory();
            });
            menu.addItem("Delete Snapshot...", [this, row]() {
                owner.selectedHistoryIndex = row;
                owner.currentSelectionType = SelectedViewType::History;
                owner.historyListBox.selectRow(row);
                owner.updateBottomBar();
                owner.executeDeleteCurrentSelection();
            });
            menu.showMenuAsync(juce::PopupMenu::Options());
            return;
        }

        owner.selectedHistoryIndex = row;
        owner.currentSelectionType = SelectedViewType::History;
        owner.updateBottomBar();
    }
}

void PresetBrowserOverlay::HistoryListModel::listBoxItemDoubleClicked(int row, const juce::MouseEvent&)
{
    if (row >= 0 && row < owner.allHistoryEntries.size())
    {
        owner.selectedHistoryIndex = row;
        owner.executeRestoreSelectedHistory();
    }
}

//==============================================================================
// Drag and Drop Audio Import
//==============================================================================
bool PresetBrowserOverlay::isInterestedInFileDrag(const juce::StringArray& files)
{
    for (const auto& file : files)
    {
        juce::String ext = juce::File(file).getFileExtension().toLowerCase();
        if (ext == ".wav" || ext == ".mp3" || ext == ".flac" || ext == ".aiff" || ext == ".ogg" || ext == ".m4a")
            return true;
    }
    return false;
}

void PresetBrowserOverlay::filesDropped(const juce::StringArray& files, int, int)
{
    for (const auto& fPath : files)
    {
        juce::File file(fPath);
        if (file.existsAsFile())
        {
            auto existingDest = presetManager.findMatchingSample(file);
            if (existingDest.existsAsFile())
                promptDuplicateSampleImport(file, existingDest);
            else
                presetManager.importSample(file, false);
        }
    }
    refreshSampleList();
}

//==============================================================================
// Keyboard Navigation
//==============================================================================
bool PresetBrowserOverlay::keyPressed(const juce::KeyPress& key)
{
    if (key == juce::KeyPress::escapeKey)
    {
        if (historyFlyout.isVisible())
        {
            hideHistoryFlyout();
            return true;
        }

        if (onClose) onClose();
        else setVisible(false);
        return true;
    }

    if (key == juce::KeyPress::returnKey)
    {
        executeLoadCurrentSelection();
        return true;
    }

    if (key == juce::KeyPress::upKey)
    {
        if (currentSelectionType == SelectedViewType::Preset && selectedPresetIndex > 0)
        {
            selectedPresetIndex--;
            activePresetFile = filteredPresets[selectedPresetIndex].file;
            presetListBox.selectRow(selectedPresetIndex);
            updateBottomBar();
            return true;
        }
        else if (currentSelectionType == SelectedViewType::Sample && selectedSampleIndex > 0)
        {
            selectedSampleIndex--;
            selectedSampleFile = allSamples[selectedSampleIndex];
            sampleListBox.selectRow(selectedSampleIndex);
            updateBottomBar();
            return true;
        }
    }
    else if (key == juce::KeyPress::downKey)
    {
        if (currentSelectionType == SelectedViewType::Preset && selectedPresetIndex < filteredPresets.size() - 1)
        {
            selectedPresetIndex++;
            activePresetFile = filteredPresets[selectedPresetIndex].file;
            presetListBox.selectRow(selectedPresetIndex);
            updateBottomBar();
            return true;
        }
        else if (currentSelectionType == SelectedViewType::Sample && selectedSampleIndex < allSamples.size() - 1)
        {
            selectedSampleIndex++;
            selectedSampleFile = allSamples[selectedSampleIndex];
            sampleListBox.selectRow(selectedSampleIndex);
            updateBottomBar();
            return true;
        }
    }

    return false;
}

//==============================================================================
// Paint / Layout
//==============================================================================
void PresetBrowserOverlay::paint(juce::Graphics& g)
{
    // Translucent dark backdrop
    g.fillAll(juce::Colour(0x0A, 0x0A, 0x0C).withAlpha(0.72f));

    // Modal Card
    auto bounds = getLocalBounds().reduced(24).toFloat();
    float corner = 8.0f;

    // Outer drop shadow
    juce::Path p;
    juce::DropShadow shadow(juce::Colours::black.withAlpha(0.28f), 16, juce::Point<int>(0, 4));
    shadow.drawForPath(g, p);

    // Modal Background
    g.setColour(juce::Colours::white);
    g.fillRoundedRectangle(bounds, corner);

    // Chassis Border
    g.setColour(juce::Colour(0xE4, 0xE4, 0xE7));
    g.drawRoundedRectangle(bounds, corner, 1.0f);

    // Header strip separator
    float headerH = 46.0f;
    g.setColour(juce::Colour(0xE4, 0xE4, 0xE7));
    g.drawHorizontalLine((int)(bounds.getY() + headerH), bounds.getX(), bounds.getRight());

    // Title text
    g.setFont(SpectralUILookAndFeel::getSpaceGrotesk(14.0f, true));
    g.setColour(juce::Colour(0x18, 0x18, 0x1B));
    g.drawText("BROWSE", (int)bounds.getX() + 18, (int)bounds.getY() + 8, 120, 20, juce::Justification::centredLeft, false);

    g.setFont(SpectralUILookAndFeel::getSpaceGrotesk(9.0f, false));
    g.setColour(juce::Colour(0x71, 0x71, 0x7A));
    g.drawText("PRESET BROWSER / SAMPLE LIBRARY", (int)bounds.getX() + 18, (int)bounds.getY() + 26, 240, 16, juce::Justification::centredLeft, false);

    // Vertical panel dividers
    float totalW = bounds.getWidth();
    float bodyTop   = bounds.getY() + headerH;
    float footerH   = 44.0f;
    float bodyBot   = bounds.getBottom() - footerH;

    g.setColour(juce::Colour(0xE4, 0xE4, 0xE7));

    if constexpr (enableLibraryPanel)
    {
        float colW = (totalW - 2.0f) / 3.0f;
        float col1Right = bounds.getX() + colW;
        float col2Right = col1Right + colW;
        g.drawVerticalLine((int)col1Right, bodyTop, bodyBot);
        g.drawVerticalLine((int)col2Right, bodyTop, bodyBot);
    }
    else
    {
        float col1Right = bounds.getX() + (totalW - 1.0f) * 0.5f;
        g.drawVerticalLine((int)col1Right, bodyTop, bodyBot);
    }

    // Footer divider
    g.drawHorizontalLine((int)bodyBot, bounds.getX(), bounds.getRight());

    // Status indicator dot
    g.setColour(SpectralUILookAndFeel::accentColour);
    g.fillEllipse(bounds.getX() + 18.0f, bodyBot + (footerH - 7.0f) * 0.5f, 7.0f, 7.0f);
}

void PresetBrowserOverlay::paintOverChildren(juce::Graphics&)
{
}

void PresetBrowserOverlay::resized()
{
    auto cardArea = getLocalBounds().reduced(24);
    int headerH = 46;
    int footerH = 44;

    // Header Controls
    closeButton.setBounds(cardArea.getRight() - 34, cardArea.getY() + 10, 26, 26);

    if constexpr (!enableLibraryPanel)
    {
        historyButton.setBounds(cardArea.getRight() - 66, cardArea.getY() + 10, 26, 26);
        libraryIndexedBadge.setBounds(cardArea.getRight() - 280, cardArea.getY() + 10, 208, 26);
        historyFlyout.setBounds(cardArea.getRight() - 368, cardArea.getY() + 46, 360, 370);
        historyBackdrop.setBounds(getLocalBounds());
    }
    else
    {
        libraryIndexedBadge.setBounds(cardArea.getRight() - 250, cardArea.getY() + 10, 208, 26);
    }

    // Content area
    auto contentArea = cardArea;
    contentArea.removeFromTop(headerH);
    contentArea.removeFromBottom(footerH);

    int totalW = contentArea.getWidth();

    if constexpr (enableLibraryPanel)
    {
        int colW = totalW / 3;

        auto col1 = contentArea.removeFromLeft(colW).reduced(12, 10);
        auto col2 = contentArea.removeFromLeft(colW).reduced(12, 10);
        auto col3 = contentArea.reduced(12, 10);

        //==========================================================================
        // PANEL 1: PRESETS (Left)
        //==========================================================================
        auto header1 = col1.removeFromTop(22);
        presetsHeaderLabel.setBounds(header1.removeFromLeft(120));
        presetsCountLabel.setBounds(header1);

        col1.removeFromTop(6);
        searchBox.setBounds(col1.removeFromTop(28));

        col1.removeFromTop(6);
        categoryViewport.setBounds(col1.removeFromTop(26));
        categoryContainer.setBounds(0, 0, 420, 26);

        col1.removeFromTop(6);
        auto toolbar1 = col1.removeFromTop(26);
        shuffleFxBtn.setBounds(toolbar1.removeFromLeft(105));
        toolbar1.removeFromLeft(4);
        favoriteFilterBtn.setBounds(toolbar1.removeFromRight(64));
        toolbar1.removeFromRight(4);
        sortSelector.setBounds(toolbar1);

        col1.removeFromTop(6);
        presetListBox.setBounds(col1);

        //==========================================================================
        // PANEL 2: BANKS (Middle)
        //==========================================================================
        auto header2 = col2.removeFromTop(22);
        banksHeaderLabel.setBounds(header2.removeFromLeft(120));
        banksCountLabel.setBounds(header2);

        col2.removeFromTop(6);
        auto toolbar2 = col2.removeFromTop(28);
        newBankBtn.setBounds(toolbar2.removeFromLeft(105));
        toolbar2.removeFromLeft(6);
        bankActionsBtn.setBounds(toolbar2.removeFromLeft(95));

        col2.removeFromTop(6);
        bankListBox.setBounds(col2);

        //==========================================================================
        // PANEL 3: SAMPLES & HISTORY (Right)
        //==========================================================================
        auto header3 = col3.removeFromTop(22);
        panel3HeaderLabel.setBounds(header3.removeFromLeft(100));
        historyTabBtn.setBounds(header3.removeFromRight(95));
        header3.removeFromRight(4);
        samplesTabBtn.setBounds(header3.removeFromRight(95));

        col3.removeFromTop(6);

        if (!isHistoryViewActive)
        {
            sampleSortSelector.setBounds(col3.removeFromTop(28));
            col3.removeFromTop(6);

            dropImportZone.setBounds(col3.removeFromBottom(36));
            col3.removeFromBottom(6);
            sampleListBox.setBounds(col3);
        }
        else
        {
            auto histBot = col3.removeFromBottom(30);
            restoreHistoryBtn.setBounds(histBot.removeFromLeft((histBot.getWidth() - 6) / 2));
            histBot.removeFromLeft(6);
            clearHistoryBtn.setBounds(histBot);

            col3.removeFromBottom(6);
            historyListBox.setBounds(col3);
        }
    }
    else
    {
        int col1W = totalW / 2;
        auto col1 = contentArea.removeFromLeft(col1W).reduced(12, 10);
        auto col2 = contentArea.reduced(12, 10);

        //==========================================================================
        // PANEL 1: PRESETS (Left, expanded)
        //==========================================================================
        auto header1 = col1.removeFromTop(22);
        presetsHeaderLabel.setBounds(header1.removeFromLeft(120));
        presetsCountLabel.setBounds(header1);

        col1.removeFromTop(6);
        searchBox.setBounds(col1.removeFromTop(28));

        col1.removeFromTop(6);
        categoryViewport.setBounds(col1.removeFromTop(26));
        categoryContainer.setBounds(0, 0, juce::jmax(420, col1.getWidth()), 26);

        col1.removeFromTop(6);
        auto toolbar1 = col1.removeFromTop(26);
        shuffleFxBtn.setBounds(toolbar1.removeFromLeft(105));
        toolbar1.removeFromLeft(6);
        favoriteFilterBtn.setBounds(toolbar1.removeFromRight(64));
        toolbar1.removeFromRight(6);
        sortSelector.setBounds(toolbar1);

        col1.removeFromTop(6);
        presetListBox.setBounds(col1);

        //==========================================================================
        // PANEL 2: BANKS (Right, expanded)
        //==========================================================================
        auto header2 = col2.removeFromTop(22);
        banksHeaderLabel.setBounds(header2.removeFromLeft(120));
        banksCountLabel.setBounds(header2);

        col2.removeFromTop(6);
        auto toolbar2 = col2.removeFromTop(28);
        newBankBtn.setBounds(toolbar2.removeFromLeft(105));
        toolbar2.removeFromLeft(6);
        bankActionsBtn.setBounds(toolbar2.removeFromLeft(95));

        col2.removeFromTop(6);
        bankListBox.setBounds(col2);
    }

    //==========================================================================
    // FOOTER STRIP
    //==========================================================================
    auto footerArea = cardArea.removeFromBottom(footerH);
    int btnH = 26;
    int botY = footerArea.getY() + (footerH - btnH) / 2;

    int curBtnX = footerArea.getRight() - 14;

    deleteBtn.setBounds(curBtnX - 110, botY, 110, btnH);
    curBtnX -= 118;

    loadMainBtn.setBounds(curBtnX - 130, botY, 130, btnH);
    curBtnX -= 138;

    saveAsBtn.setBounds(curBtnX - 85, botY, 85, btnH);
    curBtnX -= 93;

    revealFileBtn.setBounds(curBtnX - 75, botY, 75, btnH);

    bottomStatusLabel.setBounds(footerArea.getX() + 32, botY, curBtnX - 75 - (footerArea.getX() + 36), btnH);
}
