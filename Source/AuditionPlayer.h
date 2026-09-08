#pragma once

#include <JuceHeader.h>
#include "AudioResampler.h"

struct SampleMetadata
{
    juce::String fileName;
    juce::String format = "WAV";
    int bitDepth = 24;
    double sampleRate = 44100.0;
    int numChannels = 2;
    double durationSeconds = 0.0;
    juce::String detectedKey = "-";
    juce::String pathBreadcrumbs;
    juce::StringArray tags;
};

class AuditionPlayer
{
public:
    AuditionPlayer();
    ~AuditionPlayer() = default;

    void prepare(double sampleRate);

    // Audio file loading & preview
    bool loadFile(const juce::File& file, bool autoPlay = true);
    void loadSyntheticPresetWaveform(const juce::String& presetName, const juce::String& category);
    void play();
    void pause();
    void stop();
    void togglePlayPause();

    void setPlayheadNormalized(double normalizedPos);

    bool isPlaying() const { return isPlayingState.load(std::memory_order_relaxed); }
    double getPlayheadNormalized() const;
    double getCurrentTimeSeconds() const;
    double getTotalDurationSeconds() const { return currentMetadata.durationSeconds; }

    const SampleMetadata& getCurrentMetadata() const { return currentMetadata; }
    const std::vector<std::pair<float, float>>& getWaveformMinMax() const { return waveformPeaks; }

    // Mixed into processBlock without touching synth voices
    void processBlock(juce::AudioBuffer<float>& outputBuffer, int startSample, int numSamples);

    static juce::String detectKeyFromFileName(const juce::String& fileName);
    static juce::StringArray generateTags(const juce::File& file, const SampleMetadata& meta);
    static double getQuickDurationSeconds(const juce::File& file);

private:
    juce::AudioFormatManager formatManager;
    double hostSampleRate = 44100.0;

    juce::AudioBuffer<float> playBuffer;
    std::vector<std::pair<float, float>> waveformPeaks; // min, max per bin
    SampleMetadata currentMetadata;

    std::atomic<bool> isPlayingState{ false };
    std::atomic<int64_t> currentSampleIndex{ 0 };
    int64_t totalSampleCount = 0;

    juce::CriticalSection bufferLock;

    void generateWaveformPeaks(const juce::AudioBuffer<float>& buffer, int numBins = 512);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AuditionPlayer)
};
