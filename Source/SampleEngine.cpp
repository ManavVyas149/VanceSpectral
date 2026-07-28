#include "SampleEngine.h"
#include <cmath>

SampleEngine::SampleEngine()
{
}

void SampleEngine::prepare(double sr)
{
    const juce::ScopedLock sl(lock);
    targetSampleRate = sr > 0.0 ? sr : 44100.0;
    ampEnvelope.prepareToPlay(targetSampleRate);
    filterEnvelope.prepareToPlay(targetSampleRate);

    setFrequencyFilterBands(filterBands);
}

void SampleEngine::loadSample(const juce::AudioBuffer<float>& buffer, double sampleNativeRate)
{
    const juce::ScopedLock sl(lock);

    sample = buffer;
    nativeSampleRate = sampleNativeRate > 0.0 ? sampleNativeRate : 44100.0;

    regionStart = 0;
    regionEnd = sample.getNumSamples();
    currentSample = 0;

    filterStateL = 0.0f;
    filterStateR = 0.0f;

    DBG("Sample Loaded: " << sample.getNumSamples() << " samples at " << nativeSampleRate << " Hz");
}

void SampleEngine::setPlaybackMode(int modeIndex)
{
    const juce::ScopedLock sl(lock);
    modeIndex = juce::jlimit(0, 4, modeIndex);
    playbackMode = static_cast<PlaybackMode>(modeIndex);
}

void SampleEngine::setPitchMode(int modeIndex)
{
    const juce::ScopedLock sl(lock);
    modeIndex = juce::jlimit(0, 2, modeIndex);
    pitchMode = static_cast<PitchMode>(modeIndex);
}

void SampleEngine::setHostBpm(double bpm)
{
    const juce::ScopedLock sl(lock);
    if (bpm > 20.0 && bpm < 400.0)
        hostBpm = bpm;
}

void SampleEngine::play()
{
    const juce::ScopedLock sl(lock);
    if (sample.getNumSamples() == 0)
        return;

    if (playbackMode == PlaybackMode::Backward || playbackMode == PlaybackMode::BackForw)
    {
        currentSample = (double)juce::jmax(0, regionEnd - 1);
        playDirectionForward = false;
    }
    else if (playbackMode == PlaybackMode::Random)
    {
        double span = (double)(regionEnd - regionStart);
        if (span > 10.0)
            currentSample = (double)regionStart + juce::Random::getSystemRandom().nextDouble() * (span - 10.0);
        else
            currentSample = (double)regionStart;
        playDirectionForward = true;
    }
    else // Forward or ForwBackw
    {
        currentSample = (double)regionStart;
        playDirectionForward = true;
    }

    randomGrainCounter = 0;
    playing = true;

    filterStateL = 0.0f;
    filterStateR = 0.0f;

    ampEnvelope.noteOn();
    filterEnvelope.noteOn();
}

void SampleEngine::stop()
{
    const juce::ScopedLock sl(lock);

    ampEnvelope.noteOff();
    filterEnvelope.noteOff();
    playing = false;
}

bool SampleEngine::isPlaying() const
{
    return playing || ampEnvelope.isActive();
}

double SampleEngine::getPlayPositionNormalized() const
{
    const juce::ScopedLock sl(lock);
    if (sample.getNumSamples() == 0)
        return 0.0;
    return juce::jlimit(0.0, 1.0, currentSample / (double)sample.getNumSamples());
}

double SampleEngine::getRegionStartNormalized() const
{
    const juce::ScopedLock sl(lock);
    if (sample.getNumSamples() == 0)
        return 0.0;
    return juce::jlimit(0.0, 1.0, (double)regionStart / (double)sample.getNumSamples());
}

double SampleEngine::getRegionEndNormalized() const
{
    const juce::ScopedLock sl(lock);
    if (sample.getNumSamples() == 0)
        return 1.0;
    return juce::jlimit(0.0, 1.0, (double)regionEnd / (double)sample.getNumSamples());
}

void SampleEngine::setLoop(bool shouldLoop)
{
    const juce::ScopedLock sl(lock);
    looping = shouldLoop;
}

void SampleEngine::setRegion(float startNormalized, float endNormalized)
{
    const juce::ScopedLock sl(lock);

    if (sample.getNumSamples() == 0)
        return;

    regionStart = (int)(startNormalized * (float)sample.getNumSamples());
    regionEnd = (int)(endNormalized * (float)sample.getNumSamples());

    regionStart = juce::jlimit(0, sample.getNumSamples() - 1, regionStart);
    regionEnd = juce::jlimit(regionStart + 1, sample.getNumSamples(), regionEnd);
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

void SampleEngine::setFrequencyFilter(bool enabled, float minFreq, float maxFreq)
{
    juce::Array<FrequencyBand> bands;
    if (enabled)
        bands.add({ minFreq, maxFreq });
    setFrequencyFilterBands(bands);
}

void SampleEngine::setFrequencyFilterBands(const juce::Array<FrequencyBand>& bands)
{
    const juce::ScopedLock sl(lock);
    filterBands = bands;
    freqFilterEnabled = !filterBands.isEmpty();

    bandFilters.clear();
    double sr = targetSampleRate > 0.0 ? targetSampleRate : 44100.0;

    for (const auto& band : filterBands)
    {
        auto* filterPair = bandFilters.add(std::make_unique<BandFilter>());
        float minF = juce::jlimit(20.0f, (float)(sr * 0.49), band.minFreq);
        float maxF = juce::jlimit(minF + 10.0f, (float)(sr * 0.49), band.maxFreq);

        auto hpCoeffs = juce::IIRCoefficients::makeHighPass(sr, minF);
        auto lpCoeffs = juce::IIRCoefficients::makeLowPass(sr, maxF);

        filterPair->hpL.setCoefficients(hpCoeffs);
        filterPair->hpR.setCoefficients(hpCoeffs);
        filterPair->lpL.setCoefficients(lpCoeffs);
        filterPair->lpR.setCoefficients(lpCoeffs);
    }
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

    // Base speed ratio preserves sample pitch across target sample rates
    double baseSpeedRatio = (nativeSampleRate > 0.0 && targetSampleRate > 0.0) ? (nativeSampleRate / targetSampleRate) : 1.0;
    double speedRatio = baseSpeedRatio;

    // Pitch Mode Modifiers:
    // Stretch (0): Fixed natural pitch
    // Resample (1): Classical sampler speed-pitch coupling
    // Axial (2): Tempo Sync grid locked to host BPM
    if (pitchMode == PitchMode::Axial && hostBpm > 20.0)
    {
        speedRatio *= (hostBpm / 120.0);
    }

    for (int i = 0; i < numSamples; ++i)
    {
        // Playback Mode Boundaries & Direction handling
        if (playing)
        {
            if (playbackMode == PlaybackMode::Forward)
            {
                if (currentSample >= (double)regionEnd)
                {
                    if (looping)
                        currentSample = (double)regionStart;
                    else
                    {
                        playing = false;
                        ampEnvelope.noteOff();
                        filterEnvelope.noteOff();
                    }
                }
            }
            else if (playbackMode == PlaybackMode::Backward)
            {
                if (currentSample <= (double)regionStart)
                {
                    if (looping)
                        currentSample = (double)juce::jmax(0, regionEnd - 1);
                    else
                    {
                        playing = false;
                        ampEnvelope.noteOff();
                        filterEnvelope.noteOff();
                    }
                }
            }
            else if (playbackMode == PlaybackMode::ForwBackw) // Ping-Pong
            {
                if (playDirectionForward && currentSample >= (double)regionEnd)
                {
                    currentSample = (double)juce::jmax(0, regionEnd - 1);
                    playDirectionForward = false;
                }
                else if (!playDirectionForward && currentSample <= (double)regionStart)
                {
                    if (looping)
                    {
                        currentSample = (double)regionStart;
                        playDirectionForward = true;
                    }
                    else
                    {
                        playing = false;
                        ampEnvelope.noteOff();
                        filterEnvelope.noteOff();
                    }
                }
            }
            else if (playbackMode == PlaybackMode::BackForw) // Reverse Ping-Pong
            {
                if (!playDirectionForward && currentSample <= (double)regionStart)
                {
                    currentSample = (double)regionStart;
                    playDirectionForward = true;
                }
                else if (playDirectionForward && currentSample >= (double)regionEnd)
                {
                    if (looping)
                    {
                        currentSample = (double)juce::jmax(0, regionEnd - 1);
                        playDirectionForward = false;
                    }
                    else
                    {
                        playing = false;
                        ampEnvelope.noteOff();
                        filterEnvelope.noteOff();
                    }
                }
            }
            else if (playbackMode == PlaybackMode::Random)
            {
                randomGrainCounter++;
                if (randomGrainCounter >= 2205 || currentSample >= (double)regionEnd || currentSample < (double)regionStart)
                {
                    randomGrainCounter = 0;
                    double span = (double)(regionEnd - regionStart);
                    if (span > 100.0)
                        currentSample = (double)regionStart + juce::Random::getSystemRandom().nextDouble() * (span - 100.0);
                    else
                        currentSample = (double)regionStart;
                }
            }
        }

        float ampVal = ampEnvelope.getNextSample();
        float filterVal = filterEnvelope.getNextSample();

        float rawSample = 0.0f;
        int index0 = (int)currentSample;
        int index1 = index0 + 1;
        float frac = (float)(currentSample - (double)index0);

        if (index0 >= 0 && index0 < totalSamples && numSampleChannels > 0)
        {
            float s0 = sample.getSample(0, index0);
            float s1 = (index1 < totalSamples) ? sample.getSample(0, index1) : s0;
            rawSample = s0 + frac * (s1 - s0);
        }

        float cutoff = 20.0f + filterVal * 18000.0f;
        float alpha = juce::jlimit(0.01f, 0.99f, 1.0f - std::exp(-2.0f * juce::MathConstants<float>::pi * cutoff / (float)targetSampleRate));

        filterStateL += alpha * (rawSample - filterStateL);
        filterStateR += alpha * (rawSample - filterStateR);

        float outSampleL = filterStateL * ampVal;
        float outSampleR = filterStateR * ampVal;

        if (freqFilterEnabled && !bandFilters.isEmpty())
        {
            float sumL = 0.0f;
            float sumR = 0.0f;
            for (auto* bf : bandFilters)
            {
                float bL = bf->hpL.processSingleSampleRaw(outSampleL);
                bL = bf->lpL.processSingleSampleRaw(bL);

                float bR = bf->hpR.processSingleSampleRaw(outSampleR);
                bR = bf->lpR.processSingleSampleRaw(bR);

                sumL += bL;
                sumR += bR;
            }
            outSampleL = sumL;
            outSampleR = sumR;
        }

        if (output.getNumChannels() > 0)
            output.addSample(0, startSample + i, outSampleL);

        if (output.getNumChannels() > 1)
            output.addSample(1, startSample + i, outSampleR);

        if (playing)
        {
            if (playbackMode == PlaybackMode::Backward || (!playDirectionForward && (playbackMode == PlaybackMode::ForwBackw || playbackMode == PlaybackMode::BackForw)))
                currentSample -= speedRatio;
            else
                currentSample += speedRatio;
        }
    }
}