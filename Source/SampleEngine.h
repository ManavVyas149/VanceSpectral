#pragma once

#include <JuceHeader.h>
#include "EnvelopeData.h"

enum class PlaybackMode { Forward = 0, Backward, ForwBackw, BackForw, Random };
enum class PitchMode { Stretch = 0, Resample, Axial };

struct FrequencyBand {
    float minFreq = 20.0f;
    float maxFreq = 20000.0f;
};

class SampleEngine
{
public:
    SampleEngine();

    void loadSample(const juce::AudioBuffer<float>& buffer, double nativeSampleRate = 44100.0);
    void prepare(double sampleRate);

    void process(juce::AudioBuffer<float>& output,
                 int startSample,
                 int numSamples);

    void play();
    void stop();

    void setLoop(bool shouldLoop);
    void setRegion(float startNormalized, float endNormalized);

    void setPlaybackMode(int modeIndex);
    void setPitchMode(int modeIndex);
    void setHostBpm(double bpm);

    bool isPlaying() const;
    double getPlayPositionNormalized() const;
    double getRegionStartNormalized() const;
    double getRegionEndNormalized() const;

    void updateAmpADSR(float attack, float decay, float sustain, float release);
    void updateFilterADSR(float attack, float decay, float sustain, float release);

    void setFrequencyFilter(bool enabled, float minFreq, float maxFreq);
    void setFrequencyFilterBands(const juce::Array<FrequencyBand>& bands);

private:
    struct BandFilter
    {
        juce::IIRFilter hpL, hpR;
        juce::IIRFilter lpL, lpR;
    };

    juce::CriticalSection lock;

    juce::AudioBuffer<float> sample;
    double currentSample = 0.0;
    double targetSampleRate = 44100.0;
    double nativeSampleRate = 44100.0;
    double hostBpm = 120.0;

    bool playing = false;
    bool looping = false;

    int regionStart = 0;
    int regionEnd = 0;

    PlaybackMode playbackMode = PlaybackMode::Forward;
    PitchMode pitchMode = PitchMode::Stretch;

    bool playDirectionForward = true;
    int randomGrainCounter = 0;

    // Envelopes
    EnvelopeData ampEnvelope{EnvelopeCategory::AmplifierEnvelope};
    EnvelopeData filterEnvelope{EnvelopeCategory::FilterEnvelope};

    // Filter DSP states for stereo channels
    float filterStateL = 0.0f;
    float filterStateR = 0.0f;

    // Spectrogram Selection Multi-Bandpass Filter
    bool freqFilterEnabled = false;
    juce::Array<FrequencyBand> filterBands;
    juce::OwnedArray<BandFilter> bandFilters;
};