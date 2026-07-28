#pragma once

#include <JuceHeader.h>

class SpectralUILookAndFeel : public juce::LookAndFeel_V4
{
public:
    SpectralUILookAndFeel();
    ~SpectralUILookAndFeel() override = default;

    // Palette Definition
    static const juce::Colour bgColour;           // Warm off-white / bone (#F0ECE1)
    static const juce::Colour panelBgColour;      // Card / Panel matte surface (#FAF8F5)
    static const juce::Colour graphBgColour;      // Near-black graph panel (#0A0A0C)
    static const juce::Colour textMainColour;     // Charcoal text (#242320)
    static const juce::Colour textMutedColour;    // Muted warm gray text (#7E7B75)
    static const juce::Colour dividerColour;      // Hairline divider (#D4CEBF, ~0.5 alpha)
    static const juce::Colour accentColour;       // Single soft amber (#D98B4F)

    void drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height,
                          float sliderPos, float rotaryStartAngle, float rotaryEndAngle,
                          juce::Slider& slider) override;

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
    static juce::Font getMonospaceFont(float height);
};
