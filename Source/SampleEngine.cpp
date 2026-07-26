#include "SampleEngine.h"
#include <cmath>

SampleEngine::SampleEngine()
{
}

void SampleEngine::prepare(double sr)
{
    const juce::ScopedLock sl(lock);
    sampleRate = sr;
    ampEnvelope.prepareToPlay(sr);
    filterEnvelope.prepareToPlay(sr);
}

void SampleEngine::loadSample(const juce::AudioBuffer<float>& buffer)
{
    const juce::ScopedLock sl(lock);

    sample = buffer;

    regionStart = 0;
    regionEnd = sample.getNumSamples();
    currentSample = 0;

    filterStateL = 0.0f;
    filterStateR = 0.0f;

    DBG("Sample Loaded");
    DBG(sample.getNumSamples());
}

void SampleEngine::play()
{
    const juce::ScopedLock sl(lock);
    DBG("Engine Play");

    currentSample = regionStart;
    playing = true;

    filterStateL = 0.0f;
    filterStateR = 0.0f;

    ampEnvelope.noteOn();
    filterEnvelope.noteOn();
}

void SampleEngine::stop()
{
    const juce::ScopedLock sl(lock);
    DBG("Stop");

    ampEnvelope.noteOff();
    filterEnvelope.noteOff();
    playing = false;
}

bool SampleEngine::isPlaying() const
{
    return playing || ampEnvelope.isActive();
}

void SampleEngine::setLoop(bool shouldLoop)
{
    const juce::ScopedLock sl(lock);
    looping = shouldLoop;
}

void SampleEngine::setRegion(float start, float end)
{
    const juce::ScopedLock sl(lock);

    if (sample.getNumSamples() == 0)
        return;

    regionStart = (int)(start * sample.getNumSamples());
    regionEnd = (int)(end * sample.getNumSamples());

    regionStart = juce::jlimit(0,
                               sample.getNumSamples() - 1,
                               regionStart);

    regionEnd = juce::jlimit(regionStart + 1,
                             sample.getNumSamples(),
                             regionEnd);
}

void SampleEngine::updateAmpADSR(float attack, float decay, float sustain, float release)
{
    const juce::ScopedLock sl(lock);
    ampEnvelope.updateADSR(attack, decay, sustain, release);
}

void SampleEngine::updateFilterADSR(float attack, float decay, float sustain, float release)
{
    const juce::ScopedLock sl(lock);
    filterEnvelope.updateADSR(attack, decay, sustain, release);
}

void SampleEngine::process(juce::AudioBuffer<float>& output,
                            int startSample,
                            int numSamples)
{
    const juce::ScopedTryLock sl(lock);
    if (!sl.isLocked())
        return;

    if (!playing && !ampEnvelope.isActive())
        return;

    if (sample.getNumSamples() == 0)
        return;

    auto numSampleChannels = sample.getNumChannels();
    if (numSampleChannels == 0)
        return;

    auto totalSamples = sample.getNumSamples();

    for (int i = 0; i < numSamples; ++i)
    {
        if (playing && currentSample >= regionEnd)
        {
            if (looping)
            {
                currentSample = regionStart;
            }
            else
            {
                playing = false;
                ampEnvelope.noteOff();
                filterEnvelope.noteOff();
            }
        }

        float ampVal = ampEnvelope.getNextSample();
        float filterVal = filterEnvelope.getNextSample();

        float rawSample = 0.0f;
        int readIndex = (int)currentSample;
        if (readIndex >= 0 && readIndex < totalSamples && numSampleChannels > 0)
        {
            rawSample = sample.getSample(0, readIndex);
        }

        // Calculate dynamic low-pass filter cutoff modulated by filter envelope
        float cutoff = 20.0f + filterVal * 18000.0f;
        float alpha = juce::jlimit(0.01f, 0.99f, 1.0f - std::exp(-2.0f * juce::MathConstants<float>::pi * cutoff / (float)sampleRate));

        filterStateL += alpha * (rawSample - filterStateL);
        filterStateR += alpha * (rawSample - filterStateR);

        float outSampleL = filterStateL * ampVal;
        float outSampleR = filterStateR * ampVal;

        if (output.getNumChannels() > 0)
            output.addSample(0, startSample + i, outSampleL);

        if (output.getNumChannels() > 1)
            output.addSample(1, startSample + i, outSampleR);

        if (playing)
            ++currentSample;
    }
}