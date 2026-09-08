#pragma once

#include <JuceHeader.h>

class SpectralUILookAndFeel : public juce::LookAndFeel_V4
{
public:
    SpectralUILookAndFeel();
    ~SpectralUILookAndFeel() override = default;

    // Palette Definition (Minimalist Translucent Frosted-White & Lavender Theme)
    static const juce::Colour bgColour;           // Translucent frosted white chassis (#F4F4F6)
    static const juce::Colour panelBgColour;      // Frosted coated card surface (#FAF9FC)
    static const juce::Colour graphBgColour;      // Deep pitch-black display panel (#07080B)
    static const juce::Colour textMainColour;     // Charcoal technical text (#181920)
    static const juce::Colour textMutedColour;    // Muted technical gray labels (#787A86)
    static const juce::Colour dividerColour;      // Hairline card border / divider (#D4D6E0)
    static const juce::Colour accentColour;       // Restrained lavender / violet (#A78BFA)
    static const juce::Colour accentBright;       // Soft lilac highlight (#C4B5FD)
    static const juce::Colour knobBodyColour;     // Machined light-silver encoder body (#E2E4EB)
    static const juce::Colour knobInsetColour;    // Inner dark disc face (#121318)

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

    juce::Slider::SliderLayout getSliderLayout(juce::Slider& slider) override;

    void drawLinearSlider(juce::Graphics& g, int x, int y, int width, int height,
                          float sliderPos, float minSliderPos, float maxSliderPos,
                          const juce::Slider::SliderStyle style, juce::Slider& slider) override;

    void drawCornerResizer(juce::Graphics& g, int w, int h, bool isMouseOver, bool isMouseDragging) override;

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

    void drawScrollbar(juce::Graphics& g, juce::ScrollBar& scrollbar,
                       int x, int y, int width, int height,
                       bool isScrollbarVertical,
                       int thumbStartPosition, int thumbSize,
                       bool isMouseOver, bool isMouseDown) override;

    static juce::Font getGeometricFont(float height, bool bold = false);
    static juce::Font getMonospaceFont(float height, bool bold = false);
    static juce::Font getJetBrainsMono(float height, bool bold = false);
    static juce::Font getSpaceGrotesk(float height, bool bold = false);
};
