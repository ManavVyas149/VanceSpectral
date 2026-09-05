/*
  ==============================================================================

    EffectsPanel.cpp
    Created: 6 Sep 2026
    Author:  Manav Vyas / VanceSpectral Team

  ==============================================================================
*/

#include "EffectsPanel.h"

//==============================================================================
// EffectModuleComponent Implementation
//==============================================================================
EffectsPanel::EffectModuleComponent::EffectModuleComponent(
    const juce::String& name,
    const std::vector<juce::Colour>& segmentColours,
    juce::Colour glowColour)
    : effectName(name), colors(segmentColours), glow(glowColour)
{
    toggleButton.setClickingTogglesState(true);
    toggleButton.onClick = [this]() {
        if (onHoverChanged) onHoverChanged(this);
        repaint();
    };

    primarySlider.setSliderStyle(juce::Slider::LinearVertical);
    primarySlider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    primarySlider.setRange(0.0, 1.0, 0.01);
    primarySlider.onValueChange = [this]() {
        if (onValueChanged) onValueChanged((float)primarySlider.getValue());
        repaint();
    };

    secondarySlider.setSliderStyle(juce::Slider::LinearVertical);
    secondarySlider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);

    // Keep child components logically present but handled via direct module interaction
    addChildComponent(toggleButton);
    addChildComponent(primarySlider);
    addChildComponent(secondarySlider);
}

void EffectsPanel::EffectModuleComponent::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();
    bool enabled = toggleButton.getToggleState();

    // 1. LED Bar Area (Top portion)
    float barWidth = bounds.getWidth() - 4.0f;
    float barHeight = 12.0f;
    float barX = bounds.getX() + 2.0f;
    float barY = bounds.getY() + 2.0f;

    juce::Rectangle<float> ledBarRect(barX, barY, barWidth, barHeight);

    // Ambient glow emission when enabled
    if (enabled)
    {
        g.setColour(glow.withAlpha(0.32f));
        g.fillRoundedRectangle(ledBarRect.expanded(2.5f, 2.0f), 3.5f);
    }

    // Outer frame of LED bar
    g.setColour(enabled ? juce::Colour(0x1A, 0x1C, 0x24) : juce::Colour(0x12, 0x13, 0x18));
    g.fillRoundedRectangle(ledBarRect, 2.5f);
    g.setColour(enabled ? glow.withAlpha(0.65f) : juce::Colour(0x26, 0x28, 0x32));
    g.drawRoundedRectangle(ledBarRect, 2.5f, 0.8f);

    // 4 Discrete Segments
    const int numSegments = 4;
    const float gap = 1.5f;
    const float segWidth = (barWidth - 2.0f - (numSegments - 1) * gap) / (float)numSegments;
    const float segHeight = barHeight - 2.0f;

    for (int i = 0; i < numSegments; ++i)
    {
        float sx = barX + 1.0f + (float)i * (segWidth + gap);
        float sy = barY + 1.0f;
        juce::Rectangle<float> segRect(sx, sy, segWidth, segHeight);

        if (enabled)
        {
            juce::Colour segCol = (i < (int)colors.size()) ? colors[(size_t)i] : glow;
            juce::Colour topCol = segCol.brighter(0.25f);
            juce::Colour botCol = segCol.darker(0.15f);

            juce::ColourGradient grad(topCol, sx, sy, botCol, sx, sy + segHeight, false);
            g.setGradientFill(grad);
            g.fillRoundedRectangle(segRect, 1.5f);

            // Specular top highlight
            g.setColour(juce::Colours::white.withAlpha(0.38f));
            g.drawHorizontalLine((int)(sy + 1.0f), sx + 1.0f, sx + segWidth - 1.0f);
        }
        else
        {
            // Dimmed / unlit state: dark matte block with very faint ghost tint
            juce::Colour tintCol = (i < (int)colors.size()) ? colors[(size_t)i] : glow;
            g.setColour(juce::Colour(0x14, 0x16, 0x1D).interpolatedWith(tintCol, 0.08f));
            g.fillRoundedRectangle(segRect, 1.5f);

            g.setColour(juce::Colour(0x22, 0x25, 0x30));
            g.drawRoundedRectangle(segRect, 1.5f, 0.5f);
        }
    }

    // 2. Effect Name (Bottom portion)
    auto labelRect = bounds.withTrimmedTop(barHeight + 3.0f);
    g.setFont(SpectralUILookAndFeel::getMonospaceFont(9.0f, true));
    g.setColour(enabled ? juce::Colour(0xDD, 0xDB, 0xE2)
                        : (isHovered ? juce::Colour(0x90, 0x8E, 0x9A) : juce::Colour(0x56, 0x58, 0x64)));
    g.drawText(effectName, labelRect.toNearestInt(), juce::Justification::centred, false);
}

void EffectsPanel::EffectModuleComponent::mouseDown(const juce::MouseEvent& e)
{
    mouseDownPos = e.position;
    dragStartValue = (float)primarySlider.getValue();
    wasDragged = false;
    if (onHoverChanged) onHoverChanged(this);
}

void EffectsPanel::EffectModuleComponent::mouseDrag(const juce::MouseEvent& e)
{
    float dy = mouseDownPos.y - e.position.y;
    if (std::abs(dy) > 2.5f || wasDragged)
    {
        wasDragged = true;
        auto range = primarySlider.getRange();
        float delta = (float)(range.getLength() * (dy / 90.0f));
        float newVal = juce::jlimit((float)range.getStart(), (float)range.getEnd(), dragStartValue + delta);
        primarySlider.setValue(newVal, juce::sendNotification);
        if (onValueChanged) onValueChanged(newVal);
        repaint();
    }
}

void EffectsPanel::EffectModuleComponent::mouseUp(const juce::MouseEvent&)
{
    if (!wasDragged)
    {
        // Click toggles bypass state
        bool newState = !toggleButton.getToggleState();
        toggleButton.setToggleState(newState, juce::sendNotification);
        if (onHoverChanged) onHoverChanged(this);
        repaint();
    }
    wasDragged = false;
}

void EffectsPanel::EffectModuleComponent::mouseEnter(const juce::MouseEvent&)
{
    isHovered = true;
    if (onHoverChanged) onHoverChanged(this);
    repaint();
}

void EffectsPanel::EffectModuleComponent::mouseExit(const juce::MouseEvent&)
{
    isHovered = false;
    if (onHoverChanged) onHoverChanged(nullptr);
    repaint();
}

void EffectsPanel::EffectModuleComponent::mouseWheelMove(const juce::MouseEvent&, const juce::MouseWheelDetails& wheel)
{
    auto range = primarySlider.getRange();
    float delta = (float)(range.getLength() * wheel.deltaY * 0.05f);
    float newVal = juce::jlimit((float)range.getStart(), (float)range.getEnd(), (float)primarySlider.getValue() + delta);
    primarySlider.setValue(newVal, juce::sendNotification);
    if (onValueChanged) onValueChanged(newVal);
    repaint();
}

//==============================================================================
// 7-Segment Digital Character Drawing Helper
//==============================================================================
static void draw7SegmentDigit(juce::Graphics& g, juce::Rectangle<float> b, juce::juce_wchar c, juce::Colour onCol, juce::Colour offCol)
{
    // Segment mapping bits: 0:a, 1:b, 2:c, 3:d, 4:e, 5:f, 6:g
    uint8_t mask = 0;
    switch (c)
    {
        case '0': mask = 0b00111111; break;
        case '1': mask = 0b00000110; break;
        case '2': mask = 0b01011011; break;
        case '3': mask = 0b01001111; break;
        case '4': mask = 0b01100110; break;
        case '5': mask = 0b01101101; break;
        case '6': mask = 0b01111101; break;
        case '7': mask = 0b00000111; break;
        case '8': mask = 0b01111111; break;
        case '9': mask = 0b01101111; break;
        case '-': mask = 0b01000000; break;
        default:  mask = 0b00000000; break;
    }

    float w = b.getWidth();
    float h = b.getHeight();
    float t = juce::jmax(2.2f, h * 0.125f); // segment thickness
    float gap = t * 0.20f;
    float slant = w * 0.06f; // subtle digital slant

    float x = b.getX();
    float y = b.getY();
    float midY = y + h * 0.5f;

    auto drawHorizSeg = [&](bool on, float sx, float sy, float sw) {
        juce::Path p;
        p.startNewSubPath(sx + t * 0.5f + slant, sy);
        p.lineTo(sx + sw - t * 0.5f + slant, sy);
        p.lineTo(sx + sw + slant, sy + t * 0.5f);
        p.lineTo(sx + sw - t * 0.5f + slant, sy + t);
        p.lineTo(sx + t * 0.5f + slant, sy + t);
        p.lineTo(sx + slant, sy + t * 0.5f);
        p.closeSubPath();

        if (on)
        {
            g.setColour(onCol.withAlpha(0.35f));
            g.strokePath(p, juce::PathStrokeType(t * 0.75f));
            g.setColour(onCol);
            g.fillPath(p);
        }
        else
        {
            g.setColour(offCol);
            g.fillPath(p);
        }
    };

    auto drawVertSeg = [&](bool on, float sx, float sy, float sh) {
        juce::Path p;
        p.startNewSubPath(sx + slant, sy + t * 0.5f);
        p.lineTo(sx + t + slant, sy + t * 0.5f);
        p.lineTo(sx + t + slant, sy + sh - t * 0.5f);
        p.lineTo(sx + t * 0.5f + slant, sy + sh);
        p.lineTo(sx + slant, sy + sh - t * 0.5f);
        p.closeSubPath();

        if (on)
        {
            g.setColour(onCol.withAlpha(0.35f));
            g.strokePath(p, juce::PathStrokeType(t * 0.75f));
            g.setColour(onCol);
            g.fillPath(p);
        }
        else
        {
            g.setColour(offCol);
            g.fillPath(p);
        }
    };

    // a: Top
    drawHorizSeg((mask & (1 << 0)) != 0, x + gap, y, w - 2.0f * gap);
    // b: Top-Right
    drawVertSeg((mask & (1 << 1)) != 0, x + w - t, y + gap, midY - y - gap);
    // c: Bottom-Right
    drawVertSeg((mask & (1 << 2)) != 0, x + w - t, midY + gap, y + h - midY - gap);
    // d: Bottom
    drawHorizSeg((mask & (1 << 3)) != 0, x + gap, y + h - t, w - 2.0f * gap);
    // e: Bottom-Left
    drawVertSeg((mask & (1 << 4)) != 0, x, midY + gap, y + h - midY - gap);
    // f: Top-Left
    drawVertSeg((mask & (1 << 5)) != 0, x, y + gap, midY - y - gap);
    // g: Middle
    drawHorizSeg((mask & (1 << 6)) != 0, x + gap, midY - t * 0.5f, w - 2.0f * gap);
}

//==============================================================================
// EffectsPanel Implementation
//==============================================================================
EffectsPanel::EffectsPanel(juce::AudioProcessorValueTreeState& apvts)
    : driveModule("DRIVE",
                  { juce::Colour(0xFD, 0xBA, 0x74), juce::Colour(0xFB, 0x92, 0x3C),
                    juce::Colour(0xF9, 0x73, 0x16), juce::Colour(0xEA, 0x58, 0x0C) },
                  juce::Colour(0xF9, 0x73, 0x16)),
      phaserModule("PHASER",
                   { juce::Colour(0xFE, 0xF9, 0xC3), juce::Colour(0xFE, 0xF0, 0x8A),
                     juce::Colour(0xFA, 0xCC, 0x15), juce::Colour(0xCA, 0x8A, 0x04) },
                   juce::Colour(0xFA, 0xCC, 0x15)),
      delayModule("DELAY",
                  { juce::Colour(0xBB, 0xF7, 0xD0), juce::Colour(0x86, 0xEF, 0xAC),
                    juce::Colour(0x22, 0xC5, 0x5E), juce::Colour(0x16, 0xA3, 0x4A) },
                  juce::Colour(0x22, 0xC5, 0x5E)),
      chorusModule("CHORUS",
                   { juce::Colour(0xBF, 0xDB, 0xFE), juce::Colour(0x60, 0xA5, 0xFA),
                     juce::Colour(0x3B, 0x82, 0xF6), juce::Colour(0x1D, 0x4E, 0xD8) },
                   juce::Colour(0x3B, 0x82, 0xF6)),
      gateModule("GATE",
                 { juce::Colour(0xFB, 0xCF, 0xE8), juce::Colour(0xF4, 0x72, 0xB6),
                   juce::Colour(0xD9, 0x46, 0xEF), juce::Colour(0x86, 0x19, 0x8F) },
                 juce::Colour(0xD9, 0x46, 0xEF))
{
    auto setupModule = [this](EffectModuleComponent& mod) {
        addAndMakeVisible(mod);
        mod.onHoverChanged = [this](EffectModuleComponent* m) {
            focusedModule = m;
            updateDisplayedPercentage();
            repaint();
        };
        mod.onValueChanged = [this](float) {
            updateDisplayedPercentage();
            repaint();
        };
    };

    setupModule(driveModule);
    setupModule(phaserModule);
    setupModule(delayModule);
    setupModule(chorusModule);
    setupModule(gateModule);

    // Attachments to existing APVTS parameters
    driveToggleAttachment = std::make_unique<ButtonAttachment>(apvts, "FX_DRIVE_ENABLE", driveModule.toggleButton);
    driveAmountAttachment = std::make_unique<SliderAttachment>(apvts, "FX_DRIVE_AMOUNT", driveModule.primarySlider);
    driveToneAttachment = std::make_unique<SliderAttachment>(apvts, "FX_DRIVE_TONE", driveModule.secondarySlider);

    phaserToggleAttachment = std::make_unique<ButtonAttachment>(apvts, "FX_PHASER_ENABLE", phaserModule.toggleButton);
    phaserAmountAttachment = std::make_unique<SliderAttachment>(apvts, "FX_PHASER_AMOUNT", phaserModule.primarySlider);
    phaserRateAttachment = std::make_unique<SliderAttachment>(apvts, "FX_PHASER_RATE", phaserModule.secondarySlider);

    delayToggleAttachment = std::make_unique<ButtonAttachment>(apvts, "FX_DELAY_ENABLE", delayModule.toggleButton);
    delayAmountAttachment = std::make_unique<SliderAttachment>(apvts, "FX_DELAY_AMOUNT", delayModule.primarySlider);
    delayTimeAttachment = std::make_unique<SliderAttachment>(apvts, "FX_DELAY_TIME", delayModule.secondarySlider);

    chorusToggleAttachment = std::make_unique<ButtonAttachment>(apvts, "FX_CHORUS_ENABLE", chorusModule.toggleButton);
    chorusAmountAttachment = std::make_unique<SliderAttachment>(apvts, "FX_CHORUS_AMOUNT", chorusModule.primarySlider);
    chorusRateAttachment = std::make_unique<SliderAttachment>(apvts, "FX_CHORUS_RATE", chorusModule.secondarySlider);

    sidechainToggleAttachment = std::make_unique<ButtonAttachment>(apvts, "FX_SIDECHAIN_ENABLE", gateModule.toggleButton);
    sidechainMixAttachment = std::make_unique<SliderAttachment>(apvts, "FX_SIDECHAIN_MIX", gateModule.primarySlider);
    sidechainRateAttachment = std::make_unique<SliderAttachment>(apvts, "FX_SIDECHAIN_RATE", gateModule.secondarySlider);

    updateDisplayedPercentage();
}

EffectsPanel::~EffectsPanel()
{
    driveToggleAttachment.reset();
    driveAmountAttachment.reset();
    driveToneAttachment.reset();

    phaserToggleAttachment.reset();
    phaserAmountAttachment.reset();
    phaserRateAttachment.reset();

    delayToggleAttachment.reset();
    delayAmountAttachment.reset();
    delayTimeAttachment.reset();

    chorusToggleAttachment.reset();
    chorusAmountAttachment.reset();
    chorusRateAttachment.reset();

    sidechainToggleAttachment.reset();
    sidechainMixAttachment.reset();
    sidechainRateAttachment.reset();
}

void EffectsPanel::updateDisplayedPercentage()
{
    if (focusedModule != nullptr)
    {
        displayedPercentage = juce::jlimit(0, 100, (int)std::round(focusedModule->getAmount() * 100.0f));
    }
    else
    {
        // If Delay or other active effects are engaged, compute representative active amount
        float totalVal = 0.0f;
        int activeCount = 0;

        EffectModuleComponent* modules[] = { &driveModule, &phaserModule, &delayModule, &chorusModule, &gateModule };
        for (auto* m : modules)
        {
            if (m->isEffectEnabled())
            {
                totalVal += m->getAmount();
                activeCount++;
            }
        }

        if (activeCount > 0)
        {
            displayedPercentage = juce::jlimit(0, 100, (int)std::round((totalVal / (float)activeCount) * 100.0f));
        }
        else
        {
            // Default iconic reference value when at rest
            displayedPercentage = 33;
        }
    }
}

void EffectsPanel::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();

    // 1. Dark hardware rack panel chassis
    g.setColour(juce::Colour(0x0C, 0x0D, 0x11));
    g.fillRoundedRectangle(bounds, 6.0f);

    // Subtle dark-grey border
    g.setColour(juce::Colour(0x28, 0x2A, 0x34));
    g.drawRoundedRectangle(bounds, 6.0f, 1.2f);

    // Inset top specular highlight
    g.setColour(juce::Colour(0x3C, 0x40, 0x50).withAlpha(0.25f));
    g.drawHorizontalLine((int)(bounds.getY() + 1.0f), bounds.getX() + 6.0f, bounds.getRight() - 6.0f);

    // 2. Thin vertical divider separating effect controls from the digital percentage display
    if (dividerX > 0.0f)
    {
        g.setColour(juce::Colour(0x24, 0x26, 0x30));
        g.drawVerticalLine((int)dividerX, bounds.getY() + 8.0f, bounds.getBottom() - 8.0f);
    }

    // 3. Digital 7-Segment Percentage Display
    if (!digitalDisplayArea.isEmpty())
    {
        drawDigitalDisplay(g, digitalDisplayArea);
    }
}

void EffectsPanel::drawDigitalDisplay(juce::Graphics& g, juce::Rectangle<float> area)
{
    // Format digits string (e.g. "33", "100", " 5")
    int val = juce::jlimit(0, 100, displayedPercentage);
    juce::String valStr = juce::String(val);

    juce::Colour litGreen = juce::Colour(0x22, 0xC5, 0x5E);
    juce::Colour ghostGreen = juce::Colour(0x0A, 0x22, 0x12).withAlpha(0.35f);

    // Recessed background glow for display area
    g.setColour(litGreen.withAlpha(0.04f));
    g.fillRoundedRectangle(area.reduced(2.0f), 4.0f);

    float totalHeight = juce::jmin(44.0f, area.getHeight() - 16.0f);
    float digitWidth = totalHeight * 0.46f;
    float digitGap = digitWidth * 0.22f;
    float percentWidth = digitWidth * 0.85f;

    int numDigits = (val >= 100) ? 3 : 2;
    float blockWidth = (float)numDigits * digitWidth + (float)(numDigits - 1) * digitGap + digitGap * 1.5f + percentWidth;

    float startX = area.getRight() - blockWidth - 10.0f;
    float startY = area.getCentreY() - totalHeight * 0.5f;

    // Draw digits
    if (numDigits == 3)
    {
        draw7SegmentDigit(g, juce::Rectangle<float>(startX, startY, digitWidth, totalHeight),
                          valStr[0], litGreen, ghostGreen);
        draw7SegmentDigit(g, juce::Rectangle<float>(startX + digitWidth + digitGap, startY, digitWidth, totalHeight),
                          valStr[1], litGreen, ghostGreen);
        draw7SegmentDigit(g, juce::Rectangle<float>(startX + (digitWidth + digitGap) * 2.0f, startY, digitWidth, totalHeight),
                          valStr[2], litGreen, ghostGreen);
    }
    else
    {
        // 2 Digits
        juce::juce_wchar d1 = (valStr.length() >= 2) ? valStr[0] : (juce::juce_wchar)' ';
        juce::juce_wchar d2 = (valStr.length() >= 2) ? valStr[1] : valStr[0];

        draw7SegmentDigit(g, juce::Rectangle<float>(startX, startY, digitWidth, totalHeight),
                          d1, litGreen, ghostGreen);
        draw7SegmentDigit(g, juce::Rectangle<float>(startX + digitWidth + digitGap, startY, digitWidth, totalHeight),
                          d2, litGreen, ghostGreen);
    }

    // Draw '%' Symbol in digital style
    float px = startX + (float)numDigits * digitWidth + (float)(numDigits - 1) * digitGap + digitGap * 1.5f;
    float py = startY;
    float pw = percentWidth;
    float ph = totalHeight;
    float dotSize = juce::jmax(3.0f, ph * 0.14f);

    // Top dot
    g.setColour(litGreen);
    g.fillRect(px + 1.0f, py + 2.0f, dotSize, dotSize);

    // Diagonal slash
    g.setColour(litGreen.withAlpha(0.35f));
    g.drawLine(px + 2.0f, py + ph - 2.0f, px + pw - 2.0f, py + 2.0f, dotSize * 0.9f);
    g.setColour(litGreen);
    g.drawLine(px + 2.0f, py + ph - 2.0f, px + pw - 2.0f, py + 2.0f, dotSize * 0.65f);

    // Bottom dot
    g.fillRect(px + pw - dotSize - 1.0f, py + ph - dotSize - 2.0f, dotSize, dotSize);
}

void EffectsPanel::resized()
{
    auto area = getLocalBounds().toFloat().reduced(8.0f, 6.0f);
    if (area.getWidth() <= 0.0f || area.getHeight() <= 0.0f)
        return;

    // Right ~30% for Digital 7-Segment Percentage Display
    float rightWidth = juce::jmin(115.0f, area.getWidth() * 0.32f);
    digitalDisplayArea = area.removeFromRight(rightWidth);

    dividerX = digitalDisplayArea.getX() - 6.0f;
    area.removeFromRight(12.0f);

    // Left Effects Grid (DRIVE | PHASER | DELAY / CHORUS | GATE)
    float colWidth = area.getWidth() / 3.0f;
    float rowHeight = area.getHeight() / 2.0f;

    float moduleW = juce::jmin(62.0f, colWidth - 6.0f);
    float moduleH = juce::jmin(38.0f, rowHeight - 4.0f);

    // Col 1: DRIVE (top), CHORUS (bottom)
    auto col1 = area.removeFromLeft(colWidth);
    auto c1Top = col1.removeFromTop(rowHeight);
    driveModule.setBounds(c1Top.withSizeKeepingCentre(moduleW, moduleH).toNearestInt());
    chorusModule.setBounds(col1.withSizeKeepingCentre(moduleW, moduleH).toNearestInt());

    // Col 2: PHASER (top), GATE (bottom)
    auto col2 = area.removeFromLeft(colWidth);
    auto c2Top = col2.removeFromTop(rowHeight);
    phaserModule.setBounds(c2Top.withSizeKeepingCentre(moduleW, moduleH).toNearestInt());
    gateModule.setBounds(col2.withSizeKeepingCentre(moduleW, moduleH).toNearestInt());

    // Col 3: DELAY (vertically centered in column 3)
    delayModule.setBounds(area.withSizeKeepingCentre(moduleW, moduleH).toNearestInt());
}
