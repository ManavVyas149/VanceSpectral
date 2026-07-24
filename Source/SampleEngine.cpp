#include "SampleEngine.h"

SampleEngine::SampleEngine()
{
}

void SampleEngine::prepare(double sr)
{
    sampleRate = sr;
}

void SampleEngine::loadSample(const juce::AudioBuffer<float>& buffer)
{
    sample = buffer;

    regionStart = 0;
    regionEnd = sample.getNumSamples();
    currentSample = 0;

    DBG("Sample Loaded");
    DBG(sample.getNumSamples());
}

void SampleEngine::play()
{
    DBG("Engine Play");

    currentSample = regionStart;
    playing = true;
}
void SampleEngine::stop()
{
    DBG("Stop");

    playing = false;
}

bool SampleEngine::isPlaying() const
{
    return playing;
}

void SampleEngine::setLoop(bool shouldLoop)
{
    looping = shouldLoop;
}

void SampleEngine::setRegion(float start,float end)
{
    regionStart = (int)(start * sample.getNumSamples());

    regionEnd = (int)(end * sample.getNumSamples());

    regionStart = juce::jlimit(0,
                               sample.getNumSamples()-1,
                               regionStart);

    regionEnd = juce::jlimit(regionStart+1,
                             sample.getNumSamples(),
                             regionEnd);
}

void SampleEngine::process(juce::AudioBuffer<float>& output,
                           int startSample,
                           int numSamples)
{
    DBG("Output channels: " << output.getNumChannels());
    DBG("Output samples: " << output.getNumSamples());
    
    if (!playing)
        return;

    if (sample.getNumSamples() == 0)
        return;

    auto channels = juce::jmin(output.getNumChannels(),
                               sample.getNumChannels());

    for (int i = 0; i < numSamples; ++i)
    {
        if (currentSample >= regionEnd)
        {
            if (looping)
            {
                currentSample = regionStart;
            }
            else
            {
                playing = false;
                break;
            }
        }

        float s = sample.getSample(0, (int) currentSample);

        // Send mono sample to every output channel
        for (int ch = 0; ch < output.getNumChannels(); ++ch)
        {
            output.addSample(
                ch,
                startSample + i,
                s
            );
            
            if (i == 0 && ch == 0)
                DBG("Sample value: " << s);
        }

        ++currentSample;
    }
}