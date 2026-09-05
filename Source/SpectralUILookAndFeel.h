#pragma once

#include <JuceHeader.h>

class SpectralUILookAndFeel : public juce::LookAndFeel_V4
{
public:
    SpectralUILookAndFeel();
    ~SpectralUILookAndFeel() override = default;

    // Palette Definition (Brutalist Precision Instrument Theme)
    static const juce::Colour bgColour;           // Warm light cream chassis (#ECEBE4)
    static const juce::Colour panelBgColour;      // Glossy cream card surface (#F8F7F2)
    static const juce::Colour graphBgColour;      // Deep black / dark indigo display panel (#0C0D12)
    static const juce::Colour textMainColour;     // Charcoal technical text (#1E1F24)
    static const juce::Colour textMutedColour;    // Muted warm gray technical labels (#7A7874)
    static const juce::Colour dividerColour;      // Hairline card border / divider (#D0CCBE)
    static const juce::Colour accentColour;       // Muted 'Burple' blue-purple / violet (#B84DC4)
    static const juce::Colour accentBright;       // Bright lilac highlight (#F2B8FF)
    static const juce::Colour knobBodyColour;     // Precision dark knob cylinder (#1E2028)
    static const juce::Colour knobInsetColour;    // Inner dark disc face (#15161E)

    // Static Drawing Utilities for Brutalist Chassis & Instrument Panels
    static void drawChassisBackground(juce::Graphics& g, juce::Rectangle<float> bounds);
    static void drawPanelCard(juce::Graphics& g, juce::Rectangle<float> bounds,
                              const juce::String& headerText = "",
                              const juce::String& subheaderText = "");
    static void drawCornerScrew(juce::Graphics& g, float cx, float cy, float radius = 5.0f, float slotAngleRad = 0.785f);
    static void drawPcbTraces(juce::Graphics& g, juce::Rectangle<float> bounds);

    void drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height,
                          float sliderPos, float rotaryStartAngle, float rotaryEndAngle,
                          juce::Slider& slider) override;

    void drawLinearSlider(juce::Graphics& g, int x, int y, int width, int height,
                          float sliderPos, float minSliderPos, float maxSliderPos,
                          const juce::Slider::SliderStyle style, juce::Slider& slider) override;

    void drawButtonBackground(juce::Graphics& g, juce::Button& button,
                             const juce::Colour& backgroundColour,
                             bool shouldDrawButtonAsHighlighted,
                             bool shouldDrawButtonAsDown) override;

    void drawButtonText(juce::Graphics& g, juce::TextButton& button,
                        bool shouldDrawButtonAsHighlighted,
                        bool shouldDrawButtonAsDown) override;

    void drawPopupMenuBackground(juce::Graphics& g, int width, int height) override;
    void drawPopupMenuItem(juce::Graphics& g, const juce::Rectangle<int>& area,
                           bool isHighlighted, bool isHeader, bool isActive,
                           bool isTicked, bool hasSubMenu, const juce::String& text,
                           const juce::String& shortcutKeyText,
                           const juce::Drawable* icon, const juce::Colour* textColour) override;

    juce::Font getLabelFont(juce::Label& label) override;
    juce::Font getTextButtonFont(juce::TextButton& button, int buttonHeight) override;

    static juce::Font getGeometricFont(float height, bool bold = false);
    static juce::Font getMonospaceFont(float height, bool bold = false);
};
