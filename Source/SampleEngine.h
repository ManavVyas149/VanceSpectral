#pragma once

#include <JuceHeader.h>
#include "EnvelopeData.h"

class SampleEngine
{
public:
    SampleEngine();

    void loadSample(const juce::AudioBuffer<float>& buffer);

    void prepare(double sampleRate);

    void process(juce::AudioBuffer<float>& output,
                 int startSample,
                 int numSamples);

    void play();
    void stop();

    void setLoop(bool shouldLoop);

    void setRegion(float start, float end);

    bool isPlaying() const;

    void updateAmpADSR(float attack, float decay, float sustain, float release);
    void updateFilterADSR(float attack, float decay, float sustain, float release);

private:
    juce::CriticalSection lock;

    juce::AudioBuffer<float> sample;

    double currentSample = 0.0;

    double sampleRate = 44100.0;

    bool playing = false;

    bool looping = false;

    int regionStart = 0;

    int regionEnd = 0;

    // Envelopes
    EnvelopeData ampEnvelope{EnvelopeCategory::AmplifierEnvelope};
    EnvelopeData filterEnvelope{EnvelopeCategory::FilterEnvelope};

    // Filter DSP states for stereo channels
    float filterStateL = 0.0f;
    float filterStateR = 0.0f;
};