/*
  ==============================================================================

    EffectsPanel.cpp
    Created: 6 Sep 2026
    Author:  Manav Vyas / VanceSpectral Team

  ==============================================================================
*/

#include "EffectsPanel.h"

//==============================================================================
// Note Division Utilities
//==============================================================================
const juce::StringArray& EffectsPanel::getNoteDivisionNames()
{
    static const juce::StringArray names{
        "1/16", "1/8T", "1/8", "1/8D", "1/4", "1/4D", "1/2"
    };
    return names;
}

double EffectsPanel::getNoteDivisionFactor(int index)
{
    switch (index)
    {
        case 0: return 0.25;         // 1/16
        case 1: return 1.0 / 3.0;    // 1/8 Triplet
        case 2: return 0.5;          // 1/8 (Default)
        case 3: return 0.75;         // 1/8 Dotted
        case 4: return 1.0;          // 1/4
        case 5: return 1.5;          // 1/4 Dotted
        case 6: return 2.0;          // 1/2
        default: return 0.5;
    }
}

//==============================================================================
// EffectZoneComponent Implementation
//==============================================================================
EffectsPanel::EffectZoneComponent::EffectZoneComponent(
    const EffectZoneConfig& cfg,
    juce::AudioProcessorValueTreeState& apvtsRef,
    std::function<double()> bpmProviderFn,
    std::function<void(int)> onSelectReqFn,
    std::function<void(int, const juce::String&)> onParamAdjustedFn)
    : config(cfg),
      apvts(apvtsRef),
      bpmProvider(bpmProviderFn),
      onSelectRequest(onSelectReqFn),
      onParameterAdjusted(onParamAdjustedFn)
{
    setRepaintsOnMouseActivity(true);
}

void EffectsPanel::EffectZoneComponent::setSelected(bool selected)
{
    if (isExpanded != selected)
    {
        isExpanded = selected;
        repaint();
    }
}

void EffectsPanel::EffectZoneComponent::updateReadout(const juce::String& text)
{
    readoutText = text;
    readoutAlpha = 1.0f;
    readoutHoldFrames = 90; // ~1.5s hold at 60 Hz
    repaint();
}

void EffectsPanel::EffectZoneComponent::tickAnimation()
{
    bool needsRepaint = false;

    // Smooth expansion interpolation
    float targetExp = isExpanded ? 1.0f : 0.0f;
    if (std::abs(expansionProgress - targetExp) > 0.005f)
    {
        expansionProgress += (targetExp - expansionProgress) * 0.25f;
        needsRepaint = true;
    }
    else
    {
        expansionProgress = targetExp;
    }

    // Temporary digital readout hold & fade
    if (readoutHoldFrames > 0)
    {
        --readoutHoldFrames;
    }
    else if (readoutAlpha > 0.0f)
    {
        readoutAlpha = juce::jmax(0.0f, readoutAlpha - 0.045f);
        needsRepaint = true;
    }

    if (needsRepaint)
        repaint();
}

bool EffectsPanel::EffectZoneComponent::isEffectEnabled() const
{
    if (auto* param = apvts.getRawParameterValue(config.enableParamID))
        return *param >= 0.5f;
    return false;
}

float EffectsPanel::EffectZoneComponent::getNormalizedParamValue(const juce::String& paramID) const
{
    if (auto* param = apvts.getParameter(paramID))
        return param->getValue();
    return 0.0f;
}

void EffectsPanel::EffectZoneComponent::setNormalizedParamValue(const juce::String& paramID, float normVal)
{
    if (auto* param = apvts.getParameter(paramID))
    {
        param->setValueNotifyingHost(juce::jlimit(0.0f, 1.0f, normVal));
    }
}

void EffectsPanel::EffectZoneComponent::syncFromAPVTS()
{
    repaint();
}

void EffectsPanel::EffectZoneComponent::syncDelayTimeFromDivision()
{
    double bpm = 120.0;
    if (bpmProvider)
    {
        double b = bpmProvider();
        if (b >= 20.0 && b <= 400.0)
            bpm = b;
    }

    double quarterMs = 60000.0 / bpm;
    double factor = getNoteDivisionFactor(delayDivisionIdx);
    float ms = (float)juce::jlimit(10.0, 1000.0, quarterMs * factor);

    if (auto* param = apvts.getParameter("FX_DELAY_TIME"))
    {
        float normVal = param->getNormalisableRange().convertTo0to1(ms);
        param->setValueNotifyingHost(normVal);
    }
}

// Layout coordinate helpers
juce::Rectangle<float> EffectsPanel::EffectZoneComponent::getHeaderBounds() const
{
    return juce::Rectangle<float>(2.0f, 3.0f, (float)getWidth() - 4.0f, 20.0f);
}

juce::Rectangle<float> EffectsPanel::EffectZoneComponent::getMixBarBounds() const
{
    return juce::Rectangle<float>(4.0f, 25.0f, (float)getWidth() - 8.0f, 14.0f);
}

juce::Rectangle<float> EffectsPanel::EffectZoneComponent::getSecondaryControlsBounds() const
{
    float topY = 42.0f;
    float botY = (float)getHeight() - 26.0f;
    return juce::Rectangle<float>(4.0f, topY, (float)getWidth() - 8.0f, juce::jmax(0.0f, botY - topY));
}

juce::Rectangle<float> EffectsPanel::EffectZoneComponent::getReadoutBounds() const
{
    return juce::Rectangle<float>(4.0f, (float)getHeight() - 24.0f, (float)getWidth() - 8.0f, 20.0f);
}

juce::Rectangle<float> EffectsPanel::EffectZoneComponent::getBypassToggleBounds() const
{
    auto area = getSecondaryControlsBounds();
    if (config.index == 3) // DELAY: shares top row with Sync mode
    {
        return juce::Rectangle<float>(area.getX(), area.getY() + 2.0f, (area.getWidth() - 3.0f) * 0.48f, 16.0f);
    }
    return juce::Rectangle<float>(area.getX() + 2.0f, area.getY() + 2.0f, area.getWidth() - 4.0f, 16.0f);
}

juce::Rectangle<float> EffectsPanel::EffectZoneComponent::getDelaySyncToggleBounds() const
{
    auto area = getSecondaryControlsBounds();
    float x = area.getX() + (area.getWidth() - 3.0f) * 0.52f;
    float w = area.getRight() - x;
    return juce::Rectangle<float>(x, area.getY() + 2.0f, w, 16.0f);
}

juce::Rectangle<float> EffectsPanel::EffectZoneComponent::getDelayDivisionBounds() const
{
    auto area = getSecondaryControlsBounds();
    return juce::Rectangle<float>(area.getX() + 2.0f, area.getY() + 22.0f, area.getWidth() - 4.0f, 18.0f);
}

void EffectsPanel::EffectZoneComponent::resized()
{
    auto area = getSecondaryControlsBounds();

    if (config.index == 3) // DELAY
    {
        // Secondary knob 1: Time (when free)
        float knobSize = 30.0f;
        secondaryKnob1.bounds = juce::Rectangle<float>(area.getCentreX() - knobSize * 0.5f, area.getY() + 22.0f, knobSize, knobSize);
        secondaryKnob1.label = "TIME";
        secondaryKnob1.paramID = "FX_DELAY_TIME";

        // Secondary knob 2: Feedback
        secondaryKnob2.bounds = juce::Rectangle<float>(area.getCentreX() - knobSize * 0.5f, area.getY() + 56.0f, knobSize, knobSize);
        secondaryKnob2.label = "FDBK";
        secondaryKnob2.paramID = "FX_DELAY_FEEDBACK";
    }
    else
    {
        float knobSize = 34.0f;
        secondaryKnob1.bounds = juce::Rectangle<float>(area.getCentreX() - knobSize * 0.5f, area.getY() + 24.0f, knobSize, knobSize);
        secondaryKnob1.paramID = config.secParam1ID;

        if (config.index == 0) secondaryKnob1.label = "TONE";
        else secondaryKnob1.label = "RATE";
    }
}

static void drawMiniArcKnob(juce::Graphics& g, juce::Rectangle<float> b, const juce::String& label, float value01, juce::Colour color)
{
    float centreX = b.getCentreX();
    float centreY = b.getY() + b.getHeight() * 0.42f;
    float radius = juce::jmin(b.getWidth(), b.getHeight()) * 0.38f;

    // Track arc
    float startAngle = juce::degreesToRadians(140.0f);
    float endAngle = juce::degreesToRadians(400.0f);
    float currentAngle = startAngle + value01 * (endAngle - startAngle);

    juce::Path trackPath;
    trackPath.addCentredArc(centreX, centreY, radius, radius, 0.0f, startAngle, endAngle, true);
    g.setColour(juce::Colour(0x18, 0x1A, 0x24));
    g.strokePath(trackPath, juce::PathStrokeType(2.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    // Illuminated value arc
    if (value01 > 0.01f)
    {
        juce::Path fillPath;
        fillPath.addCentredArc(centreX, centreY, radius, radius, 0.0f, startAngle, currentAngle, true);
        g.setColour(color);
        g.strokePath(fillPath, juce::PathStrokeType(2.2f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }

    // Center indicator dot
    g.setColour(juce::Colour(0x0C, 0x0E, 0x14));
    g.fillEllipse(centreX - radius * 0.5f, centreY - radius * 0.5f, radius, radius);
    g.setColour(color.brighter(0.4f));
    float ptrX = centreX + radius * 0.6f * std::sin(currentAngle);
    float ptrY = centreY - radius * 0.6f * std::cos(currentAngle);
    g.fillEllipse(ptrX - 1.2f, ptrY - 1.2f, 2.4f, 2.4f);

    // Monospace Label below knob
    g.setFont(SpectralUILookAndFeel::getMonospaceFont(7.8f, false));
    g.setColour(juce::Colour(0x7A, 0x7E, 0x8E));
    juce::Rectangle<float> lblRect(b.getX(), b.getBottom() - 11.0f, b.getWidth(), 11.0f);
    g.drawText(label, lblRect.toNearestInt(), juce::Justification::centred, false);
}

void EffectsPanel::EffectZoneComponent::paint(juce::Graphics& g)
{
    bool enabled = isEffectEnabled();
    auto totalBounds = getLocalBounds().toFloat();

    // 1. Subtle selection ambient wash in dark LCD
    if (isExpanded || expansionProgress > 0.02f)
    {
        g.setColour(config.lcdColor.withAlpha(0.06f * expansionProgress));
        g.fillRect(totalBounds.reduced(1.0f, 2.0f));
    }

    // 2. Header Section
    auto headerRect = getHeaderBounds();
    if (isHeaderHovered)
    {
        g.setColour(config.lcdColor.withAlpha(0.12f));
        g.fillRoundedRectangle(headerRect, 2.5f);
    }

    // Color-coded Indicator LED Dot
    float ledX = headerRect.getX() + 4.0f;
    float ledY = headerRect.getCentreY() - 2.5f;
    float ledSize = 5.0f;
    juce::Rectangle<float> ledRect(ledX, ledY, ledSize, ledSize);

    if (enabled)
    {
        if (isExpanded)
        {
            g.setColour(config.lcdColor.withAlpha(0.40f));
            g.fillEllipse(ledRect.expanded(2.0f));
            g.setColour(config.lcdColor.brighter(0.4f));
            g.fillEllipse(ledRect);
        }
        else
        {
            g.setColour(config.lcdColor.withAlpha(0.85f));
            g.fillEllipse(ledRect);
        }
    }
    else
    {
        g.setColour(juce::Colour(0x22, 0x24, 0x2E));
        g.fillEllipse(ledRect);
        g.setColour(juce::Colour(0x36, 0x3A, 0x48));
        g.drawEllipse(ledRect, 0.5f);
    }

    // Effect Name (understated monospace)
    g.setFont(SpectralUILookAndFeel::getMonospaceFont(9.2f, true));
    if (isExpanded)
        g.setColour(config.lcdColor.brighter(0.5f));
    else if (enabled)
        g.setColour(isHeaderHovered ? juce::Colour(0xDD, 0xDB, 0xE4) : juce::Colour(0x9E, 0x9E, 0xAC));
    else
        g.setColour(isHeaderHovered ? juce::Colour(0x8A, 0x8E, 0x9A) : juce::Colour(0x56, 0x58, 0x64));

    juce::Rectangle<float> nameRect(ledX + ledSize + 4.0f, headerRect.getY(), headerRect.getWidth() - ledSize - 8.0f, headerRect.getHeight());
    g.drawText(config.name, nameRect.toNearestInt(), juce::Justification::centredLeft, false);

    // 3. Primary Control — Horizontal Mix Bar
    auto mixArea = getMixBarBounds();
    float trackHeight = 5.5f;
    float trackY = mixArea.getCentreY() - trackHeight * 0.5f;
    float trackX = mixArea.getX() + 2.0f;
    float trackW = mixArea.getWidth() - 4.0f;
    juce::Rectangle<float> trackRect(trackX, trackY, trackW, trackHeight);

    // Track channel (low opacity / dark LCD slot)
    g.setColour(juce::Colour(0x06, 0x07, 0x0B));
    g.fillRoundedRectangle(trackRect, 2.0f);
    g.setColour(isMixBarHovered ? config.lcdColor.withAlpha(0.35f) : juce::Colour(0x18, 0x1A, 0x24));
    g.drawRoundedRectangle(trackRect, 2.0f, 0.7f);

    float mixNorm = getNormalizedParamValue(config.mixParamID);
    float fillW = juce::jlimit(0.0f, trackW, mixNorm * trackW);

    if (fillW > 0.5f)
    {
        juce::Rectangle<float> fillRect(trackX, trackY, fillW, trackHeight);

        juce::Colour botCol = config.lcdColor.darker(0.6f);
        juce::Colour topCol = config.lcdColor;
        if (!enabled)
        {
            botCol = botCol.withMultipliedSaturation(0.25f).withAlpha(0.20f);
            topCol = topCol.withMultipliedSaturation(0.25f).withAlpha(0.30f);
        }

        juce::ColourGradient grad(botCol, trackX, trackY, topCol, trackX + fillW, trackY, false);
        g.setGradientFill(grad);
        g.fillRoundedRectangle(fillRect, 2.0f);

        // Indicator endpoint mark
        if (enabled)
        {
            float markX = trackX + fillW - 1.5f;
            g.setColour(config.lcdColor.brighter(0.5f));
            g.fillRoundedRectangle(markX, trackY - 1.0f, 2.0f, trackHeight + 2.0f, 1.0f);
        }
    }

    // 4. In-Place Secondary Controls (Only rendered when expanded)
    if (expansionProgress > 0.05f)
    {
        juce::Graphics::ScopedSaveState ss(g);
        g.setOpacity(expansionProgress);

        // (a) Bypass Button
        auto bypassRect = getBypassToggleBounds();
        g.setColour(enabled ? config.lcdColor.withAlpha(0.20f) : juce::Colour(0x12, 0x13, 0x1A));
        g.fillRoundedRectangle(bypassRect, 2.5f);
        g.setColour(enabled ? config.lcdColor.withAlpha(0.60f) : juce::Colour(0x28, 0x2A, 0x36));
        g.drawRoundedRectangle(bypassRect, 2.5f, 0.7f);

        g.setFont(SpectralUILookAndFeel::getMonospaceFont(8.0f, true));
        g.setColour(enabled ? config.lcdColor.brighter(0.4f) : juce::Colour(0x60, 0x64, 0x72));
        g.drawText(enabled ? "ACTIVE" : "BYPASS", bypassRect.toNearestInt(), juce::Justification::centred, false);

        // (b) Effect specific secondary controls
        if (config.index == 3) // DELAY
        {
            // Sync toggle button
            auto syncRect = getDelaySyncToggleBounds();
            g.setColour(delayIsSynced ? config.lcdColor.withAlpha(0.20f) : juce::Colour(0x12, 0x13, 0x1A));
            g.fillRoundedRectangle(syncRect, 2.5f);
            g.setColour(delayIsSynced ? config.lcdColor.withAlpha(0.60f) : juce::Colour(0x28, 0x2A, 0x36));
            g.drawRoundedRectangle(syncRect, 2.5f, 0.7f);

            g.setFont(SpectralUILookAndFeel::getMonospaceFont(8.0f, true));
            g.setColour(delayIsSynced ? config.lcdColor.brighter(0.4f) : juce::Colour(0x60, 0x64, 0x72));
            g.drawText(delayIsSynced ? "SYNC" : "FREE", syncRect.toNearestInt(), juce::Justification::centred, false);

            if (delayIsSynced)
            {
                // Note division selector pill
                auto divRect = getDelayDivisionBounds();
                g.setColour(juce::Colour(0x0C, 0x0E, 0x14));
                g.fillRoundedRectangle(divRect, 2.5f);
                g.setColour(config.lcdColor.withAlpha(0.45f));
                g.drawRoundedRectangle(divRect, 2.5f, 0.7f);

                g.setFont(SpectralUILookAndFeel::getMonospaceFont(8.5f, true));
                g.setColour(config.lcdColor.brighter(0.3f));
                const auto& names = getNoteDivisionNames();
                juce::String divName = (delayDivisionIdx >= 0 && delayDivisionIdx < names.size()) ? names[delayDivisionIdx] : "1/8";
                g.drawText("DIV: " + divName, divRect.toNearestInt(), juce::Justification::centred, false);
            }
            else
            {
                // Draw Time Knob
                auto kb = secondaryKnob1.bounds;
                float val01 = getNormalizedParamValue(secondaryKnob1.paramID);
                drawMiniArcKnob(g, kb, secondaryKnob1.label, val01, config.lcdColor);
            }

            // Draw Feedback Knob
            auto kb2 = secondaryKnob2.bounds;
            float val01_fb = getNormalizedParamValue(secondaryKnob2.paramID);
            drawMiniArcKnob(g, kb2, secondaryKnob2.label, val01_fb, config.lcdColor);
        }
        else
        {
            // Standard Mini Arc Knob (Tone / Rate)
            auto kb = secondaryKnob1.bounds;
            float val01 = getNormalizedParamValue(secondaryKnob1.paramID);
            drawMiniArcKnob(g, kb, secondaryKnob1.label, val01, config.lcdColor);
        }
    }

    // 5. Temporary Segmented Digital LCD Readout
    if (readoutAlpha > 0.01f && readoutText.isNotEmpty())
    {
        auto roRect = getReadoutBounds();
        g.setColour(juce::Colour(0x04, 0x05, 0x08).withAlpha(readoutAlpha));
        g.fillRoundedRectangle(roRect, 2.5f);
        g.setColour(config.lcdColor.withAlpha(0.30f * readoutAlpha));
        g.drawRoundedRectangle(roRect, 2.5f, 0.6f);

        // LCD digital glowing text
        g.setFont(SpectralUILookAndFeel::getMonospaceFont(8.8f, true));
        g.setColour(config.lcdColor.brighter(0.3f).withAlpha(readoutAlpha));
        g.drawText(readoutText, roRect.toNearestInt(), juce::Justification::centred, false);
    }
}

// Mouse event handling
void EffectsPanel::EffectZoneComponent::mouseDown(const juce::MouseEvent& e)
{
    // 1. Header click (selects / toggles expansion)
    if (getHeaderBounds().contains(e.position))
    {
        if (onSelectRequest)
            onSelectRequest(config.index);
        return;
    }

    // 2. Primary Mix Bar click & drag
    auto mixArea = getMixBarBounds();
    if (mixArea.contains(e.position))
    {
        isDraggingMixBar = true;
        float trackX = mixArea.getX() + 2.0f;
        float trackW = juce::jmax(1.0f, mixArea.getWidth() - 4.0f);
        float normVal = juce::jlimit(0.0f, 1.0f, (e.position.x - trackX) / trackW);

        setNormalizedParamValue(config.mixParamID, normVal);
        int pct = (int)std::round(normVal * 100.0f);
        updateReadout("MIX: " + juce::String(pct) + "%");
        if (onParameterAdjusted)
            onParameterAdjusted(config.index, "MIX: " + juce::String(pct) + "%");
        return;
    }

    // 3. Secondary controls (only if expanded)
    if (isExpanded && expansionProgress > 0.5f)
    {
        // Bypass Toggle click
        if (getBypassToggleBounds().contains(e.position))
        {
            bool nextState = !isEffectEnabled();
            if (auto* param = apvts.getParameter(config.enableParamID))
                param->setValueNotifyingHost(nextState ? 1.0f : 0.0f);

            updateReadout(nextState ? "ACTIVE" : "BYPASSED");
            if (onParameterAdjusted)
                onParameterAdjusted(config.index, nextState ? "ACTIVE" : "BYPASSED");
            repaint();
            return;
        }

        if (config.index == 3) // DELAY
        {
            // Sync Toggle click
            if (getDelaySyncToggleBounds().contains(e.position))
            {
                delayIsSynced = !delayIsSynced;
                if (delayIsSynced)
                    syncDelayTimeFromDivision();

                updateReadout(delayIsSynced ? "SYNC: ON" : "SYNC: OFF");
                if (onParameterAdjusted)
                    onParameterAdjusted(config.index, delayIsSynced ? "SYNC: ON" : "SYNC: OFF");
                resized();
                repaint();
                return;
            }

            // Division Picker click
            if (delayIsSynced && getDelayDivisionBounds().contains(e.position))
            {
                const auto& names = getNoteDivisionNames();
                delayDivisionIdx = (delayDivisionIdx + 1) % names.size();
                syncDelayTimeFromDivision();

                updateReadout("DIV: " + names[delayDivisionIdx]);
                if (onParameterAdjusted)
                    onParameterAdjusted(config.index, "DIV: " + names[delayDivisionIdx]);
                repaint();
                return;
            }

            // Free Time Knob
            if (!delayIsSynced && secondaryKnob1.bounds.contains(e.position))
            {
                secondaryKnob1.isDragging = true;
                secondaryKnob1.dragStartY = e.position.y;
                secondaryKnob1.dragStartVal = getNormalizedParamValue(secondaryKnob1.paramID);
                return;
            }

            // Feedback Knob
            if (secondaryKnob2.bounds.contains(e.position))
            {
                secondaryKnob2.isDragging = true;
                secondaryKnob2.dragStartY = e.position.y;
                secondaryKnob2.dragStartVal = getNormalizedParamValue(secondaryKnob2.paramID);
                return;
            }
        }
        else
        {
            // Tone / Rate Knob
            if (secondaryKnob1.bounds.contains(e.position))
            {
                secondaryKnob1.isDragging = true;
                secondaryKnob1.dragStartY = e.position.y;
                secondaryKnob1.dragStartVal = getNormalizedParamValue(secondaryKnob1.paramID);
                return;
            }
        }
    }
}

void EffectsPanel::EffectZoneComponent::mouseDrag(const juce::MouseEvent& e)
{
    if (isDraggingMixBar)
    {
        auto mixArea = getMixBarBounds();
        float trackX = mixArea.getX() + 2.0f;
        float trackW = juce::jmax(1.0f, mixArea.getWidth() - 4.0f);
        float normVal = juce::jlimit(0.0f, 1.0f, (e.position.x - trackX) / trackW);

        setNormalizedParamValue(config.mixParamID, normVal);
        int pct = (int)std::round(normVal * 100.0f);
        updateReadout("MIX: " + juce::String(pct) + "%");
        if (onParameterAdjusted)
            onParameterAdjusted(config.index, "MIX: " + juce::String(pct) + "%");
        return;
    }

    if (secondaryKnob1.isDragging)
    {
        float dy = secondaryKnob1.dragStartY - e.position.y;
        float nextNorm = juce::jlimit(0.0f, 1.0f, secondaryKnob1.dragStartVal + dy / 100.0f);
        setNormalizedParamValue(secondaryKnob1.paramID, nextNorm);

        if (config.index == 0) // Drive Tone
        {
            int pct = (int)std::round(nextNorm * 100.0f);
            updateReadout("TONE: " + juce::String(pct) + "%");
        }
        else if (config.index == 3) // Delay Time (ms)
        {
            float ms = 10.0f + nextNorm * (1000.0f - 10.0f);
            updateReadout("TIME: " + juce::String((int)std::round(ms)) + "ms");
        }
        else // Rate (Hz)
        {
            if (auto* param = apvts.getParameter(secondaryKnob1.paramID))
            {
                float rawVal = param->getNormalisableRange().convertFrom0to1(nextNorm);
                updateReadout("RATE: " + juce::String(rawVal, 2) + "Hz");
            }
        }
        return;
    }

    if (secondaryKnob2.isDragging)
    {
        float dy = secondaryKnob2.dragStartY - e.position.y;
        float nextNorm = juce::jlimit(0.0f, 1.0f, secondaryKnob2.dragStartVal + dy / 100.0f);
        setNormalizedParamValue(secondaryKnob2.paramID, nextNorm);

        int pct = (int)std::round(nextNorm * 100.0f);
        updateReadout("FDBK: " + juce::String(pct) + "%");
        return;
    }
}

void EffectsPanel::EffectZoneComponent::mouseUp(const juce::MouseEvent&)
{
    isDraggingMixBar = false;
    secondaryKnob1.isDragging = false;
    secondaryKnob2.isDragging = false;
}

void EffectsPanel::EffectZoneComponent::mouseDoubleClick(const juce::MouseEvent& e)
{
    if (getMixBarBounds().contains(e.position))
    {
        setNormalizedParamValue(config.mixParamID, 0.0f);
        updateReadout("MIX: 0%");
        if (onParameterAdjusted)
            onParameterAdjusted(config.index, "MIX: 0%");
    }
}

void EffectsPanel::EffectZoneComponent::mouseMove(const juce::MouseEvent& e)
{
    bool headerHover = getHeaderBounds().contains(e.position);
    bool mixHover = getMixBarBounds().contains(e.position);

    if (headerHover != isHeaderHovered || mixHover != isMixBarHovered)
    {
        isHeaderHovered = headerHover;
        isMixBarHovered = mixHover;
        repaint();
    }
}

void EffectsPanel::EffectZoneComponent::mouseExit(const juce::MouseEvent&)
{
    isHeaderHovered = false;
    isMixBarHovered = false;
    repaint();
}

//==============================================================================
// EffectsPanel Implementation
//==============================================================================
EffectsPanel::EffectsPanel(juce::AudioProcessorValueTreeState& apvtsRef,
                           std::function<double()> bpmProviderFn)
    : apvts(apvtsRef),
      bpmProvider(bpmProviderFn)
{
    setupZones();

    for (const auto& paramID : watchedParamIDs)
    {
        apvts.addParameterListener(paramID, this);
    }

    startTimerHz(60);
}

EffectsPanel::~EffectsPanel()
{
    stopTimer();

    for (const auto& paramID : watchedParamIDs)
    {
        apvts.removeParameterListener(paramID, this);
    }
}

void EffectsPanel::setupZones()
{
    zones.clear();

    const std::vector<EffectZoneConfig> defs = {
        { 0, "DRIVE",  juce::Colour(0xD9, 0x77, 0x06), "FX_DRIVE_ENABLE",     "FX_DRIVE_AMOUNT", "FX_DRIVE_TONE",     "" },
        { 1, "CHORUS", juce::Colour(0x3B, 0x82, 0xF6), "FX_CHORUS_ENABLE",    "FX_CHORUS_AMOUNT", "FX_CHORUS_RATE",     "" },
        { 2, "PHASER", juce::Colour(0x8B, 0x7A, 0xA8), "FX_PHASER_ENABLE",    "FX_PHASER_AMOUNT", "FX_PHASER_RATE",     "" },
        { 3, "DELAY",  juce::Colour(0x14, 0xB8, 0xA6), "FX_DELAY_ENABLE",     "FX_DELAY_AMOUNT", "FX_DELAY_TIME",     "FX_DELAY_FEEDBACK" },
        { 4, "GATE",   juce::Colour(0x93, 0x33, 0xEA), "FX_SIDECHAIN_ENABLE", "FX_SIDECHAIN_MIX", "FX_SIDECHAIN_RATE", "" }
    };

    for (const auto& def : defs)
    {
        auto zone = std::make_unique<EffectZoneComponent>(
            def, apvts, bpmProvider,
            [this](int selectedIdx) {
                // Expand / Collapse selection toggle
                if (selectedZoneIndex == selectedIdx)
                {
                    selectedZoneIndex = -1; // Collapse
                }
                else
                {
                    selectedZoneIndex = selectedIdx; // Expand this one, collapse others
                }

                for (auto& z : zones)
                {
                    if (z != nullptr)
                        z->setSelected(z->getZoneIndex() == selectedZoneIndex);
                }
                repaint();
            },
            [this](int zoneIdx, const juce::String& text) {
                juce::ignoreUnused(zoneIdx, text);
            });

        addAndMakeVisible(*zone);
        zones.push_back(std::move(zone));
    }
}

void EffectsPanel::parameterChanged(const juce::String&, float)
{
    // External parameter changes (automation, preset load, shuffle FX)
    juce::MessageManager::callAsync([this]() {
        for (auto& z : zones)
        {
            if (z != nullptr)
                z->syncFromAPVTS();
        }
        repaint();
    });
}

void EffectsPanel::timerCallback()
{
    for (auto& z : zones)
    {
        if (z != nullptr)
            z->tickAnimation();
    }
}

void EffectsPanel::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();

    // 1. Unified Continuous Dark Hardware LCD Background
    juce::Colour topBg = juce::Colour(0x0A, 0x0B, 0x0F);
    juce::Colour botBg = juce::Colour(0x0E, 0x0F, 0x14);
    juce::ColourGradient bgGrad(topBg, bounds.getX(), bounds.getY(), botBg, bounds.getX(), bounds.getBottom(), false);
    g.setGradientFill(bgGrad);
    g.fillRoundedRectangle(bounds, 5.0f);

    // Hairline outer bezel frame
    g.setColour(juce::Colour(0x20, 0x22, 0x2B));
    g.drawRoundedRectangle(bounds, 5.0f, 1.0f);

    // Subtle top specular highlight line
    g.setColour(juce::Colour(0x38, 0x3C, 0x4C).withAlpha(0.20f));
    g.drawHorizontalLine((int)(bounds.getY() + 1.0f), bounds.getX() + 5.0f, bounds.getRight() - 5.0f);

    // 2. Extremely subtle vertical divider guides separating the 5 zones
    float colWidth = bounds.getWidth() / 5.0f;
    g.setColour(juce::Colour(0x16, 0x18, 0x22));
    for (int i = 1; i < 5; ++i)
    {
        float divX = bounds.getX() + (float)i * colWidth;
        g.drawVerticalLine((int)divX, bounds.getY() + 4.0f, bounds.getBottom() - 4.0f);
    }
}

void EffectsPanel::resized()
{
    auto bounds = getLocalBounds();
    if (bounds.isEmpty() || zones.empty())
        return;

    int totalZones = (int)zones.size();
    float colWidth = (float)bounds.getWidth() / (float)totalZones;

    for (int i = 0; i < totalZones; ++i)
    {
        int x0 = (int)std::round((float)i * colWidth);
        int x1 = (int)std::round((float)(i + 1) * colWidth);
        zones[(size_t)i]->setBounds(x0, bounds.getY(), x1 - x0, bounds.getHeight());
    }
}
