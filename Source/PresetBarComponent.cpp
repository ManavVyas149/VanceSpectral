#include "PresetBarComponent.h"

PresetBarComponent::PresetBarComponent()
{
    addAndMakeVisible(prevButton);
    addAndMakeVisible(nextButton);

    shuffleFxButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0x22, 0x22, 0x2C));
    shuffleFxButton.setColour(juce::TextButton::buttonOnColourId, SpectralUILookAndFeel::accentColour);
    shuffleFxButton.setColour(juce::TextButton::textColourOffId, SpectralUILookAndFeel::accentColour);
    addAndMakeVisible(shuffleFxButton);

    saveStateButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0x22, 0x22, 0x2C));
    saveStateButton.setColour(juce::TextButton::buttonOnColourId, SpectralUILookAndFeel::accentColour);
    saveStateButton.setColour(juce::TextButton::textColourOffId, SpectralUILookAndFeel::accentColour);
    addAndMakeVisible(saveStateButton);

    prevButton.onClick = [this]() {
        if (onPrevClicked) onPrevClicked();
    };

    nextButton.onClick = [this]() {
        if (onNextClicked) onNextClicked();
    };

    shuffleFxButton.onClick = [this]() {
        if (onShuffleFxClicked) onShuffleFxClicked();
    };

    saveStateButton.onClick = [this]() {
        if (onSaveStateClicked) onSaveStateClicked();
    };
}

void PresetBarComponent::setPresetName(const juce::String& name)
{
    juce::String cleanName = name;
    while (cleanName.endsWithIgnoreCase("_Export") || cleanName.endsWithIgnoreCase("_Rendered_Region"))
    {
        if (cleanName.endsWithIgnoreCase("_Export"))
            cleanName = cleanName.dropLastCharacters(7);
        else if (cleanName.endsWithIgnoreCase("_Rendered_Region"))
            cleanName = cleanName.dropLastCharacters(16);
    }
    currentPresetName = cleanName;
    repaint();
}

void PresetBarComponent::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();

    // Top Bar background: matte panel background
    g.setColour(SpectralUILookAndFeel::panelBgColour);
    g.fillRoundedRectangle(bounds, 6.0f);

    // Hairline border at bottom
    g.setColour(SpectralUILookAndFeel::dividerColour);
    g.drawHorizontalLine(getHeight() - 1, 0.0f, (float)getWidth());

    // Left: Bank Name Label (positioned after SHUFFLE FX button)
    g.setFont(SpectralUILookAndFeel::getGeometricFont(10.0f, true));
    g.setColour(SpectralUILookAndFeel::textMutedColour);
    auto bankRect = juce::Rectangle<float>(118.0f, 0.0f, 90.0f, (float)getHeight());
    g.drawText(bankName, bankRect.toNearestInt(), juce::Justification::centredLeft, true);

    // Center Preset Display
    g.setFont(SpectralUILookAndFeel::getGeometricFont(13.0f, true));
    g.setColour(SpectralUILookAndFeel::textMainColour);

    float cx = bounds.getCentreX();
    float cy = bounds.getCentreY();

    juce::GlyphArrangement ga;
    ga.addLineOfText(SpectralUILookAndFeel::getGeometricFont(13.0f, true), currentPresetName, 0.0f, 0.0f);
    float textWidth = ga.getBoundingBox(0, -1, true).getWidth();

    // Draw Preset Name centered
    juce::Rectangle<float> nameRect(cx - textWidth * 0.5f - 10.0f, 0.0f, textWidth + 20.0f, (float)getHeight());
    g.drawText(currentPresetName, nameRect.toNearestInt(), juce::Justification::centred, false);

    // Draw small dropdown caret (▼) to the right of preset name
    juce::Path caret;
    float caretX = nameRect.getRight() + 2.0f;
    caret.startNewSubPath(caretX, cy - 2.0f);
    caret.lineTo(caretX + 6.0f, cy - 2.0f);
    caret.lineTo(caretX + 3.0f, cy + 2.0f);
    caret.closeSubPath();

    g.setColour(SpectralUILookAndFeel::textMutedColour);
    g.fillPath(caret);

    // Store target click bounds for browser overlay trigger
    const_cast<PresetBarComponent*>(this)->clickTargetBounds = juce::Rectangle<float>(cx - textWidth * 0.5f - 24.0f, 0.0f, textWidth + 50.0f, (float)getHeight());
}

void PresetBarComponent::resized()
{
    auto bounds = getLocalBounds().reduced(12, 4);

    // Left side SHUFFLE FX button
    shuffleFxButton.setBounds(12, bounds.getY() + 1, 98, bounds.getHeight() - 2);

    // Center chevrons flanking the preset name area
    float cx = (float)bounds.getCentreX();
    prevButton.setBounds((int)(cx - 140.0f), bounds.getY(), 24, bounds.getHeight());
    nextButton.setBounds((int)(cx + 116.0f), bounds.getY(), 24, bounds.getHeight());

    // Right side SAVE STATE button
    saveStateButton.setBounds(getWidth() - 120, (int)(bounds.getY() + 1), 105, (int)(bounds.getHeight() - 2));
}

void PresetBarComponent::mouseDown(const juce::MouseEvent& e)
{
    if (clickTargetBounds.contains(e.position) && onBrowseClicked)
    {
        onBrowseClicked();
    }
}
