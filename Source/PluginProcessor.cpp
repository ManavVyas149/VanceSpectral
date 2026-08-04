/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "PresetManager.h"

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
    return true;
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
        else if (msg.isAllNotesOff() || msg.isAllSoundOff())
        {
            sampleEngine.noteOff(-1);
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
    state.setProperty("loadedSampleFileName", loadedSampleFileName, nullptr);
    state.setProperty("loadedSampleFilePath", loadedSampleFile.getFullPathName(), nullptr);
    state.setProperty("regionStart", (double)startRegionNormalized, nullptr);
    state.setProperty("regionEnd", (double)endRegionNormalized, nullptr);
    state.setProperty("loopEnabled", loopEnabled, nullptr);
    state.setProperty("currentPresetName", currentPresetName, nullptr);
    state.setProperty("selectionsJson", juce::JSON::toString(selectionsVar), nullptr);
    state.setProperty("isInitialized", isInitialized, nullptr);

    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    if (xml != nullptr)
        copyXmlToBinary (*xml, destData);
}

void VancespectralAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState (getXmlFromBinary (data, sizeInBytes));
    if (xmlState.get() != nullptr && xmlState->hasTagName (apvts.state.getType()))
    {
        auto vt = juce::ValueTree::fromXml (*xmlState);
        apvts.replaceState (vt);

        loadedSampleFileName = vt.getProperty("loadedSampleFileName", "").toString();
        juce::String filePath = vt.getProperty("loadedSampleFilePath", "").toString();
        startRegionNormalized = (float)(double)vt.getProperty("regionStart", 0.0);
        endRegionNormalized = (float)(double)vt.getProperty("regionEnd", 1.0);
        loopEnabled = (bool)vt.getProperty("loopEnabled", false);
        currentPresetName = vt.getProperty("currentPresetName", "Custom / Unsaved").toString();
        juce::String selectionsJson = vt.getProperty("selectionsJson", "[]").toString();
        selectionsVar = juce::JSON::parse(selectionsJson);
        isInitialized = (bool)vt.getProperty("isInitialized", true);

        // Attempt to locate and reload audio sample into DSP
        juce::File sampleFile(filePath);
        if (!sampleFile.existsAsFile() && loadedSampleFileName.isNotEmpty())
        {
            PresetManager pm;
            sampleFile = pm.getSamplesFolder().getChildFile(loadedSampleFileName);
        }

        if (sampleFile.existsAsFile())
        {
            juce::AudioFormatManager formatManager;
            formatManager.registerBasicFormats();
            std::unique_ptr<juce::AudioFormatReader> reader (formatManager.createReaderFor(sampleFile));
            if (reader != nullptr)
            {
                juce::AudioBuffer<float> buffer((int)reader->numChannels, (int)reader->lengthInSamples);
                reader->read(&buffer, 0, (int)reader->lengthInSamples, 0, true, true);
                setLoadedSample(sampleFile, buffer, reader->sampleRate);
            }
        }

        setRegion(startRegionNormalized, endRegionNormalized);
        setLoop(loopEnabled);

        // Convert selectionsVar to spectral regions array for DSP engine
        if (selectionsVar.isArray())
        {
            juce::Array<SpectralRegion> regions;
            auto* arr = selectionsVar.getArray();
            int id = 1;
            for (const auto& item : *arr)
            {
                if (item.isObject())
                {
                    auto* obj = item.getDynamicObject();
                    float x = (float)(double)obj->getProperty("x");
                    float y = (float)(double)obj->getProperty("y");
                    float w = (float)(double)obj->getProperty("w");
                    float h = (float)(double)obj->getProperty("h");

                    SpectralRegion sr;
                    sr.id = id++;
                    sr.startNorm = x;
                    sr.endNorm = x + w;

                    float yTop = juce::jlimit(0.0f, 1.0f, y);
                    float yBottom = juce::jlimit(0.0f, 1.0f, y + h);

                    float maxFreq = 20.0f * std::pow(1000.0f, 1.0f - yTop);
                    float minFreq = 20.0f * std::pow(1000.0f, 1.0f - yBottom);
                    if (minFreq > maxFreq) std::swap(minFreq, maxFreq);

                    sr.minFreq = minFreq;
                    sr.maxFreq = maxFreq;
                    regions.add(sr);
                }
            }
            sampleEngine.setSpectralRegions(regions);
        }
    }
}

void VancespectralAudioProcessor::resetParametersToDefault()
{
    const char* paramIDs[] = {
        "AMP_ATTACK", "AMP_DECAY", "AMP_SUSTAIN", "AMP_RELEASE",
        "FILTER_ATTACK", "FILTER_DECAY", "FILTER_SUSTAIN", "FILTER_RELEASE",
        "PLAYBACK_MODE", "PITCH_MODE", "PITCH_SEMITONES", "EXCITER", "GAIN"
    };

    for (const auto& id : paramIDs)
    {
        if (auto* param = apvts.getParameter(id))
        {
            param->setValueNotifyingHost(param->getDefaultValue());
        }
    }
}

//==============================================================================

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new VancespectralAudioProcessor();
}