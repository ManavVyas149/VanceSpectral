#include "SpectralUILookAndFeel.h"

// Define Palette Constants (Dark Mode System)
const juce::Colour SpectralUILookAndFeel::bgColour        = juce::Colour::fromRGB(0x0E, 0x0E, 0x11); // #0E0E11 Deep Charcoal
const juce::Colour SpectralUILookAndFeel::panelBgColour   = juce::Colour::fromRGB(0x16, 0x17, 0x1E); // #16171E Dark Matte Surface
const juce::Colour SpectralUILookAndFeel::graphBgColour   = juce::Colour::fromRGB(0x08, 0x08, 0x0B); // #08080B Deep Black Graph
const juce::Colour SpectralUILookAndFeel::textMainColour  = juce::Colour::fromRGB(0xF1, 0xF3, 0xF9); // #F1F3F9 Crisp Silver White
const juce::Colour SpectralUILookAndFeel::textMutedColour = juce::Colour::fromRGB(0x94, 0x98, 0xA6); // #9498A6 Muted Warm Gray
const juce::Colour SpectralUILookAndFeel::dividerColour   = juce::Colour::fromRGB(0x2C, 0x2D, 0x3A).withAlpha(0.6f); // Hairline Divider
const juce::Colour SpectralUILookAndFeel::accentColour    = juce::Colour::fromRGB(0xD9, 0x8B, 0x4F); // #D98B4F Warm Amber Accent

SpectralUILookAndFeel::SpectralUILookAndFeel()
{
    setColour(juce::ResizableWindow::backgroundColourId, bgColour);
    setColour(juce::Label::textColourId, textMainColour);
    setColour(juce::PopupMenu::backgroundColourId, panelBgColour);
    setColour(juce::PopupMenu::headerTextColourId, textMutedColour);
    setColour(juce::PopupMenu::textColourId, textMainColour);
    setColour(juce::PopupMenu::highlightedBackgroundColourId, accentColour.withAlpha(0.15f));
    setColour(juce::PopupMenu::highlightedTextColourId, textMainColour);
}

juce::Font SpectralUILookAndFeel::getGeometricFont(float height, bool bold)
{
    juce::FontOptions options("Segoe UI", height, bold ? juce::Font::bold : juce::Font::plain);
    return juce::Font(options);
}

juce::Font SpectralUILookAndFeel::getMonospaceFont(float height)
{
    juce::FontOptions options("Consolas", height, juce::Font::plain);
    return juce::Font(options);
}

juce::Font SpectralUILookAndFeel::getLabelFont(juce::Label&)
{
    return getGeometricFont(11.0f, false);
}

juce::Font SpectralUILookAndFeel::getTextButtonFont(juce::TextButton&, int buttonHeight)
{
    return getGeometricFont(juce::jmin(12.0f, (float)buttonHeight * 0.45f), false);
}

void SpectralUILookAndFeel::drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height,
                                              float sliderPos, float rotaryStartAngle, float rotaryEndAngle,
                                              juce::Slider& slider)
{
    if (width <= 0 || height <= 0)
        return;

    auto bounds = juce::Rectangle<int>(x, y, width, height).toFloat();
    float diameter = juce::jmin(bounds.getWidth(), bounds.getHeight()) - 8.0f;
    if (diameter <= 4.0f)
        return;

    float centreX = bounds.getCentreX();
    float centreY = bounds.getCentreY();
    float radius = diameter * 0.5f;

    auto knobRect = juce::Rectangle<float>(centreX - radius, centreY - radius, diameter, diameter);
    auto angle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);

    // Subtle Ambient Shadow
    g.setColour(juce::Colour::fromRGB(0x05, 0x05, 0x08).withAlpha(0.40f));
    g.fillEllipse(knobRect.translated(0.0f, 1.5f));

    // Knob Base Disc (Dark Metallic Matte)
    g.setColour(juce::Colour::fromRGB(0x22, 0x24, 0x2E));
    g.fillEllipse(knobRect);

    // Soft 1-2px Inset Ring
    g.setColour(juce::Colour::fromRGB(0x2A, 0x2C, 0x38));
    g.drawEllipse(knobRect, 1.0f);

    auto insetRect = knobRect.reduced(2.5f);
    g.setColour(juce::Colour::fromRGB(0x1A, 0x1C, 0x24));
    g.fillEllipse(insetRect);

    // Thin Track Arc
    float arcRadius = radius - 1.0f;
    juce::Path trackBg;
    trackBg.addCentredArc(centreX, centreY, arcRadius, arcRadius, 0.0f, rotaryStartAngle, rotaryEndAngle, true);
    g.setColour(dividerColour);
    g.strokePath(trackBg, juce::PathStrokeType(1.2f));

    if (sliderPos > 0.001f)
    {
        juce::Path trackActive;
        trackActive.addCentredArc(centreX, centreY, arcRadius, arcRadius, 0.0f, rotaryStartAngle, angle, true);
        g.setColour(slider.isMouseOverOrDragging() ? accentColour : accentColour.withAlpha(0.85f));
        g.strokePath(trackActive, juce::PathStrokeType(1.6f));
    }

    // Thin Pointer Line
    float pointerLength = insetRect.getWidth() * 0.38f;
    juce::Path p;
    p.addRectangle(-0.75f, -pointerLength, 1.5f, pointerLength);
    p.applyTransform(juce::AffineTransform::rotation(angle).translated(centreX, centreY));

    g.setColour(slider.isMouseOverOrDragging() ? accentColour : textMainColour);
    g.fillPath(p);
}

void SpectralUILookAndFeel::drawLinearSlider(juce::Graphics& g, int x, int y, int width, int height,
                                              float sliderPos, float minSliderPos, float maxSliderPos,
                                              const juce::Slider::SliderStyle style, juce::Slider& slider)
{
    if (width <= 0 || height <= 0)
        return;

    auto bounds = juce::Rectangle<int>(x, y, width, height).toFloat();

    if (style == juce::Slider::LinearVertical || slider.isVertical())
    {
        bool isEnabled = slider.isEnabled();

        // 1. Readout at top (e.g., "0 st", "+3 st")
        float val = (float)slider.getValue();
        int stVal = (int)std::round(val);
        juce::String valStr = (stVal > 0 ? "+" : "") + juce::String(stVal) + " st";

        auto topArea = bounds.removeFromTop(12.0f);
        g.setFont(getMonospaceFont(9.5f));
        g.setColour(isEnabled ? (slider.isMouseOverOrDragging() ? accentColour : textMainColour) : textMutedColour);
        g.drawText(valStr, topArea, juce::Justification::centred, true);

        // 2. Track area in middle
        bounds.reduce(2.0f, 2.0f);
        float trackWidth = 5.0f;
        float trackX = bounds.getCentreX() - trackWidth * 0.5f;
        auto trackArea = juce::Rectangle<float>(trackX, bounds.getY(), trackWidth, bounds.getHeight());

        // Dark track background
        g.setColour(graphBgColour);
        g.fillRoundedRectangle(trackArea, 2.5f);

        // Hairline border
        g.setColour(dividerColour);
        g.drawRoundedRectangle(trackArea, 2.5f, 1.0f);

        // Center 0 st baseline
        float normZero = 0.5f;
        if (maxSliderPos != minSliderPos)
            normZero = juce::jlimit(0.0f, 1.0f, (float)((0.0 - slider.getMinimum()) / (slider.getMaximum() - slider.getMinimum())));

        float zeroY = trackArea.getBottom() - normZero * trackArea.getHeight();

        // Bipolar fill bar from zero baseline
        if (std::abs(sliderPos - zeroY) > 0.5f && isEnabled)
        {
            float fillY = juce::jmin(sliderPos, zeroY);
            float fillH = std::abs(sliderPos - zeroY);
            auto fillRect = juce::Rectangle<float>(trackArea.getX(), fillY, trackWidth, fillH);
            g.setColour(slider.isMouseOverOrDragging() ? accentColour : accentColour.withAlpha(0.85f));
            g.fillRoundedRectangle(fillRect, 2.5f);
        }

        // Horizontal thumb handle
        float handleWidth = 14.0f;
        float handleHeight = 3.5f;
        float handleX = bounds.getCentreX() - handleWidth * 0.5f;
        float handleY = juce::jlimit(trackArea.getY(), trackArea.getBottom() - handleHeight, sliderPos - handleHeight * 0.5f);

        g.setColour(isEnabled ? (slider.isMouseOverOrDragging() ? textMainColour : textMainColour.withAlpha(0.9f)) : textMutedColour.withAlpha(0.4f));
        g.fillRoundedRectangle(handleX, handleY, handleWidth, handleHeight, 1.5f);
        return;
    }

    juce::ignoreUnused(minSliderPos, maxSliderPos, style);

    // 1. Label 'VOLUME' on left edge
    auto labelArea = bounds.removeFromLeft(52.0f);
    g.setFont(getGeometricFont(10.0f, true));
    g.setColour(textMutedColour);
    g.drawText("VOLUME", labelArea, juce::Justification::left, true);

    // 2. Numeric dB readout on right edge
    auto readoutArea = bounds.removeFromRight(50.0f);
    float val = (float)slider.getValue();
    juce::String valStr;
    if (val <= -47.5f)
        valStr = "-inf";
    else if (val > 0.0f)
        valStr = "+" + juce::String(val, 1) + " dB";
    else
        valStr = juce::String(val, 1) + " dB";

    g.setFont(getMonospaceFont(10.0f));
    g.setColour(slider.isMouseOverOrDragging() ? accentColour : textMainColour);
    g.drawText(valStr, readoutArea, juce::Justification::right, true);

    // 3. Track area in center
    bounds.reduce(6.0f, 0.0f);
    float trackHeight = 6.0f;
    float trackY = bounds.getCentreY() - trackHeight * 0.5f;
    auto trackArea = juce::Rectangle<float>(bounds.getX(), trackY, bounds.getWidth(), trackHeight);

    // Dark charcoal / black background track
    g.setColour(graphBgColour);
    g.fillRoundedRectangle(trackArea, 3.0f);

    // Hairline border
    g.setColour(dividerColour);
    g.drawRoundedRectangle(trackArea, 3.0f, 1.0f);

    // Orange horizontal fill/progress bar
    float fillWidth = juce::jlimit(0.0f, trackArea.getWidth(), sliderPos - trackArea.getX());
    if (fillWidth > 0.0f)
    {
        auto fillRect = trackArea.withWidth(fillWidth);
        g.setColour(slider.isMouseOverOrDragging() ? accentColour : accentColour.withAlpha(0.85f));
        g.fillRoundedRectangle(fillRect, 3.0f);
    }

    // Small vertical tick handle/thumb
    float handleWidth = 3.0f;
    float handleHeight = trackHeight + 6.0f;
    float handleX = juce::jlimit(trackArea.getX(), trackArea.getRight() - handleWidth, sliderPos - handleWidth * 0.5f);
    float handleY = bounds.getCentreY() - handleHeight * 0.5f;

    g.setColour(slider.isMouseOverOrDragging() ? textMainColour : textMainColour.withAlpha(0.9f));
    g.fillRoundedRectangle(handleX, handleY, handleWidth, handleHeight, 1.5f);
}

void SpectralUILookAndFeel::drawButtonBackground(juce::Graphics& g, juce::Button& button,
                                                  const juce::Colour& backgroundColour,
                                                  bool shouldDrawButtonAsHighlighted,
                                                  bool shouldDrawButtonAsDown)
{
    juce::ignoreUnused(backgroundColour);
    auto bounds = button.getLocalBounds().toFloat();
    float cornerRadius = 4.0f;

    juce::Colour bg = juce::Colour::fromRGB(0x20, 0x21, 0x2B);
    if (button.getToggleState())
    {
        bg = accentColour.withAlpha(0.25f);
    }
    else if (shouldDrawButtonAsDown)
    {
        bg = juce::Colour::fromRGB(0x18, 0x19, 0x22);
    }
    else if (shouldDrawButtonAsHighlighted)
    {
        bg = juce::Colour::fromRGB(0x2A, 0x2C, 0x38);
    }

    g.setColour(bg);
    g.fillRoundedRectangle(bounds, cornerRadius);

    // Hairline border
    juce::Colour borderCol = button.getToggleState() ? accentColour : dividerColour;
    g.setColour(borderCol);
    g.drawRoundedRectangle(bounds, cornerRadius, 1.0f);
}

void SpectralUILookAndFeel::drawButtonText(juce::Graphics& g, juce::TextButton& button,
                                             bool shouldDrawButtonAsHighlighted,
                                             bool shouldDrawButtonAsDown)
{
    juce::ignoreUnused(shouldDrawButtonAsHighlighted, shouldDrawButtonAsDown);
    auto font = getTextButtonFont(button, button.getHeight());
    g.setFont(font);

    juce::Colour textCol = button.getToggleState() ? accentColour : textMainColour;
    if (!button.isEnabled())
        textCol = textMutedColour;

    g.setColour(textCol);
    g.drawText(button.getButtonText(), button.getLocalBounds(), juce::Justification::centred, false);
}

void SpectralUILookAndFeel::drawPopupMenuBackground(juce::Graphics& g, int width, int height)
{
    auto bounds = juce::Rectangle<float>(0, 0, (float)width, (float)height);
    g.setColour(panelBgColour);
    g.fillRoundedRectangle(bounds, 6.0f);

    g.setColour(dividerColour);
    g.drawRoundedRectangle(bounds, 6.0f, 1.0f);
}

void SpectralUILookAndFeel::drawPopupMenuItem(juce::Graphics& g, const juce::Rectangle<int>& area,
                                               bool isHighlighted, bool isHeader, bool isActive,
                                               bool isTicked, bool hasSubMenu, const juce::String& text,
                                               const juce::String& shortcutKeyText,
                                               const juce::Drawable* icon, const juce::Colour* textColour)
{
    juce::ignoreUnused(hasSubMenu, shortcutKeyText, icon, textColour);

    if (isHeader)
    {
        g.setFont(getGeometricFont(10.0f, true));
        g.setColour(textMutedColour);
        g.drawText(text.toUpperCase(), area.reduced(10, 0), juce::Justification::left, true);
        return;
    }

    auto r = area.toFloat().reduced(4.0f, 2.0f);

    if (isHighlighted && isActive)
    {
        g.setColour(accentColour.withAlpha(0.15f));
        g.fillRoundedRectangle(r, 4.0f);
    }

    g.setFont(getGeometricFont(12.0f, false));
    g.setColour(isHighlighted ? accentColour : textMainColour);

    auto textRect = r.reduced(8, 0);
    g.drawText(text, textRect, juce::Justification::left, true);

    if (isTicked)
    {
        g.setColour(accentColour);
        g.fillEllipse(r.getRight() - 14.0f, r.getCentreY() - 3.0f, 6.0f, 6.0f);
    }
}
