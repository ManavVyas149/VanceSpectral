/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================

juce::AudioProcessorValueTreeState::ParameterLayout VancespectralAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    // Amplifier Envelope Parameters (4 Knobs: Attack, Decay, Sustain, Release)
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("AMP_ATTACK", 1), "Amp Attack", juce::NormalisableRange<float>(0.001f, 5.0f, 0.001f, 0.5f), 0.1f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("AMP_DECAY", 1), "Amp Decay", juce::NormalisableRange<float>(0.001f, 5.0f, 0.001f, 0.5f), 0.2f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("AMP_SUSTAIN", 1), "Amp Sustain", juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.8f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("AMP_RELEASE", 1), "Amp Release", juce::NormalisableRange<float>(0.001f, 5.0f, 0.001f, 0.5f), 0.5f));

    // Filter Envelope Parameters (4 Knobs: Attack, Decay, Sustain, Release)
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("FILTER_ATTACK", 1), "Filter Attack", juce::NormalisableRange<float>(0.001f, 5.0f, 0.001f, 0.5f), 0.05f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("FILTER_DECAY", 1), "Filter Decay", juce::NormalisableRange<float>(0.001f, 5.0f, 0.001f, 0.5f), 0.3f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("FILTER_SUSTAIN", 1), "Filter Sustain", juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.5f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("FILTER_RELEASE", 1), "Filter Release", juce::NormalisableRange<float>(0.001f, 5.0f, 0.001f, 0.5f), 0.4f));

    return { params.begin(), params.end() };
}

VancespectralAudioProcessor::VancespectralAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
    : AudioProcessor(
        BusesProperties()
#if ! JucePlugin_IsMidiEffect
            .withOutput("Output",
                        juce::AudioChannelSet::stereo(),
                        true)
#endif
    ),
#else
    :
#endif
      apvts(*this, nullptr, "Parameters", createParameterLayout())
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

    if (buffer.getNumChannels() == 0 || buffer.getNumSamples() == 0)
        return;

    buffer.clear();

    // Update Amplifier envelope parameters from APVTS
    float ampA = *apvts.getRawParameterValue("AMP_ATTACK");
    float ampD = *apvts.getRawParameterValue("AMP_DECAY");
    float ampS = *apvts.getRawParameterValue("AMP_SUSTAIN");
    float ampR = *apvts.getRawParameterValue("AMP_RELEASE");
    sampleEngine.updateAmpADSR(ampA, ampD, ampS, ampR);

    // Update Filter envelope parameters from APVTS
    float filtA = *apvts.getRawParameterValue("FILTER_ATTACK");
    float filtD = *apvts.getRawParameterValue("FILTER_DECAY");
    float filtS = *apvts.getRawParameterValue("FILTER_SUSTAIN");
    float filtR = *apvts.getRawParameterValue("FILTER_RELEASE");
    sampleEngine.updateFilterADSR(filtA, filtD, filtS, filtR);

    // Generate audio from SampleEngine
    sampleEngine.process(
        buffer,
        0,
        buffer.getNumSamples()
    );
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
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    copyXmlToBinary (*xml, destData);
}

void VancespectralAudioProcessor::setStateInformation(
    const void* data,
    int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState (getXmlFromBinary (data, sizeInBytes));
    if (xmlState.get() != nullptr)
        if (xmlState->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xmlState));
}

//==============================================================================

juce::AudioProcessor*
JUCE_CALLTYPE createPluginFilter()
{
    return new VancespectralAudioProcessor();
}