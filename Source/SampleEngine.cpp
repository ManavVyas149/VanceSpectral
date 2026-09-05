#include "SampleEngine.h"
#include <cmath>

SampleEngine::SampleEngine()
{
}

void SampleEngine::prepare(double sr)
{
    const juce::ScopedLock sl(lock);
    targetSampleRate = sr > 0.0 ? sr : 44100.0;
    for (auto& v : voices)
    {
        v.ampEnvelope.prepareToPlay(targetSampleRate);
        v.soundTouch.setSampleRate((uint)targetSampleRate.load());
        v.soundTouch.setChannels(2);
        v.soundTouch.setSetting(SETTING_USE_AA_FILTER, 1);
        v.soundTouch.setSetting(SETTING_AA_FILTER_LENGTH, 32);
        v.soundTouch.setSetting(SETTING_USE_QUICKSEEK, 0);
        v.soundTouch.setSetting(SETTING_SEQUENCE_MS, 40);
        v.soundTouch.setSetting(SETTING_SEEKWINDOW_MS, 15);
        v.soundTouch.setSetting(SETTING_OVERLAP_MS, 8);
    }

    updateFilteredSample();
}

void SampleEngine::loadSample(const juce::AudioBuffer<float>& buffer, double sampleNativeRate, int rootNote)
{
    const juce::ScopedLock sl(lock);

    sample = buffer;
    nativeSampleRate = sampleNativeRate > 0.0 ? sampleNativeRate : 44100.0;
    rootNoteNumber = juce::jlimit(0, 127, rootNote);

    regionStart = 0;
    regionEnd = sample.getNumSamples();

    for (auto& v : voices)
        v.reset();

    playing = false;

    updateFilteredSample();

    DBG("Sample Loaded: " << sample.getNumSamples() << " samples at " << nativeSampleRate << " Hz, Root Note: " << rootNoteNumber.load());
}

void SampleEngine::setRootNote(int noteNumber)
{
    rootNoteNumber = juce::jlimit(0, 127, noteNumber);
}

int SampleEngine::findNearestZeroCrossing(const juce::AudioBuffer<float>& buffer, int targetSample, int searchWindowSamples)
{
    int totalSamples = buffer.getNumSamples();
    if (totalSamples <= 1)
        return juce::jlimit(0, juce::jmax(0, totalSamples - 1), targetSample);

    int numCh = buffer.getNumChannels();
    int clampedTarget = juce::jlimit(0, totalSamples - 1, targetSample);

    int windowStart = juce::jmax(0, clampedTarget - searchWindowSamples);
    int windowEnd   = juce::jmin(totalSamples - 2, clampedTarget + searchWindowSamples);

    int bestZeroCrossing = -1;
    int minDistance = INT_MAX;

    for (int i = windowStart; i <= windowEnd; ++i)
    {
        float s0L = buffer.getSample(0, i);
        float s1L = buffer.getSample(0, i + 1);
        float s0R = (numCh > 1) ? buffer.getSample(1, i) : s0L;
        float s1R = (numCh > 1) ? buffer.getSample(1, i + 1) : s1L;

        bool hasZeroCrossL = (s0L * s1L <= 0.0f);
        bool hasZeroCrossR = (s0R * s1R <= 0.0f);

        if (hasZeroCrossL || hasZeroCrossR)
        {
            int dist = std::abs(i - clampedTarget);
            if (dist < minDistance)
            {
                minDistance = dist;
                bestZeroCrossing = (std::abs(s0L) + std::abs(s0R) <= std::abs(s1L) + std::abs(s1R)) ? i : (i + 1);
            }
        }
    }

    if (bestZeroCrossing >= 0)
        return bestZeroCrossing;

    // Fallback: find nearest local minimum in absolute amplitude within the search window
    float minAbsAmp = FLT_MAX;
    int bestLocalMin = clampedTarget;
    int searchEnd = juce::jmin(totalSamples - 1, clampedTarget + searchWindowSamples);
    for (int i = windowStart; i <= searchEnd; ++i)
    {
        float absL = std::abs(buffer.getSample(0, i));
        float absR = (numCh > 1) ? std::abs(buffer.getSample(1, i)) : absL;
        float totalAbs = absL + absR;

        int dist = std::abs(i - clampedTarget);
        if (totalAbs < minAbsAmp || (std::abs(totalAbs - minAbsAmp) < 1e-6f && dist < std::abs(bestLocalMin - clampedTarget)))
        {
            minAbsAmp = totalAbs;
            bestLocalMin = i;
        }
    }

    return bestLocalMin;
}

juce::AudioBuffer<float> SampleEngine::computeFilteredBuffer(
    const juce::AudioBuffer<float>& inSample,
    double sr,
    const juce::Array<SpectralRegion>& regions,
    bool freqEnabled,
    const juce::Array<FrequencyBand>& bands)
{
    juce::ScopedNoDenormals noDenormals;
    int totalSamples = inSample.getNumSamples();
    int numCh = inSample.getNumChannels();

    if (totalSamples == 0 || numCh == 0)
        return inSample;

    juce::AudioBuffer<float> outBuffer = inSample;
    float nyquist = (float)(sr * 0.49);

    if (!regions.isEmpty())
    {
        int searchWindow = (int)(sr * 0.008);      // +/- 8ms zero-crossing search window
        int targetFadeSamples = (int)(sr * 0.005); // 5ms equal-power backup crossfade

        juce::OwnedArray<RegionFilterPair> filters;
        for (const auto& r : regions)
        {
            auto* pair = filters.add(std::make_unique<RegionFilterPair>());
            pair->region = r;

            int rawStart = (int)(r.startNorm * (float)totalSamples);
            int rawEnd   = (int)(r.endNorm * (float)totalSamples);
            int snappedStart = findNearestZeroCrossing(inSample, rawStart, searchWindow);
            int snappedEnd   = findNearestZeroCrossing(inSample, rawEnd, searchWindow);

            snappedStart = juce::jlimit(0, totalSamples - 1, snappedStart);
            snappedEnd   = juce::jlimit(snappedStart + 1, totalSamples, snappedEnd);

            pair->snappedStartSample = snappedStart;
            pair->snappedEndSample = snappedEnd;

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

        for (int i = 0; i < totalSamples; ++i)
        {
            float inL = inSample.getSample(0, i);
            float inR = (numCh > 1) ? inSample.getSample(1, i) : inL;

            float sumL = 0.0f;
            float sumR = 0.0f;
            float sumRegionGainSq = 0.0f;

            for (auto* rfp : filters)
            {
                int rStartSample = rfp->snappedStartSample;
                int rEndSample = rfp->snappedEndSample;
                int rLength = rEndSample - rStartSample;

                int effectiveFade = juce::jmax(1, juce::jmin(targetFadeSamples, rLength / 2));

                if (i >= rStartSample && i <= rEndSample)
                {
                    float bL = inL;
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

                    float bR = inR;
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

                    // Equal-power region boundary fading with zero-crossing safety net
                    float regionGain = 1.0f;
                    if (i < rStartSample + effectiveFade)
                    {
                        float t = (float)(i - rStartSample) / (float)effectiveFade;
                        regionGain = std::sin(t * juce::MathConstants<float>::halfPi);
                    }
                    else if (i > rEndSample - effectiveFade)
                    {
                        float t = (float)(rEndSample - i) / (float)effectiveFade;
                        regionGain = std::sin(t * juce::MathConstants<float>::halfPi);
                    }

                    sumL += bL * regionGain;
                    sumR += bR * regionGain;
                    sumRegionGainSq += regionGain * regionGain;
                }
            }

            // In gap time-range (outside selection regions), pass through full-range unfiltered sample audio
            float gapGain = std::sqrt(juce::jmax(0.0f, 1.0f - sumRegionGainSq));
            float finalL = sumL + inL * gapGain;
            float finalR = sumR + inR * gapGain;

            outBuffer.setSample(0, i, finalL);
            if (numCh > 1) outBuffer.setSample(1, i, finalR);
        }
    }
    else if (freqEnabled && !bands.isEmpty())
    {
        juce::OwnedArray<BandFilter> filters;
        for (const auto& band : bands)
        {
            auto* filterPair = filters.add(std::make_unique<BandFilter>());
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

        for (int i = 0; i < totalSamples; ++i)
        {
            float inL = inSample.getSample(0, i);
            float inR = (numCh > 1) ? inSample.getSample(1, i) : inL;

            float sumL = 0.0f;
            float sumR = 0.0f;
            for (auto* bf : filters)
            {
                float bL = bf->hpL1.processSingleSampleRaw(inL);
                bL = bf->hpL2.processSingleSampleRaw(bL);
                bL = bf->lpL1.processSingleSampleRaw(bL);
                bL = bf->lpL2.processSingleSampleRaw(bL);

                float bR = bf->hpR1.processSingleSampleRaw(inR);
                bR = bf->hpR2.processSingleSampleRaw(bR);
                bR = bf->lpR1.processSingleSampleRaw(bR);
                bR = bf->lpR2.processSingleSampleRaw(bR);

                sumL += bL;
                sumR += bR;
            }
            outBuffer.setSample(0, i, sumL);
            if (numCh > 1) outBuffer.setSample(1, i, sumR);
        }
    }

    return outBuffer;
}

void SampleEngine::updateFilteredSample()
{
    juce::AudioBuffer<float> inBuf;
    double sr;
    juce::Array<SpectralRegion> regs;
    bool freqEn;
    juce::Array<FrequencyBand> bnds;

    {
        const juce::ScopedLock sl(lock);
        inBuf = sample;
        sr = nativeSampleRate.load() > 0.0 ? nativeSampleRate.load() : 44100.0;
        regs = spectralRegions;
        freqEn = freqFilterEnabled.load();
        bnds = filterBands;
    }

    auto computed = computeFilteredBuffer(inBuf, sr, regs, freqEn, bnds);

    {
        const juce::ScopedLock sl(lock);
        filteredSample = std::move(computed);
    }
}

void SampleEngine::setPolyMode(bool isPoly)
{
    const juce::ScopedLock sl(lock);
    bool prevPoly = polyMode.load();
    polyMode = isPoly;

    if (prevPoly && !isPoly)
    {
        // Switching Poly -> Mono mid-playback: quick fade out extra active voices cleanly
        int fadeSamples = (int)(targetSampleRate.load() * 0.004);
        for (size_t i = 1; i < MAX_VOICES; ++i)
        {
            if (voices[i].active)
            {
                voices[i].startQuickFadeOut(fadeSamples);
            }
        }
    }
}

void SampleEngine::setGlideTime(float timeMs)
{
    glideTimeMs = juce::jlimit(0.0f, 2000.0f, timeMs);
}

void SampleEngine::setPlaybackMode(int modeIndex)
{
    modeIndex = juce::jlimit(0, 4, modeIndex);
    playbackMode = static_cast<PlaybackMode>(modeIndex);
}

void SampleEngine::setPitchMode(int modeIndex)
{
    pitchMode = static_cast<PitchMode>(juce::jlimit(0, 1, modeIndex));
}

void SampleEngine::setPitchSemitones(float semitones)
{
    pitchSemitones = semitones;
}

void SampleEngine::setTimbreDrift(float amount)
{
    timbreDriftAmount = juce::jlimit(0.0f, 1.0f, amount);
}

void SampleEngine::setHostBpm(double bpm)
{
    if (bpm > 20.0 && bpm < 400.0)
        hostBpm = bpm;
}

void SampleEngine::rerollRandomDirection()
{
    int randDir = juce::Random::getSystemRandom().nextInt(4);
    randomChosenDirection = static_cast<PlaybackMode>(randDir);
}

void SampleEngine::initVoice(Voice& v, int noteNumber, float velocity)
{
    v.active = true;
    v.releasing = false;
    v.isQuickFadingOut = false;
    v.quickFadeOutSamplesLeft = 0;
    v.quickFadeOutTotalSamples = 0;
    v.noteNumber = noteNumber;
    v.velocity = velocity;
    v.voiceAge = ++voiceAgeCounter;
    v.targetPitchSemitones = (float)(noteNumber - rootNoteNumber.load());
    v.timbreDriftOffset = (juce::Random::getSystemRandom().nextFloat() * 2.0f - 1.0f);

    auto pMode = playbackMode.load();
    int rEnd = regionEnd.load();
    int rStart = regionStart.load();

    PlaybackMode effDir = (pMode == PlaybackMode::Random) ? randomChosenDirection.load() : pMode;
    v.effectivePlaybackMode = effDir;

    if (effDir == PlaybackMode::Backward || effDir == PlaybackMode::BackForw)
    {
        v.currentSample = (double)juce::jmax(0, rEnd - 1);
        v.playDirectionForward = false;
    }
    else // Forward or ForwBackw
    {
        v.currentSample = (double)rStart;
        v.playDirectionForward = true;
    }

    v.randomGrainCounter = 0;
    v.samplesProcessed = 0;

    v.soundTouch.clear();
    v.lastAppliedPitchSemitones = -999.0f;
    v.lastAppliedRate = -999.0;
    v.lastAppliedPitchMode = PitchMode::Stretch;

    v.ampEnvelope.noteOn();
}

void SampleEngine::play()
{
    noteOn(currentNoteNumber.load(), 1.0f);
}

void SampleEngine::stop()
{
    noteOff(-1);
}

void SampleEngine::noteOn(int midiNoteNumber, float velocity)
{
    const juce::ScopedLock sl(lock);
    if (sample.getNumSamples() == 0)
        return;

    currentNoteNumber = midiNoteNumber;
    playing = true;

    float targetSemis = (float)(midiNoteNumber - rootNoteNumber.load());
    float glideMs = glideTimeMs.load();
    bool isPoly = polyMode.load();
    int fadeSamples = (int)(targetSampleRate.load() * 0.004);
    if (fadeSamples < 1) fadeSamples = 1;

    if (!isPoly)
    {
        // MONO MODE: legato glide when note is currently sounding, or smooth quick fade-out of sounding note on staccato / non-glide
        bool isLegato = voices[0].active && voices[0].ampEnvelope.isActive();

        if (isLegato && glideMs > 0.001f)
        {
            // Legato glide: update target pitch without retriggering envelopes or resetting sample position
            voices[0].noteNumber = midiNoteNumber;
            voices[0].targetPitchSemitones = targetSemis;
            voices[0].releasing = false;
        }
        else
        {
            // Staccato or Mono Non-Glide: start new note with quick fade-out of any sounding old voice
            Voice* targetVoice = nullptr;

            if (voices[0].active && voices[0].ampEnvelope.isActive() && !voices[0].isQuickFadingOut)
            {
                // Voice 0 is active: put voice 0 into quick fade out and find a free voice for new note
                voices[0].startQuickFadeOut(fadeSamples);

                for (size_t i = 1; i < MAX_VOICES; ++i)
                {
                    if (!voices[i].active && !voices[i].isQuickFadingOut)
                    {
                        targetVoice = &voices[i];
                        break;
                    }
                }
            }

            if (targetVoice == nullptr)
                targetVoice = &voices[0];

            targetVoice->targetPitchSemitones = targetSemis;
            if (glideMs > 0.001f)
                targetVoice->currentPitchSemitones = lastMonoPitchSemitones;
            else
                targetVoice->currentPitchSemitones = targetSemis;

            initVoice(*targetVoice, midiNoteNumber, velocity);
        }

        lastMonoPitchSemitones = targetSemis;
    }
    else
    {
        // POLY MODE: sequential notes glide from lastPolyPitchSemitones, simultaneous chords start directly at target pitch
        bool isSimultaneousChord = (globalSampleCounter - lastNoteTriggerSample) < (uint64_t)(targetSampleRate.load() * 0.005);
        float startPitch = (isSimultaneousChord || glideMs <= 0.001f) ? targetSemis : lastPolyPitchSemitones;

        // If note on midiNoteNumber is already active, quick fade it out to prevent click on retrigger
        for (auto& v : voices)
        {
            if (v.active && v.noteNumber == midiNoteNumber && !v.isQuickFadingOut)
            {
                v.startQuickFadeOut(fadeSamples);
            }
        }

        Voice* targetVoice = nullptr;

        // Find an unallocated inactive voice
        for (auto& v : voices)
        {
            if (!v.active && !v.ampEnvelope.isActive() && !v.isQuickFadingOut)
            {
                targetVoice = &v;
                break;
            }
        }

        // Voice stealing: steal oldest non-fading voice if all 16 voices are busy
        if (targetVoice == nullptr)
        {
            uint64_t oldestAge = UINT64_MAX;
            for (auto& v : voices)
            {
                if (!v.isQuickFadingOut && v.voiceAge < oldestAge)
                {
                    oldestAge = v.voiceAge;
                    targetVoice = &v;
                }
            }
            if (targetVoice != nullptr)
            {
                targetVoice->startQuickFadeOut(fadeSamples);
                targetVoice = nullptr;
                for (auto& v : voices)
                {
                    if (!v.active && !v.ampEnvelope.isActive())
                    {
                        targetVoice = &v;
                        break;
                    }
                }
            }
        }

        if (targetVoice == nullptr)
        {
            // Pick fading voice closest to silence (minimal quickFadeOutSamplesLeft)
            int minLeft = INT_MAX;
            for (auto& v : voices)
            {
                if (v.isQuickFadingOut && v.quickFadeOutSamplesLeft < minLeft)
                {
                    minLeft = v.quickFadeOutSamplesLeft;
                    targetVoice = &v;
                }
            }
        }

        if (targetVoice == nullptr)
        {
            targetVoice = &voices[0];
        }

        targetVoice->currentPitchSemitones = startPitch;
        initVoice(*targetVoice, midiNoteNumber, velocity);

        lastPolyPitchSemitones = targetSemis;
        lastNoteTriggerSample = globalSampleCounter;
    }
}

void SampleEngine::noteOff(int midiNoteNumber)
{
    const juce::ScopedLock sl(lock);
    bool isPoly = polyMode.load();

    if (!isPoly)
    {
        // MONO MODE: stop all active voices matching note number or all notes if < 0
        for (auto& v : voices)
        {
            if (v.active && (midiNoteNumber < 0 || v.noteNumber == midiNoteNumber))
            {
                v.releasing = true;
                v.ampEnvelope.noteOff();
            }
        }
    }
    else
    {
        // POLY MODE: stop only matching note number (or all notes if < 0)
        for (auto& v : voices)
        {
            if (v.active && (midiNoteNumber < 0 || v.noteNumber == midiNoteNumber))
            {
                v.releasing = true;
                v.ampEnvelope.noteOff();
            }
        }
    }
}

bool SampleEngine::isPlaying() const
{
    const juce::ScopedLock sl(lock);
    for (const auto& v : voices)
    {
        if (v.active || v.ampEnvelope.isActive())
            return true;
    }
    return false;
}

double SampleEngine::getPlayPositionNormalized() const
{
    const juce::ScopedLock sl(lock);
    if (sample.getNumSamples() == 0)
        return 0.0;

    const Voice* newestVoice = nullptr;
    uint64_t maxAge = 0;
    for (const auto& v : voices)
    {
        if ((v.active || v.ampEnvelope.isActive()) && v.voiceAge > maxAge)
        {
            maxAge = v.voiceAge;
            newestVoice = &v;
        }
    }

    if (newestVoice != nullptr)
        return juce::jlimit(0.0, 1.0, newestVoice->currentSample / (double)sample.getNumSamples());

    return 0.0;
}

juce::Array<float> SampleEngine::getActiveVoicePositionsNormalized() const
{
    const juce::ScopedLock sl(lock);
    juce::Array<float> positions;
    if (sample.getNumSamples() == 0)
        return positions;

    double totalSamples = (double)sample.getNumSamples();

    for (const auto& v : voices)
    {
        if (v.active || v.ampEnvelope.isActive() || v.isQuickFadingOut)
        {
            float normPos = (float)juce::jlimit(0.0, 1.0, v.currentSample / totalSamples);
            positions.add(normPos);
        }
    }

    return positions;
}

juce::Array<ActiveVoiceVisualInfo> SampleEngine::getActiveVoiceVisualInfos() const
{
    const juce::ScopedLock sl(lock);
    juce::Array<ActiveVoiceVisualInfo> infos;
    if (sample.getNumSamples() == 0)
        return infos;

    double totalSamples = (double)sample.getNumSamples();
    bool isRndMode = (playbackMode.load() == PlaybackMode::Random);

    for (const auto& v : voices)
    {
        if (v.active || v.ampEnvelope.isActive() || v.isQuickFadingOut)
        {
            ActiveVoiceVisualInfo info;
            info.positionNorm = (float)juce::jlimit(0.0, 1.0, v.currentSample / totalSamples);

            float envLvl = v.ampEnvelope.getCurrentLevel();
            if (v.isQuickFadingOut && v.quickFadeOutTotalSamples > 0)
            {
                float q = (float)v.quickFadeOutSamplesLeft / (float)v.quickFadeOutTotalSamples;
                envLvl *= juce::jlimit(0.0f, 1.0f, q);
            }
            info.envLevel = envLvl;
            info.isRandomMode = isRndMode;
            info.randomRegions = v.randomSpectralRegions;

            infos.add(info);
        }
    }

    return infos;
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

    int rawStart = (int)(startNormalized * (float)sample.getNumSamples());
    int rawEnd = (int)(endNormalized * (float)sample.getNumSamples());

    double sr = nativeSampleRate.load() > 0.0 ? nativeSampleRate.load() : 44100.0;
    int searchWindow = (int)(sr * 0.008); // +/- 8ms search window for zero-crossing

    int rStart = findNearestZeroCrossing(sample, rawStart, searchWindow);
    int rEnd = findNearestZeroCrossing(sample, rawEnd, searchWindow);

    rStart = juce::jlimit(0, sample.getNumSamples() - 1, rStart);
    rEnd = juce::jlimit(rStart + 1, sample.getNumSamples(), rEnd);

    int prevStart = regionStart.load();
    int prevEnd = regionEnd.load();

    regionStart = rStart;
    regionEnd = rEnd;

    // If active voices are currently playing and boundaries change significantly mid-playback,
    // trigger a short quick-fade to prevent phase tearing during live boundary drags
    if (std::abs(rStart - prevStart) > searchWindow * 2 || std::abs(rEnd - prevEnd) > searchWindow * 2)
    {
        int fadeSamples = (int)(targetSampleRate.load() * 0.006);
        for (auto& v : voices)
        {
            if (v.active && !v.isQuickFadingOut)
            {
                if (v.currentSample < (double)rStart || v.currentSample > (double)rEnd)
                {
                    v.startQuickFadeOut(fadeSamples);
                }
            }
        }
    }
}

void SampleEngine::updateAmpADSR(float attack, float decay, float sustain, float release)
{
    const juce::ScopedLock sl(lock);
    for (auto& v : voices)
        v.ampEnvelope.updateADSR(attack, decay, sustain, release);
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
    juce::AudioBuffer<float> inBuf;
    double sr;
    juce::Array<SpectralRegion> regs;

    {
        const juce::ScopedLock sl(lock);
        inBuf = sample;
        sr = nativeSampleRate.load() > 0.0 ? nativeSampleRate.load() : 44100.0;
        regs = spectralRegions;
    }

    bool freqEn = !bands.isEmpty();
    auto computed = computeFilteredBuffer(inBuf, sr, regs, freqEn, bands);

    {
        const juce::ScopedLock sl(lock);
        filterBands = bands;
        freqFilterEnabled = freqEn;
        filteredSample = std::move(computed);

        int fadeSamples = (int)(targetSampleRate.load() * 0.006);
        for (auto& v : voices)
        {
            if (v.active && !v.isQuickFadingOut)
                v.startQuickFadeOut(fadeSamples);
        }
    }
}

void SampleEngine::setSpectralRegions(const juce::Array<SpectralRegion>& regions)
{
    juce::AudioBuffer<float> inBuf;
    double sr;
    bool freqEn;
    juce::Array<FrequencyBand> bnds;

    {
        const juce::ScopedLock sl(lock);
        inBuf = sample;
        sr = nativeSampleRate.load() > 0.0 ? nativeSampleRate.load() : 44100.0;
        freqEn = freqFilterEnabled.load();
        bnds = filterBands;
    }

    auto computed = computeFilteredBuffer(inBuf, sr, regions, freqEn, bnds);

    {
        const juce::ScopedLock sl(lock);
        spectralRegions = regions;
        filteredSample = std::move(computed);

        // Quick fade-out active voices mid-playback to prevent clicks during live region updates/rerolls
        int fadeSamples = (int)(targetSampleRate.load() * 0.006);
        for (auto& v : voices)
        {
            if (v.active && !v.isQuickFadingOut)
                v.startQuickFadeOut(fadeSamples);
        }
    }
}

void SampleEngine::process(juce::AudioBuffer<float>& output,
                            int startSample,
                            int numSamples)
{
    juce::ScopedNoDenormals noDenormals;

    const juce::ScopedTryLock sl(lock);
    if (!sl.isLocked())
        return;

    // Check if any voice is active or in envelope release
    int activeCount = 0;
    for (const auto& v : voices)
    {
        if (v.active || v.ampEnvelope.isActive() || v.isQuickFadingOut)
            activeCount++;
    }

    if (activeCount == 0)
        return;

    const auto& srcBuffer = filteredSample.getNumSamples() > 0 ? filteredSample : sample;
    int totalSamples = srcBuffer.getNumSamples();
    int numSampleChannels = srcBuffer.getNumChannels();

    if (totalSamples == 0 || numSampleChannels == 0)
        return;

    double baseSpeedRatio = (nativeSampleRate > 0.0 && targetSampleRate > 0.0) ? (nativeSampleRate / targetSampleRate) : 1.0;
    int rEnd = regionEnd.load();
    int rStart = regionStart.load();
    int rLen = rEnd - rStart;
    if (rLen <= 0) return;

    bool isLooping = looping.load();
    auto pMode = playbackMode.load();
    auto pitMode = pitchMode.load();
    float semis = pitchSemitones.load();
    float driftAmt = timbreDriftAmount.load();
    bool isPoly = polyMode.load();

    float targetPolyGainScale = 1.0f / std::sqrt(juce::jmax(1.0f, (float)activeCount));

    float glideSec = glideTimeMs.load() * 0.001f;

    auto getSampleAtPos = [&](int channelIdx, double samplePos) -> float {
        if (totalSamples == 0 || numSampleChannels == 0 || std::isnan(samplePos) || std::isinf(samplePos)) return 0.0f;
        int ch = juce::jlimit(0, numSampleChannels - 1, channelIdx);

        double clampedPos = juce::jlimit(0.0, (double)(totalSamples - 1), samplePos);
        int t0 = (int)std::floor(clampedPos);
        float fr = (float)(clampedPos - (double)t0);

        int tm1 = juce::jmax(0, t0 - 1);
        int t1  = juce::jmin(totalSamples - 1, t0 + 1);
        int t2  = juce::jmin(totalSamples - 1, t0 + 2);

        float ym1 = srcBuffer.getSample(ch, tm1);
        float y0  = srcBuffer.getSample(ch, t0);
        float y1  = srcBuffer.getSample(ch, t1);
        float y2  = srcBuffer.getSample(ch, t2);

        float c0 = y0;
        float c1 = 0.5f * (y1 - ym1);
        float c2 = ym1 - 2.5f * y0 + 2.0f * y1 - 0.5f * y2;
        float c3 = 0.5f * (y2 - ym1) + 1.5f * (y0 - y1);

        return ((c3 * fr + c2) * fr + c1) * fr + c0;
    };

    auto readLoopCrossfadedSample = [&](Voice& v, int channelIdx, double pos, PlaybackMode effMode) -> float {
        if (!isLooping || rLen <= 0)
            return getSampleAtPos(channelIdx, pos);

        int targetLoopFade = (int)(targetSampleRate.load() * 0.010);
        int loopFade = juce::jmax(1, juce::jmin(targetLoopFade, rLen / 2));

        if (effMode == PlaybackMode::Forward)
        {
            double fadeStartPos = (double)(rEnd - loopFade);
            if (pos >= fadeStartPos && pos < (double)rEnd)
            {
                double t = (pos - fadeStartPos) / (double)loopFade;
                float gTail = (float)std::cos(t * juce::MathConstants<double>::halfPi);
                float gHead = (float)std::sin(t * juce::MathConstants<double>::halfPi);
                double posHead = (double)rStart + (pos - fadeStartPos);

                float sTail = getSampleAtPos(channelIdx, pos);
                float sHead = getSampleAtPos(channelIdx, posHead);
                return sTail * gTail + sHead * gHead;
            }
        }
        else if (effMode == PlaybackMode::Backward)
        {
            double fadeEndPos = (double)(rStart + loopFade);
            if (pos <= fadeEndPos && pos > (double)rStart)
            {
                double t = (fadeEndPos - pos) / (double)loopFade;
                float gHead = (float)std::cos(t * juce::MathConstants<double>::halfPi);
                float gTail = (float)std::sin(t * juce::MathConstants<double>::halfPi);
                double posTail = (double)rEnd - (fadeEndPos - pos);

                float sHead = getSampleAtPos(channelIdx, pos);
                float sTail = getSampleAtPos(channelIdx, posTail);
                return sHead * gHead + sTail * gTail;
            }
        }
        else if (effMode == PlaybackMode::ForwBackw)
        {
            if (v.playDirectionForward)
            {
                double fadeStartPos = (double)(rEnd - loopFade);
                if (pos >= fadeStartPos && pos < (double)rEnd)
                {
                    double t = (pos - fadeStartPos) / (double)loopFade;
                    float gFwd = (float)std::cos(t * juce::MathConstants<double>::halfPi);
                    float gBwd = (float)std::sin(t * juce::MathConstants<double>::halfPi);
                    double posBwd = (double)rEnd - (pos - fadeStartPos);

                    float sFwd = getSampleAtPos(channelIdx, pos);
                    float sBwd = getSampleAtPos(channelIdx, posBwd);
                    return sFwd * gFwd + sBwd * gBwd;
                }
            }
            else if (isLooping)
            {
                double fadeEndPos = (double)(rStart + loopFade);
                if (pos <= fadeEndPos && pos > (double)rStart)
                {
                    double t = (fadeEndPos - pos) / (double)loopFade;
                    float gBwd = (float)std::cos(t * juce::MathConstants<double>::halfPi);
                    float gFwd = (float)std::sin(t * juce::MathConstants<double>::halfPi);
                    double posFwd = (double)rStart + (fadeEndPos - pos);

                    float sBwd = getSampleAtPos(channelIdx, pos);
                    float sFwd = getSampleAtPos(channelIdx, posFwd);
                    return sBwd * gBwd + sFwd * gFwd;
                }
            }
        }
        else if (effMode == PlaybackMode::BackForw)
        {
            if (!v.playDirectionForward)
            {
                double fadeEndPos = (double)(rStart + loopFade);
                if (pos <= fadeEndPos && pos > (double)rStart)
                {
                    double t = (fadeEndPos - pos) / (double)loopFade;
                    float gBwd = (float)std::cos(t * juce::MathConstants<double>::halfPi);
                    float gFwd = (float)std::sin(t * juce::MathConstants<double>::halfPi);
                    double posFwd = (double)rStart + (fadeEndPos - pos);

                    float sBwd = getSampleAtPos(channelIdx, pos);
                    float sFwd = getSampleAtPos(channelIdx, posFwd);
                    return sBwd * gBwd + sFwd * gFwd;
                }
            }
            else if (isLooping)
            {
                double fadeStartPos = (double)(rEnd - loopFade);
                if (pos >= fadeStartPos && pos < (double)rEnd)
                {
                    double t = (pos - fadeStartPos) / (double)loopFade;
                    float gFwd = (float)std::cos(t * juce::MathConstants<double>::halfPi);
                    float gBwd = (float)std::sin(t * juce::MathConstants<double>::halfPi);
                    double posBwd = (double)rEnd - (pos - fadeStartPos);

                    float sFwd = getSampleAtPos(channelIdx, pos);
                    float sBwd = getSampleAtPos(channelIdx, posBwd);
                    return sFwd * gFwd + sBwd * gBwd;
                }
            }
        }

        return getSampleAtPos(channelIdx, pos);
    };

    // Temporary accumulation buffers for this block
    constexpr int MAX_BLOCK_SIZE = 4096;
    int curBlockSize = juce::jmin(numSamples, MAX_BLOCK_SIZE);
    float blockOutL[MAX_BLOCK_SIZE];
    float blockOutR[MAX_BLOCK_SIZE];
    std::fill(blockOutL, blockOutL + curBlockSize, 0.0f);
    std::fill(blockOutR, blockOutR + curBlockSize, 0.0f);

    float voiceOutInterleaved[MAX_BLOCK_SIZE * 2];
    float feedInterleaved[512 * 2];

    int fadeSamples = (int)(targetSampleRate.load() * 0.004); // 4ms micro-fade
    if (fadeSamples < 1) fadeSamples = 1;

    float glideAlpha = (glideSec > 0.0005f) ? (1.0f - std::exp(-(float)curBlockSize / (glideSec * (float)targetSampleRate.load()))) : 1.0f;

    for (auto& v : voices)
    {
        if (!v.active && !v.ampEnvelope.isActive() && !v.isQuickFadingOut)
            continue;

        // Sample-accurate exponential pitch interpolation towards target note
        v.currentPitchSemitones += glideAlpha * (v.targetPitchSemitones - v.currentPitchSemitones);

        // Compute pitch ratio for this specific voice's smoothed pitch
        float voiceDriftSemis = (isPoly && driftAmt > 0.0001f) ? (driftAmt * v.timbreDriftOffset * 0.15f) : 0.0f;
        float noteSemis = v.currentPitchSemitones;
        float totalSemis = noteSemis + semis + voiceDriftSemis;

        // Configure SoundTouch parameters
        if (pitMode == PitchMode::Resample)
        {
            double rateRatio = std::pow(2.0, (double)totalSemis / 12.0) * baseSpeedRatio;
            if (std::abs(rateRatio - v.lastAppliedRate) > 1e-5 || v.lastAppliedPitchMode != PitchMode::Resample)
            {
                v.soundTouch.setRate(rateRatio);
                v.soundTouch.setPitch(1.0);
                v.soundTouch.setTempo(1.0);
                v.lastAppliedRate = rateRatio;
                v.lastAppliedPitchMode = PitchMode::Resample;
            }
        }
        else // PitchMode::Stretch
        {
            if (std::abs(totalSemis - v.lastAppliedPitchSemitones) > 1e-4f || std::abs(baseSpeedRatio - v.lastAppliedRate) > 1e-5 || v.lastAppliedPitchMode != PitchMode::Stretch)
            {
                v.soundTouch.setPitchSemiTones((double)totalSemis);
                v.soundTouch.setRate(baseSpeedRatio);
                v.soundTouch.setTempo(1.0);
                v.lastAppliedPitchSemitones = totalSemis;
                v.lastAppliedRate = baseSpeedRatio;
                v.lastAppliedPitchMode = PitchMode::Stretch;
            }
        }

        PlaybackMode effMode = (pMode == PlaybackMode::Random) ? v.effectivePlaybackMode : pMode;

        // Feed SoundTouch pipeline from source buffer until it has enough samples for this block
        while (v.soundTouch.numSamples() < (uint)curBlockSize && v.active)
        {
            int samplesToFeed = 256;
            int samplesFed = 0;
            bool hitEnd = false;

            for (int s = 0; s < samplesToFeed; ++s)
            {
                // Boundary check
                if (effMode == PlaybackMode::Forward)
                {
                    if (v.currentSample >= (double)rEnd)
                    {
                        if (isLooping)
                        {
                            int loopFade = juce::jmax(1, juce::jmin((int)(targetSampleRate.load() * 0.010), rLen / 2));
                            v.currentSample = (double)rStart + (double)loopFade;
                        }
                        else
                        {
                            hitEnd = true;
                            break;
                        }
                    }
                }
                else if (effMode == PlaybackMode::Backward)
                {
                    if (v.currentSample <= (double)rStart)
                    {
                        if (isLooping)
                        {
                            int loopFade = juce::jmax(1, juce::jmin((int)(targetSampleRate.load() * 0.010), rLen / 2));
                            v.currentSample = (double)(rEnd - loopFade);
                        }
                        else
                        {
                            hitEnd = true;
                            break;
                        }
                    }
                }
                else if (effMode == PlaybackMode::ForwBackw)
                {
                    if (v.playDirectionForward && v.currentSample >= (double)rEnd)
                    {
                        int loopFade = juce::jmax(1, juce::jmin((int)(targetSampleRate.load() * 0.010), rLen / 2));
                        v.currentSample = (double)(rEnd - loopFade);
                        v.playDirectionForward = false;
                    }
                    else if (!v.playDirectionForward && v.currentSample <= (double)rStart)
                    {
                        if (isLooping)
                        {
                            int loopFade = juce::jmax(1, juce::jmin((int)(targetSampleRate.load() * 0.010), rLen / 2));
                            v.currentSample = (double)rStart + (double)loopFade;
                            v.playDirectionForward = true;
                        }
                        else
                        {
                            hitEnd = true;
                            break;
                        }
                    }
                }
                else if (effMode == PlaybackMode::BackForw)
                {
                    if (!v.playDirectionForward && v.currentSample <= (double)rStart)
                    {
                        int loopFade = juce::jmax(1, juce::jmin((int)(targetSampleRate.load() * 0.010), rLen / 2));
                        v.currentSample = (double)rStart + (double)loopFade;
                        v.playDirectionForward = true;
                    }
                    else if (v.playDirectionForward && v.currentSample >= (double)rEnd)
                    {
                        if (isLooping)
                        {
                            int loopFade = juce::jmax(1, juce::jmin((int)(targetSampleRate.load() * 0.010), rLen / 2));
                            v.currentSample = (double)(rEnd - loopFade);
                            v.playDirectionForward = false;
                        }
                        else
                        {
                            hitEnd = true;
                            break;
                        }
                    }
                }

                feedInterleaved[2 * s]     = readLoopCrossfadedSample(v, 0, v.currentSample, effMode);
                feedInterleaved[2 * s + 1] = readLoopCrossfadedSample(v, 1, v.currentSample, effMode);

                if (effMode == PlaybackMode::Backward || (!v.playDirectionForward && (effMode == PlaybackMode::ForwBackw || effMode == PlaybackMode::BackForw)))
                    v.currentSample -= 1.0;
                else
                    v.currentSample += 1.0;

                samplesFed++;
            }

            if (samplesFed > 0)
            {
                v.soundTouch.putSamples(feedInterleaved, (uint)samplesFed);
            }

            if (hitEnd)
            {
                v.soundTouch.flush();
                break;
            }
        }

        uint received = v.soundTouch.receiveSamples(voiceOutInterleaved, (uint)curBlockSize);
        if (received < (uint)curBlockSize)
        {
            // Smoothly micro-fade out the tail of the received audio before zeroing remaining frames
            if (received > 0)
            {
                int tailFade = juce::jmin((int)received, 64);
                int fadeStart = (int)received - tailFade;
                for (int i = 0; i < tailFade; ++i)
                {
                    float factor = 0.5f * (1.0f + std::cos(juce::MathConstants<float>::pi * (float)i / (float)tailFade));
                    voiceOutInterleaved[2 * (fadeStart + i)]     *= factor;
                    voiceOutInterleaved[2 * (fadeStart + i) + 1] *= factor;
                }
            }

            std::fill(voiceOutInterleaved + received * 2, voiceOutInterleaved + curBlockSize * 2, 0.0f);
            if (received == 0 && !isLooping)
            {
                v.active = false;
                v.soundTouch.clear();
            }
        }

        // Apply Envelope, Anti-Click and Quick Fade Out across the block
        for (int i = 0; i < curBlockSize; ++i)
        {
            float rawL = voiceOutInterleaved[2 * i];
            float rawR = voiceOutInterleaved[2 * i + 1];

            float ampVal = v.ampEnvelope.getNextSample() * v.velocity;
            float antiClickGain = 1.0f;

            if (v.samplesProcessed < fadeSamples)
            {
                antiClickGain *= (float)v.samplesProcessed / (float)fadeSamples;
            }
            v.samplesProcessed++;

            if (v.isQuickFadingOut)
            {
                if (v.quickFadeOutTotalSamples > 0)
                {
                    float qGain = (float)v.quickFadeOutSamplesLeft / (float)v.quickFadeOutTotalSamples;
                    antiClickGain *= juce::jlimit(0.0f, 1.0f, qGain);
                }
                v.quickFadeOutSamplesLeft--;
                if (v.quickFadeOutSamplesLeft <= 0)
                {
                    v.active = false;
                    v.isQuickFadingOut = false;
                    v.ampEnvelope.reset();
                    v.soundTouch.clear();
                }
            }

            blockOutL[i] += rawL * ampVal * antiClickGain;
            blockOutR[i] += rawR * ampVal * antiClickGain;

            if (!v.ampEnvelope.isActive() && v.releasing && !v.isQuickFadingOut)
            {
                v.active = false;
                v.soundTouch.clear();
            }
        }
    }

    // Polyphonic master gain compensation, exciter, and soft limiting
    for (int i = 0; i < curBlockSize; ++i)
    {
        globalSampleCounter++;
        smoothedPolyGainScale += 0.005f * (targetPolyGainScale - smoothedPolyGainScale);

        float sumL = blockOutL[i] * smoothedPolyGainScale;
        float sumR = blockOutR[i] * smoothedPolyGainScale;

        if (exciterAmount > 0.001f)
        {
            auto applyExciter = [](float x, float amt) -> float {
                if (std::abs(x) < 1e-6f) return x;
                float drive = 1.0f + amt * 2.5f;
                float driven = x * drive;
                float evenHarmonic = 0.25f * amt * (driven * driven);
                float asymmetricSignal = driven + (driven >= 0.0f ? evenHarmonic : -evenHarmonic * 0.5f);
                float saturated = std::tanh(asymmetricSignal);
                float makeupGain = 1.0f / (1.0f + amt * 0.4f);
                return (1.0f - amt) * x + amt * (saturated * makeupGain);
            };

            sumL = applyExciter(sumL, exciterAmount);
            sumR = applyExciter(sumR, exciterAmount);
        }

        sumL = std::tanh(sumL);
        sumR = std::tanh(sumR);

        if (output.getNumChannels() > 0)
            output.addSample(0, startSample + i, sumL);

        if (output.getNumChannels() > 1)
            output.addSample(1, startSample + i, sumR);
    }
}