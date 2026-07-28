#include "SegmentedControlComponent.h"

SegmentedControlComponent::SegmentedControlComponent(const juce::String& sectionLabel, const juce::StringArray& options)
    : labelText(sectionLabel), optionsList(options)
{
    for (int i = 0; i < optionsList.size(); ++i)
    {
        auto* btn = buttons.add(std::make_unique<OptionButton>(optionsList[i], i, *this));
        addAndMakeVisible(btn);

        btn->onClick = [this, i]() {
            setSelectedIndex(i, true);
        };
    }
}

void SegmentedControlComponent::setSelectedIndex(int newIndex, bool sendNotification)
{
    if (newIndex >= 0 && newIndex < optionsList.size() && newIndex != selectedIndex)
    {
        selectedIndex = newIndex;
        repaint();
        for (auto* btn : buttons)
            btn->repaint();

        if (sendNotification && onSelectionChanged)
            onSelectionChanged(selectedIndex);
    }
}

void SegmentedControlComponent::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();

    // Panel background
    g.setColour(SpectralUILookAndFeel::panelBgColour);
    g.fillRoundedRectangle(bounds, 6.0f);

    // Hairline border
    g.setColour(SpectralUILookAndFeel::dividerColour);
    g.drawRoundedRectangle(bounds, 6.0f, 1.0f);

    // Quiet section label top-left
    g.setFont(SpectralUILookAndFeel::getGeometricFont(10.0f, true));
    g.setColour(SpectralUILookAndFeel::textMutedColour);
    g.drawText(labelText.toUpperCase(), bounds.removeFromTop(18.0f).reduced(8.0f, 0.0f).toNearestInt(), juce::Justification::left, true);
}

void SegmentedControlComponent::resized()
{
    auto area = getLocalBounds().reduced(6, 4);
    area.removeFromTop(16); // Reserve for section label

    if (buttons.isEmpty())
        return;

    int numButtons = buttons.size();
    float btnWidth = (float)area.getWidth() / (float)numButtons;

    for (int i = 0; i < numButtons; ++i)
    {
        auto btnArea = area.removeFromLeft((int)btnWidth);
        buttons[i]->setBounds(btnArea.reduced(2, 1));
    }
}
