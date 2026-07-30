#pragma once

#include <JuceHeader.h>

struct PresetInfo
{
    juce::String name;
    juce::String category;
    juce::String bank;
    juce::String sampleFileName;
    bool isFavorite = false;
    bool isFactory = false;
    juce::int64 lastUsed = 0;
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
    juce::StringArray getAllBanks() const;
    juce::Array<juce::File> getAllSamples() const;

    bool createBank(const juce::String& bankName);
    bool renameBank(const juce::String& oldBankName, const juce::String& newBankName);
    bool deleteBank(const juce::String& bankName);
    bool importBankPackage(const juce::File& fileOrFolder);

    bool toggleFavorite(const juce::File& presetFile);

    bool savePreset(const juce::String& presetName,
                    const juce::String& category,
                    const juce::String& bankName,
                    const juce::String& sampleFileName,
                    juce::AudioProcessorValueTreeState& apvts,
                    float startRegion = 0.0f,
                    float endRegion = 1.0f,
                    const juce::var& selectionsVar = juce::var(),
                    bool isFavorite = false);

    bool loadPreset(const juce::File& presetFile,
                    juce::AudioProcessorValueTreeState& apvts,
                    juce::String& loadedSampleFileName,
                    float& loadedStartRegion,
                    float& loadedEndRegion,
                    juce::var& loadedSelectionsVar);

    bool deletePreset(const juce::File& presetFile);

    juce::File findMatchingSample(const juce::File& sourceFile) const;
    juce::File importSample(const juce::File& sourceFile, bool overwriteExisting = false, const juce::File& sampleToReplace = {});
    bool renameSample(const juce::File& sampleFile, const juce::String& newName);
    bool deleteSample(const juce::File& sampleFile);

    void createDefaultFactoryPresets(juce::AudioProcessorValueTreeState& apvts);

private:
    juce::File presetsFolder;
    juce::File samplesFolder;

    void ensureDirectoriesExist();
    void createFactoryPreset(const juce::String& name, const juce::String& category,
                             float ampA, float ampD, float ampS, float ampR,
                             float filtA, float filtD, float filtS, float filtR);
};
