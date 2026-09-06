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

    // The currently browsed/scanned root folder. Defaults to getPresetsFolder() and persists
    // across sessions (see loadPersistedRoot()/persistRoot()). Banks are its immediate
    // subdirectories; presets are files directly inside a bank subdirectory.
    juce::File getPresetsRoot() const { return presetsRoot; }

    // Returns true if newRoot was a valid, existing directory and became the new root.
    // Returns false if newRoot was invalid/missing, in which case the root falls back to
    // getPresetsFolder() (the default presets directory) instead.
    bool setPresetsRoot(const juce::File& newRoot);

    juce::Array<PresetInfo> getAllPresets() const;
    juce::StringArray getAllBanks() const;
    juce::Array<PresetInfo> getPresetsInBank(const juce::String& bankName) const;
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
                    bool isFavorite = false,
                    bool loopEnabled = false,
                    const juce::AudioBuffer<float>* sampleAudioBuffer = nullptr,
                    double sampleRate = 44100.0);

    bool loadPreset(const juce::File& presetFile,
                    juce::AudioProcessorValueTreeState& apvts,
                    juce::String& loadedSampleFileName,
                    float& loadedStartRegion,
                    float& loadedEndRegion,
                    juce::var& loadedSelectionsVar,
                    bool& loadedLoopEnabled,
                    juce::AudioBuffer<float>* outAudioBuffer = nullptr,
                    double* outSampleRate = nullptr);

    bool deletePreset(const juce::File& presetFile);

    static juce::String audioBufferToBase64Wav(const juce::AudioBuffer<float>& buffer, double sampleRate = 44100.0);
    static bool base64WavToAudioBuffer(const juce::String& base64Str, juce::AudioBuffer<float>& outBuffer, double& outSampleRate);

    juce::File findMatchingSample(const juce::File& sourceFile) const;
    juce::File importSample(const juce::File& sourceFile, bool overwriteExisting = false, const juce::File& sampleToReplace = {});
    bool renameSample(const juce::File& sampleFile, const juce::String& newName);
    bool deleteSample(const juce::File& sampleFile);

    void createDefaultFactoryPresets(juce::AudioProcessorValueTreeState& apvts);
    void invalidateCache() const { isCacheValid = false; }

private:
    juce::File presetsFolder;
    juce::File samplesFolder;
    juce::File presetsRoot;
    juce::File rootSettingsFile;

    mutable juce::Array<PresetInfo> cachedPresets;
    mutable bool isCacheValid = false;

    void ensureDirectoriesExist();
    void ensureRootExists();
    void loadPersistedRoot();
    void persistRoot() const;
    juce::Array<PresetInfo> scanBankDirectory(const juce::File& bankDir, const juce::String& bankName) const;
    void createFactoryPreset(const juce::String& name, const juce::String& category,
                             float ampA, float ampD, float ampS, float ampR);
};
