/*
  ==============================================================================

    EffectsEngine.h
    Created: 6 Sep 2026
    Author:  Manav Vyas / VanceSpectral Team

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "Airwindows/AirwindowsSpiral.h"
#include "Airwindows/AirwindowsChorus.h"

//==============================================================================
/**
    5-Stage Master Wet Effects Chain:
    1. Sidechain: Internal rhythmic volume-ducking envelope pump (Rate & Mix)
    2. Chorus: juce::dsp::Chorus layered with Airwindows modulation character
    3. Phaser: juce::dsp::Phaser
    4. Delay: juce::dsp::DelayLine with feedback and wet/dry mix
    5. Drive: Airwindows Spiral nonlinear saturation with 2x oversampling
*/
class EffectsEngine
{
public:
    EffectsEngine();
    ~EffectsEngine();

    void prepare(double sampleRate, int maxBlockSize);
    void reset();

    // Process stereo audio buffer in-place
    void process(juce::AudioBuffer<float>& buffer);

    // Parameter Setters with smoothing
    void setSidechainEnabled(bool enabled);
    void setSidechainMix(float mix01);
    void setSidechainRate(float rateHz);

    void setChorusEnabled(bool enabled);
    void setChorusAmount(float amount01);
    void setChorusRate(float rateHz);

    void setPhaserEnabled(bool enabled);
    void setPhaserAmount(float amount01);
    void setPhaserRate(float rateHz);

    void setDelayEnabled(bool enabled);
    void setDelayAmount(float amount01);
    void setDelayTime(float timeMs);
    void setDelayFeedback(float feedback01);

    void setDriveEnabled(bool enabled);
    void setDriveAmount(float amount01);
    void setDriveTone(float tone01);

private:
    double currentSampleRate{ 44100.0 };
    int maxExpectedBlockSize{ 512 };

    // DSP Processors & State
    double sidechainPhase{ 0.0 };
    juce::dsp::Chorus<float> juceChorus;
    Airwindows::ChorusEnsemble airChorus;
    juce::dsp::Phaser<float> jucePhaser;
    juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Linear> delayLine{ 96000 };
    Airwindows::Spiral airDrive;
    std::unique_ptr<juce::dsp::Oversampling<float>> oversampler;

    // Smoothed Bypass crossfaders (15ms fade for click-safety and true zero-CPU bypass)
    juce::LinearSmoothedValue<float> sidechainBypassGain;
    juce::LinearSmoothedValue<float> chorusBypassGain;
    juce::LinearSmoothedValue<float> phaserBypassGain;
    juce::LinearSmoothedValue<float> delayBypassGain;
    juce::LinearSmoothedValue<float> driveBypassGain;

    // Smoothed continuous parameter values
    juce::LinearSmoothedValue<float> sidechainMix;
    juce::LinearSmoothedValue<float> sidechainRate;
    juce::LinearSmoothedValue<float> chorusMix;
    juce::LinearSmoothedValue<float> chorusRate;
    juce::LinearSmoothedValue<float> phaserMix;
    juce::LinearSmoothedValue<float> phaserRate;
    juce::LinearSmoothedValue<float> delayMix;
    juce::LinearSmoothedValue<float> delayTimeSamples;
    juce::LinearSmoothedValue<float> delayFeedback;
    juce::LinearSmoothedValue<float> driveAmount;
    juce::LinearSmoothedValue<float> driveTone;

    // Delay internal state
    float delayFeedbackL{ 0.0f };
    float delayFeedbackR{ 0.0f };

    // Drive tone filter (1-pole lowpass)
    float driveFilterL{ 0.0f };
    float driveFilterR{ 0.0f };

    // Scratch buffers
    juce::AudioBuffer<float> tempEffectBuffer;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(EffectsEngine)
};
