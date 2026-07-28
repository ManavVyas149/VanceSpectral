#include "PresetManager.h"

PresetManager::PresetManager()
{
    ensureDirectoriesExist();
}

void PresetManager::ensureDirectoriesExist()
{
    auto userDocs = juce::File::getSpecialLocation(juce::File::userDocumentsDirectory);
    auto appDir = userDocs.getChildFile("VanceSpectral");
    
    if (!appDir.exists())
        appDir.createDirectory();

    presetsFolder = appDir.getChildFile("Presets");
    samplesFolder = appDir.getChildFile("Samples");

    if (!presetsFolder.exists())
        presetsFolder.createDirectory();

    if (!samplesFolder.exists())
        samplesFolder.createDirectory();

    // Auto-create factory presets if empty
    auto existingFiles = presetsFolder.findChildFiles(juce::File::TypesOfFileToFind::findFiles, false, "*.json");
    if (existingFiles.isEmpty())
    {
        createFactoryPreset("Cold Synth", "Synth", 0.05f, 0.20f, 0.8f, 0.40f, 0.02f, 0.30f, 0.5f, 0.35f);
        createFactoryPreset("Spectral Lead", "Lead", 0.01f, 0.15f, 0.9f, 0.25f, 0.01f, 0.20f, 0.7f, 0.20f);
        createFactoryPreset("Cyber Bass", "Bass", 0.005f, 0.30f, 0.6f, 0.20f, 0.005f, 0.40f, 0.4f, 0.15f);
        createFactoryPreset("Neon Pad", "Pad", 0.80f, 1.20f, 0.85f, 1.50f, 0.60f, 1.00f, 0.75f, 1.20f);
        createFactoryPreset("Vibe Synth", "Synth", 0.08f, 0.25f, 0.75f, 0.50f, 0.04f, 0.35f, 0.6f, 0.40f);
        createFactoryPreset("Glitch Pulse", "FX", 0.001f, 0.08f, 0.3f, 0.10f, 0.001f, 0.10f, 0.2f, 0.08f);
    }
}

void PresetManager::createFactoryPreset(const juce::String& name, const juce::String& category,
                                          float ampA, float ampD, float ampS, float ampR,
                                          float filtA, float filtD, float filtS, float filtR)
{
    juce::File presetFile = presetsFolder.getChildFile(name + ".json");
    
    auto* obj = new juce::DynamicObject();
    obj->setProperty("name", name);
    obj->setProperty("category", category);
    obj->setProperty("sample", "");

    auto* paramsObj = new juce::DynamicObject();
    paramsObj->setProperty("AMP_ATTACK", ampA);
    paramsObj->setProperty("AMP_DECAY", ampD);
    paramsObj->setProperty("AMP_SUSTAIN", ampS);
    paramsObj->setProperty("AMP_RELEASE", ampR);
    paramsObj->setProperty("FILTER_ATTACK", filtA);
    paramsObj->setProperty("FILTER_DECAY", filtD);
    paramsObj->setProperty("FILTER_SUSTAIN", filtS);
    paramsObj->setProperty("FILTER_RELEASE", filtR);

    obj->setProperty("parameters", paramsObj);

    juce::var presetVar(obj);
    juce::String jsonString = juce::JSON::toString(presetVar);

    juce::FileOutputStream stream(presetFile);
    if (stream.openedOk())
    {
        stream.setPosition(0);
        stream.truncate();
        stream.writeText(jsonString, false, false, nullptr);
        stream.flush();
    }
}

juce::Array<PresetInfo> PresetManager::getAllPresets() const
{
    juce::Array<PresetInfo> presets;
    if (!presetsFolder.exists())
        return presets;

    auto files = presetsFolder.findChildFiles(juce::File::TypesOfFileToFind::findFiles, false, "*.json");

    for (const auto& file : files)
    {
        juce::var parsed = juce::JSON::parse(file.loadFileAsString());
        if (parsed.isObject())
        {
            PresetInfo info;
            info.file = file;
            info.name = parsed.getProperty("name", file.getFileNameWithoutExtension()).toString();
            info.category = parsed.getProperty("category", "General").toString();
            info.sampleFileName = parsed.getProperty("sample", "").toString();
            presets.add(info);
        }
    }

    return presets;
}

juce::Array<juce::File> PresetManager::getAllSamples() const
{
    juce::Array<juce::File> samples;
    if (!samplesFolder.exists())
        return samples;

    auto files = samplesFolder.findChildFiles(juce::File::TypesOfFileToFind::findFiles, false, "*.wav;*.mp3;*.flac;*.aiff;*.ogg;*.m4a");
    for (const auto& file : files)
    {
        samples.add(file);
    }

    return samples;
}

bool PresetManager::savePreset(const juce::String& presetName,
                                const juce::String& category,
                                const juce::String& sampleFileName,
                                juce::AudioProcessorValueTreeState& apvts)
{
    ensureDirectoriesExist();

    juce::String finalName = presetName.trim();
    if (finalName.isEmpty())
    {
        auto existing = getAllPresets();
        finalName = "Preset " + juce::String(existing.size() + 1);
    }

    juce::String cleanName = juce::File::createLegalFileName(finalName);
    if (cleanName.isEmpty())
        cleanName = "Preset_1";

    juce::File presetFile = presetsFolder.getChildFile(cleanName + ".json");
    presetFile.getParentDirectory().createDirectory();

    auto* obj = new juce::DynamicObject();
    obj->setProperty("name", finalName);
    obj->setProperty("category", category.trim().isEmpty() ? "User" : category.trim());
    obj->setProperty("sample", sampleFileName);

    auto* paramsObj = new juce::DynamicObject();

    const char* paramIDs[] = {
        "AMP_ATTACK", "AMP_DECAY", "AMP_SUSTAIN", "AMP_RELEASE",
        "FILTER_ATTACK", "FILTER_DECAY", "FILTER_SUSTAIN", "FILTER_RELEASE"
    };

    for (const auto& id : paramIDs)
    {
        if (auto* param = apvts.getParameter(id))
        {
            paramsObj->setProperty(id, param->getValue());
        }
    }

    obj->setProperty("parameters", paramsObj);

    juce::var presetVar(obj);
    juce::String jsonString = juce::JSON::toString(presetVar);

    juce::FileOutputStream stream(presetFile);
    if (stream.openedOk())
    {
        stream.setPosition(0);
        stream.truncate();
        stream.writeText(jsonString, false, false, nullptr);
        stream.flush();
        return true;
    }

    return false;
}

bool PresetManager::loadPreset(const juce::File& presetFile,
                                juce::AudioProcessorValueTreeState& apvts,
                                juce::String& loadedSampleFileName)
{
    if (!presetFile.existsAsFile())
        return false;

    juce::var parsed = juce::JSON::parse(presetFile.loadFileAsString());
    if (!parsed.isObject())
        return false;

    loadedSampleFileName = parsed.getProperty("sample", "").toString();

    auto paramsVar = parsed.getProperty("parameters", juce::var());
    if (paramsVar.isObject())
    {
        auto* paramsObj = paramsVar.getDynamicObject();
        for (const auto& prop : paramsObj->getProperties())
        {
            juce::String paramID = prop.name.toString();
            float normValue = (float)prop.value;

            if (auto* param = apvts.getParameter(paramID))
            {
                param->setValueNotifyingHost(normValue);
            }
        }
    }

    return true;
}

bool PresetManager::deletePreset(const juce::File& presetFile)
{
    if (presetFile.existsAsFile())
    {
        return presetFile.deleteFile();
    }
    return false;
}

juce::File PresetManager::importSample(const juce::File& sourceFile)
{
    ensureDirectoriesExist();

    if (!sourceFile.existsAsFile())
        return {};

    juce::File targetFile = samplesFolder.getChildFile(sourceFile.getFileName());
    if (targetFile.existsAsFile())
    {
        targetFile = samplesFolder.getChildFile(sourceFile.getFileNameWithoutExtension() + "_" + juce::String(juce::Random::getSystemRandom().nextInt(1000)) + sourceFile.getFileExtension());
    }

    if (sourceFile.copyFileTo(targetFile))
    {
        return targetFile;
    }

    return {};
}

void PresetManager::createDefaultFactoryPresets(juce::AudioProcessorValueTreeState& apvts)
{
    auto existing = getAllPresets();
    if (!existing.isEmpty())
        return;

    savePreset("Cold Synth", "Synth", "", apvts);
    savePreset("Spectral Lead", "Lead", "", apvts);
    savePreset("Cyber Bass", "Bass", "", apvts);
    savePreset("Neon Pad", "Pad", "", apvts);
    savePreset("Vibe Synth", "Synth", "", apvts);
    savePreset("Glitch Pulse", "FX", "", apvts);
}
