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

    // Playback Mode Parameter (5 states: Forward, Backward, Forw-Backw, Back-Forw, Random)
    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID("PLAYBACK_MODE", 1), "Playback Mode",
        juce::StringArray{ "Forward", "Backward", "Forw-Backw", "Back-Forw", "Random" }, 0));

    // Pitch Mode Parameter (3 states: Stretch, Resample, Axial)
    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID("PITCH_MODE", 1), "Pitch Mode",
        juce::StringArray{ "Stretch", "Resample", "Axial" }, 0));

    // Pitch Semitones Offset Parameter (-24 to +24 semitones, default 0 st)
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("PITCH_SEMITONES", 1), "Pitch Shift", juce::NormalisableRange<float>(-24.0f, 24.0f, 1.0f), 0.0f));

    // Exciter Parameter (0.0 to 1.0, default 0.0)
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("EXCITER", 1), "Exciter", juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.0f));

    // Master Gain Parameter (-48.0 dB to +6.0 dB, default 0.0 dB)
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("GAIN", 1), "Master Gain", juce::NormalisableRange<float>(-48.0f, 6.0f, 0.1f), 0.0f));

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
    if (output != juce::AudioChannelSet::mono() && output != juce::AudioChannelSet::stereo())
        return false;
    return true;
#endif
}

#endif

//==============================================================================

void VancespectralAudioProcessor::processBlock(
    juce::AudioBuffer<float>& buffer,
    juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;

    if (buffer.getNumChannels() == 0 || buffer.getNumSamples() == 0)
        return;

    buffer.clear();

    for (const auto metadata : midiMessages)
    {
        auto msg = metadata.getMessage();
        if (msg.isNoteOn())
        {
            sampleEngine.noteOn(msg.getNoteNumber(), msg.getVelocity() / 127.0f);
        }
        else if (msg.isNoteOff())
        {
            sampleEngine.noteOff(msg.getNoteNumber());
        }
        else if (msg.isAllNotesOff())
        {
            sampleEngine.stop();
        }
    }

    float ampA = *apvts.getRawParameterValue("AMP_ATTACK");
    float ampD = *apvts.getRawParameterValue("AMP_DECAY");
    float ampS = *apvts.getRawParameterValue("AMP_SUSTAIN");
    float ampR = *apvts.getRawParameterValue("AMP_RELEASE");
    sampleEngine.updateAmpADSR(ampA, ampD, ampS, ampR);

    float filtA = *apvts.getRawParameterValue("FILTER_ATTACK");
    float filtD = *apvts.getRawParameterValue("FILTER_DECAY");
    float filtS = *apvts.getRawParameterValue("FILTER_SUSTAIN");
    float filtR = *apvts.getRawParameterValue("FILTER_RELEASE");
    sampleEngine.updateFilterADSR(filtA, filtD, filtS, filtR);

    int playMode = (int)*apvts.getRawParameterValue("PLAYBACK_MODE");
    int pitchMode = (int)*apvts.getRawParameterValue("PITCH_MODE");
    float pitchSemis = *apvts.getRawParameterValue("PITCH_SEMITONES");
    float exciterVal = *apvts.getRawParameterValue("EXCITER");
    sampleEngine.setPlaybackMode(playMode);
    sampleEngine.setPitchMode(pitchMode);
    sampleEngine.setPitchSemitones(pitchSemis);
    sampleEngine.setExciterAmount(exciterVal);

    if (auto* ph = getPlayHead())
    {
        if (auto position = ph->getPosition())
        {
            if (position->getBpm().hasValue())
                sampleEngine.setHostBpm(*position->getBpm());
        }
    }

    sampleEngine.process(buffer, 0, buffer.getNumSamples());

    float masterGainDb = *apvts.getRawParameterValue("GAIN");
    float masterGainLinear = juce::Decibels::decibelsToGain(masterGainDb);
    buffer.applyGain(masterGainLinear);
}

//==============================================================================

bool VancespectralAudioProcessor::hasEditor() const
{
    return true;
}

juce::AudioProcessorEditor* VancespectralAudioProcessor::createEditor()
{
    return new VancespectralAudioProcessorEditor(*this);
}

//==============================================================================

void VancespectralAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    copyXmlToBinary (*xml, destData);
}

void VancespectralAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState (getXmlFromBinary (data, sizeInBytes));
    if (xmlState.get() != nullptr)
        if (xmlState->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xmlState));
}

//==============================================================================

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new VancespectralAudioProcessor();
}