#pragma once

#include <JuceHeader.h>

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

private:

    juce::AudioBuffer<float> sample;

    double currentSample = 0.0;

    double sampleRate = 44100.0;

    bool playing = false;

    bool looping = false;

    int regionStart = 0;

    int regionEnd = 0;
};