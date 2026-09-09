#include "PresetBarComponent.h"
#include "BinaryFontData.h"

PresetBarComponent::PresetBarComponent()
{
    juce::MemoryInputStream stream(BinaryData::VanceLogo_png, (size_t)BinaryData::VanceLogo_pngSize, false);
    logoImage = juce::PNGImageFormat().decodeImage(stream);

    addAndMakeVisible(prevButton);
    addAndMakeVisible(nextButton);

    shuffleFxButton.setColour(juce::TextButton::buttonColourId, SpectralUILookAndFeel::panelBgColour);
    shuffleFxButton.setColour(juce::TextButton::buttonOnColourId, SpectralUILookAndFeel::accentColour);
    shuffleFxButton.setColour(juce::TextButton::textColourOffId, SpectralUILookAndFeel::accentColour);
    addAndMakeVisible(shuffleFxButton);

    browseButton.setColour(juce::TextButton::buttonColourId, SpectralUILookAndFeel::panelBgColour);
    browseButton.setColour(juce::TextButton::buttonOnColourId, SpectralUILookAndFeel::accentColour);
    browseButton.setColour(juce::TextButton::textColourOffId, SpectralUILookAndFeel::accentColour);
    addAndMakeVisible(browseButton);

    saveStateButton.setColour(juce::TextButton::buttonColourId, SpectralUILookAndFeel::panelBgColour);
    saveStateButton.setColour(juce::TextButton::buttonOnColourId, SpectralUILookAndFeel::accentColour);
    saveStateButton.setColour(juce::TextButton::textColourOffId, SpectralUILookAndFeel::accentColour);
    addAndMakeVisible(saveStateButton);

    prevButton.onClick = [this]() {
        if (onPrevClicked) onPrevClicked();
    };

    nextButton.onClick = [this]() {
        if (onNextClicked) onNextClicked();
    };

    browseButton.onClick = [this]() {
        if (onBrowseClicked) onBrowseClicked();
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

juce::Rectangle<int> PresetBarComponent::getPolyButtonArea() const
{
    int btnW = 50;
    int btnH = juce::jmin(22, getHeight() - 8);
    int y = (getHeight() - btnH) / 2;
    int x = prevButton.getX() - 8 - btnW;
    return { x, y, btnW, btnH };
}

void PresetBarComponent::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();

    // Top Bar Card Surface with subtle gloss sheen & hairline border
    SpectralUILookAndFeel::drawPanelCard(g, bounds);

    // Top-left: authentic Vance logo image (scaled proportionally with transparency)
    if (logoImage.isValid())
    {
        float logoH = 20.0f;
        float logoW = logoH * (793.0f / 1024.0f); // ~15.5px
        float logoY = (bounds.getHeight() - logoH) * 0.5f;
        juce::Rectangle<float> logoBounds(8.0f, logoY, logoW, logoH);
        g.drawImageWithin(logoImage, (int)logoBounds.getX(), (int)logoBounds.getY(),
                          (int)logoBounds.getWidth(), (int)logoBounds.getHeight(),
                          juce::RectanglePlacement::centred | juce::RectanglePlacement::onlyReduceInSize, false);
    }

    // Left: Bank Name Label (now leftmost, placed directly beside the top-left logo)
    g.setFont(SpectralUILookAndFeel::getJetBrainsMono(11.5f, true));
    g.setColour(SpectralUILookAndFeel::textMutedColour);
    int bankX = 32;
    int bankW = juce::jmax(40, shuffleFxButton.getX() - 10 - bankX);
    auto bankRect = juce::Rectangle<float>((float)bankX, 0.0f, (float)bankW, (float)getHeight());
    g.drawText(bankName.toUpperCase(), bankRect.toNearestInt(), juce::Justification::centredLeft, true);

    // Center Preset Display Pill (Pure Read-Only Label)
    g.setFont(SpectralUILookAndFeel::getJetBrainsMono(12.5f, true));
    float cx = bounds.getCentreX();

    float pillW = 180.0f;
    juce::Rectangle<float> pillRect(cx - pillW * 0.5f, 4.0f, pillW, (float)getHeight() - 8.0f);
    g.setColour(juce::Colour(0xEE, 0xF0, 0xF8));
    g.fillRoundedRectangle(pillRect, 4.0f);
    g.setColour(SpectralUILookAndFeel::dividerColour);
    g.drawRoundedRectangle(pillRect, 4.0f, 0.8f);

    // Draw Preset Name centered within pill
    g.setColour(SpectralUILookAndFeel::textMainColour);
    juce::Rectangle<float> nameRect = pillRect.reduced(8.0f, 0.0f);
    g.drawText(currentPresetName, nameRect.toNearestInt(), juce::Justification::centred, true);
}

void PresetBarComponent::resized()
{
    auto bounds = getLocalBounds().reduced(8, 3);

    // Left side: Bank name is leftmost (beside the logo at x=32), followed by SHUFFLE FX button
    int bankX = 32;
    auto font = SpectralUILookAndFeel::getJetBrainsMono(11.5f, true);
    int textW = juce::GlyphArrangement::getStringWidthInt(font, bankName.toUpperCase());
    int bankW = juce::jlimit(45, 110, textW + 4);
    int shuffleX = bankX + bankW + 10;
    shuffleFxButton.setBounds(shuffleX, bounds.getY() + 1, 90, bounds.getHeight() - 2);

    // Center chevrons flanking the preset name area & dedicated Browse button
    float cx = (float)bounds.getCentreX();
    int btnW = 22;
    int pillW = 180;
    int gap = 5;
    int browseW = 70;

    int pillX = (int)(cx - pillW * 0.5f);
    prevButton.setBounds(pillX - gap - btnW, bounds.getY(), btnW, bounds.getHeight());
    nextButton.setBounds(pillX + pillW + gap, bounds.getY(), btnW, bounds.getHeight());
    browseButton.setBounds(nextButton.getRight() + 6, bounds.getY() + 1, browseW, bounds.getHeight() - 2);

    // Right side SAVE STATE button
    saveStateButton.setBounds(getWidth() - 104, bounds.getY() + 1, 96, bounds.getHeight() - 2);
}
