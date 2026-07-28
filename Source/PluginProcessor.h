/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "SampleEngine.h"

//==============================================================================
/**
*/
class VancespectralAudioProcessor : public juce::AudioProcessor
{
public:
    //==============================================================================
    VancespectralAudioProcessor();
    ~VancespectralAudioProcessor() override;

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

   #ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
   #endif

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    //==============================================================================
    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    //==============================================================================
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;
    
    void loadSample(const juce::AudioBuffer<float>& buffer, double nativeSampleRate = 44100.0)
    {
        sampleEngine.loadSample(buffer, nativeSampleRate);
    }

    void playSample()
    {
        DBG("Processor Play");
        sampleEngine.play();
    }

    void stopSample()
    {
        sampleEngine.stop();
    }

    bool isPlaying() const
    {
        return sampleEngine.isPlaying();
    }

    double getPlayheadPosition() const
    {
        return sampleEngine.getPlayPositionNormalized();
    }

    double getRegionStart() const
    {
        return sampleEngine.getRegionStartNormalized();
    }

    double getRegionEnd() const
    {
        return sampleEngine.getRegionEndNormalized();
    }

    void setLoop(bool enabled)
    {
        sampleEngine.setLoop(enabled);
    }

    void setRegion(float start, float end)
    {
        sampleEngine.setRegion(start, end);
    }

    void setFrequencyFilter(bool enabled, float minFreq, float maxFreq)
    {
        sampleEngine.setFrequencyFilter(enabled, minFreq, maxFreq);
    }

    void setFrequencyFilterBands(const juce::Array<FrequencyBand>& bands)
    {
        sampleEngine.setFrequencyFilterBands(bands);
    }

    juce::AudioProcessorValueTreeState& getAPVTS() { return apvts; }
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

private:
    SampleEngine sampleEngine;
    juce::AudioProcessorValueTreeState apvts;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VancespectralAudioProcessor)
};
