#pragma once

#include <JuceHeader.h>

struct PresetInfo
{
    juce::String name;
    juce::String category;
    juce::String sampleFileName;
    juce::File file;
};

class PresetManager
{
public:
    PresetManager();
    ~PresetManager() = default;

    juce::File getPresetsFolder() const { return presetsFolder; }
    juce::File getSamplesFolder() const { return samplesFolder; }

    juce::Array<PresetInfo> getAllPresets() const;
    juce::Array<juce::File> getAllSamples() const;

    bool savePreset(const juce::String& presetName,
                    const juce::String& category,
                    const juce::String& sampleFileName,
                    juce::AudioProcessorValueTreeState& apvts);

    bool loadPreset(const juce::File& presetFile,
                    juce::AudioProcessorValueTreeState& apvts,
                    juce::String& loadedSampleFileName);

    bool deletePreset(const juce::File& presetFile);

    juce::File importSample(const juce::File& sourceFile);

    void createDefaultFactoryPresets(juce::AudioProcessorValueTreeState& apvts);

private:
    juce::File presetsFolder;
    juce::File samplesFolder;

    void ensureDirectoriesExist();
    void createFactoryPreset(const juce::String& name, const juce::String& category,
                         float ampA, float ampD, float ampS, float ampR,
                         float filtA, float filtD, float filtS, float filtR);
};
