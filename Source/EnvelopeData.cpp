#include "EnvelopeData.h"

EnvelopeData::EnvelopeData(EnvelopeCategory category)
    : envelopeCategory(category)
{
    adsrParams.attack = 0.1f;
    adsrParams.decay = 0.2f;
    adsrParams.sustain = 0.8f;
    adsrParams.release = 0.5f;
    adsr.setParameters(adsrParams);
}

void EnvelopeData::prepareToPlay(double sampleRate)
{
    adsr.setSampleRate(sampleRate);
}

void EnvelopeData::updateADSR(float attack, float decay, float sustain, float release)
{
    adsrParams.attack = attack;
    adsrParams.decay = decay;
    adsrParams.sustain = sustain;
    adsrParams.release = release;
    adsr.setParameters(adsrParams);
}

void EnvelopeData::noteOn()
{
    adsr.noteOn();
}

void EnvelopeData::noteOff()
{
    adsr.noteOff();
}

void EnvelopeData::reset()
{
    adsr.reset();
    currentLevel = 0.0f;
}

float EnvelopeData::getNextSample()
{
    currentLevel = adsr.getNextSample();
    return currentLevel;
}

void EnvelopeData::applyEnvelopeToBuffer(juce::AudioBuffer<float>& buffer, int startSample, int numSamples)
{
    adsr.applyEnvelopeToBuffer(buffer, startSample, numSamples);
}

bool EnvelopeData::isActive() const
{
    return adsr.isActive();
}
