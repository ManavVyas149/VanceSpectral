#include "SpectralUILookAndFeel.h"

// =============================================================================
// Minimalist Translucent Frosted-White & Lavender Palette Constants
// =============================================================================
const juce::Colour SpectralUILookAndFeel::bgColour        = juce::Colour::fromRGB(0xF4, 0xF4, 0xF7); // #F4F4F7 Translucent Frosted-White Chassis
const juce::Colour SpectralUILookAndFeel::panelBgColour   = juce::Colour::fromRGB(0xFA, 0xFA, 0xFC); // #FAF9FC Frosted Coated Card Surface
const juce::Colour SpectralUILookAndFeel::graphBgColour   = juce::Colour::fromRGB(0x07, 0x08, 0x0B); // #07080B Deep Pitch-Black Display Panel
const juce::Colour SpectralUILookAndFeel::textMainColour  = juce::Colour::fromRGB(0x18, 0x19, 0x20); // #181920 Charcoal Technical Text
const juce::Colour SpectralUILookAndFeel::textMutedColour = juce::Colour::fromRGB(0x78, 0x7A, 0x86); // #787A86 Muted Technical Gray Labels
const juce::Colour SpectralUILookAndFeel::dividerColour   = juce::Colour::fromRGB(0xD4, 0xD6, 0xE0); // #D4D6E0 Hairline Card Border / Divider
const juce::Colour SpectralUILookAndFeel::accentColour    = juce::Colour::fromRGB(0xA7, 0x8B, 0xFA); // #A78BFA Restrained Lavender / Violet
const juce::Colour SpectralUILookAndFeel::accentBright    = juce::Colour::fromRGB(0xC4, 0xB5, 0xFD); // #C4B5FD Soft Lilac Highlight
const juce::Colour SpectralUILookAndFeel::knobBodyColour  = juce::Colour::fromRGB(0xDF, 0xE1, 0xEA); // #DFE1EA Machined Light-Silver Cylinder
const juce::Colour SpectralUILookAndFeel::knobInsetColour = juce::Colour::fromRGB(0x12, 0x13, 0x18); // #121318 Inner Dark Disc Face

SpectralUILookAndFeel::SpectralUILookAndFeel()
{
    setColour(juce::ResizableWindow::backgroundColourId, bgColour);
    setColour(juce::Label::textColourId, textMainColour);
    setColour(juce::PopupMenu::backgroundColourId, panelBgColour);
    setColour(juce::PopupMenu::headerTextColourId, textMutedColour);
    setColour(juce::PopupMenu::textColourId, textMainColour);
    setColour(juce::PopupMenu::highlightedBackgroundColourId, accentColour.withAlpha(0.15f));
    setColour(juce::PopupMenu::highlightedTextColourId, textMainColour);

    setColour(juce::ScrollBar::thumbColourId, accentColour.withAlpha(0.50f));
    setColour(juce::ScrollBar::trackColourId, juce::Colours::transparentBlack);
    setColour(juce::ScrollBar::backgroundColourId, juce::Colours::transparentBlack);
}

#include "BinaryFontData.h"

static juce::Typeface::Ptr getJetBrainsMonoBoldTypeface()
{
    static juce::Typeface::Ptr tf = juce::Typeface::createSystemTypefaceFor(
        BinaryData::JetBrainsMonoBold_ttf,
        (size_t)BinaryData::JetBrainsMonoBold_ttfSize);
    return tf;
}

static juce::Typeface::Ptr getJetBrainsMonoRegularTypeface()
{
    static juce::Typeface::Ptr tf = juce::Typeface::createSystemTypefaceFor(
        BinaryData::JetBrainsMonoRegular_ttf,
        (size_t)BinaryData::JetBrainsMonoRegular_ttfSize);
    return tf;
}

static juce::Typeface::Ptr getSpaceGroteskTypeface()
{
    static juce::Typeface::Ptr tf = juce::Typeface::createSystemTypefaceFor(
        BinaryData::SpaceGrotesk_ttf,
        (size_t)BinaryData::SpaceGrotesk_ttfSize);
    return tf;
}

juce::Font SpectralUILookAndFeel::getJetBrainsMono(float height, bool bold)
{
    auto tf = bold ? getJetBrainsMonoBoldTypeface() : getJetBrainsMonoRegularTypeface();
    if (tf != nullptr)
        return juce::Font(juce::FontOptions(tf).withHeight(height));

    juce::FontOptions options(juce::Font::getDefaultMonospacedFontName(), height, bold ? juce::Font::bold : juce::Font::plain);
    return juce::Font(options);
}

juce::Font SpectralUILookAndFeel::getSpaceGrotesk(float height, bool bold)
{
    auto tf = getSpaceGroteskTypeface();
    if (tf != nullptr)
    {
        auto font = juce::Font(juce::FontOptions(tf).withHeight(height));
        if (bold) font.setBold(true);
        return font;
    }
    juce::FontOptions options("Segoe UI", height, bold ? juce::Font::bold : juce::Font::plain);
    return juce::Font(options);
}

juce::Font SpectralUILookAndFeel::getGeometricFont(float height, bool bold)
{
    return getSpaceGrotesk(height, bold);
}

juce::Font SpectralUILookAndFeel::getMonospaceFont(float height, bool bold)
{
    return getJetBrainsMono(height, bold);
}

juce::Font SpectralUILookAndFeel::getLabelFont(juce::Label&)
{
    return getSpaceGrotesk(10.0f, false);
}

juce::Font SpectralUILookAndFeel::getTextButtonFont(juce::TextButton&, int buttonHeight)
{
    return getJetBrainsMono(juce::jmin(12.2f, (float)buttonHeight * 0.50f), true);
}

// =============================================================================
// Static Precision Drawing Helpers
// =============================================================================

void SpectralUILookAndFeel::drawCornerScrew(juce::Graphics& g, float cx, float cy, float radius, float slotAngleRad)
{
    // Ambient soft drop shadow
    g.setColour(juce::Colour(0x00, 0x00, 0x00).withAlpha(0.12f));
    g.fillEllipse(cx - radius, cy - radius + 1.0f, radius * 2.0f, radius * 2.0f);

    // Metallic screw head disc (machined brushed aluminum gradient)
    juce::ColourGradient grad(juce::Colour::fromRGB(0xEB, 0xEC, 0xF2), cx - radius * 0.5f, cy - radius * 0.5f,
                              juce::Colour::fromRGB(0xB8, 0xBC, 0xC8), cx + radius * 0.5f, cy + radius * 0.5f, false);
    g.setGradientFill(grad);
    g.fillEllipse(cx - radius, cy - radius, radius * 2.0f, radius * 2.0f);

    // Subtle 1px rim highlight
    g.setColour(juce::Colour::fromRGB(0xFF, 0xFF, 0xFF).withAlpha(0.75f));
    g.drawEllipse(cx - radius, cy - radius, radius * 2.0f, radius * 2.0f, 0.8f);

    // Beveled screw slot
    juce::Path slot;
    float slotHalfLen = radius * 0.68f;
    float slotHalfW = 0.80f;
    slot.addRectangle(-slotHalfLen, -slotHalfW, slotHalfLen * 2.0f, slotHalfW * 2.0f);
    slot.applyTransform(juce::AffineTransform::rotation(slotAngleRad).translated(cx, cy));

    g.setColour(juce::Colour::fromRGB(0x38, 0x3A, 0x44));
    g.fillPath(slot);

    // Slot 1-sided micro highlight
    juce::Path slotHi;
    slotHi.addRectangle(-slotHalfLen, -slotHalfW - 0.4f, slotHalfLen * 2.0f, 0.5f);
    slotHi.applyTransform(juce::AffineTransform::rotation(slotAngleRad).translated(cx, cy));
    g.setColour(juce::Colour::fromRGB(0xFF, 0xFF, 0xFF).withAlpha(0.40f));
    g.fillPath(slotHi);
}

void SpectralUILookAndFeel::drawPcbTraces(juce::Graphics& g, juce::Rectangle<float> bounds)
{
    // Sophisticated PCB circuit traces underneath translucent chassis
    float w = bounds.getWidth();
    float h = bounds.getHeight();
    float x0 = bounds.getX();
    float y0 = bounds.getY();

    // Subtle silvery-lavender traces
    juce::Colour traceCol = juce::Colour(0x8A, 0x8E, 0xA5).withAlpha(0.14f);
    juce::Colour accentTraceCol = accentColour.withAlpha(0.10f);

    juce::Path p;
    // Trace Line 1 (Top Left Header Bus)
    p.startNewSubPath(x0 + 15.0f, y0 + 55.0f);
    p.lineTo(x0 + 110.0f, y0 + 55.0f);
    p.lineTo(x0 + 145.0f, y0 + 90.0f);
    p.lineTo(x0 + 260.0f, y0 + 90.0f);

    // Trace Line 2 (Top Left Secondary Run)
    p.startNewSubPath(x0 + 15.0f, y0 + 68.0f);
    p.lineTo(x0 + 95.0f, y0 + 68.0f);
    p.lineTo(x0 + 130.0f, y0 + 103.0f);
    p.lineTo(x0 + 220.0f, y0 + 103.0f);

    // Trace Line 3 (Top Right Bus)
    p.startNewSubPath(x0 + w - 30.0f, y0 + 42.0f);
    p.lineTo(x0 + w - 170.0f, y0 + 42.0f);
    p.lineTo(x0 + w - 210.0f, y0 + 82.0f);
    p.lineTo(x0 + w - 330.0f, y0 + 82.0f);

    // Trace Line 4 (Top Right Parallel Run)
    p.startNewSubPath(x0 + w - 30.0f, y0 + 54.0f);
    p.lineTo(x0 + w - 155.0f, y0 + 54.0f);
    p.lineTo(x0 + w - 195.0f, y0 + 94.0f);
    p.lineTo(x0 + w - 290.0f, y0 + 94.0f);

    // Trace Line 5 (Bottom Left Traces)
    p.startNewSubPath(x0 + 35.0f, y0 + h - 45.0f);
    p.lineTo(x0 + 160.0f, y0 + h - 45.0f);
    p.lineTo(x0 + 200.0f, y0 + h - 85.0f);
    p.lineTo(x0 + 340.0f, y0 + h - 85.0f);

    // Trace Line 6 (Bottom Center-Right Bus)
    p.startNewSubPath(x0 + w - 45.0f, y0 + h - 40.0f);
    p.lineTo(x0 + w - 220.0f, y0 + h - 40.0f);
    p.lineTo(x0 + w - 260.0f, y0 + h - 80.0f);
    p.lineTo(x0 + w - 380.0f, y0 + h - 80.0f);

    g.setColour(traceCol);
    g.strokePath(p, juce::PathStrokeType(1.1f));

    // Accent traces
    juce::Path pAcc;
    pAcc.startNewSubPath(x0 + w * 0.5f - 40.0f, y0 + h - 18.0f);
    pAcc.lineTo(x0 + w * 0.5f - 40.0f, y0 + h - 6.0f);
    pAcc.startNewSubPath(x0 + w * 0.5f - 20.0f, y0 + h - 18.0f);
    pAcc.lineTo(x0 + w * 0.5f - 20.0f, y0 + h - 6.0f);
    pAcc.startNewSubPath(x0 + w * 0.5f, y0 + h - 18.0f);
    pAcc.lineTo(x0 + w * 0.5f, y0 + h - 6.0f);
    pAcc.startNewSubPath(x0 + w * 0.5f + 20.0f, y0 + h - 18.0f);
    pAcc.lineTo(x0 + w * 0.5f + 20.0f, y0 + h - 6.0f);
    pAcc.startNewSubPath(x0 + w * 0.5f + 40.0f, y0 + h - 18.0f);
    pAcc.lineTo(x0 + w * 0.5f + 40.0f, y0 + h - 6.0f);

    g.setColour(accentTraceCol);
    g.strokePath(pAcc, juce::PathStrokeType(1.4f));

    // Circular Vias / Solder Test Points
    auto drawVia = [&](float vx, float vy) {
        g.setColour(traceCol);
        g.drawEllipse(vx - 3.5f, vy - 3.5f, 7.0f, 7.0f, 0.9f);
        g.setColour(accentColour.withAlpha(0.25f));
        g.fillEllipse(vx - 1.5f, vy - 1.5f, 3.0f, 3.0f);
    };

    drawVia(x0 + 260.0f, y0 + 90.0f);
    drawVia(x0 + 220.0f, y0 + 103.0f);
    drawVia(x0 + w - 330.0f, y0 + 82.0f);
    drawVia(x0 + w - 290.0f, y0 + 94.0f);
    drawVia(x0 + 340.0f, y0 + h - 85.0f);
    drawVia(x0 + w - 380.0f, y0 + h - 80.0f);
}

void SpectralUILookAndFeel::drawChassisBackground(juce::Graphics& g, juce::Rectangle<float> bounds)
{
    // 1. Outer frosted translucent white chassis fill
    g.setColour(bgColour);
    g.fillRoundedRectangle(bounds, 10.0f);

    // 2. Subtle diagonal glass reflection sheen
    juce::ColourGradient glassSheen(juce::Colour(0xFF, 0xFF, 0xFF).withAlpha(0.40f), bounds.getX(), bounds.getY(),
                                    juce::Colour(0xFF, 0xFF, 0xFF).withAlpha(0.04f), bounds.getRight(), bounds.getBottom(), false);
    g.setGradientFill(glassSheen);
    g.fillRoundedRectangle(bounds, 10.0f);

    // 3. Crisp hairline chassis border
    g.setColour(dividerColour);
    g.drawRoundedRectangle(bounds, 10.0f, 1.0f);

    // 4. Subtle inner highlight line along top & left
    g.setColour(juce::Colour(0xFF, 0xFF, 0xFF).withAlpha(0.80f));
    g.drawRoundedRectangle(bounds.reduced(1.0f), 9.0f, 0.8f);

    // 5. Embedded PCB traces
    drawPcbTraces(g, bounds);

    // 6. Corner Mounting Screws (top 2 precision slotted screws; bottom screws removed for clean chassis)
    constexpr float screwInset = 16.0f;
    constexpr float screwRadius = 5.0f;

    drawCornerScrew(g, bounds.getX() + screwInset, bounds.getY() + screwInset, screwRadius, 0.65f);
    drawCornerScrew(g, bounds.getRight() - screwInset, bounds.getY() + screwInset, screwRadius, 2.10f);
}

void SpectralUILookAndFeel::drawPanelCard(juce::Graphics& g, juce::Rectangle<float> bounds,
                                          const juce::String& headerText,
                                          const juce::String& subheaderText)
{
    if (bounds.getWidth() <= 0 || bounds.getHeight() <= 0)
        return;

    // Subtle ambient card drop shadow
    g.setColour(juce::Colour(0x00, 0x00, 0x00).withAlpha(0.04f));
    g.fillRoundedRectangle(bounds.translated(0.0f, 1.0f), 5.0f);

    // Card frosted surface fill
    g.setColour(panelBgColour);
    g.fillRoundedRectangle(bounds, 5.0f);

    // Glass gloss diagonal sheen
    juce::ColourGradient sheen(juce::Colour(0xFF, 0xFF, 0xFF).withAlpha(0.50f), bounds.getX(), bounds.getY(),
                               juce::Colour(0xFF, 0xFF, 0xFF).withAlpha(0.05f), bounds.getRight(), bounds.getBottom(), false);
    g.setGradientFill(sheen);
    g.fillRoundedRectangle(bounds, 5.0f);

    // Crisp hairline border
    g.setColour(dividerColour);
    g.drawRoundedRectangle(bounds, 5.0f, 1.0f);

    // Header Label & Secondary Subheader
    if (headerText.isNotEmpty())
    {
        auto headerArea = bounds.removeFromTop(20.0f).reduced(8.0f, 2.0f);

        // Small section indicator dot
        float dotY = headerArea.getCentreY();
        g.setColour(accentColour.withAlpha(0.70f));
        g.fillEllipse(headerArea.getX(), dotY - 2.0f, 4.0f, 4.0f);

        auto textRect = headerArea.withTrimmedLeft(8);
        g.setFont(getJetBrainsMono(11.8f, true));
        g.setColour(textMainColour);
        g.drawText(headerText.toUpperCase(), textRect, juce::Justification::left, true);

        if (subheaderText.isNotEmpty())
        {
            g.setFont(getMonospaceFont(8.5f));
            g.setColour(accentColour);
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
    bool isHot = slider.isMouseOverOrDragging();

    // 1. Soft Ambient Drop Shadow under knob
    g.setColour(juce::Colour(0x00, 0x00, 0x00).withAlpha(0.10f));
    g.fillEllipse(knobRect.translated(0.0f, 1.5f));

    // 2. Precision Calibration Tick Marks around perimeter
    {
        constexpr int numTicks = 11;
        float tickOuterR = radius + 2.5f;
        float tickInnerR = radius + 0.8f;
        for (int i = 0; i < numTicks; ++i)
        {
            float t = (float)i / (float)(numTicks - 1);
            float a = rotaryStartAngle + t * (rotaryEndAngle - rotaryStartAngle);
            float cosA = std::cos(a - juce::MathConstants<float>::halfPi);
            float sinA = std::sin(a - juce::MathConstants<float>::halfPi);

            bool isPastValue = (t <= sliderPos);
            g.setColour(isPastValue ? accentColour.withAlpha(isHot ? 0.9f : 0.6f)
                                    : dividerColour.withAlpha(0.6f));
            g.drawLine(centreX + cosA * tickInnerR, centreY + sinA * tickInnerR,
                       centreX + cosA * tickOuterR, centreY + sinA * tickOuterR, 0.8f);
        }
    }

    // 3. Machined Light-Silver Outer Cylinder Body
    juce::ColourGradient knobGrad(juce::Colour::fromRGB(0xEB, 0xED, 0xF4), centreX - radius * 0.3f, centreY - radius * 0.3f,
                                  juce::Colour::fromRGB(0xD0, 0xD4, 0xDF), centreX + radius, centreY + radius, true);
    g.setGradientFill(knobGrad);
    g.fillEllipse(knobRect);

    // 4. Subtle Outer Bevel Rim
    g.setColour(juce::Colour::fromRGB(0xBC, 0xC0, 0xCE));
    g.drawEllipse(knobRect, 0.9f);

    // 5. Deep Recessed Matte Dark Face Disc
    auto insetRect = knobRect.reduced(3.2f);
    g.setColour(knobInsetColour);
    g.fillEllipse(insetRect);

    // Inset rim shadow
    g.setColour(juce::Colour(0x00, 0x00, 0x00).withAlpha(0.35f));
    g.drawEllipse(insetRect, 0.8f);

    // 6. Active Lavender Arc Track (subtle glowing arc on the dark face)
    if (sliderPos > 0.001f)
    {
        float arcRadius = insetRect.getWidth() * 0.5f - 1.0f;
        juce::Path trackActive;
        trackActive.addCentredArc(centreX, centreY, arcRadius, arcRadius, 0.0f, rotaryStartAngle, angle, true);

        g.setColour(isHot ? accentBright : accentColour);
        g.strokePath(trackActive, juce::PathStrokeType(1.6f));

        if (isHot)
        {
            g.setColour(accentColour.withAlpha(0.30f));
            g.strokePath(trackActive, juce::PathStrokeType(3.2f));
        }
    }

    // 7. Center Pointer Line (Ultra-Crisp Lavender Technical Indicator)
    float pointerLength = insetRect.getWidth() * 0.40f;
    juce::Path p;
    p.addRoundedRectangle(-0.80f, -pointerLength, 1.6f, pointerLength, 0.4f);
    p.applyTransform(juce::AffineTransform::rotation(angle).translated(centreX, centreY));

    g.setColour(isHot ? accentBright : accentColour);
    g.fillPath(p);
}

juce::Slider::SliderLayout SpectralUILookAndFeel::getSliderLayout(juce::Slider& slider)
{
    if (slider.getName() == "VOLUME" ||
        (slider.getSliderStyle() == juce::Slider::LinearHorizontal && slider.getTextBoxPosition() == juce::Slider::NoTextBox && slider.getWidth() >= 140))
    {
        juce::Slider::SliderLayout layout;
        auto bounds = slider.getLocalBounds();
        if (bounds.getWidth() > 110)
        {
            bounds.removeFromLeft(50);
            bounds.removeFromRight(56);
            bounds.reduce(6, 0);
        }
        layout.sliderBounds = bounds;
        return layout;
    }
    return juce::LookAndFeel_V4::getSliderLayout(slider);
}

void SpectralUILookAndFeel::drawCornerResizer(juce::Graphics& g, int w, int h, bool isMouseOver, bool isMouseDragging)
{
    auto colour = (isMouseOver || isMouseDragging) ? accentColour : textMutedColour.withAlpha(0.6f);
    g.setColour(colour);

    float stroke = 1.2f;
    g.drawLine((float)w - 11.0f, (float)h - 3.0f, (float)w - 3.0f, (float)h - 11.0f, stroke);
    g.drawLine((float)w - 7.5f,  (float)h - 3.0f, (float)w - 3.0f, (float)h - 7.5f, stroke);
    g.drawLine((float)w - 4.0f,  (float)h - 3.0f, (float)w - 3.0f, (float)h - 4.0f, stroke);
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
    bounds = juce::Rectangle<float>((float)x, (float)y, (float)width, (float)height);
    bool isVolumeSlider = (slider.getName() == "VOLUME" ||
                           (slider.getSliderStyle() == juce::Slider::LinearHorizontal && slider.getTextBoxPosition() == juce::Slider::NoTextBox && slider.getWidth() >= 140));

    if (isVolumeSlider)
    {
        // Horizontal Volume Fader (Bottom Bar)
        auto compBounds = slider.getLocalBounds().toFloat();
        if (compBounds.getWidth() > 110.0f)
        {
            // 1. Label 'VOLUME' on left edge
            auto labelArea = compBounds.removeFromLeft(50.0f);
            g.setFont(getJetBrainsMono(9.0f, true));
            g.setColour(textMutedColour);
            g.drawText("VOLUME", labelArea, juce::Justification::centredLeft, true);

            // 2. Numeric dB readout on right edge
            auto readoutArea = compBounds.removeFromRight(56.0f);
            float val = (float)slider.getValue();
            juce::String valStr;
            if (val <= -47.5f)
                valStr = "-inf";
            else if (val > 0.0f)
                valStr = "+" + juce::String(val, 1) + " dB";
            else
                valStr = juce::String(val, 1) + " dB";

            g.setFont(getJetBrainsMono(9.0f, true));
            g.setColour(slider.isMouseOverOrDragging() ? accentColour : textMainColour);
            g.drawText(valStr, readoutArea, juce::Justification::centredRight, true);
        }

        // 3. Track area in center (matches sliderBounds passed via x, y, width, height)
        if (width <= 4)
            return;

        float trackHeight = 5.0f;
        float trackY = (float)y + ((float)height - trackHeight) * 0.5f;
        auto trackArea = juce::Rectangle<float>((float)x, trackY, (float)width, trackHeight);

        // Dark track background
        g.setColour(graphBgColour);
        g.fillRoundedRectangle(trackArea, 2.5f);

        // Hairline border
        g.setColour(dividerColour);
        g.drawRoundedRectangle(trackArea, 2.5f, 1.0f);

        // Horizontal progress fill (sliderPos directly maps from x at min to x + width at +6 dB)
        float fillWidth = juce::jlimit(0.0f, trackArea.getWidth(), sliderPos - trackArea.getX());
        if (fillWidth > 0.0f && slider.isEnabled())
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
        float handleY = (float)y + ((float)height - handleHeight) * 0.5f;

        g.setColour(slider.isEnabled() ? (slider.isMouseOverOrDragging() ? accentBright : textMainColour) : textMutedColour.withAlpha(0.4f));
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
    float cornerRadius = 3.5f;

    juce::Colour bg = panelBgColour;
    if (button.getToggleState())
    {
        bg = accentColour.withAlpha(0.18f);
    }
    else if (shouldDrawButtonAsDown)
    {
        bg = juce::Colour::fromRGB(0xEA, 0xEC, 0xF4);
    }
    else if (shouldDrawButtonAsHighlighted)
    {
        bg = juce::Colour::fromRGB(0xF2, 0xF4, 0xF9);
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

void SpectralUILookAndFeel::drawScrollbar(juce::Graphics& g, juce::ScrollBar& scrollbar,
                                          int x, int y, int width, int height,
                                          bool isScrollbarVertical,
                                          int thumbStartPosition, int thumbSize,
                                          bool isMouseOver, bool isMouseDown)
{
    juce::ignoreUnused(scrollbar);

    if (thumbSize <= 0)
        return;

    juce::Rectangle<float> thumbBounds;
    if (isScrollbarVertical)
    {
        float w = juce::jmin(4.0f, (float)width);
        float thumbX = (float)x + ((float)width - w) * 0.5f;
        thumbBounds = juce::Rectangle<float>(thumbX, (float)thumbStartPosition, w, (float)thumbSize);
    }
    else
    {
        float h = juce::jmin(4.0f, (float)height);
        float thumbY = (float)y + ((float)height - h) * 0.5f;
        thumbBounds = juce::Rectangle<float>((float)thumbStartPosition, thumbY, (float)thumbSize, h);
    }

    juce::Colour col = accentColour.withAlpha(0.55f);
    if (isMouseDown)
        col = accentBright;
    else if (isMouseOver)
        col = accentColour.withAlpha(0.90f);

    g.setColour(col);
    g.fillRoundedRectangle(thumbBounds, 2.0f);
}
