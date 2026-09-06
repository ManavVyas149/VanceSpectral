/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "SampleEngine.h"
#include "HistoryManager.h"
#include "AudioResampler.h"
#include "EffectsEngine.h"

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
    
    void setLoadedSample(const juce::File& file, const juce::AudioBuffer<float>& buffer, double nativeSampleRate, int rootNote = 60)
    {
        loadedSampleFile = file;
        loadedSampleFileName = file.getFileName();
        sampleLoaded = true;

        double currentSr = getSampleRate() > 0.0 ? getSampleRate() : 44100.0;
        juce::AudioBuffer<float> resampled = AudioResampler::resampleIfNeeded(buffer, nativeSampleRate, currentSr);
        sampleEngine.loadSample(resampled, currentSr, rootNote);
    }

    const juce::AudioBuffer<float>& getLoadedSampleBuffer() const { return sampleEngine.getLoadedSample(); }
    juce::File getLoadedSampleFile() const { return loadedSampleFile; }
    juce::String getLoadedSampleFileName() const { return loadedSampleFileName; }
    bool isSampleLoaded() const { return sampleLoaded; }

    void setRegion(float start, float end)
    {
        startRegionNormalized = juce::jlimit(0.0f, 1.0f, start);
        endRegionNormalized = juce::jlimit(0.0f, 1.0f, end);
        sampleEngine.setRegion(startRegionNormalized, endRegionNormalized);
    }
    float getRegionStartNormalized() const { return startRegionNormalized; }
    float getRegionEndNormalized() const { return endRegionNormalized; }

    void setLoop(bool enabled)
    {
        loopEnabled = enabled;
        sampleEngine.setLoop(enabled);
    }
    bool getLoopEnabled() const { return loopEnabled; }

    void setSelectionsVar(const juce::var& var) { selectionsVar = var; }
    juce::var getSelectionsVar() const { return selectionsVar; }

    void setCurrentPresetName(const juce::String& name) { currentPresetName = name; }
    juce::String getCurrentPresetName() const { return currentPresetName; }

    bool isPluginInitialized() const { return isInitialized; }
    void setPluginInitialized(bool init) { isInitialized = init; }

    double getHostBpm() const { return sampleEngine.getHostBpm(); }

    void loadSample(const juce::AudioBuffer<float>& buffer, double nativeSampleRate = 44100.0, int rootNote = 60)
    {
        sampleEngine.loadSample(buffer, nativeSampleRate, rootNote);
    }

    void noteOn(int midiNoteNumber, float velocity = 1.0f)
    {
        sampleEngine.noteOn(midiNoteNumber, velocity);
    }

    void noteOff(int midiNoteNumber = -1)
    {
        sampleEngine.noteOff(midiNoteNumber);
    }

    void playSample(int midiNoteNumber = 60, float velocity = 1.0f)
    {
        DBG("Processor Play");
        noteOn(midiNoteNumber, velocity);
    }

    void stopSample(int midiNoteNumber = -1)
    {
        noteOff(midiNoteNumber);
    }

    bool isPlaying() const
    {
        return sampleEngine.isPlaying();
    }

    double getPlayheadPosition() const
    {
        return sampleEngine.getPlayPositionNormalized();
    }

    juce::Array<float> getActiveVoicePositions() const
    {
        return sampleEngine.getActiveVoicePositionsNormalized();
    }

    juce::Array<ActiveVoiceVisualInfo> getActiveVoiceVisualInfos() const
    {
        return sampleEngine.getActiveVoiceVisualInfos();
    }

    PlaybackMode getPlaybackMode() const
    {
        return sampleEngine.getPlaybackMode();
    }

    void rerollRandomDirection()
    {
        sampleEngine.rerollRandomDirection();
    }

    bool isPolyMode() const
    {
        return sampleEngine.getPolyMode();
    }

    double getRegionStart() const
    {
        return sampleEngine.getRegionStartNormalized();
    }

    double getRegionEnd() const
    {
        return sampleEngine.getRegionEndNormalized();
    }

    void setFrequencyFilter(bool enabled, float minFreq, float maxFreq)
    {
        sampleEngine.setFrequencyFilter(enabled, minFreq, maxFreq);
    }

    void setFrequencyFilterBands(const juce::Array<FrequencyBand>& bands)
    {
        sampleEngine.setFrequencyFilterBands(bands);
    }

    void setSpectralRegions(const juce::Array<SpectralRegion>& regions)
    {
        sampleEngine.setSpectralRegions(regions);
    }

    SampleEngine& getSampleEngine() { return sampleEngine; }
    EffectsEngine& getEffectsEngine() { return effectsEngine; }
    HistoryManager& getHistoryManager() { return historyManager; }

    bool checkpointHistoryState(const juce::String& label, const juce::AudioBuffer<float>* audioBuffer = nullptr, double sampleRate = 44100.0)
    {
        return historyManager.pushHistoryState(
            label,
            loadedSampleFileName,
            apvts,
            startRegionNormalized,
            endRegionNormalized,
            loopEnabled,
            selectionsVar,
            audioBuffer != nullptr ? *audioBuffer : juce::AudioBuffer<float>(),
            sampleRate
        );
    }

    void resetParametersToDefault();

    juce::AudioProcessorValueTreeState& getAPVTS() { return apvts; }
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

private:
    SampleEngine sampleEngine;
    EffectsEngine effectsEngine;
    HistoryManager historyManager;
    juce::AudioProcessorValueTreeState apvts;
    juce::LinearSmoothedValue<float> masterGainSmoother;

    juce::File loadedSampleFile;
    juce::String loadedSampleFileName;
    bool sampleLoaded = false;

    float startRegionNormalized = 0.0f;
    float endRegionNormalized = 1.0f;
    bool loopEnabled = false;

    juce::var selectionsVar;
    juce::String currentPresetName = "Custom / Unsaved";
    bool isInitialized = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VancespectralAudioProcessor)
};
