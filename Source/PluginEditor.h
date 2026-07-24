/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "SpectrogramComponent.h"

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
    
    std::unique_ptr<SpectrogramComponent> spectrogram;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VancespectralAudioProcessorEditor)
};
