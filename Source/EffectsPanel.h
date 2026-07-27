/*
  ==============================================================================

    EffectsPanel.h
    Created: 24 Jul 2026 2:36:23pm
    Author:  MANAV VYAS

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

class EffectsPanel : public juce::Component
{
public:
    EffectsPanel() = default;
    ~EffectsPanel() override = default;

    void paint(juce::Graphics& g) override {}
    void resized() override {}

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(EffectsPanel)
};
