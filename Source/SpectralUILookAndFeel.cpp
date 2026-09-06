#include "SpectralUILookAndFeel.h"

// =============================================================================
// Brutalist Precision Instrument Palette Constants
// =============================================================================
const juce::Colour SpectralUILookAndFeel::bgColour        = juce::Colour::fromRGB(0xEC, 0xEB, 0xE4); // #ECEBE4 Warm Light Cream Chassis
const juce::Colour SpectralUILookAndFeel::panelBgColour   = juce::Colour::fromRGB(0xF7, 0xF6, 0xF0); // #F7F6F0 Glossy Coated Card Surface
const juce::Colour SpectralUILookAndFeel::graphBgColour   = juce::Colour::fromRGB(0x0C, 0x0D, 0x12); // #0C0D12 Deep Black Display Panel
const juce::Colour SpectralUILookAndFeel::textMainColour  = juce::Colour::fromRGB(0x1E, 0x1F, 0x24); // #1E1F24 Charcoal Technical Text
const juce::Colour SpectralUILookAndFeel::textMutedColour = juce::Colour::fromRGB(0x7A, 0x78, 0x74); // #7A7874 Warm Gray Label Text
const juce::Colour SpectralUILookAndFeel::dividerColour   = juce::Colour::fromRGB(0xD0, 0xCC, 0xBE); // #D0CCBE Hairline Card Border
const juce::Colour SpectralUILookAndFeel::accentColour    = juce::Colour::fromRGB(0xB8, 0x4D, 0xC4); // #B84DC4 Desaturated 'Burple' (Blue-Purple / Violet)
const juce::Colour SpectralUILookAndFeel::accentBright    = juce::Colour::fromRGB(0xF2, 0xB8, 0xFF); // #F2B8FF Bright Lilac Highlight
const juce::Colour SpectralUILookAndFeel::knobBodyColour  = juce::Colour::fromRGB(0x1E, 0x20, 0x28); // #1E2028 Precision Dark Knob Cylinder
const juce::Colour SpectralUILookAndFeel::knobInsetColour = juce::Colour::fromRGB(0x15, 0x16, 0x1E); // #15161E Inner Dark Disc Face

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

juce::Font SpectralUILookAndFeel::getMonospaceFont(float height, bool bold)
{
    juce::FontOptions options("Consolas", height, bold ? juce::Font::bold : juce::Font::plain);
    return juce::Font(options);
}

juce::Font SpectralUILookAndFeel::getLabelFont(juce::Label&)
{
    return getMonospaceFont(10.5f);
}

juce::Font SpectralUILookAndFeel::getTextButtonFont(juce::TextButton&, int buttonHeight)
{
    return getGeometricFont(juce::jmin(12.0f, (float)buttonHeight * 0.45f), true);
}

// =============================================================================
// Static Brutalist Precision Drawing Helpers
// =============================================================================

void SpectralUILookAndFeel::drawCornerScrew(juce::Graphics& g, float cx, float cy, float radius, float slotAngleRad)
{
    // Ambient soft drop shadow
    g.setColour(juce::Colour(0x00, 0x00, 0x00).withAlpha(0.14f));
    g.fillEllipse(cx - radius, cy - radius + 1.2f, radius * 2.0f, radius * 2.0f);

    // Metallic screw head disc (brushed steel gradient)
    juce::ColourGradient grad(juce::Colour::fromRGB(0xDA, 0xD8, 0xCF), cx - radius * 0.5f, cy - radius * 0.5f,
                              juce::Colour::fromRGB(0xB0, 0xAC, 0xA0), cx + radius * 0.5f, cy + radius * 0.5f, false);
    g.setGradientFill(grad);
    g.fillEllipse(cx - radius, cy - radius, radius * 2.0f, radius * 2.0f);

    // Subtle 1px rim highlight
    g.setColour(juce::Colour::fromRGB(0xFF, 0xFF, 0xFF).withAlpha(0.65f));
    g.drawEllipse(cx - radius, cy - radius, radius * 2.0f, radius * 2.0f, 0.8f);

    // Beveled screw slot
    juce::Path slot;
    float slotHalfLen = radius * 0.70f;
    float slotHalfW = 0.85f;
    slot.addRectangle(-slotHalfLen, -slotHalfW, slotHalfLen * 2.0f, slotHalfW * 2.0f);
    slot.applyTransform(juce::AffineTransform::rotation(slotAngleRad).translated(cx, cy));

    g.setColour(juce::Colour::fromRGB(0x4A, 0x47, 0x40));
    g.fillPath(slot);

    // Slot 1-sided micro highlight
    juce::Path slotHi;
    slotHi.addRectangle(-slotHalfLen, -slotHalfW - 0.5f, slotHalfLen * 2.0f, 0.6f);
    slotHi.applyTransform(juce::AffineTransform::rotation(slotAngleRad).translated(cx, cy));
    g.setColour(juce::Colour::fromRGB(0xFF, 0xFF, 0xFF).withAlpha(0.35f));
    g.fillPath(slotHi);
}

void SpectralUILookAndFeel::drawPcbTraces(juce::Graphics& g, juce::Rectangle<float> bounds)
{
    // Faint embedded PCB circuit traces (low opacity: 3.5% alpha)
    juce::Colour traceCol = juce::Colour(0x35, 0x2A, 0x42).withAlpha(0.038f);
    g.setColour(traceCol);

    float w = bounds.getWidth();
    float h = bounds.getHeight();
    float x0 = bounds.getX();
    float y0 = bounds.getY();

    juce::Path p;
    // Trace line 1 (Top left trace)
    p.startNewSubPath(x0 + 20.0f, y0 + 60.0f);
    p.lineTo(x0 + 120.0f, y0 + 60.0f);
    p.lineTo(x0 + 160.0f, y0 + 100.0f);
    p.lineTo(x0 + 280.0f, y0 + 100.0f);

    // Trace line 2 (Top right bus traces)
    p.startNewSubPath(x0 + w - 40.0f, y0 + 40.0f);
    p.lineTo(x0 + w - 180.0f, y0 + 40.0f);
    p.lineTo(x0 + w - 220.0f, y0 + 80.0f);
    p.lineTo(x0 + w - 340.0f, y0 + 80.0f);

    // Trace line 3 (Bottom left traces)
    p.startNewSubPath(x0 + 40.0f, y0 + h - 50.0f);
    p.lineTo(x0 + 180.0f, y0 + h - 50.0f);
    p.lineTo(x0 + 220.0f, y0 + h - 90.0f);
    p.lineTo(x0 + 360.0f, y0 + h - 90.0f);

    // Trace line 4 (Bottom right traces)
    p.startNewSubPath(x0 + w - 60.0f, y0 + h - 45.0f);
    p.lineTo(x0 + w - 240.0f, y0 + h - 45.0f);
    p.lineTo(x0 + w - 280.0f, y0 + h - 85.0f);
    p.lineTo(x0 + w - 400.0f, y0 + h - 85.0f);

    g.strokePath(p, juce::PathStrokeType(1.2f));

    // Circular Vias / Test Points
    auto drawVia = [&](float vx, float vy) {
        g.drawEllipse(vx - 3.5f, vy - 3.5f, 7.0f, 7.0f, 1.0f);
        g.fillEllipse(vx - 1.5f, vy - 1.5f, 3.0f, 3.0f);
    };

    drawVia(x0 + 280.0f, y0 + 100.0f);
    drawVia(x0 + w - 340.0f, y0 + 80.0f);
    drawVia(x0 + 360.0f, y0 + h - 90.0f);
    drawVia(x0 + w - 400.0f, y0 + h - 85.0f);
}

void SpectralUILookAndFeel::drawChassisBackground(juce::Graphics& g, juce::Rectangle<float> bounds)
{
    // Outer chassis fill in warm cream
    g.setColour(bgColour);
    g.fillRoundedRectangle(bounds, 12.0f);

    // Subtle 1px chassis border
    g.setColour(dividerColour);
    g.drawRoundedRectangle(bounds, 12.0f, 1.0f);

    // Embedded PCB traces
    drawPcbTraces(g, bounds);

    // 4 Corner Screws (slotted precision metal screws)
    constexpr float screwInset = 16.0f;
    constexpr float screwRadius = 5.2f;

    drawCornerScrew(g, bounds.getX() + screwInset, bounds.getY() + screwInset, screwRadius, 0.65f);
    drawCornerScrew(g, bounds.getRight() - screwInset, bounds.getY() + screwInset, screwRadius, 2.10f);
    drawCornerScrew(g, bounds.getX() + screwInset, bounds.getBottom() - screwInset, screwRadius, 1.45f);
    drawCornerScrew(g, bounds.getRight() - screwInset, bounds.getBottom() - screwInset, screwRadius, 0.35f);
}

void SpectralUILookAndFeel::drawPanelCard(juce::Graphics& g, juce::Rectangle<float> bounds,
                                          const juce::String& headerText,
                                          const juce::String& subheaderText)
{
    if (bounds.getWidth() <= 0 || bounds.getHeight() <= 0)
        return;

    // Subtle ambient card drop shadow (low-opacity)
    g.setColour(juce::Colour(0x00, 0x00, 0x00).withAlpha(0.05f));
    g.fillRoundedRectangle(bounds.translated(0.0f, 1.5f), 6.0f);

    // Card background fill
    g.setColour(panelBgColour);
    g.fillRoundedRectangle(bounds, 6.0f);

    // Glossy diagonal highlight sheen (coated precision surface)
    juce::ColourGradient sheen(juce::Colour(0xFF, 0xFF, 0xFF).withAlpha(0.32f), bounds.getX(), bounds.getY(),
                               juce::Colour(0xFF, 0xFF, 0xFF).withAlpha(0.02f), bounds.getRight(), bounds.getBottom(), false);
    g.setGradientFill(sheen);
    g.fillRoundedRectangle(bounds, 6.0f);

    // Crisp hairline border
    g.setColour(dividerColour);
    g.drawRoundedRectangle(bounds, 6.0f, 1.0f);

    // Header Label & Secondary Subheader
    if (headerText.isNotEmpty())
    {
        auto headerArea = bounds.removeFromTop(20.0f).reduced(10.0f, 4.0f);

        g.setFont(getMonospaceFont(10.0f));
        g.setColour(textMainColour);
        g.drawText(headerText.toUpperCase(), headerArea, juce::Justification::left, true);

        if (subheaderText.isNotEmpty())
        {
            g.setFont(getMonospaceFont(8.5f));
            g.setColour(accentColour.withAlpha(0.85f));
            g.drawText(subheaderText.toUpperCase(), headerArea, juce::Justification::right, true);
        }
    }
}

// =============================================================================
// LookAndFeel Component Draw Overrides
// =============================================================================

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

    // 1. Soft Ambient Drop Shadow under knob
    g.setColour(juce::Colour(0x00, 0x00, 0x00).withAlpha(0.12f));
    g.fillEllipse(knobRect.translated(0.0f, 1.8f));

    // 2. Knob Base Cylinder (Dark Metallic Radial Gradient)
    juce::ColourGradient knobGrad(juce::Colour::fromRGB(0x28, 0x2A, 0x34), centreX - radius * 0.3f, centreY - radius * 0.3f,
                                  knobBodyColour, centreX + radius, centreY + radius, true);
    g.setGradientFill(knobGrad);
    g.fillEllipse(knobRect);

    // 3. Beveled Rim Ring
    g.setColour(juce::Colour::fromRGB(0x38, 0x3B, 0x48));
    g.drawEllipse(knobRect, 1.0f);

    // 4. Inner Dark Face Disc
    auto insetRect = knobRect.reduced(3.0f);
    g.setColour(knobInsetColour);
    g.fillEllipse(insetRect);

    // 5. Inactive Background Track Arc
    float arcRadius = radius - 1.2f;
    juce::Path trackBg;
    trackBg.addCentredArc(centreX, centreY, arcRadius, arcRadius, 0.0f, rotaryStartAngle, rotaryEndAngle, true);
    g.setColour(juce::Colour::fromRGB(0x2E, 0x30, 0x3C));
    g.strokePath(trackBg, juce::PathStrokeType(1.4f));

    // 6. Active 'Burple' Arc Ring Indicator
    if (sliderPos > 0.001f)
    {
        juce::Path trackActive;
        trackActive.addCentredArc(centreX, centreY, arcRadius, arcRadius, 0.0f, rotaryStartAngle, angle, true);

        bool isHot = slider.isMouseOverOrDragging();
        g.setColour(isHot ? accentBright : accentColour);
        g.strokePath(trackActive, juce::PathStrokeType(1.8f));

        // Subtle glow when hovering
        if (isHot)
        {
            g.setColour(accentColour.withAlpha(0.35f));
            g.strokePath(trackActive, juce::PathStrokeType(3.5f));
        }
    }

    // 7. Center Pointer Line (Crisp Technical Indicator)
    float pointerLength = insetRect.getWidth() * 0.36f;
    juce::Path p;
    p.addRoundedRectangle(-0.85f, -pointerLength, 1.7f, pointerLength, 0.5f);
    p.applyTransform(juce::AffineTransform::rotation(angle).translated(centreX, centreY));

    g.setColour(slider.isMouseOverOrDragging() ? accentBright : juce::Colour::fromRGB(0xEE, 0xEA, 0xF5));
    g.fillPath(p);
}

void SpectralUILookAndFeel::drawLinearSlider(juce::Graphics& g, int x, int y, int width, int height,
                                              float sliderPos, float minSliderPos, float maxSliderPos,
                                              const juce::Slider::SliderStyle style, juce::Slider& slider)
{
    if (width <= 0 || height <= 0)
        return;

    auto bounds = juce::Rectangle<int>(x, y, width, height).toFloat();
    bool isEnabled = slider.isEnabled();

    if (style == juce::Slider::LinearVertical || slider.isVertical())
    {
        if (bounds.getHeight() < 12.0f)
            return;

        // 1. Numeric Readout at top in monospace
        float val = (float)slider.getValue();
        int stVal = (int)std::round(val);
        juce::String valStr = (stVal > 0 ? "+" : "") + juce::String(stVal) + " st";

        auto topArea = bounds.removeFromTop(12.0f);
        g.setFont(getMonospaceFont(9.5f));
        g.setColour(isEnabled ? (slider.isMouseOverOrDragging() ? accentColour : textMainColour) : textMutedColour);
        g.drawText(valStr, topArea, juce::Justification::centred, true);

        // 2. Track area in middle
        bounds.reduce(2.0f, 2.0f);
        if (bounds.getHeight() <= 2.0f)
            return;

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

        // Bipolar fill bar in burple from zero baseline
        if (std::abs(sliderPos - zeroY) > 0.5f && isEnabled)
        {
            float fillY = juce::jmin(sliderPos, zeroY);
            float fillH = std::abs(sliderPos - zeroY);
            auto fillRect = juce::Rectangle<float>(trackArea.getX(), fillY, trackWidth, fillH);
            g.setColour(slider.isMouseOverOrDragging() ? accentBright : accentColour);
            g.fillRoundedRectangle(fillRect, 2.5f);
        }

        // Horizontal thumb handle
        float handleWidth = juce::jmin(14.0f, bounds.getWidth());
        float handleHeight = 3.5f;
        float handleX = bounds.getCentreX() - handleWidth * 0.5f;
        float minHandleY = trackArea.getY();
        float maxHandleY = juce::jmax(minHandleY, trackArea.getBottom() - handleHeight);
        float handleY = juce::jlimit(minHandleY, maxHandleY, sliderPos - handleHeight * 0.5f);

        g.setColour(isEnabled ? (slider.isMouseOverOrDragging() ? accentBright : textMainColour) : textMutedColour.withAlpha(0.4f));
        g.fillRoundedRectangle(handleX, handleY, handleWidth, handleHeight, 1.5f);
        return;
    }

    juce::ignoreUnused(minSliderPos, maxSliderPos, style);

    // Horizontal Sliders
    bool isVolumeSlider = (slider.getName() == "VOLUME" ||
                           (slider.getTextBoxPosition() == juce::Slider::NoTextBox && bounds.getWidth() >= 140.0f));

    if (isVolumeSlider)
    {
        // Horizontal Volume Fader (Bottom Bar)
        // 1. Label 'VOLUME' on left edge
        if (bounds.getWidth() > 110.0f)
        {
            auto labelArea = bounds.removeFromLeft(52.0f);
            g.setFont(getMonospaceFont(9.5f));
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

            g.setFont(getMonospaceFont(9.5f));
            g.setColour(slider.isMouseOverOrDragging() ? accentColour : textMainColour);
            g.drawText(valStr, readoutArea, juce::Justification::right, true);
        }

        // 3. Track area in center
        bounds.reduce(4.0f, 0.0f);
        if (bounds.getWidth() <= 4.0f)
            return;

        float trackHeight = 5.0f;
        float trackY = bounds.getCentreY() - trackHeight * 0.5f;
        auto trackArea = juce::Rectangle<float>(bounds.getX(), trackY, bounds.getWidth(), trackHeight);

        // Dark track background
        g.setColour(graphBgColour);
        g.fillRoundedRectangle(trackArea, 2.5f);

        // Hairline border
        g.setColour(dividerColour);
        g.drawRoundedRectangle(trackArea, 2.5f, 1.0f);

        // Horizontal progress fill
        float fillWidth = juce::jlimit(0.0f, trackArea.getWidth(), sliderPos - trackArea.getX());
        if (fillWidth > 0.0f && isEnabled)
        {
            auto fillRect = trackArea.withWidth(fillWidth);
            g.setColour(slider.isMouseOverOrDragging() ? accentBright : accentColour);
            g.fillRoundedRectangle(fillRect, 2.5f);
        }

        // Precision vertical thumb tick
        float handleWidth = 3.5f;
        float handleHeight = trackHeight + 6.0f;
        float minHandleX = trackArea.getX();
        float maxHandleX = juce::jmax(minHandleX, trackArea.getRight() - handleWidth);
        float handleX = juce::jlimit(minHandleX, maxHandleX, sliderPos - handleWidth * 0.5f);
        float handleY = bounds.getCentreY() - handleHeight * 0.5f;

        g.setColour(isEnabled ? (slider.isMouseOverOrDragging() ? accentBright : textMainColour) : textMutedColour.withAlpha(0.4f));
        g.fillRoundedRectangle(handleX, handleY, handleWidth, handleHeight, 1.5f);
    }
    else
    {
        // Standard Linear Horizontal Slider (e.g. popovers, parameters, dialogs)
        bounds.reduce(2.0f, 0.0f);
        if (bounds.getWidth() <= 4.0f)
            return;

        float trackHeight = 4.0f;
        float trackY = bounds.getCentreY() - trackHeight * 0.5f;
        auto trackArea = juce::Rectangle<float>(bounds.getX(), trackY, bounds.getWidth(), trackHeight);

        // Dark track background
        g.setColour(graphBgColour);
        g.fillRoundedRectangle(trackArea, 2.0f);

        // Hairline border
        g.setColour(dividerColour);
        g.drawRoundedRectangle(trackArea, 2.0f, 0.8f);

        // Progress fill
        float fillWidth = juce::jlimit(0.0f, trackArea.getWidth(), sliderPos - trackArea.getX());
        if (fillWidth > 0.0f && isEnabled)
        {
            auto fillRect = trackArea.withWidth(fillWidth);
            g.setColour(slider.isMouseOverOrDragging() ? accentBright : accentColour);
            g.fillRoundedRectangle(fillRect, 2.0f);
        }

        // Vertical thumb tick
        float handleWidth = 3.0f;
        float handleHeight = trackHeight + 6.0f;
        float minHandleX = trackArea.getX();
        float maxHandleX = juce::jmax(minHandleX, trackArea.getRight() - handleWidth);
        float handleX = juce::jlimit(minHandleX, maxHandleX, sliderPos - handleWidth * 0.5f);
        float handleY = bounds.getCentreY() - handleHeight * 0.5f;

        g.setColour(isEnabled ? (slider.isMouseOverOrDragging() ? accentBright : textMainColour) : textMutedColour.withAlpha(0.4f));
        g.fillRoundedRectangle(handleX, handleY, handleWidth, handleHeight, 1.0f);
    }
}

void SpectralUILookAndFeel::drawButtonBackground(juce::Graphics& g, juce::Button& button,
                                                  const juce::Colour& backgroundColour,
                                                  bool shouldDrawButtonAsHighlighted,
                                                  bool shouldDrawButtonAsDown)
{
    juce::ignoreUnused(backgroundColour);
    auto bounds = button.getLocalBounds().toFloat();
    float cornerRadius = 4.0f;

    juce::Colour bg = juce::Colour::fromRGB(0x20, 0x22, 0x2A);
    if (button.getToggleState())
    {
        bg = accentColour;
    }
    else if (shouldDrawButtonAsDown)
    {
        bg = juce::Colour::fromRGB(0x16, 0x18, 0x20);
    }
    else if (shouldDrawButtonAsHighlighted)
    {
        bg = juce::Colour::fromRGB(0x2C, 0x2E, 0x3A);
    }

    g.setColour(bg);
    g.fillRoundedRectangle(bounds, cornerRadius);

    // Hairline border
    juce::Colour borderCol = button.getToggleState() ? accentBright : dividerColour;
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

    juce::Colour textCol = button.getToggleState() ? juce::Colours::black : juce::Colour::fromRGB(0xEE, 0xEA, 0xF5);
    if (!button.isEnabled())
        textCol = textMutedColour.withAlpha(0.4f);

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
        g.setFont(getMonospaceFont(9.5f));
        g.setColour(textMutedColour);
        g.drawText(text.toUpperCase(), area.reduced(10, 0), juce::Justification::left, true);
        return;
    }

    auto r = area.toFloat().reduced(4.0f, 2.0f);

    if (isHighlighted && isActive)
    {
        g.setColour(accentColour.withAlpha(0.18f));
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
