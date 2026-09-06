/*
  ==============================================================================

    EffectsPanel.cpp
    Created: 6 Sep 2026
    Author:  Manav Vyas / VanceSpectral Team

  ==============================================================================
*/

#include "EffectsPanel.h"

EffectsPanel::EffectsPanel(juce::AudioProcessorValueTreeState& apvts)
{
    auto setupToggle = [this](juce::TextButton& btn) {
        btn.setClickingTogglesState(true);
        btn.setColour(juce::TextButton::buttonColourId, juce::Colour(0x22, 0x22, 0x2C));
        btn.setColour(juce::TextButton::buttonOnColourId, SpectralUILookAndFeel::accentColour);
        btn.setColour(juce::TextButton::textColourOffId, SpectralUILookAndFeel::textMutedColour);
        btn.setColour(juce::TextButton::textColourOnId, juce::Colours::black);
        addAndMakeVisible(btn);
    };

    auto setupSlider = [this](juce::Slider& slider, juce::Label& label, const juce::String& name) {
        addAndMakeVisible(slider);
        label.setText(name, juce::dontSendNotification);
        label.setFont(SpectralUILookAndFeel::getGeometricFont(10.0f, false));
        label.setJustificationType(juce::Justification::centred);
        label.setColour(juce::Label::textColourId, SpectralUILookAndFeel::textMutedColour);
        addAndMakeVisible(label);
    };

    // 1. GATE
    setupToggle(gateToggle);
    setupSlider(gateAmountSlider, gateAmountLabel, "Thresh");
    gateToggleAttachment = std::make_unique<ButtonAttachment>(apvts, "FX_GATE_ENABLE", gateToggle);
    gateAmountAttachment = std::make_unique<SliderAttachment>(apvts, "FX_GATE_AMOUNT", gateAmountSlider);

    // 2. CHORUS
    setupToggle(chorusToggle);
    setupSlider(chorusAmountSlider, chorusAmountLabel, "Mix");
    setupSlider(chorusRateSlider, chorusRateLabel, "Rate");
    chorusToggleAttachment = std::make_unique<ButtonAttachment>(apvts, "FX_CHORUS_ENABLE", chorusToggle);
    chorusAmountAttachment = std::make_unique<SliderAttachment>(apvts, "FX_CHORUS_AMOUNT", chorusAmountSlider);
    chorusRateAttachment = std::make_unique<SliderAttachment>(apvts, "FX_CHORUS_RATE", chorusRateSlider);

    // 3. PHASER
    setupToggle(phaserToggle);
    setupSlider(phaserAmountSlider, phaserAmountLabel, "Mix");
    setupSlider(phaserRateSlider, phaserRateLabel, "Rate");
    phaserToggleAttachment = std::make_unique<ButtonAttachment>(apvts, "FX_PHASER_ENABLE", phaserToggle);
    phaserAmountAttachment = std::make_unique<SliderAttachment>(apvts, "FX_PHASER_AMOUNT", phaserAmountSlider);
    phaserRateAttachment = std::make_unique<SliderAttachment>(apvts, "FX_PHASER_RATE", phaserRateSlider);

    // 4. DELAY
    setupToggle(delayToggle);
    setupSlider(delayAmountSlider, delayAmountLabel, "Mix");
    setupSlider(delayTimeSlider, delayTimeLabel, "Time");
    delayToggleAttachment = std::make_unique<ButtonAttachment>(apvts, "FX_DELAY_ENABLE", delayToggle);
    delayAmountAttachment = std::make_unique<SliderAttachment>(apvts, "FX_DELAY_AMOUNT", delayAmountSlider);
    delayTimeAttachment = std::make_unique<SliderAttachment>(apvts, "FX_DELAY_TIME", delayTimeSlider);

    // 5. DRIVE
    setupToggle(driveToggle);
    setupSlider(driveAmountSlider, driveAmountLabel, "Drive");
    setupSlider(driveToneSlider, driveToneLabel, "Tone");
    driveToggleAttachment = std::make_unique<ButtonAttachment>(apvts, "FX_DRIVE_ENABLE", driveToggle);
    driveAmountAttachment = std::make_unique<SliderAttachment>(apvts, "FX_DRIVE_AMOUNT", driveAmountSlider);
    driveToneAttachment = std::make_unique<SliderAttachment>(apvts, "FX_DRIVE_TONE", driveToneSlider);
}

EffectsPanel::~EffectsPanel()
{
    gateToggleAttachment.reset();
    gateAmountAttachment.reset();

    chorusToggleAttachment.reset();
    chorusAmountAttachment.reset();
    chorusRateAttachment.reset();

    phaserToggleAttachment.reset();
    phaserAmountAttachment.reset();
    phaserRateAttachment.reset();

    delayToggleAttachment.reset();
    delayAmountAttachment.reset();
    delayTimeAttachment.reset();

    driveToggleAttachment.reset();
    driveAmountAttachment.reset();
    driveToneAttachment.reset();
}

void EffectsPanel::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();

    // Matte panel background
    g.setColour(SpectralUILookAndFeel::panelBgColour);
    g.fillRoundedRectangle(bounds, 6.0f);

    // Hairline border
    g.setColour(SpectralUILookAndFeel::dividerColour);
    g.drawRoundedRectangle(bounds, 6.0f, 1.0f);

    // Header label
    if (!headerArea.isEmpty())
    {
        g.setFont(SpectralUILookAndFeel::getGeometricFont(10.0f, true));
        g.setColour(SpectralUILookAndFeel::textMutedColour);
        g.drawText("EFFECTS", headerArea, juce::Justification::left, true);
    }

    // Vertical dividers between effect columns
    g.setColour(SpectralUILookAndFeel::dividerColour);
    for (int i = 0; i < 4; ++i)
    {
        if (dividerXs[i] > 0)
        {
            g.drawVerticalLine(dividerXs[i], 12.0f, (float)getHeight() - 12.0f);
        }
    }
}

void EffectsPanel::resized()
{
    auto area = getLocalBounds().reduced(8, 6);
    if (area.getWidth() <= 0 || area.getHeight() <= 0)
        return;

    headerArea = area.removeFromTop(14);
    area.removeFromTop(4);

    const int numSections = 5;
    const int totalWidth = area.getWidth();
    const int sectionWidth = totalWidth / numSections;

    auto layoutSection = [](juce::Rectangle<int> secArea,
                            juce::TextButton& toggle,
                            juce::Slider& primSlider, juce::Label& primLabel,
                            juce::Slider* secSlider = nullptr, juce::Label* secLabel = nullptr)
    {
        if (secArea.getWidth() <= 0 || secArea.getHeight() <= 0)
            return;

        // Top: toggle button
        auto topArea = secArea.removeFromTop(16).reduced(2, 0);
        toggle.setBounds(topArea);

        secArea.removeFromTop(2);

        if (secSlider != nullptr && secLabel != nullptr)
        {
            int colW = secArea.getWidth() / 2;
            auto leftCol = secArea.removeFromLeft(colW);
            auto rightCol = secArea;

            primLabel.setBounds(leftCol.removeFromBottom(12));
            primSlider.setBounds(leftCol.reduced(1, 1));

            secLabel->setBounds(rightCol.removeFromBottom(12));
            secSlider->setBounds(rightCol.reduced(1, 1));
        }
        else
        {
            primLabel.setBounds(secArea.removeFromBottom(12));
            primSlider.setBounds(secArea.reduced(2, 1));
        }
    };

    // Section 1: GATE
    auto gateArea = area.removeFromLeft(sectionWidth);
    dividerXs[0] = gateArea.getRight() + 1;
    area.removeFromLeft(3);
    layoutSection(gateArea, gateToggle, gateAmountSlider, gateAmountLabel);

    // Section 2: CHORUS
    auto chorusArea = area.removeFromLeft(sectionWidth);
    dividerXs[1] = chorusArea.getRight() + 1;
    area.removeFromLeft(3);
    layoutSection(chorusArea, chorusToggle, chorusAmountSlider, chorusAmountLabel, &chorusRateSlider, &chorusRateLabel);

    // Section 3: PHASER
    auto phaserArea = area.removeFromLeft(sectionWidth);
    dividerXs[2] = phaserArea.getRight() + 1;
    area.removeFromLeft(3);
    layoutSection(phaserArea, phaserToggle, phaserAmountSlider, phaserAmountLabel, &phaserRateSlider, &phaserRateLabel);

    // Section 4: DELAY
    auto delayArea = area.removeFromLeft(sectionWidth);
    dividerXs[3] = delayArea.getRight() + 1;
    area.removeFromLeft(3);
    layoutSection(delayArea, delayToggle, delayAmountSlider, delayAmountLabel, &delayTimeSlider, &delayTimeLabel);

    // Section 5: DRIVE
    layoutSection(area, driveToggle, driveAmountSlider, driveAmountLabel, &driveToneSlider, &driveToneLabel);
}
