#pragma once

#include <JuceHeader.h>
#include "PresetManager.h"

struct HistoryEntry
{
    juce::String id;
    juce::String label;
    juce::int64 timestampMs = 0;
    juce::String formattedTime;

    juce::String sampleFileName;
    float startRegion = 0.0f;
    float endRegion = 1.0f;
    bool loopEnabled = false;
    juce::var selectionsVar;

    juce::File snapshotFile;
};

class HistoryManager
{
public:
    static constexpr int MAX_HISTORY_ITEMS = 10;

    HistoryManager();
    ~HistoryManager() = default;

    juce::Array<HistoryEntry> getHistoryEntries() const;

    bool pushHistoryState(const juce::String& label,
                          const juce::String& sampleFileName,
                          juce::AudioProcessorValueTreeState& apvts,
                          float startRegion,
                          float endRegion,
                          bool loopEnabled,
                          const juce::var& selectionsVar,
                          const juce::AudioBuffer<float>& sampleAudio,
                          double sampleRate);

    bool restoreHistoryEntry(const HistoryEntry& entry,
                             juce::AudioProcessorValueTreeState& apvts,
                             juce::String& loadedSampleFileName,
                             float& loadedStartRegion,
                             float& loadedEndRegion,
                             juce::var& loadedSelectionsVar,
                             bool& loadedLoopEnabled,
                             juce::AudioBuffer<float>& outAudioBuffer,
                             double& outSampleRate);

    void clearHistory();

private:
    juce::File historyFolder;

    void ensureHistoryFolderExists();
    void pruneOldest();
};
