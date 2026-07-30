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
    modeIndex = juce::jlimit(0, 4, modeIndex);
    playbackMode = static_cast<PlaybackMode>(modeIndex);
}

void SampleEngine::setPitchMode(int modeIndex)
{
    pitchMode = static_cast<PitchMode>(juce::jlimit(0, 2, modeIndex));
    updatePitchRatio();
}

void SampleEngine::setPitchSemitones(float semitones)
{
    pitchSemitones = semitones;
    updatePitchRatio();
}

void SampleEngine::updatePitchRatio()
{
    auto pMode = pitchMode.load();
    float semis = pitchSemitones.load();
    int note = currentNoteNumber.load();
    float totalSemis = (pMode == PitchMode::Axial) ? semis : ((float)(note - 60) + semis);
    currentPitchRatio = std::pow(2.0f, totalSemis / 12.0f);
}

void SampleEngine::setHostBpm(double bpm)
{
    if (bpm > 20.0 && bpm < 400.0)
        hostBpm = bpm;
}

void SampleEngine::play()
{
    const juce::ScopedLock sl(lock);
    if (sample.getNumSamples() == 0)
        return;

    auto pMode = playbackMode.load();
    int rEnd = regionEnd.load();
    int rStart = regionStart.load();

    if (pMode == PlaybackMode::Backward || pMode == PlaybackMode::BackForw)
    {
        currentSample = (double)juce::jmax(0, rEnd - 1);
        playDirectionForward = false;
    }
    else if (pMode == PlaybackMode::Random)
    {
        double span = (double)(rEnd - rStart);
        if (span > 10.0)
            currentSample = (double)rStart + juce::Random::getSystemRandom().nextDouble() * (span - 10.0);
        else
            currentSample = (double)rStart;
        playDirectionForward = true;
    }
    else // Forward or ForwBackw
    {
        currentSample = (double)rStart;
        playDirectionForward = true;
    }

    randomGrainCounter = 0;
    pitchPhase = 0.0;
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

void SampleEngine::noteOn(int midiNoteNumber, float velocity)
{
    juce::ignoreUnused(velocity);
    const juce::ScopedLock sl(lock);
    currentNoteNumber = midiNoteNumber;
    updatePitchRatio();
    pitchPhase = 0.0;
    play();
}

void SampleEngine::noteOff(int midiNoteNumber)
{
    const juce::ScopedLock sl(lock);
    if (midiNoteNumber == currentNoteNumber)
    {
        ampEnvelope.noteOff();
        filterEnvelope.noteOff();
    }
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

    int rStart = (int)(startNormalized * (float)sample.getNumSamples());
    int rEnd = (int)(endNormalized * (float)sample.getNumSamples());

    rStart = juce::jlimit(0, sample.getNumSamples() - 1, rStart);
    rEnd = juce::jlimit(rStart + 1, sample.getNumSamples(), rEnd);

    regionStart = rStart;
    regionEnd = rEnd;
}

void SampleEngine::updateAmpADSR(float attack, float decay, float sustain, float release)
{
    ampEnvelope.updateADSR(attack, decay, sustain, release);
}

void SampleEngine::updateFilterADSR(float attack, float decay, float sustain, float release)
{
    filterEnvelope.updateADSR(attack, decay, sustain, release);
}

void SampleEngine::setExciterAmount(float amount)
{
    exciterAmount = juce::jlimit(0.0f, 1.0f, amount);
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
    double targetSr = targetSampleRate.load();
    double sr = targetSr > 0.0 ? targetSr : 44100.0;
    float nyquist = (float)(sr * 0.49);

    for (const auto& band : filterBands)
    {
        auto* filterPair = bandFilters.add(std::make_unique<BandFilter>());
        float minF = juce::jlimit(20.0f, nyquist - 20.0f, band.minFreq);
        float maxF = juce::jlimit(minF + 10.0f, nyquist, band.maxFreq);

        auto hpCoeffs = juce::IIRCoefficients::makeHighPass(sr, minF);
        auto lpCoeffs = juce::IIRCoefficients::makeLowPass(sr, maxF);

        filterPair->hpL1.setCoefficients(hpCoeffs);
        filterPair->hpL2.setCoefficients(hpCoeffs);
        filterPair->hpR1.setCoefficients(hpCoeffs);
        filterPair->hpR2.setCoefficients(hpCoeffs);

        filterPair->lpL1.setCoefficients(lpCoeffs);
        filterPair->lpL2.setCoefficients(lpCoeffs);
        filterPair->lpR1.setCoefficients(lpCoeffs);
        filterPair->lpR2.setCoefficients(lpCoeffs);
    }
}

void SampleEngine::setSpectralRegions(const juce::Array<SpectralRegion>& regions)
{
    const juce::ScopedLock sl(lock);
    spectralRegions = regions;
    regionFilterPairs.clear();

    double targetSr = targetSampleRate.load();
    double sr = targetSr > 0.0 ? targetSr : 44100.0;
    float nyquist = (float)(sr * 0.49);

    for (const auto& r : spectralRegions)
    {
        auto* pair = regionFilterPairs.add(std::make_unique<RegionFilterPair>());
        pair->region = r;

        float minF = juce::jlimit(20.0f, nyquist - 20.0f, r.minFreq);
        float maxF = juce::jlimit(minF + 10.0f, nyquist, r.maxFreq);

        auto hpCoeffs = juce::IIRCoefficients::makeHighPass(sr, minF);
        auto lpCoeffs = juce::IIRCoefficients::makeLowPass(sr, maxF);

        pair->hpL1.setCoefficients(hpCoeffs);
        pair->hpL2.setCoefficients(hpCoeffs);
        pair->hpR1.setCoefficients(hpCoeffs);
        pair->hpR2.setCoefficients(hpCoeffs);

        pair->lpL1.setCoefficients(lpCoeffs);
        pair->lpL2.setCoefficients(lpCoeffs);
        pair->lpR1.setCoefficients(lpCoeffs);
        pair->lpR2.setCoefficients(lpCoeffs);
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

    // Pitch Mode Rules:
    // Stretch (0): Pitch shifted independently via Dual-Tap Grain Shifter while duration/speed stays constant (baseSpeedRatio).
    // Resample (1): Classical sampler speed-pitch coupling (speedRatio = baseSpeedRatio * currentPitchRatio). Higher pitch = faster/shorter.
    // Axial (2): Fixed pitch and fixed speed (speedRatio = baseSpeedRatio, pitchRatio = 1.0f). Ignores MIDI note.
    double speedRatio = baseSpeedRatio;
    float activePitchRatio = 1.0f;

    if (pitchMode == PitchMode::Resample)
    {
        speedRatio = baseSpeedRatio * (double)currentPitchRatio;
        activePitchRatio = 1.0f;
    }
    else if (pitchMode == PitchMode::Stretch)
    {
        speedRatio = baseSpeedRatio;
        activePitchRatio = currentPitchRatio;
    }
    else // Axial (Fixed)
    {
        speedRatio = baseSpeedRatio;
        activePitchRatio = 1.0f;
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

        auto getSampleAtPos = [&](int channelIdx, double samplePos) -> float {
            if (totalSamples == 0 || numSampleChannels == 0 || std::isnan(samplePos) || std::isinf(samplePos)) return 0.0f;
            int ch = juce::jlimit(0, numSampleChannels - 1, channelIdx);

            int t0 = (int)samplePos;
            if (t0 < 0 || t0 >= totalSamples) return 0.0f;

            int t1 = juce::jmin(totalSamples - 1, t0 + 1);
            float fr = (float)(samplePos - (double)t0);

            float s0 = sample.getSample(ch, t0);
            float s1 = sample.getSample(ch, t1);
            return s0 + fr * (s1 - s0);
        };

        float rawSampleL = 0.0f;
        float rawSampleR = 0.0f;

        if (pitchMode == PitchMode::Stretch && std::abs(activePitchRatio - 1.0f) > 0.001f)
        {
            int grainSize = (int)(targetSampleRate * 0.035);
            if (grainSize < 64) grainSize = 64;
            int halfGrain = grainSize / 2;

            double normPhase1 = std::fmod(pitchPhase, (double)grainSize) / (double)grainSize;
            double normPhase2 = std::fmod(pitchPhase + (double)halfGrain, (double)grainSize) / (double)grainSize;

            float win1 = 0.5f * (1.0f - std::cos(2.0f * juce::MathConstants<float>::pi * (float)normPhase1));
            float win2 = 0.5f * (1.0f - std::cos(2.0f * juce::MathConstants<float>::pi * (float)normPhase2));

            double shift1 = (normPhase1 - 0.5) * (double)grainSize * (1.0 - (double)activePitchRatio);
            double shift2 = (normPhase2 - 0.5) * (double)grainSize * (1.0 - (double)activePitchRatio);

            rawSampleL = win1 * getSampleAtPos(0, currentSample + shift1) + win2 * getSampleAtPos(0, currentSample + shift2);
            rawSampleR = win1 * getSampleAtPos(1, currentSample + shift1) + win2 * getSampleAtPos(1, currentSample + shift2);

            pitchPhase += 1.0;
        }
        else
        {
            rawSampleL = getSampleAtPos(0, currentSample);
            rawSampleR = getSampleAtPos(1, currentSample);
        }

        float cutoff = 20.0f + filterVal * 18000.0f;
        float alpha = juce::jlimit(0.01f, 0.99f, 1.0f - std::exp(-2.0f * juce::MathConstants<float>::pi * cutoff / (float)targetSampleRate));

        filterStateL += alpha * (rawSampleL - filterStateL);
        filterStateR += alpha * (rawSampleR - filterStateR);

        float outSampleL = filterStateL * ampVal;
        float outSampleR = filterStateR * ampVal;

        if (!regionFilterPairs.isEmpty())
        {
            float playheadNorm = (totalSamples > 0) ? (float)(currentSample / (double)totalSamples) : 0.0f;

            float sumL = 0.0f;
            float sumR = 0.0f;
            bool activeRegionFound = false;

            for (auto* rfp : regionFilterPairs)
            {
                if (playheadNorm >= rfp->region.startNorm && playheadNorm <= rfp->region.endNorm)
                {
                    activeRegionFound = true;

                    float bL = outSampleL;
                    if (rfp->region.minFreq > 22.0f)
                    {
                        bL = rfp->hpL1.processSingleSampleRaw(bL);
                        bL = rfp->hpL2.processSingleSampleRaw(bL);
                    }
                    if (rfp->region.maxFreq < 19500.0f)
                    {
                        bL = rfp->lpL1.processSingleSampleRaw(bL);
                        bL = rfp->lpL2.processSingleSampleRaw(bL);
                    }

                    float bR = outSampleR;
                    if (rfp->region.minFreq > 22.0f)
                    {
                        bR = rfp->hpR1.processSingleSampleRaw(bR);
                        bR = rfp->hpR2.processSingleSampleRaw(bR);
                    }
                    if (rfp->region.maxFreq < 19500.0f)
                    {
                        bR = rfp->lpR1.processSingleSampleRaw(bR);
                        bR = rfp->lpR2.processSingleSampleRaw(bR);
                    }

                    sumL += bL;
                    sumR += bR;
                }
            }

            if (activeRegionFound)
            {
                outSampleL = sumL;
                outSampleR = sumR;
            }
            else
            {
                outSampleL = 0.0f;
                outSampleR = 0.0f;
            }
        }
        else if (freqFilterEnabled && !bandFilters.isEmpty())
        {
            float sumL = 0.0f;
            float sumR = 0.0f;
            for (auto* bf : bandFilters)
            {
                float bL = bf->hpL1.processSingleSampleRaw(outSampleL);
                bL = bf->hpL2.processSingleSampleRaw(bL);
                bL = bf->lpL1.processSingleSampleRaw(bL);
                bL = bf->lpL2.processSingleSampleRaw(bL);

                float bR = bf->hpR1.processSingleSampleRaw(outSampleR);
                bR = bf->hpR2.processSingleSampleRaw(bR);
                bR = bf->lpR1.processSingleSampleRaw(bR);
                bR = bf->lpR2.processSingleSampleRaw(bR);

                sumL += bL;
                sumR += bR;
            }
            outSampleL = sumL;
            outSampleR = sumR;
        }

        // Apply SCORCH-style Exciter processing (harmonic saturation, subtle texture artifacts, & generative harmonic/melodic content)
        if (exciterAmount > 0.001f)
        {
            // Generative consonant musical counter-melody & overtone layer generator
            auto getGenerativeHarmonics = [&](int channelIdx, float amt) -> float {
                if (amt < 0.05f || totalSamples == 0 || regionEnd <= regionStart)
                    return 0.0f;

                int ch = juce::jlimit(0, numSampleChannels - 1, channelIdx);
                double span = (double)juce::jmax(1, regionEnd - regionStart);

                // Consonant musical interval ratios (Unison, Major 3rd, Perfect 5th, Major 6th, Octave, Octave+5th)
                constexpr float consonantRatios[6] = { 1.0f, 1.25f, 1.5f, 1.667f, 2.0f, 2.5f };

                // Slow rhythmic step tracking quarter-note intervals (generates consonant counter-melodic movement)
                double stepTime = targetSampleRate > 0.0 ? (targetSampleRate * 0.25) : 11025.0;
                int noteStep = (int)std::floor(currentSample / stepTime) % 6;
                if (noteStep < 0) noteStep += 6;

                float ratio1 = consonantRatios[noteStep];
                float ratio2 = 2.0f; // Constant octave overtone

                // Interpolate sample at tap positions
                auto sampleTap = [&](float ratio) -> float {
                    double offset = std::fmod((currentSample - (double)regionStart) * (double)ratio, span);
                    if (offset < 0.0) offset += span;
                    double tapPos = (double)regionStart + offset;

                    int t0 = (int)tapPos;
                    int t1 = juce::jmin(totalSamples - 1, t0 + 1);
                    float fr = (float)(tapPos - (double)t0);

                    if (t0 >= 0 && t0 < totalSamples)
                    {
                        float s0 = sample.getSample(ch, t0);
                        float s1 = sample.getSample(ch, t1);
                        return s0 + fr * (s1 - s0);
                    }
                    return 0.0f;
                };

                float melTone = sampleTap(ratio1);
                float octTone = sampleTap(ratio2);

                // Combine generative counter-melody and octave shimmer
                return (0.6f * melTone + 0.4f * octTone) * ampVal;
            };

            float genHarmonicL = getGenerativeHarmonics(0, exciterAmount);
            float genHarmonicR = getGenerativeHarmonics(1, exciterAmount);

            auto applyExciter = [](float x, float genTone, float amt, double currentSampleIdx) -> float {
                if (std::abs(x) < 1e-6f && std::abs(genTone) < 1e-6f)
                    return x;

                // Blend in generative consonant melodic & harmonic content smoothly with exciter amount
                float blendedInput = x + amt * 0.25f * genTone;

                float drive = 1.0f + amt * 2.5f;
                float driven = blendedInput * drive;

                // Asymmetric warmth (odd + even harmonics)
                float evenHarmonic = 0.25f * amt * (driven * driven);
                float asymmetricSignal = driven + (driven >= 0.0f ? evenHarmonic : -evenHarmonic * 0.5f);

                // Warm analog tanh saturation
                float saturated = std::tanh(asymmetricSignal);

                // Subtle soft textural artifacts at higher settings (>0.3)
                float texture = 0.0f;
                if (amt > 0.3f)
                {
                    float textureScale = (amt - 0.3f) / 0.7f;
                    float softGrain = std::tanh(std::sin((float)currentSampleIdx * 0.15f + blendedInput * 4.0f) * 1.5f);
                    texture = textureScale * 0.025f * softGrain * std::abs(driven);
                }

                float exciterOut = saturated + texture;
                float makeupGain = 1.0f / (1.0f + amt * 0.4f);
                return (1.0f - amt) * x + amt * (exciterOut * makeupGain);
            };

            outSampleL = applyExciter(outSampleL, genHarmonicL, exciterAmount, currentSample);
            outSampleR = applyExciter(outSampleR, genHarmonicR, exciterAmount, currentSample);
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