#pragma once

#include <JuceHeader.h>

enum class EnvelopeCategory
{
    AmplifierEnvelope
};

class EnvelopeData
{
public:
    EnvelopeData(EnvelopeCategory category = EnvelopeCategory::AmplifierEnvelope);

    void prepareToPlay(double sampleRate);
    void updateADSR(float attack, float decay, float sustain, float release);
    
    void noteOn();
    void noteOff();
    void reset();

    float getNextSample();
    float getCurrentLevel() const { return currentLevel; }
    void applyEnvelopeToBuffer(juce::AudioBuffer<float>& buffer, int startSample, int numSamples);
    
    bool isActive() const;
    EnvelopeCategory getCategory() const { return envelopeCategory; }

private:
    EnvelopeCategory envelopeCategory;
    juce::ADSR adsr;
    juce::ADSR::Parameters adsrParams;
    float currentLevel = 0.0f;
};
