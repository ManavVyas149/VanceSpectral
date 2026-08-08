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
        v.filterEnvelope.prepareToPlay(targetSampleRate);
    }

    updateFilteredSample();
}

void SampleEngine::loadSample(const juce::AudioBuffer<float>& buffer, double sampleNativeRate)
{
    const juce::ScopedLock sl(lock);

    sample = buffer;
    nativeSampleRate = sampleNativeRate > 0.0 ? sampleNativeRate : 44100.0;

    regionStart = 0;
    regionEnd = sample.getNumSamples();

    for (auto& v : voices)
        v.reset();

    playing = false;

    updateFilteredSample();

    DBG("Sample Loaded: " << sample.getNumSamples() << " samples at " << nativeSampleRate << " Hz");
}

void SampleEngine::updateFilteredSample()
{
    // Pre-filter the source sample buffer using active frequency regions/bands FIRST
    // This produces filteredSample, which becomes the source material for all pitch-shifted voices
    filteredSample = sample;

    int totalSamples = sample.getNumSamples();
    int numCh = sample.getNumChannels();

    if (totalSamples == 0 || numCh == 0)
        return;

    double sr = nativeSampleRate.load() > 0.0 ? nativeSampleRate.load() : 44100.0;
    float nyquist = (float)(sr * 0.49);

    if (!spectralRegions.isEmpty())
    {
        juce::OwnedArray<RegionFilterPair> filters;
        for (const auto& r : spectralRegions)
        {
            auto* pair = filters.add(std::make_unique<RegionFilterPair>());
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

        int targetFadeSamples = (int)(sr * 0.010); // 10ms equal-power crossfade

        for (int i = 0; i < totalSamples; ++i)
        {
            float inL = sample.getSample(0, i);
            float inR = (numCh > 1) ? sample.getSample(1, i) : inL;

            float sumL = 0.0f;
            float sumR = 0.0f;
            float sumRegionGainSq = 0.0f;

            for (auto* rfp : filters)
            {
                int rStartSample = (int)(rfp->region.startNorm * (float)totalSamples);
                int rEndSample = (int)(rfp->region.endNorm * (float)totalSamples);
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

                    // Equal-power region boundary fading
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

            filteredSample.setSample(0, i, finalL);
            if (numCh > 1) filteredSample.setSample(1, i, finalR);
        }
    }
    else if (freqFilterEnabled && !filterBands.isEmpty())
    {
        juce::OwnedArray<BandFilter> filters;
        for (const auto& band : filterBands)
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
            float inL = sample.getSample(0, i);
            float inR = (numCh > 1) ? sample.getSample(1, i) : inL;

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
            filteredSample.setSample(0, i, sumL);
            if (numCh > 1) filteredSample.setSample(1, i, sumR);
        }
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
    pitchMode = static_cast<PitchMode>(juce::jlimit(0, 2, modeIndex));
}

void SampleEngine::setPitchSemitones(float semitones)
{
    pitchSemitones = semitones;
}

void SampleEngine::setTimbreSemitones(float semitones)
{
    timbreSemitones = juce::jlimit(-24.0f, 24.0f, semitones);
}

void SampleEngine::setTimbreLink(bool linked)
{
    timbreLink = linked;
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
    v.targetPitchSemitones = (float)(noteNumber - 60);
    v.timbreDriftOffset = (juce::Random::getSystemRandom().nextFloat() * 2.0f - 1.0f);
    v.filterStateL = 0.0f;
    v.filterStateR = 0.0f;

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
    v.pitchPhase = 0.0;
    v.samplesProcessed = 0;

    v.ampEnvelope.noteOn();
    v.filterEnvelope.noteOn();
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

    float targetSemis = (float)(midiNoteNumber - 60);
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
                v.filterEnvelope.noteOff();
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
                v.filterEnvelope.noteOff();
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

    int rStart = (int)(startNormalized * (float)sample.getNumSamples());
    int rEnd = (int)(endNormalized * (float)sample.getNumSamples());

    rStart = juce::jlimit(0, sample.getNumSamples() - 1, rStart);
    rEnd = juce::jlimit(rStart + 1, sample.getNumSamples(), rEnd);

    regionStart = rStart;
    regionEnd = rEnd;
}

void SampleEngine::updateAmpADSR(float attack, float decay, float sustain, float release)
{
    const juce::ScopedLock sl(lock);
    for (auto& v : voices)
        v.ampEnvelope.updateADSR(attack, decay, sustain, release);
}

void SampleEngine::updateFilterADSR(float attack, float decay, float sustain, float release)
{
    const juce::ScopedLock sl(lock);
    for (auto& v : voices)
        v.filterEnvelope.updateADSR(attack, decay, sustain, release);
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
    updateFilteredSample();
}

void SampleEngine::setSpectralRegions(const juce::Array<SpectralRegion>& regions)
{
    const juce::ScopedLock sl(lock);
    spectralRegions = regions;

    // Quick fade-out active voices mid-playback to prevent clicks during live region updates/rerolls
    int fadeSamples = (int)(targetSampleRate.load() * 0.004);
    for (auto& v : voices)
    {
        if (v.active && !v.isQuickFadingOut)
            v.startQuickFadeOut(fadeSamples);
    }

    updateFilteredSample();
}

void SampleEngine::process(juce::AudioBuffer<float>& output,
                            int startSample,
                            int numSamples)
{
    const juce::ScopedTryLock sl(lock);
    if (!sl.isLocked())
        return;

    // Check if any voice is active or in envelope release
    int activeCount = 0;
    for (const auto& v : voices)
    {
        if (v.active || v.ampEnvelope.isActive())
            activeCount++;
    }

    if (activeCount == 0)
        return;

    const auto& srcBuffer = (filteredSample.getNumSamples() > 0) ? filteredSample : sample;
    int totalSamples = srcBuffer.getNumSamples();
    int numSampleChannels = srcBuffer.getNumChannels();

    if (totalSamples == 0 || numSampleChannels == 0)
        return;

    double baseSpeedRatio = (nativeSampleRate > 0.0 && targetSampleRate > 0.0) ? (nativeSampleRate / targetSampleRate) : 1.0;
    int rEnd = regionEnd.load();
    int rStart = regionStart.load();
    bool isLooping = looping.load();
    auto pMode = playbackMode.load();
    auto pitMode = pitchMode.load();
    float semis = pitchSemitones.load();
    float tSemis = timbreSemitones.load();
    bool tLinked = timbreLink.load();
    float driftAmt = timbreDriftAmount.load();
    bool isPoly = polyMode.load();

    // Voice-count aware gain compensation for polyphony
    float targetPolyGainScale = 1.0f / std::sqrt(juce::jmax(1.0f, (float)activeCount));

    float glideSec = glideTimeMs.load() * 0.001f;
    float glideAlpha = (glideSec > 0.0005f) ? (1.0f - std::exp(-1.0f / (glideSec * (float)targetSampleRate.load()))) : 1.0f;

    for (int i = 0; i < numSamples; ++i)
    {
        globalSampleCounter++;
        smoothedPolyGainScale += 0.005f * (targetPolyGainScale - smoothedPolyGainScale);
        float sumL = 0.0f;
        float sumR = 0.0f;

        for (auto& v : voices)
        {
            if (!v.active && !v.ampEnvelope.isActive() && !v.isQuickFadingOut)
                continue;

            // Sample-accurate exponential pitch interpolation towards target note
            v.currentPitchSemitones += glideAlpha * (v.targetPitchSemitones - v.currentPitchSemitones);

            // Compute pitch ratio for this specific voice's smoothed pitch
            float totalSemis = (pitMode == PitchMode::Axial) ? semis : (v.currentPitchSemitones + semis);
            float voicePitchRatio = std::pow(2.0f, totalSemis / 12.0f);

            float voiceDriftSemis = (isPoly && driftAmt > 0.0001f) ? (driftAmt * v.timbreDriftOffset * 4.0f) : 0.0f;

            float effectiveFormantSemis = 0.0f;
            if (pitMode == PitchMode::Axial)
            {
                effectiveFormantSemis = 0.0f;
            }
            else if (tLinked)
            {
                effectiveFormantSemis = totalSemis + voiceDriftSemis;
            }
            else
            {
                effectiveFormantSemis = tSemis + voiceDriftSemis;
            }
            float voiceFormantRatio = std::pow(2.0f, effectiveFormantSemis / 12.0f);

            double speedRatio = baseSpeedRatio;
            float activePitchRatio = 1.0f;

            if (pitMode == PitchMode::Resample)
            {
                if (tLinked)
                {
                    speedRatio = baseSpeedRatio * (double)voicePitchRatio;
                    activePitchRatio = 1.0f;
                }
                else
                {
                    speedRatio = baseSpeedRatio * (double)voiceFormantRatio;
                    activePitchRatio = voicePitchRatio / juce::jmax(0.001f, voiceFormantRatio);
                }
            }
            else if (pitMode == PitchMode::Stretch)
            {
                if (tLinked)
                {
                    speedRatio = baseSpeedRatio;
                    activePitchRatio = voicePitchRatio;
                }
                else
                {
                    speedRatio = baseSpeedRatio * (double)voiceFormantRatio;
                    activePitchRatio = voicePitchRatio / juce::jmax(0.001f, voiceFormantRatio);
                }
            }
            else // Axial (Fixed)
            {
                speedRatio = baseSpeedRatio;
                activePitchRatio = 1.0f;
            }

            // Boundary & direction logic per voice
            PlaybackMode effMode = (pMode == PlaybackMode::Random) ? v.effectivePlaybackMode : pMode;
            int rLen = rEnd - rStart;

            if (v.active)
            {
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
                            int fadeSamples = (int)(targetSampleRate.load() * 0.004);
                            v.startQuickFadeOut(fadeSamples);
                            v.ampEnvelope.noteOff();
                            v.filterEnvelope.noteOff();
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
                            int fadeSamples = (int)(targetSampleRate.load() * 0.004);
                            v.startQuickFadeOut(fadeSamples);
                            v.ampEnvelope.noteOff();
                            v.filterEnvelope.noteOff();
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
                            int fadeSamples = (int)(targetSampleRate.load() * 0.004);
                            v.startQuickFadeOut(fadeSamples);
                            v.ampEnvelope.noteOff();
                            v.filterEnvelope.noteOff();
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
                            int fadeSamples = (int)(targetSampleRate.load() * 0.004);
                            v.startQuickFadeOut(fadeSamples);
                            v.ampEnvelope.noteOff();
                            v.filterEnvelope.noteOff();
                        }
                    }
                }
            }

            float ampVal = v.ampEnvelope.getNextSample() * v.velocity;
            float filterVal = v.filterEnvelope.getNextSample();

            auto getSampleAtPos = [&](int channelIdx, double samplePos) -> float {
                if (totalSamples == 0 || numSampleChannels == 0 || std::isnan(samplePos) || std::isinf(samplePos)) return 0.0f;
                int ch = juce::jlimit(0, numSampleChannels - 1, channelIdx);

                double clampedPos = juce::jlimit(0.0, (double)(totalSamples - 1), samplePos);
                int t0 = (int)std::floor(clampedPos);
                int t1 = juce::jmin(totalSamples - 1, t0 + 1);
                float fr = (float)(clampedPos - (double)t0);

                float s0 = srcBuffer.getSample(ch, t0);
                float s1 = srcBuffer.getSample(ch, t1);
                return s0 + fr * (s1 - s0);
            };

            auto readLoopCrossfadedSample = [&](int channelIdx, double pos) -> float {
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

            float rawSampleL = 0.0f;
            float rawSampleR = 0.0f;

            if (pitMode != PitchMode::Axial && std::abs(activePitchRatio - 1.0f) > 0.001f)
            {
                int grainSize = (int)(targetSampleRate * 0.035);
                if (grainSize < 64) grainSize = 64;
                int halfGrain = grainSize / 2;

                double normPhase1 = std::fmod(v.pitchPhase, (double)grainSize) / (double)grainSize;
                double normPhase2 = std::fmod(v.pitchPhase + (double)halfGrain, (double)grainSize) / (double)grainSize;

                float win1 = 0.5f * (1.0f - std::cos(2.0f * juce::MathConstants<float>::pi * (float)normPhase1));
                float win2 = 0.5f * (1.0f - std::cos(2.0f * juce::MathConstants<float>::pi * (float)normPhase2));

                double shift1 = (normPhase1 - 0.5) * (double)grainSize * (1.0 - (double)activePitchRatio);
                double shift2 = (normPhase2 - 0.5) * (double)grainSize * (1.0 - (double)activePitchRatio);

                rawSampleL = win1 * readLoopCrossfadedSample(0, v.currentSample + shift1) + win2 * readLoopCrossfadedSample(0, v.currentSample + shift2);
                rawSampleR = win1 * readLoopCrossfadedSample(1, v.currentSample + shift1) + win2 * readLoopCrossfadedSample(1, v.currentSample + shift2);

                v.pitchPhase += 1.0;
            }
            else
            {
                rawSampleL = readLoopCrossfadedSample(0, v.currentSample);
                rawSampleR = readLoopCrossfadedSample(1, v.currentSample);
            }

            float cutoff = 20.0f + filterVal * 18000.0f;
            float alpha = juce::jlimit(0.01f, 0.99f, 1.0f - std::exp(-2.0f * juce::MathConstants<float>::pi * cutoff / (float)targetSampleRate));

            v.filterStateL += alpha * (rawSampleL - v.filterStateL);
            v.filterStateR += alpha * (rawSampleR - v.filterStateR);

            int fadeSamples = (int)(targetSampleRate.load() * 0.004); // 4ms subtle anti-click fade
            if (fadeSamples < 1) fadeSamples = 1;

            float antiClickGain = 1.0f;

            // Subtle anti-click fade-in at very start of voice playback (0..4ms)
            if (v.samplesProcessed < fadeSamples)
            {
                antiClickGain *= (float)v.samplesProcessed / (float)fadeSamples;
            }
            v.samplesProcessed++;

            // Subtle anti-click fade-out at end of region boundary (when not looping)
            if (!isLooping)
            {
                if (effMode == PlaybackMode::Forward || (v.playDirectionForward && (effMode == PlaybackMode::ForwBackw || effMode == PlaybackMode::BackForw)))
                {
                    double remaining = (double)rEnd - v.currentSample;
                    if (remaining >= 0.0 && remaining < (double)fadeSamples)
                    {
                        antiClickGain *= (float)juce::jlimit(0.0, 1.0, remaining / (double)fadeSamples);
                    }
                }
                else if (effMode == PlaybackMode::Backward || (!v.playDirectionForward && (effMode == PlaybackMode::ForwBackw || effMode == PlaybackMode::BackForw)))
                {
                    double remaining = v.currentSample - (double)rStart;
                    if (remaining >= 0.0 && remaining < (double)fadeSamples)
                    {
                        antiClickGain *= (float)juce::jlimit(0.0, 1.0, remaining / (double)fadeSamples);
                    }
                }
            }

            // Quick fade-out when voice is cut short or stolen
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
                    v.filterEnvelope.reset();
                }
            }

            sumL += v.filterStateL * ampVal * antiClickGain;
            sumR += v.filterStateR * ampVal * antiClickGain;

            if (v.active)
            {
                if (effMode == PlaybackMode::Backward || (!v.playDirectionForward && (effMode == PlaybackMode::ForwBackw || effMode == PlaybackMode::BackForw)))
                    v.currentSample -= speedRatio;
                else
                    v.currentSample += speedRatio;
            }

            if (!v.ampEnvelope.isActive() && v.releasing && !v.isQuickFadingOut)
                v.active = false;
        }

        // Apply gain compensation scaling across polyphonic voices
        sumL *= smoothedPolyGainScale;
        sumR *= smoothedPolyGainScale;

        // Apply SCORCH-style Exciter processing at the full polyphonic bus level
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

        // Soft limiting to prevent master bus clipping on dense chords
        sumL = std::tanh(sumL);
        sumR = std::tanh(sumR);

        if (output.getNumChannels() > 0)
            output.addSample(0, startSample + i, sumL);

        if (output.getNumChannels() > 1)
            output.addSample(1, startSample + i, sumR);
    }
}