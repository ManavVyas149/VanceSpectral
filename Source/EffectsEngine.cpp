/*
  ==============================================================================

    EffectsEngine.cpp
    Created: 6 Sep 2026
    Author:  Manav Vyas / VanceSpectral Team

  ==============================================================================
*/

#include "EffectsEngine.h"

EffectsEngine::EffectsEngine()
{
    // Initialize 2x oversampler with 2 stages (4x factor or 2x factor) for drive
    oversampler = std::make_unique<juce::dsp::Oversampling<float>>(2, 1, juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR, true, false);
}

EffectsEngine::~EffectsEngine()
{
}

void EffectsEngine::prepare(double sampleRate, int maxBlockSize)
{
    currentSampleRate = sampleRate > 0.0 ? sampleRate : 44100.0;
    maxExpectedBlockSize = maxBlockSize > 0 ? maxBlockSize : 512;

    juce::dsp::ProcessSpec spec;
    spec.sampleRate = currentSampleRate;
    spec.maximumBlockSize = (juce::uint32)maxExpectedBlockSize;
    spec.numChannels = 2;

    // 1. Sidechain
    sidechainPhase = 0.0;

    // 2. Chorus
    juceChorus.prepare(spec);
    juceChorus.setRate(1.0f);
    juceChorus.setDepth(0.3f);
    juceChorus.setCentreDelay(7.0f);
    juceChorus.setFeedback(0.0f);
    juceChorus.setMix(0.0f);
    airChorus.prepare(currentSampleRate);

    // 3. Phaser
    jucePhaser.prepare(spec);
    jucePhaser.setRate(0.5f);
    jucePhaser.setDepth(0.5f);
    jucePhaser.setCentreFrequency(1200.0f);
    jucePhaser.setFeedback(0.2f);
    jucePhaser.setMix(0.0f);

    // 4. Delay
    delayLine.setMaximumDelayInSamples((int)(currentSampleRate * 2.0)); // 2 seconds max
    delayLine.prepare(spec);
    delayFeedbackL = 0.0f;
    delayFeedbackR = 0.0f;

    // 5. Drive & Oversampling
    oversampler->initProcessing(maxExpectedBlockSize);
    airDrive.reset();
    driveFilterL = 0.0f;
    driveFilterR = 0.0f;

    // Smoothed bypass controllers (15ms crossfade ramp)
    sidechainBypassGain.reset(currentSampleRate, 0.015);
    chorusBypassGain.reset(currentSampleRate, 0.015);
    phaserBypassGain.reset(currentSampleRate, 0.015);
    delayBypassGain.reset(currentSampleRate, 0.015);
    driveBypassGain.reset(currentSampleRate, 0.015);

    sidechainBypassGain.setCurrentAndTargetValue(0.0f);
    chorusBypassGain.setCurrentAndTargetValue(0.0f);
    phaserBypassGain.setCurrentAndTargetValue(0.0f);
    delayBypassGain.setCurrentAndTargetValue(0.0f);
    driveBypassGain.setCurrentAndTargetValue(0.0f);

    // Continuous parameter smoothers (20ms smoothing to eliminate zipper noise)
    sidechainMix.reset(currentSampleRate, 0.02);
    sidechainMix.setCurrentAndTargetValue(0.0f);
    sidechainRate.reset(currentSampleRate, 0.02);
    sidechainRate.setCurrentAndTargetValue(2.0f);

    chorusMix.reset(currentSampleRate, 0.02);
    chorusMix.setCurrentAndTargetValue(0.0f);
    chorusRate.reset(currentSampleRate, 0.02);
    chorusRate.setCurrentAndTargetValue(1.0f);

    phaserMix.reset(currentSampleRate, 0.02);
    phaserMix.setCurrentAndTargetValue(0.0f);
    phaserRate.reset(currentSampleRate, 0.02);
    phaserRate.setCurrentAndTargetValue(0.5f);

    delayMix.reset(currentSampleRate, 0.02);
    delayMix.setCurrentAndTargetValue(0.0f);
    delayTimeSamples.reset(currentSampleRate, 0.05); // slightly longer smoothing on delay time to glide smoothly
    delayTimeSamples.setCurrentAndTargetValue((float)(currentSampleRate * 0.25)); // 250ms default
    delayFeedback.reset(currentSampleRate, 0.02);
    delayFeedback.setCurrentAndTargetValue(0.35f);

    driveAmount.reset(currentSampleRate, 0.02);
    driveAmount.setCurrentAndTargetValue(0.0f);
    driveTone.reset(currentSampleRate, 0.02);
    driveTone.setCurrentAndTargetValue(0.5f);

    tempEffectBuffer.setSize(2, maxExpectedBlockSize);
}

void EffectsEngine::reset()
{
    sidechainPhase = 0.0;
    juceChorus.reset();
    airChorus.reset();
    jucePhaser.reset();
    delayLine.reset();
    delayFeedbackL = 0.0f;
    delayFeedbackR = 0.0f;
    oversampler->reset();
    airDrive.reset();
    driveFilterL = 0.0f;
    driveFilterR = 0.0f;
}

void EffectsEngine::setSidechainEnabled(bool enabled)
{
    sidechainBypassGain.setTargetValue(enabled ? 1.0f : 0.0f);
}

void EffectsEngine::setSidechainMix(float mix01)
{
    sidechainMix.setTargetValue(juce::jlimit(0.0f, 1.0f, mix01));
}

void EffectsEngine::setSidechainRate(float rateHz)
{
    sidechainRate.setTargetValue(juce::jlimit(0.5f, 20.0f, rateHz));
}

void EffectsEngine::setChorusEnabled(bool enabled)
{
    chorusBypassGain.setTargetValue(enabled ? 1.0f : 0.0f);
}

void EffectsEngine::setChorusAmount(float amount01)
{
    chorusMix.setTargetValue(juce::jlimit(0.0f, 1.0f, amount01));
}

void EffectsEngine::setChorusRate(float rateHz)
{
    chorusRate.setTargetValue(juce::jlimit(0.1f, 5.0f, rateHz));
}

void EffectsEngine::setPhaserEnabled(bool enabled)
{
    phaserBypassGain.setTargetValue(enabled ? 1.0f : 0.0f);
}

void EffectsEngine::setPhaserAmount(float amount01)
{
    phaserMix.setTargetValue(juce::jlimit(0.0f, 1.0f, amount01));
}

void EffectsEngine::setPhaserRate(float rateHz)
{
    phaserRate.setTargetValue(juce::jlimit(0.05f, 4.0f, rateHz));
}

void EffectsEngine::setDelayEnabled(bool enabled)
{
    delayBypassGain.setTargetValue(enabled ? 1.0f : 0.0f);
}

void EffectsEngine::setDelayAmount(float amount01)
{
    delayMix.setTargetValue(juce::jlimit(0.0f, 1.0f, amount01));
}

void EffectsEngine::setDelayTime(float timeMs)
{
    float ms = juce::jlimit(10.0f, 1000.0f, timeMs);
    float samples = (float)((ms / 1000.0) * currentSampleRate);
    delayTimeSamples.setTargetValue(samples);
}

void EffectsEngine::setDelayFeedback(float feedback01)
{
    delayFeedback.setTargetValue(juce::jlimit(0.0f, 0.85f, feedback01));
}

void EffectsEngine::setDriveEnabled(bool enabled)
{
    driveBypassGain.setTargetValue(enabled ? 1.0f : 0.0f);
}

void EffectsEngine::setDriveAmount(float amount01)
{
    driveAmount.setTargetValue(juce::jlimit(0.0f, 1.0f, amount01));
}

void EffectsEngine::setDriveTone(float tone01)
{
    driveTone.setTargetValue(juce::jlimit(0.0f, 1.0f, tone01));
}

void EffectsEngine::process(juce::AudioBuffer<float>& buffer)
{
    const int numSamples = buffer.getNumSamples();
    const int numChannels = buffer.getNumChannels();

    if (numSamples == 0 || numChannels < 2)
        return;

    if (tempEffectBuffer.getNumSamples() < numSamples)
        tempEffectBuffer.setSize(2, numSamples, false, false, true);

    // =========================================================================
    // Stage 1: Sidechain (Internal Rhythmic Volume-Ducking Pump)
    // =========================================================================
    bool isSidechainActive = sidechainBypassGain.isSmoothing() || sidechainBypassGain.getCurrentValue() > 0.001f;
    if (isSidechainActive)
    {
        float* left = buffer.getWritePointer(0);
        float* right = buffer.getWritePointer(1);

        for (int i = 0; i < numSamples; ++i)
        {
            float bypassG = sidechainBypassGain.getNextValue();
            float mixVal = sidechainMix.getNextValue();
            float rateHz = sidechainRate.getNextValue();

            // Advance phase accumulator continuously
            double phaseInc = (double)rateHz / currentSampleRate;
            sidechainPhase += phaseInc;
            if (sidechainPhase >= 1.0)
                sidechainPhase -= 1.0;

            // Calculate ducking envelope E(phase) in [0.0, 1.0]
            // At phase = 0.0 (downbeat start), fast smooth dip; smooth exponential recovery to 1.0
            float env = 1.0f;
            constexpr double attackWindow = 0.04; // 4% of cycle for fast click-free attack dip

            if (sidechainPhase < attackWindow)
            {
                // Smooth cosine dip from 1.0 down to 0.0
                double normAtt = sidechainPhase / attackWindow;
                env = static_cast<float>(0.5 * (1.0 + std::cos(juce::MathConstants<double>::pi * normAtt)));
            }
            else
            {
                // Smooth exponential recovery from 0.0 back to 1.0
                double normRec = (sidechainPhase - attackWindow) / (1.0 - attackWindow);
                // Shaped curve: (1 - exp(-4 * normRec)) / (1 - exp(-4))
                constexpr double expFactor = 4.0;
                double denominator = 1.0 - std::exp(-expFactor);
                env = static_cast<float>((1.0 - std::exp(-expFactor * normRec)) / denominator);
            }

            // Effective gain for ducking with wet/dry mix
            // mix = 0.0 -> gain = 1.0 (bit-identical dry)
            // mix = 1.0 -> gain = env (full ducking)
            float duckGain = 1.0f - (mixVal * (1.0f - env));

            // Crossfade with bypass gain
            float finalGain = (1.0f - bypassG) + (bypassG * duckGain);

            left[i] *= finalGain;
            right[i] *= finalGain;
        }
    }
    else
    {
        sidechainBypassGain.skip(numSamples);
        sidechainMix.skip(numSamples);
        sidechainRate.skip(numSamples);
    }

    // =========================================================================
    // Stage 2: Chorus (JUCE Chorus + Airwindows Character Layer)
    // =========================================================================
    bool isChorusActive = chorusBypassGain.isSmoothing() || chorusBypassGain.getCurrentValue() > 0.001f;
    if (isChorusActive)
    {
        float cRate = chorusRate.getNextValue();
        float cMix = chorusMix.getNextValue();

        juceChorus.setRate(cRate);
        juceChorus.setMix(cMix);

        tempEffectBuffer.copyFrom(0, 0, buffer.getReadPointer(0), numSamples);
        tempEffectBuffer.copyFrom(1, 0, buffer.getReadPointer(1), numSamples);

        // 1. Process JUCE chorus
        juce::dsp::AudioBlock<float> block(tempEffectBuffer.getArrayOfWritePointers(), 2, 0, numSamples);
        juce::dsp::ProcessContextReplacing<float> context(block);
        juceChorus.process(context);

        // 2. Airwindows ensemble layer for richer texture
        airChorus.processStereo(tempEffectBuffer.getWritePointer(0),
                                tempEffectBuffer.getWritePointer(1),
                                numSamples, cRate, cMix * 0.7f, cMix * 0.5f);

        // Crossfade dry and chorused
        for (int i = 0; i < numSamples; ++i)
        {
            float g = chorusBypassGain.getNextValue();
            buffer.setSample(0, i, buffer.getSample(0, i) * (1.0f - g) + tempEffectBuffer.getSample(0, i) * g);
            buffer.setSample(1, i, buffer.getSample(1, i) * (1.0f - g) + tempEffectBuffer.getSample(1, i) * g);
        }
    }
    else
    {
        chorusBypassGain.skip(numSamples);
        chorusMix.skip(numSamples);
        chorusRate.skip(numSamples);
    }

    // =========================================================================
    // Stage 3: Phaser (juce::dsp::Phaser)
    // =========================================================================
    bool isPhaserActive = phaserBypassGain.isSmoothing() || phaserBypassGain.getCurrentValue() > 0.001f;
    if (isPhaserActive)
    {
        float pRate = phaserRate.getNextValue();
        float pMix = phaserMix.getNextValue();

        jucePhaser.setRate(pRate);
        jucePhaser.setMix(pMix);

        tempEffectBuffer.copyFrom(0, 0, buffer.getReadPointer(0), numSamples);
        tempEffectBuffer.copyFrom(1, 0, buffer.getReadPointer(1), numSamples);

        juce::dsp::AudioBlock<float> block(tempEffectBuffer.getArrayOfWritePointers(), 2, 0, numSamples);
        juce::dsp::ProcessContextReplacing<float> context(block);
        jucePhaser.process(context);

        for (int i = 0; i < numSamples; ++i)
        {
            float g = phaserBypassGain.getNextValue();
            buffer.setSample(0, i, buffer.getSample(0, i) * (1.0f - g) + tempEffectBuffer.getSample(0, i) * g);
            buffer.setSample(1, i, buffer.getSample(1, i) * (1.0f - g) + tempEffectBuffer.getSample(1, i) * g);
        }
    }
    else
    {
        phaserBypassGain.skip(numSamples);
        phaserMix.skip(numSamples);
        phaserRate.skip(numSamples);
    }

    // =========================================================================
    // Stage 4: Delay Line with Feedback & Wet/Dry Mix
    // =========================================================================
    bool isDelayActive = delayBypassGain.isSmoothing() || delayBypassGain.getCurrentValue() > 0.001f;
    if (isDelayActive)
    {
        float* left = buffer.getWritePointer(0);
        float* right = buffer.getWritePointer(1);

        for (int i = 0; i < numSamples; ++i)
        {
            float bypassG = delayBypassGain.getNextValue();
            float wetMix = delayMix.getNextValue();
            float delaySamples = delayTimeSamples.getNextValue();
            float fb = delayFeedback.getNextValue();

            delayLine.setDelay(delaySamples);

            float inL = left[i];
            float inR = right[i];

            // Read delayed samples
            float delayedL = delayLine.popSample(0);
            float delayedR = delayLine.popSample(1);

            // Push input + soft-clipped feedback into delay line
            float nextDelayInL = inL + std::tanh(delayedL * fb);
            float nextDelayInR = inR + std::tanh(delayedR * fb);

            delayLine.pushSample(0, nextDelayInL);
            delayLine.pushSample(1, nextDelayInR);

            float wetL = inL * (1.0f - (wetMix * 0.5f)) + delayedL * wetMix;
            float wetR = inR * (1.0f - (wetMix * 0.5f)) + delayedR * wetMix;

            left[i] = inL * (1.0f - bypassG) + wetL * bypassG;
            right[i] = inR * (1.0f - bypassG) + wetR * bypassG;
        }
    }
    else
    {
        delayBypassGain.skip(numSamples);
        delayMix.skip(numSamples);
        delayTimeSamples.skip(numSamples);
        delayFeedback.skip(numSamples);
    }

    // =========================================================================
    // Stage 5: Drive (Airwindows Spiral + 2x Oversampling)
    // =========================================================================
    bool isDriveActive = driveBypassGain.isSmoothing() || driveBypassGain.getCurrentValue() > 0.001f;
    if (isDriveActive)
    {
        tempEffectBuffer.copyFrom(0, 0, buffer.getReadPointer(0), numSamples);
        tempEffectBuffer.copyFrom(1, 0, buffer.getReadPointer(1), numSamples);

        juce::dsp::AudioBlock<float> baseBlock(tempEffectBuffer.getArrayOfWritePointers(), 2, 0, numSamples);
        juce::dsp::AudioBlock<float> oversampledBlock = oversampler->processSamplesUp(baseBlock);

        float dAmt = driveAmount.getNextValue();
        float tone = driveTone.getNextValue();

        // 1-pole filter coefficient for Tone control (dark warmth to bright bite)
        float filterCoeff = 0.2f + (tone * 0.75f);

        int osNumSamples = (int)oversampledBlock.getNumSamples();
        float* osLeft = oversampledBlock.getChannelPointer(0);
        float* osRight = oversampledBlock.getChannelPointer(1);

        // Process through Airwindows Spiral saturation on oversampled signal
        airDrive.processStereo(osLeft, osRight, osNumSamples, dAmt, 1.0f);

        // Tone filter post-drive
        for (int i = 0; i < osNumSamples; ++i)
        {
            driveFilterL += (osLeft[i] - driveFilterL) * filterCoeff;
            driveFilterR += (osRight[i] - driveFilterR) * filterCoeff;
            osLeft[i] = driveFilterL;
            osRight[i] = driveFilterR;
        }

        oversampler->processSamplesDown(baseBlock);

        for (int i = 0; i < numSamples; ++i)
        {
            float g = driveBypassGain.getNextValue();
            buffer.setSample(0, i, buffer.getSample(0, i) * (1.0f - g) + tempEffectBuffer.getSample(0, i) * g);
            buffer.setSample(1, i, buffer.getSample(1, i) * (1.0f - g) + tempEffectBuffer.getSample(1, i) * g);
        }
    }
    else
    {
        driveBypassGain.skip(numSamples);
        driveAmount.skip(numSamples);
        driveTone.skip(numSamples);
    }
}
