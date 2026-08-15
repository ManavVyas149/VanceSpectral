#include "HistoryManager.h"

HistoryManager::HistoryManager()
{
    ensureHistoryFolderExists();
}

void HistoryManager::ensureHistoryFolderExists()
{
    auto docsDir = juce::File::getSpecialLocation(juce::File::userDocumentsDirectory);
    auto appDir = docsDir.getChildFile("VanceSpectral");
    auto presetsDir = appDir.getChildFile("Presets");
    
    if (!presetsDir.exists())
        presetsDir.createDirectory();

    historyFolder = presetsDir.getChildFile(".history");
    if (!historyFolder.exists())
        historyFolder.createDirectory();
}

void HistoryManager::pruneOldest()
{
    ensureHistoryFolderExists();
    auto files = historyFolder.findChildFiles(juce::File::TypesOfFileToFind::findFiles, false, "*.json");

    if (files.size() <= MAX_HISTORY_ITEMS)
        return;

    // Sort files by creation / modification time or filename timestamp
    juce::Array<juce::File> sortedFiles = files;
    std::sort(sortedFiles.begin(), sortedFiles.end(), [](const juce::File& a, const juce::File& b) {
        return a.getLastModificationTime() < b.getLastModificationTime();
    });

    while (sortedFiles.size() > MAX_HISTORY_ITEMS)
    {
        auto oldest = sortedFiles.removeAndReturn(0);
        oldest.deleteFile();
    }
}

juce::Array<HistoryEntry> HistoryManager::getHistoryEntries() const
{
    juce::Array<HistoryEntry> entries;
    if (!historyFolder.exists())
        return entries;

    auto files = historyFolder.findChildFiles(juce::File::TypesOfFileToFind::findFiles, false, "*.json");

    for (const auto& file : files)
    {
        juce::var parsed = juce::JSON::parse(file.loadFileAsString());
        if (parsed.isObject())
        {
            HistoryEntry entry;
            entry.snapshotFile = file;
            entry.id = parsed.getProperty("id", "").toString();
            entry.label = parsed.getProperty("label", "Edit State").toString();
            entry.timestampMs = (juce::int64)parsed.getProperty("timestamp", (juce::int64)file.getLastModificationTime().toMilliseconds());
            
            juce::Time t(entry.timestampMs);
            entry.formattedTime = parsed.getProperty("formattedTime", t.formatted("%b %d, %H:%M:%S")).toString();

            entry.sampleFileName = parsed.getProperty("sampleFileName", "").toString();
            entry.startRegion = (float)(double)parsed.getProperty("startRegion", 0.0);
            entry.endRegion = (float)(double)parsed.getProperty("endRegion", 1.0);
            entry.loopEnabled = (bool)parsed.getProperty("loopEnabled", false);
            entry.selectionsVar = parsed.getProperty("selections", juce::var());

            entries.add(entry);
        }
    }

    // Sort newest first
    std::sort(entries.begin(), entries.end(), [](const HistoryEntry& a, const HistoryEntry& b) {
        return a.timestampMs > b.timestampMs;
    });

    return entries;
}

bool HistoryManager::pushHistoryState(const juce::String& label,
                                      const juce::String& sampleFileName,
                                      juce::AudioProcessorValueTreeState& apvts,
                                      float startRegion,
                                      float endRegion,
                                      bool loopEnabled,
                                      const juce::var& selectionsVar,
                                      const juce::AudioBuffer<float>& sampleAudio,
                                      double sampleRate)
{
    ensureHistoryFolderExists();

    juce::int64 nowMs = juce::Time::currentTimeMillis();
    juce::Time nowTime(nowMs);
    juce::String timeStr = nowTime.formatted("%b %d, %H:%M:%S");

    // Deduplication check against the most recent entry
    auto existingEntries = getHistoryEntries();
    if (!existingEntries.isEmpty())
    {
        const auto& newest = existingEntries[0];
        // If snapshot taken less than 2 seconds ago with same sample, ignore
        if (std::abs(nowMs - newest.timestampMs) < 2000 && newest.sampleFileName == sampleFileName)
            return false;
    }

    juce::String snapshotName = "hist_" + juce::String(nowMs) + ".json";
    juce::File snapshotFile = historyFolder.getChildFile(snapshotName);

    std::vector<std::pair<juce::String, float>> paramValues;
    const char* paramIDs[] = {
        "AMP_ATTACK", "AMP_DECAY", "AMP_SUSTAIN", "AMP_RELEASE",
        "FILTER_ATTACK", "FILTER_DECAY", "FILTER_SUSTAIN", "FILTER_RELEASE",
        "PLAYBACK_MODE", "PITCH_MODE", "PITCH_SEMITONES",
        "TIMBRE_SEMITONES", "TIMBRE_LINK", "TIMBRE_DRIFT",
        "EXCITER", "POLY_MODE", "GLIDE", "GAIN"
    };
    for (const auto& id : paramIDs)
    {
        if (auto* param = apvts.getParameter(id))
            paramValues.push_back({ id, param->getValue() });
    }

    juce::AudioBuffer<float> audioCopy = sampleAudio;

    juce::Thread::launch([this, snapshotFile, label, nowMs, timeStr, sampleFileName, startRegion, endRegion, loopEnabled, selectionsVar, audioCopy, sampleRate, paramValues]() {
        auto* obj = new juce::DynamicObject();
        obj->setProperty("id", juce::Uuid().toString());
        obj->setProperty("label", label.isEmpty() ? "Edit Snapshot" : label);
        obj->setProperty("timestamp", (juce::int64)nowMs);
        obj->setProperty("formattedTime", timeStr);
        obj->setProperty("sampleFileName", sampleFileName);
        obj->setProperty("startRegion", startRegion);
        obj->setProperty("endRegion", endRegion);
        obj->setProperty("loopEnabled", loopEnabled);
        obj->setProperty("selections", selectionsVar);

        if (audioCopy.getNumSamples() > 0)
        {
            juce::String b64Wav = PresetManager::audioBufferToBase64Wav(audioCopy, sampleRate);
            if (b64Wav.isNotEmpty())
                obj->setProperty("audioData", b64Wav);
        }

        auto* paramsObj = new juce::DynamicObject();
        for (const auto& [id, val] : paramValues)
        {
            paramsObj->setProperty(id, val);
        }
        obj->setProperty("parameters", paramsObj);

        juce::var snapshotVar(obj);
        juce::String jsonStr = juce::JSON::toString(snapshotVar);

        juce::FileOutputStream stream(snapshotFile);
        if (stream.openedOk())
        {
            stream.setPosition(0);
            stream.truncate();
            stream.writeText(jsonStr, false, false, nullptr);
            stream.flush();

            pruneOldest();
        }
    });

    return true;
}

bool HistoryManager::restoreHistoryEntry(const HistoryEntry& entry,
                                          juce::AudioProcessorValueTreeState& apvts,
                                          juce::String& loadedSampleFileName,
                                          float& loadedStartRegion,
                                          float& loadedEndRegion,
                                          juce::var& loadedSelectionsVar,
                                          bool& loadedLoopEnabled,
                                          juce::AudioBuffer<float>& outAudioBuffer,
                                          double& outSampleRate)
{
    if (!entry.snapshotFile.existsAsFile())
        return false;

    juce::var parsed = juce::JSON::parse(entry.snapshotFile.loadFileAsString());
    if (!parsed.isObject())
        return false;

    loadedSampleFileName = parsed.getProperty("sampleFileName", "").toString();
    loadedStartRegion = (float)(double)parsed.getProperty("startRegion", 0.0);
    loadedEndRegion = (float)(double)parsed.getProperty("endRegion", 1.0);
    loadedLoopEnabled = (bool)parsed.getProperty("loopEnabled", false);
    loadedSelectionsVar = parsed.getProperty("selections", juce::var());

    juce::String audioDataStr = parsed.getProperty("audioData", "").toString();
    if (audioDataStr.isNotEmpty())
    {
        double sr = 44100.0;
        if (PresetManager::base64WavToAudioBuffer(audioDataStr, outAudioBuffer, sr))
        {
            outSampleRate = sr;
        }
    }

    auto paramsVar = parsed.getProperty("parameters", juce::var());
    if (paramsVar.isObject())
    {
        auto* paramsObj = paramsVar.getDynamicObject();
        for (const auto& prop : paramsObj->getProperties())
        {
            juce::String paramID = prop.name.toString();
            float normValue = static_cast<float>(static_cast<double>(prop.value));

            if (auto* param = apvts.getParameter(paramID))
            {
                param->setValueNotifyingHost(normValue);
            }
        }
    }

    return true;
}

void HistoryManager::clearHistory()
{
    ensureHistoryFolderExists();
    auto files = historyFolder.findChildFiles(juce::File::TypesOfFileToFind::findFiles, false, "*.json");
    for (const auto& f : files)
        f.deleteFile();
}
