/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================

VancespectralAudioProcessor::VancespectralAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
    : AudioProcessor(
        BusesProperties()
#if ! JucePlugin_IsMidiEffect
            .withOutput("Output",
                        juce::AudioChannelSet::stereo(),
                        true)
#endif
    )
#endif
{
}
//==============================================================================

VancespectralAudioProcessor::~VancespectralAudioProcessor()
{
}

//==============================================================================

const juce::String VancespectralAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool VancespectralAudioProcessor::acceptsMidi() const
{
#if JucePlugin_WantsMidiInput
    return true;
#else
    return false;
#endif
}

bool VancespectralAudioProcessor::producesMidi() const
{
#if JucePlugin_ProducesMidiOutput
    return true;
#else
    return false;
#endif
}

bool VancespectralAudioProcessor::isMidiEffect() const
{
#if JucePlugin_IsMidiEffect
    return true;
#else
    return false;
#endif
}

double VancespectralAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

//==============================================================================

int VancespectralAudioProcessor::getNumPrograms()
{
    return 1;
}

int VancespectralAudioProcessor::getCurrentProgram()
{
    return 0;
}

void VancespectralAudioProcessor::setCurrentProgram(int index)
{
    juce::ignoreUnused(index);
}

const juce::String VancespectralAudioProcessor::getProgramName(int index)
{
    juce::ignoreUnused(index);
    return {};
}

void VancespectralAudioProcessor::changeProgramName(
    int index,
    const juce::String& newName)
{
    juce::ignoreUnused(index, newName);
}

//==============================================================================

void VancespectralAudioProcessor::prepareToPlay(
    double sampleRate,
    int samplesPerBlock)
{
    juce::ignoreUnused(samplesPerBlock);

    sampleEngine.prepare(sampleRate);

    DBG("================================");
    DBG("prepareToPlay");
    DBG("Sample Rate: " << sampleRate);
    DBG("Block Size: " << samplesPerBlock);
}

//==============================================================================

void VancespectralAudioProcessor::releaseResources()
{
}

//==============================================================================

#ifndef JucePlugin_PreferredChannelConfigurations

bool VancespectralAudioProcessor::isBusesLayoutSupported(
    const BusesLayout& layouts) const
{
#if JucePlugin_IsMidiEffect

    juce::ignoreUnused(layouts);
    return true;

#else

    auto output = layouts.getMainOutputChannelSet();

    if (output != juce::AudioChannelSet::mono()
        && output != juce::AudioChannelSet::stereo())
    {
        return false;
    }

    return true;

#endif
}

#endif

//==============================================================================

void VancespectralAudioProcessor::processBlock(
    juce::AudioBuffer<float>& buffer,
    juce::MidiBuffer& midiMessages)
{
    juce::ignoreUnused(midiMessages);

    juce::ScopedNoDenormals noDenormals;

    DBG("================================");
    DBG("Processor processBlock");

    DBG("Channels: "
        << buffer.getNumChannels());

    DBG("Samples: "
        << buffer.getNumSamples());

    //========================================================
    // Check for valid audio buffer
    //========================================================

    if (buffer.getNumChannels() == 0)
    {
        DBG("ERROR: No audio output channels!");
        return;
    }

    if (buffer.getNumSamples() == 0)
    {
        DBG("ERROR: No audio samples!");
        return;
    }

    //========================================================
    // Clear output
    //========================================================

    buffer.clear();

    //========================================================
    // Generate audio from SampleEngine
    //========================================================

    sampleEngine.process(
        buffer,
        0,
        buffer.getNumSamples()
    );

    //========================================================
    // Debug output
    //========================================================

    float firstSample =
        buffer.getSample(0, 0);

    float peak =
        buffer.getMagnitude(
            0,
            0,
            buffer.getNumSamples()
        );

    DBG("Output first sample: "
        << firstSample);

    DBG("Output peak: "
        << peak);
}

//==============================================================================

bool VancespectralAudioProcessor::hasEditor() const
{
    return true;
}

juce::AudioProcessorEditor*
VancespectralAudioProcessor::createEditor()
{
    return new VancespectralAudioProcessorEditor(*this);
}

//==============================================================================

void VancespectralAudioProcessor::getStateInformation(
    juce::MemoryBlock& destData)
{
    juce::ignoreUnused(destData);
}

void VancespectralAudioProcessor::setStateInformation(
    const void* data,
    int sizeInBytes)
{
    juce::ignoreUnused(data, sizeInBytes);
}

//==============================================================================

juce::AudioProcessor*
JUCE_CALLTYPE createPluginFilter()
{
    return new VancespectralAudioProcessor();
}