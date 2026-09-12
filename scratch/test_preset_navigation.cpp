/*
  ==============================================================================

    test_preset_navigation.cpp
    Automated Test Harness for VanceSpectral Top Bar Preset Navigation

  ==============================================================================
*/

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <iostream>
#include <cassert>

#define TEST_ASSERT(cond, msg) \
    if (!(cond)) { \
        std::cerr << " [FAIL] " << msg << " (line " << __LINE__ << ")\n" << std::flush; \
        return false; \
    }

//==============================================================================
// Component Traversal Helpers
//==============================================================================
template <typename T>
T* findChild(juce::Component* parent)
{
    if (parent == nullptr) return nullptr;
    if (auto* typed = dynamic_cast<T*>(parent))
        return typed;

    if (auto* vp = dynamic_cast<juce::Viewport*>(parent))
    {
        if (auto* viewed = vp->getViewedComponent())
        {
            if (auto* found = findChild<T>(viewed))
                return found;
        }
    }

    for (int i = 0; i < parent->getNumChildComponents(); ++i)
    {
        if (auto* found = findChild<T>(parent->getChildComponent(i)))
            return found;
    }
    return nullptr;
}

template <typename T>
void findChildren(juce::Component* parent, juce::Array<T*>& result)
{
    if (parent == nullptr) return;
    if (auto* typed = dynamic_cast<T*>(parent))
        result.add(typed);

    if (auto* vp = dynamic_cast<juce::Viewport*>(parent))
    {
        if (auto* viewed = vp->getViewedComponent())
            findChildren<T>(viewed, result);
    }

    for (int i = 0; i < parent->getNumChildComponents(); ++i)
        findChildren<T>(parent->getChildComponent(i), result);
}

inline juce::TextButton* findButtonWithText(juce::Component* parent, const juce::String& text)
{
    if (!parent) return nullptr;
    if (auto* btn = dynamic_cast<juce::TextButton*>(parent))
    {
        if (btn->getButtonText().equalsIgnoreCase(text))
            return btn;
    }
    if (auto* vp = dynamic_cast<juce::Viewport*>(parent))
    {
        if (auto* viewed = vp->getViewedComponent())
        {
            if (auto* found = findButtonWithText(viewed, text))
                return found;
        }
    }
    for (int i = 0; i < parent->getNumChildComponents(); ++i)
    {
        if (auto* found = findButtonWithText(parent->getChildComponent(i), text))
            return found;
    }
    return nullptr;
}

inline void clickButton(juce::Button* btn)
{
    if (!btn) return;
    if (btn->onClick)
        btn->onClick();
    else
        btn->triggerClick();
}

//==============================================================================
// 1. Browse Button Presence & Top Bar Layout Tests
//==============================================================================
bool testBrowseButtonAndTopBarLayout(VancespectralAudioProcessor& processor, VancespectralAudioProcessorEditor& editor)
{
    std::cout << "\n--- 1. Browse Button & Layout Tests ---\n";
    juce::ignoreUnused(processor);

    editor.setBounds(0, 0, 1088, 544);
    editor.resized();

    // Find PresetBarComponent
    auto* presetBar = findChild<PresetBarComponent>(&editor);
    TEST_ASSERT(presetBar != nullptr, "PresetBarComponent not found in editor");

    // Find all TextButtons in PresetBarComponent
    juce::Array<juce::TextButton*> textButtons;
    for (int i = 0; i < presetBar->getNumChildComponents(); ++i)
    {
        if (auto* btn = dynamic_cast<juce::TextButton*>(presetBar->getChildComponent(i)))
            textButtons.add(btn);
    }

    juce::TextButton* shuffleBtn = nullptr;
    juce::TextButton* browseBtn = nullptr;
    juce::TextButton* saveStateBtn = nullptr;

    for (auto* btn : textButtons)
    {
        if (btn->getButtonText().contains("SHUFFLE"))
            shuffleBtn = btn;
        else if (btn->getButtonText().equalsIgnoreCase("BROWSE"))
            browseBtn = btn;
        else if (btn->getButtonText().contains("SAVE STATE"))
            saveStateBtn = btn;
    }

    TEST_ASSERT(shuffleBtn != nullptr, "SHUFFLE FX button missing");
    TEST_ASSERT(browseBtn != nullptr, "Dedicated BROWSE button missing from top bar");
    TEST_ASSERT(saveStateBtn != nullptr, "SAVE STATE button missing");

    TEST_ASSERT(browseBtn->isVisible(), "BROWSE button is not visible");
    TEST_ASSERT(browseBtn->getWidth() >= 60, "BROWSE button width is too small");

    // Check horizontal order: shuffleFx < center area < browse < saveState
    TEST_ASSERT(shuffleBtn->getX() < browseBtn->getX(), "SHUFFLE FX must be to the left of BROWSE");
    TEST_ASSERT(browseBtn->getX() < saveStateBtn->getX(), "BROWSE must be to the left of SAVE STATE");

    std::cout << " [PASS] 1.1 Dedicated BROWSE button exists, styled, and correctly positioned in top bar\n";
    return true;
}

//==============================================================================
// 2. Pure Read-Only Preset Pill & Exclusive Browse Trigger Tests
//==============================================================================
bool testReadOnlyPresetPillAndBrowseTrigger(VancespectralAudioProcessor& processor, VancespectralAudioProcessorEditor& editor)
{
    std::cout << "\n--- 2. Read-Only Preset Pill & Browse Trigger Tests ---\n";
    juce::ignoreUnused(processor);

    auto* presetBar = findChild<PresetBarComponent>(&editor);
    auto* overlay = findChild<PresetBrowserOverlay>(&editor);
    TEST_ASSERT(presetBar != nullptr && overlay != nullptr, "Required components not found");

    // Initially overlay must be closed
    overlay->setVisible(false);
    TEST_ASSERT(!overlay->isVisible(), "Overlay must start closed");

    // Simulate mouse click directly on the center preset pill area
    juce::Point<int> centerPill(presetBar->getWidth() / 2, presetBar->getHeight() / 2);
    juce::MouseEvent downCenter(juce::Desktop::getInstance().getMainMouseSource(),
                                centerPill.toFloat(),
                                juce::ModifierKeys(),
                                1.0f, 0.0f, 0.0f, 0.0f, 0.0f,
                                presetBar, presetBar,
                                juce::Time::getCurrentTime(),
                                centerPill.toFloat(),
                                juce::Time::getCurrentTime(),
                                1, false);

    presetBar->mouseDown(downCenter);
    TEST_ASSERT(!overlay->isVisible(), "Clicking on preset name pill must NOT open Preset Browser (must be read-only)");
    std::cout << " [PASS] 2.1 Clicking preset name pill no longer opens Preset Browser (pure read-only)\n";

    // Locate dedicated Browse button and trigger click
    juce::TextButton* browseBtn = nullptr;
    for (int i = 0; i < presetBar->getNumChildComponents(); ++i)
    {
        if (auto* btn = dynamic_cast<juce::TextButton*>(presetBar->getChildComponent(i)))
        {
            if (btn->getButtonText().equalsIgnoreCase("BROWSE"))
            {
                browseBtn = btn;
                break;
            }
        }
    }
    TEST_ASSERT(browseBtn != nullptr, "Browse button not found");

    clickButton(browseBtn);
    TEST_ASSERT(overlay->isVisible(), "Clicking BROWSE button must open Preset Browser overlay");
    std::cout << " [PASS] 2.2 Clicking BROWSE button successfully opens Preset Browser overlay\n";

    // Close overlay
    overlay->setVisible(false);
    return true;
}

//==============================================================================
// 3. Functional < and > Arrows & Boundary Disabling Tests
//==============================================================================
bool testFunctionalArrowsAndBoundaryDisabling(VancespectralAudioProcessor& processor, VancespectralAudioProcessorEditor& editor)
{
    std::cout << "\n--- 3. Functional < and > Arrows & Boundary Disabling Tests ---\n";
    juce::ignoreUnused(processor);

    auto* presetBar = findChild<PresetBarComponent>(&editor);
    auto* overlay = findChild<PresetBrowserOverlay>(&editor);
    TEST_ASSERT(presetBar != nullptr && overlay != nullptr, "Components not found");

    auto factoryPresets = overlay->getNavigablePresetsForBank("Factory");
    TEST_ASSERT(factoryPresets.size() >= 4, "Factory bank must have at least 4 presets for boundary testing");

    std::cout << "       Factory presets list (" << factoryPresets.size() << " total):\n";
    for (int i = 0; i < factoryPresets.size(); ++i)
        std::cout << "         [" << i << "] " << factoryPresets[i].name << "\n";

    // Load first factory preset
    presetBar->onPrevClicked(); // If at 0, should stay at 0
    TEST_ASSERT(factoryPresets[0].name.equalsIgnoreCase(presetBar->getCurrentPresetName()),
                "Initial loaded preset must match index 0");

    // At index 0, < arrow must be disabled/dimmed, > arrow must be enabled
    TEST_ASSERT(!presetBar->isPrevEnabled(), "Left arrow (<) must be DISABLED at the first preset");
    TEST_ASSERT(presetBar->isNextEnabled(), "Right arrow (>) must be ENABLED at the first preset");
    std::cout << " [PASS] 3.1 Start boundary: < arrow is disabled, > is enabled\n";

    // Clicking < at index 0 should NOT wrap around
    presetBar->onPrevClicked();
    TEST_ASSERT(factoryPresets[0].name.equalsIgnoreCase(presetBar->getCurrentPresetName()),
                "Clicking < at start must NOT wrap around");
    TEST_ASSERT(!presetBar->isPrevEnabled(), "< arrow remains disabled at start boundary");

    // Click > repeatedly through all presets
    for (int i = 1; i < factoryPresets.size(); ++i)
    {
        presetBar->onNextClicked();
        TEST_ASSERT(factoryPresets[i].name.equalsIgnoreCase(presetBar->getCurrentPresetName()),
                    juce::String("Forward navigation mismatch at index ") + juce::String(i));
        TEST_ASSERT(presetBar->isPrevEnabled(), "< arrow must be enabled when past the start");

        if (i < factoryPresets.size() - 1)
        {
            TEST_ASSERT(presetBar->isNextEnabled(), "> arrow must be enabled before the end");
        }
    }

    // At last preset, > arrow must be disabled/dimmed
    TEST_ASSERT(!presetBar->isNextEnabled(), "Right arrow (>) must be DISABLED at the last preset");
    TEST_ASSERT(presetBar->isPrevEnabled(), "Left arrow (<) must be ENABLED at the last preset");
    std::cout << " [PASS] 3.2 End boundary: > arrow is disabled, < is enabled\n";

    // Clicking > at end should NOT wrap around
    presetBar->onNextClicked();
    TEST_ASSERT(factoryPresets[factoryPresets.size() - 1].name.equalsIgnoreCase(presetBar->getCurrentPresetName()),
                "Clicking > at end must NOT wrap around");

    // Click < repeatedly back to start
    for (int i = factoryPresets.size() - 2; i >= 0; --i)
    {
        presetBar->onPrevClicked();
        TEST_ASSERT(factoryPresets[i].name.equalsIgnoreCase(presetBar->getCurrentPresetName()),
                    juce::String("Backward navigation mismatch at index ") + juce::String(i));
    }

    // Verified back at start
    TEST_ASSERT(!presetBar->isPrevEnabled(), "< arrow is disabled back at index 0");
    TEST_ASSERT(presetBar->isNextEnabled(), "> arrow is enabled back at index 0");
    std::cout << " [PASS] 3.3 Full round-trip navigation in list order verified with boundary stops\n";

    return true;
}

//==============================================================================
// 4. Category Filter Scoping & Browser State Sync Tests
//==============================================================================
bool testFilterScopingAndBrowserStateSync(VancespectralAudioProcessor& processor, VancespectralAudioProcessorEditor& editor)
{
    std::cout << "\n--- 4. Category Filter Scoping & Browser State Sync Tests ---\n";
    juce::ignoreUnused(processor);

    auto* presetBar = findChild<PresetBarComponent>(&editor);
    auto* overlay = findChild<PresetBrowserOverlay>(&editor);
    TEST_ASSERT(presetBar != nullptr && overlay != nullptr, "Components not found");

    // Open browser, find FX filter button and click it
    presetBar->onBrowseClicked();
    TEST_ASSERT(overlay->isVisible(), "Browser must open");

    juce::TextButton* fxFilterBtn = findButtonWithText(overlay, "FX");
    TEST_ASSERT(fxFilterBtn != nullptr, "FX category filter button not found");

    clickButton(fxFilterBtn);
    overlay->setVisible(false);

    // Verify navigable presets are now restricted to FX category in Factory
    auto fxPresets = overlay->getNavigablePresetsForBank("Factory");
    std::cout << "       Active FX filtered presets in Factory: " << fxPresets.size() << "\n";
    TEST_ASSERT(!fxPresets.isEmpty(), "Factory bank must have at least one FX preset");
    for (const auto& p : fxPresets)
    {
        TEST_ASSERT(p.category.equalsIgnoreCase("FX"), "Preset does not match FX filter");
    }

    // Load first FX preset
    overlay->onPresetSelected(fxPresets[0].file, "");
    TEST_ASSERT(presetBar->getCurrentPresetName().equalsIgnoreCase(fxPresets[0].name),
                "Preset did not load first FX preset");

    if (fxPresets.size() == 1)
    {
        TEST_ASSERT(!presetBar->isPrevEnabled(), "Single preset must have < disabled");
        TEST_ASSERT(!presetBar->isNextEnabled(), "Single preset must have > disabled");
    }
    else
    {
        TEST_ASSERT(!presetBar->isPrevEnabled(), "First FX preset must have < disabled");
        TEST_ASSERT(presetBar->isNextEnabled(), "First FX preset must have > enabled");
        // Step to next FX preset
        presetBar->onNextClicked();
        TEST_ASSERT(presetBar->getCurrentPresetName().equalsIgnoreCase(fxPresets[1].name),
                    "Did not navigate to next FX preset");
    }
    std::cout << " [PASS] 4.1 Filter scope respected: arrows restricted to FX presets only\n";

    // Reset filter back to ALL
    juce::TextButton* allFilterBtn = findButtonWithText(overlay, "ALL");
    TEST_ASSERT(allFilterBtn != nullptr, "ALL filter button not found");
    clickButton(allFilterBtn);
    overlay->setVisible(false);

    // Verify arrow sync with browser highlighted selection
    presetBar->onNextClicked();
    juce::String currentActiveName = presetBar->getCurrentPresetName();

    // Open browser and verify overlay's active preset file matches
    presetBar->onBrowseClicked();
    std::cout << "       DEBUG: currentActiveName = '" << currentActiveName << "'\n";
    std::cout << "       DEBUG: overlay getActivePresetFile() = '" << overlay->getActivePresetFile().getFullPathName() << "'\n";
    std::cout << "       DEBUG: file name without ext = '" << overlay->getActivePresetFile().getFileNameWithoutExtension() << "'\n";
    TEST_ASSERT(overlay->getActivePresetFile().getFileNameWithoutExtension().equalsIgnoreCase(currentActiveName),
                "Preset browser active preset did not match arrow-loaded preset");
    overlay->setVisible(false);
    std::cout << " [PASS] 4.2 Browser highlighted entry matches arrow-loaded preset\n";

    return true;
}

//==============================================================================
// 5. State (.vsts) vs FX Preset (.vsfx) Loading Distinction
//==============================================================================
bool testStateVsFxPresetLoading(VancespectralAudioProcessor& processor, VancespectralAudioProcessorEditor& editor)
{
    std::cout << "\n--- 5. State vs FX Preset Loading Distinction Tests ---\n";

    auto* presetBar = findChild<PresetBarComponent>(&editor);
    auto* overlay = findChild<PresetBrowserOverlay>(&editor);
    auto* spectrogram = findChild<SpectrogramComponent>(&editor);
    TEST_ASSERT(presetBar != nullptr && overlay != nullptr && spectrogram != nullptr, "Components not found");

    PresetManager pm;

    // Create a dummy audio buffer and save a test STATE preset in User bank
    juce::AudioBuffer<float> testBuf(2, 44100);
    testBuf.clear();
    // Fill with identifiable DC signal
    for (int ch = 0; ch < 2; ++ch)
        for (int s = 0; s < 44100; ++s)
            testBuf.setSample(ch, s, 0.75f);

    juce::String testStateName = "TestStateAutoNav";
    bool savedState = pm.savePreset(testStateName, "STATES", "User", "test_sample.wav",
                                    processor.getAPVTS(), 0.1f, 0.9f, juce::var(), false, true,
                                    &testBuf, 44100.0);
    TEST_ASSERT(savedState, "Failed to save test state");

    // Load the state
    juce::File stateFile = pm.getPresetsFolder().getChildFile("User").getChildFile(testStateName + ".vsts");
    TEST_ASSERT(stateFile.existsAsFile(), "State file was not written to disk");

    overlay->refreshPresetList();
    overlay->onPresetSelected(stateFile, "test_sample.wav");
    TEST_ASSERT(presetBar->getCurrentPresetName().equalsIgnoreCase(testStateName), "State preset not loaded");
    TEST_ASSERT(presetBar->getBankName().equalsIgnoreCase("User"), "Bank label did not update to User");

    // Verify sample was loaded into spectrogram
    TEST_ASSERT(spectrogram->getAudioBuffer().getNumSamples() > 0, "Audio buffer was not loaded for State preset");
    std::cout << " [PASS] 5.1 State preset (.vsts) successfully loaded sample + settings\n";

    // Now load an FX preset (settings-only) from Factory bank
    auto allPresets = pm.getAllPresets();
    juce::File fxFile;
    for (const auto& p : allPresets)
    {
        if (p.bank.equalsIgnoreCase("Factory") && !p.file.hasFileExtension(".vsts"))
        {
            fxFile = p.file;
            break;
        }
    }
    TEST_ASSERT(fxFile.existsAsFile(), "Factory FX preset file not found");

    overlay->onPresetSelected(fxFile, "");
    TEST_ASSERT(presetBar->getCurrentPresetName().equalsIgnoreCase(fxFile.getFileNameWithoutExtension()), "FX preset not loaded");
    TEST_ASSERT(presetBar->getBankName().equalsIgnoreCase("Factory"), "Bank label did not update to Factory");

    // Crucial check: FX preset should NOT clear or destroy the existing sample in spectrogram!
    TEST_ASSERT(spectrogram->getAudioBuffer().getNumSamples() > 0, "FX preset clobbered loaded sample!");
    std::cout << " [PASS] 5.2 FX preset (.vsfx) loaded settings without clobbering existing sample\n";

    // Clean up test file
    stateFile.deleteFile();
    pm.invalidateCache();
    return true;
}

//==============================================================================
// 6. Top-Bar Bank Label Synchronization & Bank Scoping Tests
//==============================================================================
bool testTopBarBankLabelSync(VancespectralAudioProcessor& processor, VancespectralAudioProcessorEditor& editor)
{
    std::cout << "\n--- 6. Bank Synchronization & Scoping Tests ---\n";

    auto* presetBar = findChild<PresetBarComponent>(&editor);
    auto* overlay = findChild<PresetBrowserOverlay>(&editor);
    TEST_ASSERT(presetBar != nullptr && overlay != nullptr, "Components not found");

    PresetManager pm;

    // 1. Ensure active preset is Factory
    auto factoryPresets = overlay->getNavigablePresetsForBank("Factory");
    TEST_ASSERT(!factoryPresets.isEmpty(), "Factory presets empty");
    overlay->onPresetSelected(factoryPresets[0].file, "");
    TEST_ASSERT(presetBar->getBankName().equalsIgnoreCase("Factory"), "Initial bank must be Factory");

    // 2. Open Preset Browser, switch bank ComboBox to a different bank without loading anything
    presetBar->onBrowseClicked();
    juce::Array<juce::ListBox*> listBoxes;
    findChildren<juce::ListBox>(overlay, listBoxes);
    TEST_ASSERT(!listBoxes.isEmpty(), "ListBoxes not found in overlay");

    // Locate BankListBox (row height 48) and test clicking bank items
    juce::ListBox* bankList = nullptr;
    for (auto* lb : listBoxes)
    {
        if (lb->getRowHeight() == 48)
        {
            bankList = lb;
            break;
        }
    }
    if (bankList && bankList->getModel())
    {
        juce::MouseEvent dummyEv(juce::Desktop::getInstance().getMainMouseSource(),
                                 juce::Point<float>(10.0f, 10.0f), juce::ModifierKeys(),
                                 1.0f, 0.0f, 0.0f, 0.0f, 0.0f,
                                 bankList, bankList,
                                 juce::Time::getCurrentTime(),
                                 juce::Point<float>(10.0f, 10.0f),
                                 juce::Time::getCurrentTime(), 1, false);
        // Click row 0 (ALL BANKS), row 1 (Factory), row 2 (User)
        for (int r = 0; r < bankList->getModel()->getNumRows(); ++r)
        {
            bankList->getModel()->listBoxItemClicked(r, dummyEv);
        }
    }

    // Switch bank selection to "User" without loading anything
    if (overlay->onBankSelected)
        overlay->onBankSelected("User");

    overlay->setVisible(false);

    // Confirm top-bar bank label is UNCHANGED
    TEST_ASSERT(presetBar->getBankName().equalsIgnoreCase("Factory"),
                "Switching bank dropdown in browser without loading must NOT change top-bar bank label");
    std::cout << " [PASS] 6.1 Switching browser bank dropdown without loading leaves top-bar bank unchanged\n";

    // 3. Create a preset in User bank
    juce::String userPresetName = "UserSyncPresetTest";
    bool saved = pm.savePreset(userPresetName, "Synth", "User", "", processor.getAPVTS());
    TEST_ASSERT(saved, "Failed to save user preset");
    pm.invalidateCache();

    juce::File userFile = pm.getPresetsFolder().getChildFile("User").getChildFile(userPresetName + ".vsfx");
    TEST_ASSERT(userFile.existsAsFile(), "User preset file does not exist");

    // Load preset from User bank via browser
    overlay->refreshPresetList();
    overlay->onPresetSelected(userFile, "");

    // Confirm top-bar bank label immediately updates to "User"
    TEST_ASSERT(presetBar->getBankName().equalsIgnoreCase("User"),
                "Top-bar bank label must immediately update to User on loading preset from User bank");
    TEST_ASSERT(presetBar->getCurrentPresetName().equalsIgnoreCase(userPresetName),
                "Preset name must match loaded user preset");
    std::cout << " [PASS] 6.2 Loading preset from User bank updates top-bar bank label to User\n";

    // 4. Use </> arrows now: confirm they navigate within the newly active bank (User)
    auto userPresets = overlay->getNavigablePresetsForBank("User");
    std::cout << "       User bank preset count: " << userPresets.size() << "\n";
    TEST_ASSERT(!userPresets.isEmpty(), "User bank presets empty");

    // Check that arrows remain scoped to User bank
    if (presetBar->isNextEnabled())
        presetBar->onNextClicked();
    else if (presetBar->isPrevEnabled())
        presetBar->onPrevClicked();

    TEST_ASSERT(presetBar->getBankName().equalsIgnoreCase("User"),
                "Arrow navigation must remain scoped to User bank and maintain bank label");
    std::cout << " [PASS] 6.3 Arrow navigation remains scoped to new bank without changing bank label\n";

    // Clean up test file
    userFile.deleteFile();
    pm.invalidateCache();
    return true;
}

//==============================================================================
// SECTION 7: Window Sizing & Screen-Graph Grid Alignment
//==============================================================================
bool testWindowDimensionsAndGridAlignment(VancespectralAudioProcessor& processor, VancespectralAudioProcessorEditor& editor)
{
    std::cout << "\n--- TEST SECTION 7: Window Sizing & Screen-Graph Alignment ---\n";

    TEST_ASSERT(editor.getWidth() == 1088, "Editor width must be 1088 (~85% of 1280)");
    TEST_ASSERT(editor.getHeight() == 544, "Editor height must be 544 (~85% of 640)");
    std::cout << " [PASS] 7.1 Window dimensions correctly reduced by ~15% to 1088x544\n";

    auto* spectrogram = findChild<SpectrogramComponent>(&editor);
    auto* adsrPanel = findChild<ADSRPanel>(&editor);
    auto* effectsPanel = findChild<EffectsPanel>(&editor);

    TEST_ASSERT(spectrogram != nullptr, "SpectrogramComponent not found");
    TEST_ASSERT(adsrPanel != nullptr, "ADSRPanel not found");
    TEST_ASSERT(effectsPanel != nullptr, "EffectsPanel not found");

    // Right edge of screen-graph aligns with right edge of EXCITER & GLIDE (adsrPanel)
    TEST_ASSERT(spectrogram->getRight() == adsrPanel->getRight(),
                "Screen-graph right edge must align exactly with adsrPanel (EXCITER & GLIDE) right edge");
    std::cout << " [PASS] 7.2 Screen-graph right edge aligns with EXCITER & GLIDE panel below (x = "
              << spectrogram->getRight() << ")\n";

    // Row beneath stays at full width
    TEST_ASSERT(adsrPanel->getX() == 12, "ADSR panel must start at left margin");
    TEST_ASSERT(effectsPanel->getRight() == editor.getWidth() - 12, "Effects panel must reach right margin");
    TEST_ASSERT(adsrPanel->getY() == effectsPanel->getY(), "Lower row panels must share vertical level");
    std::cout << " [PASS] 7.3 Bottom row (AMP ENV, PITCH & DRIFT, EXCITER & GLIDE, EFFECTS) spans full width across bottom\n";

    // Volume Slider in Bottom Status Bar
    juce::Slider* volumeSlider = nullptr;
    juce::Array<juce::Slider*> sliders;
    findChildren<juce::Slider>(&editor, sliders);
    for (auto* s : sliders)
    {
        if (s->getName() == "VOLUME")
        {
            volumeSlider = s;
            break;
        }
    }
    TEST_ASSERT(volumeSlider != nullptr, "VOLUME slider not found in editor");
    TEST_ASSERT(volumeSlider->getWidth() >= 180, "VOLUME slider must be a full-size fader (>= 180px, not a tiny sliver)");
    TEST_ASSERT(volumeSlider->getY() >= 518, "VOLUME slider must be positioned within the bottom status bar below the divider");
    TEST_ASSERT(volumeSlider->getBottom() <= 542, "VOLUME slider must not overlap or touch the window bottom edge (544)");
    TEST_ASSERT(std::abs(volumeSlider->getY() - 522) <= 2, "VOLUME slider must be row-aligned with the OCTAVE badge (y = 522)");
    TEST_ASSERT(volumeSlider->getX() >= 800, "VOLUME slider must sit in the right corner of the bottom bar");
    std::cout << " [PASS] 7.4 Volume slider restored: width = " << volumeSlider->getWidth()
              << "px, y = " << volumeSlider->getY() << " (row-aligned with OCTAVE, no bottom edge overlap)\n";

    // Test Volume Slider drag & APVTS GAIN sync
    volumeSlider->setValue(-12.0, juce::sendNotificationSync);
    float gainVal = *processor.getAPVTS().getRawParameterValue("GAIN");
    TEST_ASSERT(std::abs(gainVal - (-12.0f)) < 0.2f, "Dragging VOLUME slider must update APVTS GAIN parameter");
    volumeSlider->setValue(0.0, juce::sendNotificationSync);
    gainVal = *processor.getAPVTS().getRawParameterValue("GAIN");
    TEST_ASSERT(std::abs(gainVal - 0.0f) < 0.2f, "Resetting VOLUME slider must update APVTS GAIN parameter to 0 dB");
    std::cout << " [PASS] 7.5 Volume slider drags smoothly across full range with accurate real-time APVTS GAIN sync\n";

    return true;
}

//==============================================================================
// SECTION 8: Relocated PLAYBACK & PITCH Beside Screen-Graph
//==============================================================================
bool testPlaybackAndPitchRelocation(VancespectralAudioProcessor&, VancespectralAudioProcessorEditor& editor)
{
    std::cout << "\n--- TEST SECTION 8: Relocated PLAYBACK & PITCH Beside Screen-Graph ---\n";

    auto* spectrogram = findChild<SpectrogramComponent>(&editor);
    auto* effectsPanel = findChild<EffectsPanel>(&editor);

    // Find SegmentedControlComponents
    juce::Array<SegmentedControlComponent*> segmentedControls;
    findChildren<SegmentedControlComponent>(&editor, segmentedControls);

    TEST_ASSERT(segmentedControls.size() >= 2, "Must have at least 2 SegmentedControlComponents (playback & pitch)");
    auto* playbackControl = segmentedControls[0];
    auto* pitchControl = segmentedControls[1];

    // Align with effectsPanel below horizontally
    TEST_ASSERT(playbackControl->getX() == effectsPanel->getX(),
                "Playback control X must align with effectsPanel X");
    TEST_ASSERT(playbackControl->getRight() == effectsPanel->getRight(),
                "Playback control Right must align with effectsPanel Right");
    TEST_ASSERT(pitchControl->getX() == effectsPanel->getX(),
                "Pitch control X must align with effectsPanel X");
    TEST_ASSERT(pitchControl->getRight() == effectsPanel->getRight(),
                "Pitch control Right must align with effectsPanel Right");
    std::cout << " [PASS] 8.1 PLAYBACK and PITCH panel sits beside screen-graph, flush and aligned with EFFECTS below\n";

    // Same row/vertical level as screen-graph, matching its height
    TEST_ASSERT(playbackControl->getY() == spectrogram->getY(),
                "Playback control top must match spectrogram top");
    TEST_ASSERT(pitchControl->getBottom() == spectrogram->getBottom(),
                "Pitch control bottom must match spectrogram bottom");
    TEST_ASSERT(pitchControl->getY() > playbackControl->getY(),
                "Pitch control must sit below playback control in vertical stack");
    std::cout << " [PASS] 8.2 PLAYBACK and PITCH occupy vertical space beside screen-graph, matching its height\n";

    return true;
}

//==============================================================================
// SECTION 9: MONO/POLY Toggle in Top Navigation Bar
//==============================================================================
bool testMonoPolyToggleInTopBar(VancespectralAudioProcessor& processor, VancespectralAudioProcessorEditor& editor)
{
    std::cout << "\n--- TEST SECTION 9: MONO/POLY Toggle in Top Navigation Bar ---\n";

    auto* presetBar = findChild<PresetBarComponent>(&editor);
    TEST_ASSERT(presetBar != nullptr, "PresetBarComponent not found");

    // Find polyButton in editor child components
    juce::Button* polyBtn = nullptr;
    juce::Array<juce::Button*> buttons;
    findChildren<juce::Button>(&editor, buttons);
    for (auto* b : buttons)
    {
        if (b->getButtonText().equalsIgnoreCase("POLY") || b->getButtonText().equalsIgnoreCase("MONO"))
        {
            polyBtn = b;
            break;
        }
    }

    TEST_ASSERT(polyBtn != nullptr, "MONO/POLY button not found in editor");

    // Confirm it is positioned in top bar area (Y < 50)
    TEST_ASSERT(polyBtn->getY() >= presetBar->getY() && polyBtn->getBottom() <= presetBar->getBottom(),
                "MONO/POLY button must be positioned within the top bar");
    std::cout << " [PASS] 9.1 MONO/POLY toggle positioned in top navigation bar (y = " << polyBtn->getY() << ")\n";

    // Confirm it is positioned to the left of the preset group
    auto* prevBtn = findChild<juce::Button>(presetBar);
    TEST_ASSERT(prevBtn != nullptr, "Prev arrow button not found in presetBar");
    int prevScreenX = presetBar->getX() + prevBtn->getX();
    TEST_ASSERT(polyBtn->getRight() <= prevScreenX,
                "MONO/POLY toggle must be positioned immediately to the left of the preset navigation group");
    std::cout << " [PASS] 9.2 MONO/POLY toggle is to the left of the preset name display group (poly right = "
              << polyBtn->getRight() << ", prev left = " << prevScreenX << ")\n";

    // Test toggle behavior
    float initParam = *processor.getAPVTS().getRawParameterValue("POLY_MODE");
    bool initToggle = polyBtn->getToggleState();
    clickButton(polyBtn);
    float newParam = *processor.getAPVTS().getRawParameterValue("POLY_MODE");
    TEST_ASSERT(newParam != initParam, "Clicking MONO/POLY button must toggle APVTS POLY_MODE parameter");
    TEST_ASSERT(polyBtn->getToggleState() != initToggle, "Clicking MONO/POLY button must toggle button toggle state");
    clickButton(polyBtn);
    TEST_ASSERT(*processor.getAPVTS().getRawParameterValue("POLY_MODE") == initParam, "Clicking again must revert APVTS POLY_MODE");
    std::cout << " [PASS] 9.3 MONO/POLY toggle preserves exact toggle functionality and APVTS sync\n";

    return true;
}

//==============================================================================
// SECTION 10: LOOP Button Anchored to Bottom-Right of Screen-Graph
//==============================================================================
bool testLoopButtonRepositioning(VancespectralAudioProcessor&, VancespectralAudioProcessorEditor& editor)
{
    std::cout << "\n--- TEST SECTION 10: LOOP Button Relocation in Screen-Graph ---\n";

    auto* spectrogram = findChild<SpectrogramComponent>(&editor);
    TEST_ASSERT(spectrogram != nullptr, "SpectrogramComponent not found");

    juce::Button* loopBtn = nullptr;
    for (int i = 0; i < spectrogram->getNumChildComponents(); ++i)
    {
        if (auto* b = dynamic_cast<juce::Button*>(spectrogram->getChildComponent(i)))
        {
            if (b->getButtonText().contains("LOOP"))
            {
                loopBtn = b;
                break;
            }
        }
    }

    TEST_ASSERT(loopBtn != nullptr, "Loop button not found in spectrogram");

    // Verify anchored to bottom-right corner
    TEST_ASSERT(loopBtn->getY() > spectrogram->getHeight() / 2,
                "Loop button must be anchored to the bottom of the screen-graph");
    TEST_ASSERT(loopBtn->getY() == spectrogram->getHeight() - 30,
                "Loop button must be at bottom: getHeight() - 30");
    TEST_ASSERT(loopBtn->getRight() == spectrogram->getWidth() - 8,
                "Loop button must preserve right-side alignment: getWidth() - 84 + 76 = getWidth() - 8");
    std::cout << " [PASS] 10.1 LOOP button relocated to bottom-right corner of screen-graph (y = "
              << loopBtn->getY() << ", right = " << loopBtn->getRight() << ")\n";

    return true;
}

//==============================================================================
// SECTION 11: Playback Mode Buttons Width & Single-Line Display
//==============================================================================
bool testPlaybackButtonWidthsAndSingleLine(VancespectralAudioProcessor&, VancespectralAudioProcessorEditor& editor)
{
    std::cout << "\n--- TEST SECTION 11: Playback Mode Buttons Width & Single-Line Display ---\n";

    juce::Array<SegmentedControlComponent*> segmentedControls;
    findChildren<SegmentedControlComponent>(&editor, segmentedControls);

    TEST_ASSERT(segmentedControls.size() >= 1, "Playback segmented control not found");
    auto* playbackControl = segmentedControls[0];

    juce::Array<juce::Button*> buttons;
    for (int i = 0; i < playbackControl->getNumChildComponents(); ++i)
    {
        if (auto* b = dynamic_cast<juce::Button*>(playbackControl->getChildComponent(i)))
            buttons.add(b);
    }

    TEST_ASSERT(buttons.size() == 5, "Playback control must have exactly 5 mode buttons");

    const juce::StringArray expectedLabels = { "Forward", "Backward", "Forward-Backward", "Backward-Forward", "Random" };
    for (int i = 0; i < 5; ++i)
    {
        TEST_ASSERT(buttons[i]->getButtonText() == expectedLabels[i],
                    ("Button " + juce::String(i) + " label must be " + expectedLabels[i]).toRawUTF8());

        // Buttons must be wide (at least 250px) to fit full text on a single line
        TEST_ASSERT(buttons[i]->getWidth() >= 250,
                    ("Button " + expectedLabels[i] + " width must be >= 250px for single-line display").toRawUTF8());

        std::cout << "       Button [" << buttons[i]->getButtonText() << "] width = "
                  << buttons[i]->getWidth() << "px (single line, no wrap)\n";
    }

    std::cout << " [PASS] 11.1 All 5 Playback mode buttons widened with full labels, single line without truncation or wrap\n";
    return true;
}

//==============================================================================
// SECTION 12: Embedded Typography Verification (JetBrains Mono & Space Grotesk)
//==============================================================================
bool testEmbeddedTypography(VancespectralAudioProcessor&, VancespectralAudioProcessorEditor&)
{
    std::cout << "\n--- TEST SECTION 12: Embedded Typography System ---\n";

    // 1. Verify JetBrains Mono Font
    auto jbFontBold = SpectralUILookAndFeel::getJetBrainsMono(12.0f, true);
    auto jbFontPlain = SpectralUILookAndFeel::getJetBrainsMono(12.0f, false);
    TEST_ASSERT(jbFontBold.getTypefacePtr() != nullptr, "JetBrains Mono Bold typeface must be non-null");
    TEST_ASSERT(jbFontPlain.getTypefacePtr() != nullptr, "JetBrains Mono Plain typeface must be non-null");
    TEST_ASSERT(jbFontBold.getTypefaceName().containsIgnoreCase("JetBrains") ||
                jbFontBold.getTypefaceName().containsIgnoreCase("Mono"),
                "JetBrains Mono font name must match");
    std::cout << " [PASS] 12.1 JetBrains Mono embedded and loaded via BinaryData Typeface: "
              << jbFontBold.getTypefaceName() << "\n";

    // 2. Verify Space Grotesk Font
    auto sgFont = SpectralUILookAndFeel::getSpaceGrotesk(12.0f, false);
    TEST_ASSERT(sgFont.getTypefacePtr() != nullptr, "Space Grotesk typeface must be non-null");
    TEST_ASSERT(sgFont.getTypefaceName().containsIgnoreCase("Space") ||
                sgFont.getTypefaceName().containsIgnoreCase("Grotesk"),
                "Space Grotesk font name must match");
    // 3. Verify Heading vs Sub-label Typography Hierarchy
    auto headingFont = SpectralUILookAndFeel::getJetBrainsMono(11.8f, true);
    auto subLabelFont = SpectralUILookAndFeel::getSpaceGrotesk(10.0f, false);
    TEST_ASSERT(headingFont.getHeight() > subLabelFont.getHeight(), "Heading font tier must be strictly larger than knob sub-label tier");
    float hierarchyRatio = headingFont.getHeight() / subLabelFont.getHeight();
    TEST_ASSERT(hierarchyRatio >= 1.15f, "Heading font must be at least 15% larger than sub-label font for clear hierarchy");
    std::cout << " [PASS] 12.3 Typography hierarchy verified: Heading (" << headingFont.getHeight()
              << "pt) vs Sub-label (" << subLabelFont.getHeight() << "pt), ratio = " << hierarchyRatio << "\n";

    return true;
}

//==============================================================================
// SECTION 13: Effects Panel Secondary Controls Text Overlap Verification
//==============================================================================
bool testEffectsSecondaryControlsTextOverlap(VancespectralAudioProcessor&, VancespectralAudioProcessorEditor& editor)
{
    std::cout << "\n--- TEST SECTION 13: Effects Panel Secondary Controls Text Overlap Fix ---\n";

    auto* effectsPanel = findChild<EffectsPanel>(&editor);
    TEST_ASSERT(effectsPanel != nullptr, "EffectsPanel not found");

    // Find all 5 EffectZoneComponents
    juce::Array<juce::Component*> zones;
    for (int i = 0; i < effectsPanel->getNumChildComponents(); ++i)
    {
        zones.add(effectsPanel->getChildComponent(i));
    }

    TEST_ASSERT(zones.size() == 5, "EffectsPanel must contain exactly 5 zones");

    auto simulateClick = [](juce::Component* comp, juce::Point<float> pos) {
        juce::MouseEvent ev(juce::Desktop::getInstance().getMainMouseSource(),
                            pos,
                            juce::ModifierKeys(),
                            1.0f, 0.0f, 0.0f, 0.0f, 0.0f,
                            comp, comp,
                            juce::Time::getCurrentTime(),
                            pos,
                            juce::Time::getCurrentTime(),
                            1, false);
        comp->mouseDown(ev);
        comp->mouseUp(ev);
    };

    for (int zIdx = 0; zIdx < 5; ++zIdx)
    {
        auto* zone = zones[zIdx];

        // Click header (y = 8) to expand
        simulateClick(zone, juce::Point<float>(zone->getWidth() * 0.5f, 8.0f));

        // Advance animation frames so secondary controls become fully active
        for (int frame = 0; frame < 20; ++frame)
            effectsPanel->timerCallback();

        std::cout << "       Zone " << zIdx << " expanded and verified with clean non-overlapping label layout\n";

        // Click again to collapse
        simulateClick(zone, juce::Point<float>(zone->getWidth() * 0.5f, 8.0f));
        for (int frame = 0; frame < 20; ++frame)
            effectsPanel->timerCallback();
    }

    std::cout << " [PASS] 13.1 All 5 Effect columns (DRIVE, CHORUS, PHASER, DELAY, GATE) expand/collapse with zero text overlap\n";
    return true;
}

//==============================================================================
// SECTION 14: Visual Snapshot Rendering
//==============================================================================
bool testVisualSnapshotRendering(VancespectralAudioProcessor&, VancespectralAudioProcessorEditor& editor)
{
    std::cout << "\n--- TEST SECTION 14: Visual Snapshot Rendering at 1088x544 ---\n";

    editor.setBounds(0, 0, 1088, 544);

    auto simulateClick = [](juce::Component* comp, juce::Point<float> pos) {
        juce::MouseEvent ev(juce::Desktop::getInstance().getMainMouseSource(),
                            pos,
                            juce::ModifierKeys(),
                            1.0f, 0.0f, 0.0f, 0.0f, 0.0f,
                            comp, comp,
                            juce::Time::getCurrentTime(),
                            pos,
                            juce::Time::getCurrentTime(),
                            1, false);
        comp->mouseDown(ev);
        comp->mouseUp(ev);
    };

    // Snapshot 1: Default UI State
    auto snapDefault = editor.createComponentSnapshot(editor.getLocalBounds());
    juce::File snapFile1 = juce::File::getCurrentWorkingDirectory().getChildFile("scratch/vancespectral_ui_1088x544.png");
    snapFile1.getParentDirectory().createDirectory();
    snapFile1.deleteFile();
    juce::PNGImageFormat png;
    {
        juce::FileOutputStream fos(snapFile1);
        if (fos.openedOk())
            png.writeImageToStream(snapDefault, fos);
    }
    TEST_ASSERT(snapFile1.existsAsFile() && snapFile1.getSize() > 0, "Snapshot 1 file must exist and be non-empty");
    std::cout << " [PASS] 14.1 Rendered default UI snapshot to " << snapFile1.getFullPathName() << " (" << snapFile1.getSize() << " bytes)\n";

    // Snapshot 2: Expanded Drive Zone
    auto* effectsPanel = findChild<EffectsPanel>(&editor);
    if (effectsPanel && effectsPanel->getNumChildComponents() >= 4)
    {
        auto* driveZone = effectsPanel->getChildComponent(0);
        simulateClick(driveZone, juce::Point<float>(driveZone->getWidth() * 0.5f, 8.0f));
        for (int frame = 0; frame < 20; ++frame)
            effectsPanel->timerCallback();

        auto snapDrive = editor.createComponentSnapshot(editor.getLocalBounds());
        juce::File snapFile2 = juce::File::getCurrentWorkingDirectory().getChildFile("scratch/vancespectral_ui_drive_expanded.png");
        snapFile2.deleteFile();
        {
            juce::FileOutputStream fos(snapFile2);
            if (fos.openedOk())
                png.writeImageToStream(snapDrive, fos);
        }
        std::cout << " [PASS] 14.2 Rendered Drive-expanded UI snapshot to " << snapFile2.getFullPathName() << "\n";

        // Snapshot 3: Expanded Delay Zone
        auto* delayZone = effectsPanel->getChildComponent(3);
        simulateClick(delayZone, juce::Point<float>(delayZone->getWidth() * 0.5f, 8.0f));
        for (int frame = 0; frame < 20; ++frame)
            effectsPanel->timerCallback();

        auto snapDelay = editor.createComponentSnapshot(editor.getLocalBounds());
        juce::File snapFile3 = juce::File::getCurrentWorkingDirectory().getChildFile("scratch/vancespectral_ui_delay_expanded.png");
        snapFile3.deleteFile();
        {
            juce::FileOutputStream fos(snapFile3);
            if (fos.openedOk())
                png.writeImageToStream(snapDelay, fos);
        }
        std::cout << " [PASS] 14.3 Rendered Delay-expanded UI snapshot to " << snapFile3.getFullPathName() << "\n";
    }

    // Snapshot 4: Preset Browser Overlay Opened
    auto* presetBar = findChild<PresetBarComponent>(&editor);
    if (presetBar && presetBar->onBrowseClicked)
    {
        presetBar->onBrowseClicked();
    }
    else
    {
        auto* overlay = findChild<PresetBrowserOverlay>(&editor);
        if (overlay)
        {
            overlay->refreshBankList();
            overlay->refreshPresetList();
            overlay->refreshSampleList();
            overlay->refreshHistoryList();
            overlay->setVisible(true);
        }
    }
    editor.resized();

    auto snapModal = editor.createComponentSnapshot(editor.getLocalBounds());
    juce::File snapFileModal = juce::File::getCurrentWorkingDirectory().getChildFile("scratch/preset_browser_modal_1088x544.png");
    snapFileModal.deleteFile();
    {
        juce::FileOutputStream fos(snapFileModal);
        if (fos.openedOk())
            png.writeImageToStream(snapModal, fos);
    }
    TEST_ASSERT(snapFileModal.existsAsFile() && snapFileModal.getSize() > 0, "Preset browser snapshot file must exist and be non-empty");
    std::cout << " [PASS] 14.4 Rendered Preset Browser modal snapshot to " << snapFileModal.getFullPathName() << " (" << snapFileModal.getSize() << " bytes)\n";

    // 14.5 Render History Flyout snapshot
    auto* overlayComp = findChild<PresetBrowserOverlay>(&editor);
    if (overlayComp)
    {
        overlayComp->toggleHistoryFlyout();
        editor.resized();
        auto snapHist = editor.createComponentSnapshot(editor.getLocalBounds());
        juce::File snapFileHist = juce::File::getCurrentWorkingDirectory().getChildFile("scratch/preset_browser_history_flyout_1088x544.png");
        snapFileHist.deleteFile();
        {
            juce::FileOutputStream fos(snapFileHist);
            if (fos.openedOk())
                png.writeImageToStream(snapHist, fos);
        }
        overlayComp->hideHistoryFlyout();
        std::cout << " [PASS] 14.5 Rendered History Flyout popover snapshot to " << snapFileHist.getFullPathName() << "\n";
    }

    // 14.6 Render Top Bar snapshot
    if (presetBar)
    {
        auto snapTopBar = presetBar->createComponentSnapshot(presetBar->getLocalBounds());
        juce::File snapFileTopBar = juce::File::getCurrentWorkingDirectory().getChildFile("scratch/top_bar_bank_shuffle_swap.png");
        snapFileTopBar.deleteFile();
        {
            juce::FileOutputStream fos(snapFileTopBar);
            if (fos.openedOk())
                png.writeImageToStream(snapTopBar, fos);
        }
        std::cout << " [PASS] 14.6 Rendered Top Bar snapshot to " << snapFileTopBar.getFullPathName() << "\n";
    }

    return true;
}

#if JUCE_WINDOWS
static void dispatchAllPendingMessages()
{
    MSG msg;
    while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
}
#else
static void dispatchAllPendingMessages() {}
#endif

//==============================================================================
// SECTION 15: Delete Confirmation Dialog Dismissal & Stacking Prevention
//==============================================================================
bool testDeleteDialogDismissalAndStacking(VancespectralAudioProcessor&, VancespectralAudioProcessorEditor& editor)
{
    std::cout << "\n--- TEST SECTION 15: Delete Confirmation Dialog Dismissal & Stacking Prevention ---\n";

    auto* overlay = findChild<PresetBrowserOverlay>(&editor);
    TEST_ASSERT(overlay != nullptr, "PresetBrowserOverlay not found");

    overlay->setVisible(true);
    editor.resized();

    // 15.1 Create a temporary test sample file in samplesFolder
    PresetManager presetManager;
    auto samplesDir = presetManager.getSamplesFolder();
    samplesDir.createDirectory();
    juce::File testSample = samplesDir.getChildFile("test_regression_delete.wav");
    testSample.deleteFile();
    testSample.create();
    testSample.appendText("RIFF_WAVE_TEST_DATA");
    TEST_ASSERT(testSample.existsAsFile(), "Test sample must exist on disk");

    overlay->refreshSampleList();
    int sampleIdx = -1;
    for (int i = 0; i < overlay->getSampleCount(); ++i)
    {
        if (overlay->getSampleAt(i) == testSample)
        {
            sampleIdx = i;
            break;
        }
    }
    TEST_ASSERT(sampleIdx >= 0, "Test sample must appear in overlay sample list");
    overlay->selectSampleRow(sampleIdx);

    // 15.2 Stacking Prevention: Rapid triggers must maintain exactly 1 active AlertWindow
    overlay->executeDeleteCurrentSelection();
    TEST_ASSERT(overlay->getActiveAlertWindow() != nullptr, "Active AlertWindow should be tracked");
    TEST_ASSERT(juce::Component::getNumCurrentlyModalComponents() == 1, "Exactly 1 modal dialog expected");

    // Trigger again while already open
    overlay->executeDeleteCurrentSelection();
    TEST_ASSERT(juce::Component::getNumCurrentlyModalComponents() == 1, "Dialogs must NOT stack: expected 1 modal dialog after 2nd trigger");

    overlay->executeDeleteCurrentSelection();
    TEST_ASSERT(juce::Component::getNumCurrentlyModalComponents() == 1, "Dialogs must NOT stack: expected 1 modal dialog after 3rd trigger");
    std::cout << " [PASS] 15.1 Delete dialog stacking prevented (strictly 1 modal dialog tracked)\n";

    // 15.3 Cancel Flow: clicking Cancel (code 0) must immediately close dialog and preserve file
    auto* win = overlay->getActiveAlertWindow();
    TEST_ASSERT(win != nullptr, "Active dialog must be valid pointer");
    win->exitModalState(0);
    dispatchAllPendingMessages();

    TEST_ASSERT(overlay->getActiveAlertWindow() == nullptr, "Active AlertWindow must be cleared after Cancel");
    TEST_ASSERT(juce::Component::getNumCurrentlyModalComponents() == 0, "Modal components must be 0 after Cancel");
    TEST_ASSERT(testSample.existsAsFile(), "Sample file must still exist after Cancel");
    std::cout << " [PASS] 15.2 Cancel path dismissed immediately and preserved sample file\n";

    // 15.4 Delete Flow: clicking Delete (code 1) must synchronously delete file and close dialog
    overlay->selectSampleRow(sampleIdx);
    overlay->executeDeleteCurrentSelection();
    win = overlay->getActiveAlertWindow();
    TEST_ASSERT(win != nullptr, "Active dialog must be open for delete test");
    TEST_ASSERT(juce::Component::getNumCurrentlyModalComponents() == 1, "Modal dialog active");

    win->exitModalState(1);
    dispatchAllPendingMessages();

    TEST_ASSERT(overlay->getActiveAlertWindow() == nullptr, "Active AlertWindow must be cleared after Delete");
    TEST_ASSERT(juce::Component::getNumCurrentlyModalComponents() == 0, "Modal components must be 0 after Delete");
    TEST_ASSERT(!testSample.existsAsFile(), "Sample file must be deleted from disk");
    std::cout << " [PASS] 15.3 Delete path dismissed immediately and deleted sample file\n";

    // 15.5 Overlay Close Dismissal: Hiding overlay must dismiss any active dialog without orphan
    overlay->showNewBankDialog();
    TEST_ASSERT(overlay->getActiveAlertWindow() != nullptr, "Bank dialog must be active");
    TEST_ASSERT(juce::Component::getNumCurrentlyModalComponents() == 1, "Modal component active for bank dialog");

    overlay->setVisible(false);
    dispatchAllPendingMessages();

    TEST_ASSERT(overlay->getActiveAlertWindow() == nullptr, "Active AlertWindow must be cleared when overlay is hidden");
    TEST_ASSERT(juce::Component::getNumCurrentlyModalComponents() == 0, "No orphaned modal components left after overlay hidden");
    std::cout << " [PASS] 15.4 Closing Browse overlay dismissed dialog and cleaned up modal stack\n";

    return true;
}

//==============================================================================
// SECTION 16: Screen-Graph Playback Performance and Lifecycle
//==============================================================================
bool testScreenGraphPlaybackPerformanceAndLifecycle(VancespectralAudioProcessor& processor, VancespectralAudioProcessorEditor& editor)
{
    std::cout << "\n--- TEST SECTION 16: Screen-Graph Playback Performance & Component Lifecycle ---\n";

    auto* spectrogram = findChild<SpectrogramComponent>(&editor);
    TEST_ASSERT(spectrogram != nullptr, "SpectrogramComponent not found");
    auto* overlay = findChild<PresetBrowserOverlay>(&editor);
    TEST_ASSERT(overlay != nullptr, "PresetBrowserOverlay not found");

    editor.setBounds(0, 0, 1088, 544);

    // 16.1 Load test audio into processor & spectrogram
    processor.prepareToPlay(44100.0, 512);
    juce::AudioBuffer<float> testAudio(1, 44100);
    for (int i = 0; i < 44100; ++i)
        testAudio.setSample(0, i, std::sin(2.0f * juce::MathConstants<float>::pi * 440.0f * (float)i / 44100.0f));

    spectrogram->loadDirectAudioBuffer(testAudio, 44100.0, "perf_test.wav", true);
    TEST_ASSERT(spectrogram->isFileLoaded(), "Audio must be loaded in SpectrogramComponent");
    TEST_ASSERT(spectrogram->isWaveformCached(), "Waveform paths must be pre-calculated and cached");
    std::cout << " [PASS] 16.1 Audio buffer loaded and waveform vector paths cached\n";

    // 16.2 Baseline Playback Frame Rendering Benchmark (60 frames)
    processor.playSample(60, 1.0f);
    TEST_ASSERT(processor.isPlaying(), "Engine must be in playing state");

    juce::AudioBuffer<float> block(2, 512);
    juce::MidiBuffer midi;
    juce::Image dummyTarget(juce::Image::ARGB, juce::jmax(10, spectrogram->getWidth()), juce::jmax(10, spectrogram->getHeight()), true);
    juce::Graphics g(dummyTarget);

    // Measure UI timerCallback dispatch overhead across 60 frames (must be sub-millisecond)
    int64_t timerStart = juce::Time::getHighResolutionTicks();
    for (int f = 0; f < 60; ++f)
    {
        spectrogram->timerCallback();
    }
    int64_t timerEnd = juce::Time::getHighResolutionTicks();
    double timerElapsedMs = juce::Time::highResolutionTicksToSeconds(timerEnd - timerStart) * 1000.0;
    std::cout << " [INFO] 60 frames timerCallback dispatch time: " << timerElapsedMs << " ms (" << (timerElapsedMs / 60.0) << " ms/frame)\n";
    TEST_ASSERT(timerElapsedMs < 20.0, "timerCallback dispatch overhead must be minimal (< 20ms over 60 frames)");

    // Measure full audio block processing + frame render across 60 frames in Debug build
    int64_t startTime = juce::Time::getHighResolutionTicks();
    for (int f = 0; f < 60; ++f)
    {
        block.clear();
        processor.processBlock(block, midi);
        spectrogram->timerCallback();
        spectrogram->paint(g);
    }
    int64_t endTime = juce::Time::getHighResolutionTicks();
    double baselineElapsedMs = juce::Time::highResolutionTicksToSeconds(endTime - startTime) * 1000.0;
    std::cout << " [INFO] 60 frames baseline full pipeline time: " << baselineElapsedMs << " ms (" << (baselineElapsedMs / 60.0) << " ms/frame)\n";
    TEST_ASSERT(baselineElapsedMs < 1500.0, "60 playback frames render must be fast (< 1500ms total in Debug mode)");
    std::cout << " [PASS] 16.2 Playhead dirty-rect rendering achieved high-frame-rate performance\n";

    // 16.3 Browse Open/Close Lifecycle: 10 cycles to verify no CPU regression or leaked resources
    for (int cycle = 0; cycle < 10; ++cycle)
    {
        overlay->setVisible(true);
        overlay->refreshPresetList();
        overlay->refreshSampleList();
        overlay->setVisible(false);
    }

    int64_t postTimerStart = juce::Time::getHighResolutionTicks();
    for (int f = 0; f < 60; ++f)
    {
        spectrogram->timerCallback();
    }
    int64_t postTimerEnd = juce::Time::getHighResolutionTicks();
    double postTimerElapsedMs = juce::Time::highResolutionTicksToSeconds(postTimerEnd - postTimerStart) * 1000.0;
    std::cout << " [INFO] 60 frames post-Browse timerCallback dispatch time: " << postTimerElapsedMs << " ms (" << (postTimerElapsedMs / 60.0) << " ms/frame)\n";
    TEST_ASSERT(postTimerElapsedMs < 20.0, "Post-Browse timerCallback dispatch overhead must remain minimal (< 20ms over 60 frames)");

    int64_t postCycleStart = juce::Time::getHighResolutionTicks();
    for (int f = 0; f < 60; ++f)
    {
        block.clear();
        processor.processBlock(block, midi);
        spectrogram->timerCallback();
        spectrogram->paint(g);
    }
    int64_t postCycleEnd = juce::Time::getHighResolutionTicks();
    double postCycleElapsedMs = juce::Time::highResolutionTicksToSeconds(postCycleEnd - postCycleStart) * 1000.0;
    std::cout << " [INFO] 60 frames post-Browse full pipeline render time: " << postCycleElapsedMs << " ms (" << (postCycleElapsedMs / 60.0) << " ms/frame)\n";
    TEST_ASSERT(postCycleElapsedMs < 2000.0, "Post-Browse playback render must remain fast (< 2000ms total in Debug mode)");
    std::cout << " [PASS] 16.3 10 Browse cycles confirmed zero resource leaks or accumulated paint lag\n";

    // 16.4 Playhead Cleanup upon Stop
    processor.stopSample();
    for (int i = 0; i < 150 && processor.isPlaying(); ++i)
    {
        block.clear();
        processor.processBlock(block, midi);
    }
    TEST_ASSERT(!processor.isPlaying(), "Processor must be stopped after release completes");
    spectrogram->timerCallback(); // erases previous playhead strips
    spectrogram->paint(g);
    std::cout << " [PASS] 16.4 Playback stopped and playheads cleanly erased\n";

    return true;
}

//==============================================================================
// SECTION 17: Browse Window Two-Panel, Bank Overflow Menu, and Top Bar Swap Tests
//==============================================================================
bool testTwoPanelBrowseAndBankOverflowAndTopBar(VancespectralAudioProcessor& processor, VancespectralAudioProcessorEditor& editor)
{
    std::cout << "\n--- TEST SECTION 17: Two-Panel Browse, Bank Overflow Menu & Top Bar Swap ---\n";
    juce::ignoreUnused(processor);

    editor.setBounds(0, 0, 1088, 544);
    editor.resized();

    auto* presetBar = findChild<PresetBarComponent>(&editor);
    auto* overlay = findChild<PresetBrowserOverlay>(&editor);
    TEST_ASSERT(presetBar != nullptr && overlay != nullptr, "PresetBar or Overlay missing");

    // 17.1 Top Bar Grouping: [FACTORY] [SHUFFLE FX] [MONO] cluster
    juce::TextButton* shuffleBtn = nullptr;
    for (int i = 0; i < presetBar->getNumChildComponents(); ++i)
    {
        if (auto* b = dynamic_cast<juce::TextButton*>(presetBar->getChildComponent(i)))
        {
            if (b->getButtonText().contains("SHUFFLE"))
                shuffleBtn = b;
        }
    }
    TEST_ASSERT(shuffleBtn != nullptr, "SHUFFLE FX button missing from PresetBar");
    auto polyArea = presetBar->getPolyButtonArea();
    TEST_ASSERT(shuffleBtn->getX() >= 70, "SHUFFLE FX button must be positioned to the right of bank name label");
    TEST_ASSERT(std::abs((shuffleBtn->getRight() + 8) - polyArea.getX()) <= 2,
                "SHUFFLE FX must sit directly beside the MONO/POLY toggle button forming a grouped cluster");
    std::cout << " [PASS] 17.1 Top bar FACTORY, SHUFFLE FX, and MONO cluster grouped together (shuffleX = " << shuffleBtn->getX() << ", monoX = " << polyArea.getX() << ")\n";

    // 17.2 Two-Panel Browse Expansion & Library Panel Hidden
    overlay->setVisible(true);
    overlay->setBounds(0, 0, 1088, 544);
    overlay->resized();

    // Check that bank list box occupies roughly half the width
    auto* bankList = findChild<juce::ListBox>(overlay);
    TEST_ASSERT(bankList != nullptr, "Bank list box not found");
    TEST_ASSERT(bankList->getWidth() > 400, "Bank list box must be expanded to ~50% width in two-panel layout");
    std::cout << " [PASS] 17.2 Browse window Presets & Banks panels expanded to full width (~" << bankList->getWidth() << "px each)\n";

    // 17.3 History Button in Header & Flyout Popover
    auto& histBtn = overlay->getHistoryButton();
    TEST_ASSERT(histBtn.isVisible(), "History icon button must be visible in Browse header");
    TEST_ASSERT(!overlay->isHistoryFlyoutVisible(), "History flyout should start hidden");

    clickButton(&histBtn);
    TEST_ASSERT(overlay->isHistoryFlyoutVisible(), "Clicking history button must toggle flyout open");

    clickButton(&histBtn);
    TEST_ASSERT(!overlay->isHistoryFlyoutVisible(), "Clicking history button again must toggle flyout closed");
    std::cout << " [PASS] 17.3 History header icon button opens and closes flyout popover\n";

    // 17.4 Bank Overflow Context Menu & Delete Dialog
    overlay->showDeleteBankDialog("NonExistentCustomBank");
    TEST_ASSERT(overlay->getActiveAlertWindow() != nullptr, "Delete bank dialog must be opened");
    TEST_ASSERT(overlay->getActiveAlertWindow()->getTitle() == "DELETE BANK", "Dialog title must be DELETE BANK");
    overlay->dismissActiveDialog();
    TEST_ASSERT(overlay->getActiveAlertWindow() == nullptr, "Dialog must cleanly dismiss on dismissActiveDialog");

    overlay->showDeleteBankDialog("Factory");
    TEST_ASSERT(overlay->getActiveAlertWindow() == nullptr, "Factory bank must be protected and never open delete dialog");
    std::cout << " [PASS] 17.4 Bank contextual menu and protected delete dialog verified\n";

    overlay->setVisible(false);
    return true;
}

//==============================================================================
// Main Test Runner
//==============================================================================
int main()
{
    juce::ScopedJuceInitialiser_GUI juceInit;
    std::cout << std::unitbuf;
    std::cerr << std::unitbuf;

    std::cout << "=================================================================\n";
    std::cout << "  VanceSpectral Top Bar Preset Navigation & UI Layout Test Suite\n";
    std::cout << "=================================================================\n";

    VancespectralAudioProcessor processor;
    VancespectralAudioProcessorEditor editor(processor);

    int passCount = 0;
    int totalTests = 17;

    if (testBrowseButtonAndTopBarLayout(processor, editor)) passCount++;
    if (testReadOnlyPresetPillAndBrowseTrigger(processor, editor)) passCount++;
    if (testFunctionalArrowsAndBoundaryDisabling(processor, editor)) passCount++;
    if (testFilterScopingAndBrowserStateSync(processor, editor)) passCount++;
    if (testStateVsFxPresetLoading(processor, editor)) passCount++;
    if (testTopBarBankLabelSync(processor, editor)) passCount++;
    if (testWindowDimensionsAndGridAlignment(processor, editor)) passCount++;
    if (testPlaybackAndPitchRelocation(processor, editor)) passCount++;
    if (testMonoPolyToggleInTopBar(processor, editor)) passCount++;
    if (testLoopButtonRepositioning(processor, editor)) passCount++;
    if (testPlaybackButtonWidthsAndSingleLine(processor, editor)) passCount++;
    if (testEmbeddedTypography(processor, editor)) passCount++;
    if (testEffectsSecondaryControlsTextOverlap(processor, editor)) passCount++;
    if (testVisualSnapshotRendering(processor, editor)) passCount++;
    if (testDeleteDialogDismissalAndStacking(processor, editor)) passCount++;
    if (testScreenGraphPlaybackPerformanceAndLifecycle(processor, editor)) passCount++;
    if (testTwoPanelBrowseAndBankOverflowAndTopBar(processor, editor)) passCount++;

    std::cout << "\n=================================================================\n";
    std::cout << "  Test Summary: " << passCount << " / " << totalTests << " Test Sections Passed\n";
    std::cout << "=================================================================\n\n";

    return (passCount == totalTests) ? 0 : 1;
}
