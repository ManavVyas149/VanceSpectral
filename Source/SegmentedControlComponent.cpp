#include "SegmentedControlComponent.h"

SegmentedControlComponent::SegmentedControlComponent(const juce::String& sectionLabel, const juce::StringArray& options, LayoutMode mode)
    : labelText(sectionLabel), optionsList(options), layoutMode(mode)
{
    for (int i = 0; i < optionsList.size(); ++i)
    {
        auto* btn = buttons.add(std::make_unique<OptionButton>(optionsList[i], i, *this));
        addAndMakeVisible(btn);

        btn->onClick = [this, i]() {
            bool isReclick = (i == selectedIndex);
            setSelectedIndex(i, true);
            if (onOptionClicked)
                onOptionClicked(i, isReclick);
        };
    }
}

void SegmentedControlComponent::setLayoutMode(LayoutMode mode)
{
    if (layoutMode != mode)
    {
        layoutMode = mode;
        resized();
        repaint();
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
    SpectralUILookAndFeel::drawPanelCard(g, bounds, labelText.toUpperCase());
}

void SegmentedControlComponent::resized()
{
    auto area = getLocalBounds().reduced(8, 6);
    area.removeFromTop(20); // Reserve for section label

    if (buttons.isEmpty())
        return;

    int numButtons = buttons.size();

    if (layoutMode == LayoutMode::Vertical)
    {
        float totalH = (float)area.getHeight();
        float gap = 3.0f;
        float btnHeight = (totalH - (numButtons - 1) * gap) / (float)numButtons;

        for (int i = 0; i < numButtons; ++i)
        {
            int y = (int)(area.getY() + i * (btnHeight + gap));
            int h = (int)btnHeight;
            buttons[i]->setBounds(area.getX(), y, area.getWidth(), h);
        }
    }
    else
    {
        float btnWidth = (float)area.getWidth() / (float)numButtons;

        for (int i = 0; i < numButtons; ++i)
        {
            auto btnArea = area.removeFromLeft((int)btnWidth);
            buttons[i]->setBounds(btnArea.reduced(2, 1));
        }
    }
}
