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


    // Playback Mode Parameter (5 states: Forward, Backward, Forw-Backw, Back-Forw, Random)
    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID("PLAYBACK_MODE", 1), "Playback Mode",
        juce::StringArray{ "Forward", "Backward", "Forw-Backw", "Back-Forw", "Random" }, 0));

    // Pitch Mode Parameter (2 states: Stretch, Resample)
    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID("PITCH_MODE", 1), "Pitch Mode",
        juce::StringArray{ "Stretch", "Resample" }, 0));

    // Pitch Semitones Offset Parameter (-24 to +24 semitones, default 0 st)
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("PITCH_SEMITONES", 1), "Pitch Shift", juce::NormalisableRange<float>(-24.0f, 24.0f, 1.0f), 0.0f));

    // Voice Drift Amount Parameter (0.0 to 1.0, default 0.0)
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("TIMBRE_DRIFT", 1), "Voice Drift", juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.0f));

    // Exciter Parameter (0.0 to 1.0, default 0.0)
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("EXCITER", 1), "Exciter", juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.0f));

    // Polyphony Mode Parameter (2 states: Mono [0], Poly [1], default Mono)
    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID("POLY_MODE", 1), "Polyphony Mode",
        juce::StringArray{ "Mono", "Poly" }, 0));

    // Glide Parameter (0.0s to 2.0s, default 0.0s)
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("GLIDE", 1), "Glide Time", juce::NormalisableRange<float>(0.0f, 2.0f, 0.001f, 0.5f), 0.0f));

    // Master Gain Parameter (-48.0 dB to +6.0 dB, default 0.0 dB)
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("GAIN", 1), "Master Gain", juce::NormalisableRange<float>(-48.0f, 6.0f, 0.1f), 0.0f));

    // Master Wet Effects (Sidechain, Chorus, Phaser, Delay, Drive)
    params.push_back(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID("FX_SIDECHAIN_ENABLE", 1), "Sidechain Enable", false));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("FX_SIDECHAIN_MIX", 1), "Sidechain Mix", juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("FX_SIDECHAIN_RATE", 1), "Sidechain Rate", juce::NormalisableRange<float>(0.5f, 20.0f, 0.01f, 0.5f), 2.0f));

    params.push_back(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID("FX_CHORUS_ENABLE", 1), "Chorus Enable", false));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("FX_CHORUS_AMOUNT", 1), "Chorus Mix", juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("FX_CHORUS_RATE", 1), "Chorus Rate", juce::NormalisableRange<float>(0.1f, 5.0f, 0.01f, 0.5f), 1.0f));

    params.push_back(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID("FX_PHASER_ENABLE", 1), "Phaser Enable", false));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("FX_PHASER_AMOUNT", 1), "Phaser Mix", juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("FX_PHASER_RATE", 1), "Phaser Rate", juce::NormalisableRange<float>(0.05f, 4.0f, 0.01f, 0.5f), 0.5f));

    params.push_back(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID("FX_DELAY_ENABLE", 1), "Delay Enable", false));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("FX_DELAY_AMOUNT", 1), "Delay Mix", juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("FX_DELAY_TIME", 1), "Delay Time", juce::NormalisableRange<float>(10.0f, 1000.0f, 1.0f, 0.5f), 250.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("FX_DELAY_FEEDBACK", 1), "Delay Feedback", juce::NormalisableRange<float>(0.0f, 0.85f, 0.01f), 0.35f));

    params.push_back(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID("FX_DRIVE_ENABLE", 1), "Drive Enable", false));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("FX_DRIVE_AMOUNT", 1), "Drive Amount", juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("FX_DRIVE_TONE", 1), "Drive Tone", juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.5f));

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
    sampleEngine.prepare(sampleRate);
    effectsEngine.prepare(sampleRate, samplesPerBlock);
    masterGainSmoother.reset(sampleRate, 0.02); // 20ms gain smoothing
    float masterGainDb = *apvts.getRawParameterValue("GAIN");
    masterGainSmoother.setCurrentAndTargetValue(juce::Decibels::decibelsToGain(masterGainDb));
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


    int playMode = (int)*apvts.getRawParameterValue("PLAYBACK_MODE");
    int pitchMode = (int)*apvts.getRawParameterValue("PITCH_MODE");
    float pitchSemis = *apvts.getRawParameterValue("PITCH_SEMITONES");
    float timbreDriftVal = *apvts.getRawParameterValue("TIMBRE_DRIFT");
    float exciterVal = *apvts.getRawParameterValue("EXCITER");
    int polyModeVal = (int)*apvts.getRawParameterValue("POLY_MODE");
    float glideValSec = *apvts.getRawParameterValue("GLIDE");

    sampleEngine.setPlaybackMode(playMode);
    sampleEngine.setPitchMode(pitchMode);
    sampleEngine.setPitchSemitones(pitchSemis);
    sampleEngine.setTimbreDrift(timbreDriftVal);
    sampleEngine.setExciterAmount(exciterVal);
    sampleEngine.setPolyMode(polyModeVal == 1);
    sampleEngine.setGlideTime(glideValSec * 1000.0f);

    if (auto* ph = getPlayHead())
    {
        if (auto position = ph->getPosition())
        {
            if (position->getBpm().hasValue())
                sampleEngine.setHostBpm(*position->getBpm());
        }
    }

    sampleEngine.process(buffer, 0, buffer.getNumSamples());

    // 5-Stage Master Effects Chain (Sidechain -> Chorus -> Phaser -> Delay -> Drive -> Exciter [inside sampleEngine] -> Volume)
    effectsEngine.setSidechainEnabled(*apvts.getRawParameterValue("FX_SIDECHAIN_ENABLE") >= 0.5f);
    effectsEngine.setSidechainMix(*apvts.getRawParameterValue("FX_SIDECHAIN_MIX"));
    effectsEngine.setSidechainRate(*apvts.getRawParameterValue("FX_SIDECHAIN_RATE"));

    effectsEngine.setChorusEnabled(*apvts.getRawParameterValue("FX_CHORUS_ENABLE") >= 0.5f);
    effectsEngine.setChorusAmount(*apvts.getRawParameterValue("FX_CHORUS_AMOUNT"));
    effectsEngine.setChorusRate(*apvts.getRawParameterValue("FX_CHORUS_RATE"));

    effectsEngine.setPhaserEnabled(*apvts.getRawParameterValue("FX_PHASER_ENABLE") >= 0.5f);
    effectsEngine.setPhaserAmount(*apvts.getRawParameterValue("FX_PHASER_AMOUNT"));
    effectsEngine.setPhaserRate(*apvts.getRawParameterValue("FX_PHASER_RATE"));

    effectsEngine.setDelayEnabled(*apvts.getRawParameterValue("FX_DELAY_ENABLE") >= 0.5f);
    effectsEngine.setDelayAmount(*apvts.getRawParameterValue("FX_DELAY_AMOUNT"));
    effectsEngine.setDelayTime(*apvts.getRawParameterValue("FX_DELAY_TIME"));
    effectsEngine.setDelayFeedback(*apvts.getRawParameterValue("FX_DELAY_FEEDBACK"));

    effectsEngine.setDriveEnabled(*apvts.getRawParameterValue("FX_DRIVE_ENABLE") >= 0.5f);
    effectsEngine.setDriveAmount(*apvts.getRawParameterValue("FX_DRIVE_AMOUNT"));
    effectsEngine.setDriveTone(*apvts.getRawParameterValue("FX_DRIVE_TONE"));

    effectsEngine.process(buffer);

    float masterGainDb = *apvts.getRawParameterValue("GAIN");
    float masterGainLinear = juce::Decibels::decibelsToGain(masterGainDb);
    masterGainSmoother.setTargetValue(masterGainLinear);

    if (masterGainSmoother.isSmoothing())
    {
        masterGainSmoother.applyGain(buffer, buffer.getNumSamples());
    }
    else
    {
        buffer.applyGain(masterGainSmoother.getCurrentValue());
    }
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
                int rootNote = 60;
                if (reader->metadataValues.containsKey("sampleRootNote"))
                    rootNote = reader->metadataValues.getValue("sampleRootNote", "60").getIntValue();
                else if (reader->metadataValues.containsKey("RootNote"))
                    rootNote = reader->metadataValues.getValue("RootNote", "60").getIntValue();

                setLoadedSample(sampleFile, buffer, reader->sampleRate, rootNote);
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
    const char* idsToReset[] = {
        "AMP_ATTACK", "AMP_DECAY", "AMP_SUSTAIN", "AMP_RELEASE",
        "PLAYBACK_MODE", "PITCH_MODE", "PITCH_SEMITONES", "TIMBRE_DRIFT", "EXCITER", "POLY_MODE", "GLIDE", "GAIN",
        "FX_SIDECHAIN_ENABLE", "FX_SIDECHAIN_MIX", "FX_SIDECHAIN_RATE",
        "FX_CHORUS_ENABLE", "FX_CHORUS_AMOUNT", "FX_CHORUS_RATE",
        "FX_PHASER_ENABLE", "FX_PHASER_AMOUNT", "FX_PHASER_RATE",
        "FX_DELAY_ENABLE", "FX_DELAY_AMOUNT", "FX_DELAY_TIME", "FX_DELAY_FEEDBACK",
        "FX_DRIVE_ENABLE", "FX_DRIVE_AMOUNT", "FX_DRIVE_TONE"
    };

    for (const auto& id : idsToReset)
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