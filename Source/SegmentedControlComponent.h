#pragma once

#include <JuceHeader.h>
#include "SpectralUILookAndFeel.h"

class SegmentedControlComponent : public juce::Component
{
public:
    SegmentedControlComponent(const juce::String& sectionLabel, const juce::StringArray& options);
    ~SegmentedControlComponent() override = default;

    void paint(juce::Graphics& g) override;
    void resized() override;

    int getSelectedIndex() const { return selectedIndex; }
    void setSelectedIndex(int newIndex, bool sendNotification = true);

    std::function<void(int)> onSelectionChanged;
    std::function<void(int index, bool isReclick)> onOptionClicked;

private:
    juce::String labelText;
    juce::StringArray optionsList;
    int selectedIndex = 0;

    class OptionButton : public juce::Button
    {
    public:
        OptionButton(const juce::String& text, int index, SegmentedControlComponent& parentRef)
            : juce::Button(text), optionIndex(index), parent(parentRef) {}

        void paintButton(juce::Graphics& g, bool isHighlighted, bool isDown) override
        {
            juce::ignoreUnused(isDown);
            auto bounds = getLocalBounds().toFloat();
            bool isActive = (parent.getSelectedIndex() == optionIndex);

            // Active pill background or accent underline
            if (isActive)
            {
                g.setColour(SpectralUILookAndFeel::accentColour.withAlpha(0.12f));
                g.fillRoundedRectangle(bounds.reduced(1.0f, 2.0f), 4.0f);

                g.setColour(SpectralUILookAndFeel::accentColour);
                g.drawRoundedRectangle(bounds.reduced(1.0f, 2.0f), 4.0f, 1.0f);
            }
            else if (isHighlighted)
            {
                g.setColour(juce::Colour::fromRGB(0x2A, 0x2C, 0x38));
                g.fillRoundedRectangle(bounds.reduced(1.0f, 2.0f), 4.0f);
            }

            g.setFont(SpectralUILookAndFeel::getGeometricFont(11.0f, isActive));
            g.setColour(isActive ? SpectralUILookAndFeel::accentColour
                                 : (isHighlighted ? SpectralUILookAndFeel::textMainColour
                                                  : SpectralUILookAndFeel::textMutedColour));

            g.drawText(getButtonText(), bounds, juce::Justification::centred, false);
        }

    private:
        int optionIndex;
        SegmentedControlComponent& parent;
    };

    juce::OwnedArray<OptionButton> buttons;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SegmentedControlComponent)
};
