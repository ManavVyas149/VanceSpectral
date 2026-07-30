#pragma once

#include <JuceHeader.h>
#include <atomic>
#include "EnvelopeData.h"

enum class PlaybackMode { Forward = 0, Backward, ForwBackw, BackForw, Random };
enum class PitchMode { Stretch = 0, Resample, Axial };

struct FrequencyBand {
    float minFreq = 20.0f;
    float maxFreq = 20000.0f;
};

struct SpectralRegion {
    int id = 1;
    float startNorm = 0.0f;
    float endNorm = 1.0f;
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

    void noteOn(int midiNoteNumber, float velocity = 1.0f);
    void noteOff(int midiNoteNumber);

    void setLoop(bool shouldLoop);
    void setRegion(float startNormalized, float endNormalized);

    void setPlaybackMode(int modeIndex);
    void setPitchMode(int modeIndex);
    void setPitchSemitones(float semitones);
    void setHostBpm(double bpm);

    bool isPlaying() const;
    double getPlayPositionNormalized() const;
    double getRegionStartNormalized() const;
    double getRegionEndNormalized() const;

    void updateAmpADSR(float attack, float decay, float sustain, float release);
    void updateFilterADSR(float attack, float decay, float sustain, float release);

    void setFrequencyFilter(bool enabled, float minFreq, float maxFreq);
    void setFrequencyFilterBands(const juce::Array<FrequencyBand>& bands);
    void setSpectralRegions(const juce::Array<SpectralRegion>& regions);

    void setExciterAmount(float amount);

private:
    struct BandFilter
    {
        juce::IIRFilter hpL1, hpL2, hpR1, hpR2;
        juce::IIRFilter lpL1, lpL2, lpR1, lpR2;
    };

    struct RegionFilterPair {
        SpectralRegion region;
        juce::IIRFilter hpL1, hpL2, hpR1, hpR2;
        juce::IIRFilter lpL1, lpL2, lpR1, lpR2;
    };

    juce::CriticalSection lock;

    juce::AudioBuffer<float> sample;
    std::atomic<double> currentSample{ 0.0 };
    std::atomic<double> targetSampleRate{ 44100.0 };
    std::atomic<double> nativeSampleRate{ 44100.0 };
    std::atomic<double> hostBpm{ 120.0 };

    std::atomic<bool> playing{ false };
    std::atomic<bool> looping{ false };

    std::atomic<int> regionStart{ 0 };
    std::atomic<int> regionEnd{ 0 };

    std::atomic<PlaybackMode> playbackMode{ PlaybackMode::Forward };
    std::atomic<PitchMode> pitchMode{ PitchMode::Stretch };

    std::atomic<bool> playDirectionForward{ true };
    std::atomic<int> randomGrainCounter{ 0 };

    // Pitch Tracking & Pitch Shifter States
    std::atomic<int> currentNoteNumber{ 60 };
    std::atomic<float> pitchSemitones{ 0.0f };
    std::atomic<float> currentPitchRatio{ 1.0f };
    std::atomic<double> pitchPhase{ 0.0 };

    void updatePitchRatio();

    // Envelopes
    EnvelopeData ampEnvelope{EnvelopeCategory::AmplifierEnvelope};
    EnvelopeData filterEnvelope{EnvelopeCategory::FilterEnvelope};

    // Filter DSP states for stereo channels
    float filterStateL = 0.0f;
    float filterStateR = 0.0f;

    // Exciter state
    std::atomic<float> exciterAmount{ 0.0f };

    // Spectrogram Selection Multi-Bandpass Filter
    std::atomic<bool> freqFilterEnabled{ false };
    juce::Array<FrequencyBand> filterBands;
    juce::OwnedArray<BandFilter> bandFilters;

    // Time & Frequency Spectral Region Isolation Filters
    juce::Array<SpectralRegion> spectralRegions;
    juce::OwnedArray<RegionFilterPair> regionFilterPairs;
};