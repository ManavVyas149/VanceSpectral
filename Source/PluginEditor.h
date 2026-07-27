/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "SpectrogramComponent.h"
#include "ADSRPanel.h"
#include "ToolbarComponent.h"

//==============================================================================
class PresetHeaderComponent : public juce::Component
{
public:
    PresetHeaderComponent()
    {
        addAndMakeVisible(randomButton);
        addAndMakeVisible(prevButton);
        addAndMakeVisible(nextButton);

        presets = {
            "Cold Synth",
            "Spectral Lead",
            "Cyber Bass",
            "Neon Pad",
            "Vibe Synth",
            "Glitch Pulse"
        };
        currentPresetIndex = 0;

        randomButton.onClick = [this]()
        {
            if (!presets.isEmpty())
            {
                int newIndex = juce::Random::getSystemRandom().nextInt((int)presets.size());
                currentPresetIndex = newIndex;
                repaint();
            }
            if (onPresetChanged)
                onPresetChanged(getCurrentPreset());
        };

        prevButton.onClick = [this]()
        {
            if (!presets.isEmpty())
            {
                currentPresetIndex = (currentPresetIndex - 1 + (int)presets.size()) % (int)presets.size();
                repaint();
            }
            if (onPresetChanged)
                onPresetChanged(getCurrentPreset());
        };

        nextButton.onClick = [this]()
        {
            if (!presets.isEmpty())
            {
                currentPresetIndex = (currentPresetIndex + 1) % (int)presets.size();
                repaint();
            }
            if (onPresetChanged)
                onPresetChanged(getCurrentPreset());
        };
    }

    ~PresetHeaderComponent() override = default;

    juce::String getCurrentPreset() const
    {
        if (currentPresetIndex >= 0 && currentPresetIndex < presets.size())
            return presets[currentPresetIndex];
        return "Cold Synth";
    }

    std::function<void(const juce::String&)> onPresetChanged;

    void paint(juce::Graphics& g) override
    {
        auto bounds = getLocalBounds().toFloat();
        float cornerRadius = 10.0f;

        // Dark capsule background matching reference image
        g.setColour(juce::Colour(12, 12, 14));
        g.fillRoundedRectangle(bounds, cornerRadius);

        g.setColour(juce::Colour(45, 45, 52));
        g.drawRoundedRectangle(bounds, cornerRadius, 1.2f);

        // Center Area for Preset Title & Mini Waveform Graphics
        auto centerArea = bounds.reduced(80.0f, 0.0f);
        juce::String presetText = getCurrentPreset();

        juce::Font font(juce::FontOptions("Consolas", 16.0f, juce::Font::bold));
        g.setFont(font);
        g.setColour(juce::Colours::white);

        juce::GlyphArrangement ga;
        ga.addLineOfText(font, presetText, 0.0f, 0.0f);
        float textWidth = ga.getBoundingBox(0, -1, true).getWidth();

        float centerX = centerArea.getCentreX();
        float centerY = centerArea.getCentreY();

        // Draw Preset Name
        g.drawText(presetText, centerArea, juce::Justification::centred, false);

        // Draw Mini Audio Waveform Accent Graphics flanking the preset text
        auto drawMiniWaveform = [&](float startX, float y, float width, bool flip)
        {
            juce::Path wave;
            float step = width / 12.0f;
            wave.startNewSubPath(startX, y);

            float heights[] = { 0.0f, 2.5f, -4.0f, 5.0f, -6.0f, 3.5f, -5.0f, 4.0f, -2.5f, 3.0f, -1.5f, 0.0f };
            for (int i = 0; i < 12; ++i)
            {
                float px = startX + (flip ? (12 - i) * step : i * step);
                float py = y + heights[i];
                wave.lineTo(px, py);
            }

            g.setColour(juce::Colour(220, 225, 235));
            g.strokePath(wave, juce::PathStrokeType(1.2f));
        };

        float waveWidth = 45.0f;
        drawMiniWaveform(centerX - textWidth * 0.5f - waveWidth - 12.0f, centerY, waveWidth, false);
        drawMiniWaveform(centerX + textWidth * 0.5f + 12.0f, centerY, waveWidth, true);
    }

    void resized() override
    {
        auto bounds = getLocalBounds().reduced(8, 6);

        // Left RANDOM button
        randomButton.setBounds(bounds.removeFromLeft(72));

        // Right Arrow Buttons
        auto rightArea = bounds.removeFromRight(56);
        prevButton.setBounds(rightArea.removeFromLeft(26));
        rightArea.removeFromLeft(4);
        nextButton.setBounds(rightArea.removeFromLeft(26));
    }

private:
    class RandomButton : public juce::Button
    {
    public:
        RandomButton() : juce::Button("RANDOM") {}
        ~RandomButton() override = default;

        void paintButton(juce::Graphics& g, bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override
        {
            auto bounds = getLocalBounds().toFloat().reduced(1.0f);
            float radius = 5.0f;

            juce::Colour borderCol = shouldDrawButtonAsHighlighted ? juce::Colour(255, 80, 90) : juce::Colour(220, 40, 50);
            juce::Colour bgCol = shouldDrawButtonAsDown ? juce::Colour(200, 30, 40).withAlpha(0.35f) : juce::Colour(150, 20, 30).withAlpha(0.18f);
            juce::Colour textCol = shouldDrawButtonAsHighlighted ? juce::Colour(255, 120, 130) : juce::Colour(255, 60, 70);

            g.setColour(bgCol);
            g.fillRoundedRectangle(bounds, radius);

            g.setColour(borderCol);
            g.drawRoundedRectangle(bounds, radius, 1.2f);

            g.setColour(textCol);
            g.setFont(juce::FontOptions("Consolas", 10.0f, juce::Font::bold));
            g.drawText("RANDOM", bounds, juce::Justification::centred, false);
        }
    };

    class ArrowButton : public juce::Button
    {
    public:
        ArrowButton(bool isRight) : juce::Button(isRight ? ">" : "<"), pointsRight(isRight) {}
        ~ArrowButton() override = default;

        void paintButton(juce::Graphics& g, bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override
        {
            auto bounds = getLocalBounds().toFloat();
            juce::Colour iconCol = shouldDrawButtonAsDown ? juce::Colour(0, 220, 255)
                                  : (shouldDrawButtonAsHighlighted ? juce::Colour(255, 255, 255) : juce::Colour(200, 205, 215));

            juce::Path p;
            float cx = bounds.getCentreX();
            float cy = bounds.getCentreY();
            float size = 6.0f;

            if (pointsRight)
            {
                p.startNewSubPath(cx - size * 0.5f, cy - size);
                p.lineTo(cx + size * 0.5f, cy);
                p.lineTo(cx - size * 0.5f, cy + size);
            }
            else
            {
                p.startNewSubPath(cx + size * 0.5f, cy - size);
                p.lineTo(cx - size * 0.5f, cy);
                p.lineTo(cx + size * 0.5f, cy + size);
            }

            g.setColour(iconCol);
            g.strokePath(p, juce::PathStrokeType(1.8f, juce::PathStrokeType::mitered, juce::PathStrokeType::square));
        }

    private:
        bool pointsRight = false;
    };

    RandomButton randomButton;
    ArrowButton prevButton{ false };
    ArrowButton nextButton{ true };

    juce::StringArray presets;
    int currentPresetIndex = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PresetHeaderComponent)
};

//==============================================================================
/**
*/
class VancespectralAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    VancespectralAudioProcessorEditor (VancespectralAudioProcessor&);
    ~VancespectralAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    VancespectralAudioProcessor& audioProcessor;
    
    ToolbarComponent toolbar;
    PresetHeaderComponent presetHeader;
    std::unique_ptr<SpectrogramComponent> spectrogram;
    ADSRPanel ampADSRPanel;
    ADSRPanel filterADSRPanel;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VancespectralAudioProcessorEditor)
};
