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

struct ActiveVoiceVisualInfo {
    float positionNorm = 0.0f;
    float envLevel = 1.0f;
    bool isRandomMode = false;
    juce::Array<SpectralRegion> randomRegions;
};

struct VoiceRegionFilter {
    float startNorm = 0.0f;
    float endNorm = 1.0f;
    float minFreq = 20.0f;
    float maxFreq = 20000.0f;
    juce::IIRFilter hpL1, hpL2, hpR1, hpR2;
    juce::IIRFilter lpL1, lpL2, lpR1, lpR2;
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
    PlaybackMode getPlaybackMode() const { return playbackMode.load(); }
    void rerollRandomDirection();
    PlaybackMode getRandomChosenDirection() const { return randomChosenDirection.load(); }
    void setPitchMode(int modeIndex);
    void setPitchSemitones(float semitones);
    void setHostBpm(double bpm);

    bool isPlaying() const;
    double getPlayPositionNormalized() const;
    juce::Array<float> getActiveVoicePositionsNormalized() const;
    juce::Array<ActiveVoiceVisualInfo> getActiveVoiceVisualInfos() const;
    double getRegionStartNormalized() const;
    double getRegionEndNormalized() const;

    void updateAmpADSR(float attack, float decay, float sustain, float release);
    void updateFilterADSR(float attack, float decay, float sustain, float release);

    void setFrequencyFilter(bool enabled, float minFreq, float maxFreq);
    void setFrequencyFilterBands(const juce::Array<FrequencyBand>& bands);
    void setSpectralRegions(const juce::Array<SpectralRegion>& regions);

    void setTimbreSemitones(float semitones);
    void setTimbreLink(bool linked);
    void setTimbreDrift(float amount);

    void setExciterAmount(float amount);
    void setPolyMode(bool isPoly);
    bool getPolyMode() const { return polyMode.load(); }
    void setGlideTime(float timeMs);

    static constexpr int MAX_VOICES = 32;

    struct Voice
    {
        bool active = false;
        bool releasing = false;
        bool isQuickFadingOut = false;
        int quickFadeOutSamplesLeft = 0;
        int quickFadeOutTotalSamples = 0;

        int noteNumber = 60;
        float velocity = 1.0f;
        uint64_t voiceAge = 0;

        float currentPitchSemitones = 0.0f;
        float targetPitchSemitones = 0.0f;
        float timbreDriftOffset = 0.0f;

        double currentSample = 0.0;
        bool playDirectionForward = true;
        PlaybackMode effectivePlaybackMode = PlaybackMode::Forward;
        juce::Array<SpectralRegion> randomSpectralRegions;
        juce::OwnedArray<VoiceRegionFilter> voiceFilters;

        int randomGrainCounter = 0;
        double pitchPhase = 0.0;
        int samplesProcessed = 0;

        EnvelopeData ampEnvelope{ EnvelopeCategory::AmplifierEnvelope };
        EnvelopeData filterEnvelope{ EnvelopeCategory::FilterEnvelope };

        float filterStateL = 0.0f;
        float filterStateR = 0.0f;

        void startQuickFadeOut(int totalSamples)
        {
            if (active && !isQuickFadingOut)
            {
                isQuickFadingOut = true;
                quickFadeOutTotalSamples = juce::jmax(1, totalSamples);
                quickFadeOutSamplesLeft = quickFadeOutTotalSamples;
            }
        }

        void reset()
        {
            active = false;
            releasing = false;
            isQuickFadingOut = false;
            quickFadeOutSamplesLeft = 0;
            quickFadeOutTotalSamples = 0;
            noteNumber = 60;
            velocity = 1.0f;
            currentPitchSemitones = 0.0f;
            targetPitchSemitones = 0.0f;
            timbreDriftOffset = 0.0f;
            voiceAge = 0;
            currentSample = 0.0;
            playDirectionForward = true;
            effectivePlaybackMode = PlaybackMode::Forward;
            randomSpectralRegions.clear();
            voiceFilters.clear();
            randomGrainCounter = 0;
            pitchPhase = 0.0;
            samplesProcessed = 0;
            filterStateL = 0.0f;
            filterStateR = 0.0f;
            ampEnvelope.reset();
            filterEnvelope.reset();
        }
    };

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
    juce::AudioBuffer<float> filteredSample; // Pre-filtered source audio (spectral regions/bands applied first)

    std::array<Voice, MAX_VOICES> voices;
    std::atomic<bool> polyMode{ false };
    std::atomic<float> glideTimeMs{ 0.0f };
    uint64_t voiceAgeCounter = 0;

    float lastMonoPitchSemitones = 0.0f;
    float lastPolyPitchSemitones = 0.0f;
    uint64_t lastNoteTriggerSample = 0;
    uint64_t globalSampleCounter = 0;
    float smoothedPolyGainScale = 1.0f;

    void updateFilteredSample();
    void initVoice(Voice& v, int noteNumber, float velocity);

    std::atomic<double> targetSampleRate{ 44100.0 };
    std::atomic<double> nativeSampleRate{ 44100.0 };
    std::atomic<double> hostBpm{ 120.0 };

    std::atomic<bool> playing{ false };
    std::atomic<bool> looping{ false };

    std::atomic<int> regionStart{ 0 };
    std::atomic<int> regionEnd{ 0 };

    std::atomic<PlaybackMode> playbackMode{ PlaybackMode::Forward };
    std::atomic<PlaybackMode> randomChosenDirection{ PlaybackMode::Forward };
    std::atomic<PitchMode> pitchMode{ PitchMode::Stretch };

    // Pitch Tracking & Pitch Shifter States
    std::atomic<int> currentNoteNumber{ 60 };
    std::atomic<float> pitchSemitones{ 0.0f };
    std::atomic<float> timbreSemitones{ 0.0f };
    std::atomic<bool> timbreLink{ true };
    std::atomic<float> timbreDriftAmount{ 0.0f };

    // Exciter state
    std::atomic<float> exciterAmount{ 0.0f };

    // Spectrogram Selection Multi-Bandpass Filter
    std::atomic<bool> freqFilterEnabled{ false };
    juce::Array<FrequencyBand> filterBands;

    // Time & Frequency Spectral Region Isolation Filters
    juce::Array<SpectralRegion> spectralRegions;
};