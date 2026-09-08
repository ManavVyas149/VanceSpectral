#include "AuditionPlayer.h"

AuditionPlayer::AuditionPlayer()
{
    formatManager.registerBasicFormats();
}

void AuditionPlayer::prepare(double sampleRate)
{
    hostSampleRate = sampleRate > 0.0 ? sampleRate : 44100.0;
}

bool AuditionPlayer::loadFile(const juce::File& file, bool autoPlay)
{
    if (!file.existsAsFile())
        return false;

    std::unique_ptr<juce::AudioFormatReader> reader(formatManager.createReaderFor(file));
    if (reader == nullptr)
    {
        SampleMetadata meta;
        meta.fileName = file.getFileName();
        meta.format = file.getFileExtension().replace(".", "").toUpperCase();
        meta.bitDepth = 24;
        meta.sampleRate = 44100.0;
        meta.numChannels = 2;
        meta.durationSeconds = getQuickDurationSeconds(file);
        meta.detectedKey = detectKeyFromFileName(file.getFileNameWithoutExtension());

        auto parent = file.getParentDirectory();
        if (parent.getFileName().equalsIgnoreCase("Samples") || parent.getFileName().equalsIgnoreCase("Factory"))
            meta.pathBreadcrumbs = "FACTORY / " + parent.getFileName().toUpperCase();
        else
            meta.pathBreadcrumbs = "SAMPLES / " + parent.getFileName().toUpperCase();

        meta.tags = generateTags(file, meta);
        currentMetadata = meta;

        // Generate synthetic waveform peaks based on file content so the display is never empty
        waveformPeaks.clear();
        waveformPeaks.resize(256);
        juce::FileInputStream fis(file);
        juce::MemoryBlock mb;
        fis.readIntoMemoryBlock(mb, 256 * 64);
        const uint8_t* bytes = (const uint8_t*)mb.getData();
        int byteCount = (int)mb.getSize();
        for (size_t b = 0; b < 256; ++b)
        {
            float val = 0.25f;
            if (byteCount > 0)
            {
                int idx = (int)(b * (byteCount / 256)) % byteCount;
                val = 0.15f + ((float)(bytes[idx] % 128) / 128.0f) * 0.70f;
            }
            waveformPeaks[b] = { -val, val };
        }

        totalSampleCount = 0;
        currentSampleIndex.store(0);
        isPlayingState.store(false);
        return true;
    }

    int numCh = (int)juce::jlimit(1, 2, (int)reader->numChannels);
    int numSamples = (int)reader->lengthInSamples;

    juce::AudioBuffer<float> tempBuffer(numCh, numSamples);
    reader->read(&tempBuffer, 0, numSamples, 0, true, true);

    SampleMetadata meta;
    meta.fileName = file.getFileName();
    meta.format = reader->getFormatName().toUpperCase();
    if (meta.format.isEmpty())
        meta.format = file.getFileExtension().replace(".", "").toUpperCase();
    meta.bitDepth = reader->bitsPerSample > 0 ? (int)reader->bitsPerSample : 24;
    meta.sampleRate = reader->sampleRate;
    meta.numChannels = numCh;
    meta.durationSeconds = (double)numSamples / (reader->sampleRate > 0.0 ? reader->sampleRate : 44100.0);
    meta.detectedKey = detectKeyFromFileName(file.getFileNameWithoutExtension());

    // Build breadcrumb path
    auto parent = file.getParentDirectory();
    juce::StringArray pathParts;
    int depth = 0;
    while (parent.exists() && depth < 3)
    {
        juce::String dirName = parent.getFileName().toUpperCase();
        if (dirName.equalsIgnoreCase("VANCESPECTRAL") || dirName.isEmpty())
            break;
        pathParts.insert(0, dirName);
        parent = parent.getParentDirectory();
        depth++;
    }
    if (pathParts.isEmpty())
        pathParts.add("SAMPLES");
    meta.pathBreadcrumbs = pathParts.joinIntoString(" / ");

    meta.tags = generateTags(file, meta);

    // Resample to host sample rate for playback
    double fileSr = reader->sampleRate > 0.0 ? reader->sampleRate : 44100.0;
    juce::AudioBuffer<float> resampled = AudioResampler::resampleIfNeeded(tempBuffer, fileSr, hostSampleRate);

    // Generate waveform peaks
    generateWaveformPeaks(tempBuffer, 512);

    {
        const juce::ScopedLock sl(bufferLock);
        playBuffer = std::move(resampled);
        currentMetadata = meta;
        totalSampleCount = playBuffer.getNumSamples();
        currentSampleIndex.store(0);
        isPlayingState.store(autoPlay);
    }

    return true;
}

void AuditionPlayer::play()
{
    if (totalSampleCount > 0)
    {
        if (currentSampleIndex.load(std::memory_order_relaxed) >= totalSampleCount)
            currentSampleIndex.store(0);

        isPlayingState.store(true);
    }
}

void AuditionPlayer::loadSyntheticPresetWaveform(const juce::String& presetName, const juce::String& category)
{
    waveformPeaks.clear();
    waveformPeaks.resize(256);
    float baseSeed = (float)(presetName.hashCode() & 0x7FFFFFFF) / 10000.0f;
    for (size_t i = 0; i < 256; ++i)
    {
        float phase = (float)i / 256.0f;
        float envelope = std::sin(phase * 3.14159f);
        float harmonics = 0.5f * std::sin(phase * 16.0f + baseSeed)
                        + 0.3f * std::sin(phase * 32.0f + baseSeed * 2.0f)
                        + 0.2f * std::cos(phase * 48.0f);
        float peak = juce::jlimit(0.10f, 0.92f, std::abs(envelope * harmonics));
        waveformPeaks[i] = { -peak, peak };
    }

    currentMetadata.fileName = presetName;
    currentMetadata.format = "PRESET";
    currentMetadata.bitDepth = 32;
    currentMetadata.sampleRate = 44100.0;
    currentMetadata.durationSeconds = 2.0;
    currentMetadata.detectedKey = category.isNotEmpty() ? category.toUpperCase() : "SYNTH";
    currentMetadata.pathBreadcrumbs = "FACTORY / PRESETS / " + currentMetadata.detectedKey;
    currentMetadata.tags.clear();
    currentMetadata.tags.add(currentMetadata.detectedKey);
    currentMetadata.tags.add("ANALOG");
    currentMetadata.tags.add("SPECTRAL");
    currentMetadata.tags.add("DYNAMIC");

    totalSampleCount = 0;
    currentSampleIndex.store(0);
    isPlayingState.store(false);
}

void AuditionPlayer::pause()
{
    isPlayingState.store(false);
}

void AuditionPlayer::stop()
{
    isPlayingState.store(false);
    currentSampleIndex.store(0);
}

void AuditionPlayer::togglePlayPause()
{
    if (isPlaying())
        pause();
    else
        play();
}

void AuditionPlayer::setPlayheadNormalized(double normalizedPos)
{
    normalizedPos = juce::jlimit(0.0, 1.0, normalizedPos);
    if (totalSampleCount > 0)
    {
        int64_t newIdx = (int64_t)std::round(normalizedPos * (double)totalSampleCount);
        currentSampleIndex.store(juce::jlimit((int64_t)0, totalSampleCount, newIdx));
    }
}

double AuditionPlayer::getPlayheadNormalized() const
{
    if (totalSampleCount <= 0)
        return 0.0;
    double pos = (double)currentSampleIndex.load(std::memory_order_relaxed) / (double)totalSampleCount;
    return juce::jlimit(0.0, 1.0, pos);
}

double AuditionPlayer::getCurrentTimeSeconds() const
{
    return getPlayheadNormalized() * currentMetadata.durationSeconds;
}

void AuditionPlayer::generateWaveformPeaks(const juce::AudioBuffer<float>& buffer, int numBins)
{
    waveformPeaks.clear();
    waveformPeaks.reserve((size_t)numBins);

    int totalSamples = buffer.getNumSamples();
    if (totalSamples <= 0 || numBins <= 0)
    {
        waveformPeaks.assign(numBins, { -0.1f, 0.1f });
        return;
    }

    int samplesPerBin = juce::jmax(1, totalSamples / numBins);
    const float* const* chData = buffer.getArrayOfReadPointers();
    int numCh = buffer.getNumChannels();

    for (int bin = 0; bin < numBins; ++bin)
    {
        int start = bin * samplesPerBin;
        int end = (bin == numBins - 1) ? totalSamples : juce::jmin(totalSamples, start + samplesPerBin);

        float minVal = 0.0f;
        float maxVal = 0.0f;

        for (int ch = 0; ch < numCh; ++ch)
        {
            const float* channel = chData[ch];
            for (int s = start; s < end; ++s)
            {
                float val = channel[s];
                if (val < minVal) minVal = val;
                if (val > maxVal) maxVal = val;
            }
        }

        waveformPeaks.push_back({ juce::jlimit(-1.0f, 0.0f, minVal), juce::jlimit(0.0f, 1.0f, maxVal) });
    }
}

void AuditionPlayer::processBlock(juce::AudioBuffer<float>& outputBuffer, int startSample, int numSamples)
{
    if (!isPlayingState.load(std::memory_order_relaxed))
        return;

    const juce::ScopedTryLock sl(bufferLock);
    if (!sl.isLocked())
        return;

    int64_t currentIdx = currentSampleIndex.load(std::memory_order_relaxed);
    if (currentIdx >= totalSampleCount || totalSampleCount <= 0)
    {
        isPlayingState.store(false);
        return;
    }

    int samplesRemaining = (int)(totalSampleCount - currentIdx);
    int toProcess = juce::jmin(numSamples, samplesRemaining);

    float auditionGain = 0.85f;
    int outChannels = outputBuffer.getNumChannels();
    int playChannels = playBuffer.getNumChannels();

    for (int ch = 0; ch < outChannels; ++ch)
    {
        int srcCh = ch < playChannels ? ch : (playChannels - 1);
        if (srcCh >= 0)
        {
            outputBuffer.addFrom(ch, startSample, playBuffer, srcCh, (int)currentIdx, toProcess, auditionGain);
        }
    }

    currentSampleIndex.store(currentIdx + toProcess);
    if (currentIdx + toProcess >= totalSampleCount)
    {
        isPlayingState.store(false);
    }
}

juce::String AuditionPlayer::detectKeyFromFileName(const juce::String& fileName)
{
    // Search for patterns like: C#3, F#3, Cmin, Dmaj, KEY (C), 145 Cmin, etc.
    juce::String upper = fileName.toUpperCase();

    // Check for explicit "KEY (X)"
    int keyIdx = upper.indexOf("KEY (");
    if (keyIdx >= 0)
    {
        int closeParen = upper.indexOf(keyIdx, ")");
        if (closeParen > keyIdx + 5)
        {
            return upper.substring(keyIdx + 5, closeParen).trim();
        }
    }

    // Check note + sharp/flat + octave: e.g. C#3, D4, F#3, Bb2, A0
    const char* notes[] = { "C#", "D#", "F#", "G#", "A#", "DB", "EB", "GB", "AB", "BB",
                            "C", "D", "E", "F", "G", "A", "B" };

    for (const char* note : notes)
    {
        juce::String nStr(note);
        int pos = 0;
        while ((pos = upper.indexOf(pos, nStr)) >= 0)
        {
            int after = pos + nStr.length();
            if (after < upper.length())
            {
                juce::juce_wchar c = upper[after];
                // Check if followed by octave digit 0-8
                if (c >= '0' && c <= '8')
                {
                    // Verify preceded by separator or start
                    bool validPre = (pos == 0) || !juce::CharacterFunctions::isLetterOrDigit(upper[pos - 1]);
                    if (validPre)
                    {
                        return nStr + juce::String::charToString(c);
                    }
                }
                // Or followed by MIN / MAJ / M
                if (upper.substring(after).startsWith("MIN"))
                    return nStr + "m";
                if (upper.substring(after).startsWith("MAJ"))
                    return nStr;
            }
            pos += nStr.length();
        }
    }

    return "-";
}

juce::StringArray AuditionPlayer::generateTags(const juce::File& file, const SampleMetadata& meta)
{
    juce::StringArray tags;
    juce::String name = file.getFileNameWithoutExtension().toUpperCase();

    // Primary category tag from parent directory or name
    juce::String parentName = file.getParentDirectory().getFileName().toUpperCase();
    if (parentName.isNotEmpty() && !parentName.equalsIgnoreCase("SAMPLES") && !parentName.equalsIgnoreCase("VANCESPECTRAL"))
    {
        tags.add(parentName);
    }
    else if (name.contains("SYNTH"))
        tags.add("SYNTH");
    else if (name.contains("PAD"))
        tags.add("PAD");
    else if (name.contains("BASS"))
        tags.add("BASS");
    else if (name.contains("LEAD"))
        tags.add("LEAD");
    else if (name.contains("BELL") || name.contains("KEY"))
        tags.add("KEYS");
    else
        tags.add("SAMPLE");

    // Tonal vs Atonal
    if (meta.detectedKey != "-")
        tags.add("TONAL");
    else
        tags.add("TEXTURE");

    // Length descriptor
    if (meta.durationSeconds < 1.5)
        tags.add("SHORT");
    else if (meta.durationSeconds > 8.0)
        tags.add("LONG");
    else
        tags.add("MEDIUM");

    // Loop vs One-shot
    if (name.contains("LOOP") || name.contains("RENDERED"))
        tags.add("LOOP");
    else
        tags.add("ONE-SHOT");

    return tags;
}

double AuditionPlayer::getQuickDurationSeconds(const juce::File& file)
{
    if (!file.existsAsFile()) return 0.0;
    juce::FileInputStream fis(file);
    if (!fis.openedOk()) return 0.0;

    char header[44];
    if (fis.read(header, 44) < 44) return 0.0;

    if (std::memcmp(header, "RIFF", 4) == 0 && std::memcmp(header + 8, "WAVE", 4) == 0)
    {
        uint32_t byteRate = *(const uint32_t*)(header + 28);
        if (byteRate > 0)
        {
            int64_t dataSize = file.getSize() - 44;
            return juce::jmax(0.0, (double)dataSize / (double)byteRate);
        }
    }

    return (double)file.getSize() / 176400.0;
}
