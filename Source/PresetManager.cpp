#include "PresetManager.h"

PresetManager::PresetManager()
{
    ensureDirectoriesExist();
}

void PresetManager::ensureDirectoriesExist()
{
    auto docsDir = juce::File::getSpecialLocation(juce::File::userDocumentsDirectory);
    auto appDir = docsDir.getChildFile("VanceSpectral");
    
    if (!appDir.exists())
    {
        appDir.createDirectory();
    }

    // Migrate any legacy presets from AppData\Roaming\VanceSpectral if they exist
    auto legacyAppDataDir = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory).getChildFile("VanceSpectral");
    if (legacyAppDataDir.exists())
    {
        auto legacyPresets = legacyAppDataDir.getChildFile("Presets");
        if (legacyPresets.exists())
        {
            legacyPresets.copyDirectoryTo(appDir.getChildFile("Presets"));
        }
        auto legacySamples = legacyAppDataDir.getChildFile("Samples");
        if (legacySamples.exists())
        {
            legacySamples.copyDirectoryTo(appDir.getChildFile("Samples"));
        }
    }

    presetsFolder = appDir.getChildFile("Presets");
    samplesFolder = appDir.getChildFile("Samples");

    if (!presetsFolder.exists())
        presetsFolder.createDirectory();

    if (!samplesFolder.exists())
        samplesFolder.createDirectory();

    auto factoryDir = presetsFolder.getChildFile("Factory");
    if (!factoryDir.exists())
        factoryDir.createDirectory();

    auto userDir = presetsFolder.getChildFile("User");
    if (!userDir.exists())
        userDir.createDirectory();

    // Legacy migration: automatically convert any *.json files in Presets folder to .vsts or .vsfx
    auto allJsonFiles = presetsFolder.findChildFiles(juce::File::TypesOfFileToFind::findFiles, true, "*.json");
    for (const auto& f : allJsonFiles)
    {
        juce::String relPath = f.getRelativePathFrom(presetsFolder);
        if (relPath.startsWith(".") || relPath.contains("/.") || relPath.contains("\\."))
            continue;

        juce::String fName = f.getFileNameWithoutExtension();
        if (fName.equalsIgnoreCase("Prod_verve") || fName.equalsIgnoreCase("Test"))
        {
            f.deleteFile();
            continue;
        }

        juce::var parsed = juce::JSON::parse(f.loadFileAsString());
        if (parsed.isObject())
        {
            bool isState = parsed.hasProperty("audioData")
                        || parsed.getProperty("category", "").toString().equalsIgnoreCase("STATES")
                        || fName.containsIgnoreCase("State");

            juce::String targetExt = isState ? ".vsts" : ".vsfx";
            juce::String cat = isState ? "STATES" : parsed.getProperty("category", "FX").toString();

            parsed.getDynamicObject()->setProperty("category", cat);
            f.replaceWithText(juce::JSON::toString(parsed));

            juce::File newFile = f.withFileExtension(targetExt);
            f.moveFileTo(newFile);
        }
    }

    // Auto-create factory presets if empty
    auto existingFiles = presetsFolder.findChildFiles(juce::File::TypesOfFileToFind::findFiles, true, "*.vsts;*.vsfx;*.json");
    if (existingFiles.isEmpty())
    {
        createFactoryPreset("Cold Synth", "Synth", 0.05f, 0.20f, 0.8f, 0.40f);
        createFactoryPreset("Spectral Lead", "Lead", 0.01f, 0.15f, 0.9f, 0.25f);
        createFactoryPreset("Cyber Bass", "Bass", 0.005f, 0.30f, 0.6f, 0.20f);
        createFactoryPreset("Neon Pad", "Pad", 0.80f, 1.20f, 0.85f, 1.50f);
        createFactoryPreset("Vibe Synth", "Synth", 0.08f, 0.25f, 0.75f, 0.50f);
        createFactoryPreset("Glitch Pulse", "FX", 0.001f, 0.08f, 0.3f, 0.10f);
    }
}

void PresetManager::createFactoryPreset(const juce::String& name, const juce::String& category,
                                          float ampA, float ampD, float ampS, float ampR)
{
    auto factoryFolder = presetsFolder.getChildFile("Factory");
    factoryFolder.createDirectory();
    juce::File presetFile = factoryFolder.getChildFile(name + ".vsfx");
    
    auto* obj = new juce::DynamicObject();
    obj->setProperty("name", name);
    obj->setProperty("category", category);
    obj->setProperty("bank", "Factory");
    obj->setProperty("sample", "");
    obj->setProperty("isFavorite", false);
    obj->setProperty("isFactory", true);
    obj->setProperty("lastUsed", (juce::int64)0);

    auto* paramsObj = new juce::DynamicObject();
    paramsObj->setProperty("AMP_ATTACK", ampA);
    paramsObj->setProperty("AMP_DECAY", ampD);
    paramsObj->setProperty("AMP_SUSTAIN", ampS);
    paramsObj->setProperty("AMP_RELEASE", ampR);
    paramsObj->setProperty("PLAYBACK_MODE", 0.0f);
    paramsObj->setProperty("PITCH_MODE", 0.0f);
    paramsObj->setProperty("PITCH_SEMITONES", 0.0f);
    paramsObj->setProperty("EXCITER", 0.0f);
    paramsObj->setProperty("POLY_MODE", 0.0f);
    paramsObj->setProperty("GLIDE", 0.0f);
    paramsObj->setProperty("GAIN", 0.0f);
    paramsObj->setProperty("FX_SIDECHAIN_ENABLE", 0.0f);
    paramsObj->setProperty("FX_SIDECHAIN_MIX", 0.0f);
    paramsObj->setProperty("FX_SIDECHAIN_RATE", 2.0f);
    paramsObj->setProperty("FX_CHORUS_ENABLE", 0.0f);
    paramsObj->setProperty("FX_CHORUS_AMOUNT", 0.0f);
    paramsObj->setProperty("FX_CHORUS_RATE", 1.0f);
    paramsObj->setProperty("FX_PHASER_ENABLE", 0.0f);
    paramsObj->setProperty("FX_PHASER_AMOUNT", 0.0f);
    paramsObj->setProperty("FX_PHASER_RATE", 0.5f);
    paramsObj->setProperty("FX_DELAY_ENABLE", 0.0f);
    paramsObj->setProperty("FX_DELAY_AMOUNT", 0.0f);
    paramsObj->setProperty("FX_DELAY_TIME", 250.0f);
    paramsObj->setProperty("FX_DELAY_FEEDBACK", 0.35f);
    paramsObj->setProperty("FX_DRIVE_ENABLE", 0.0f);
    paramsObj->setProperty("FX_DRIVE_AMOUNT", 0.0f);
    paramsObj->setProperty("FX_DRIVE_TONE", 0.5f);

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
    if (isCacheValid)
        return cachedPresets;

    cachedPresets.clear();
    if (!presetsFolder.exists())
    {
        isCacheValid = true;
        return cachedPresets;
    }

    auto files = presetsFolder.findChildFiles(juce::File::TypesOfFileToFind::findFiles, true, "*.vsts;*.vsfx;*.json");

    for (const auto& file : files)
    {
        // Skip files in hidden directories like .history
        juce::String relPath = file.getRelativePathFrom(presetsFolder);
        if (relPath.startsWith(".") || relPath.contains("/.") || relPath.contains("\\."))
            continue;

        juce::var parsed = juce::JSON::parse(file.loadFileAsString());
        if (parsed.isObject())
        {
            PresetInfo info;
            info.file = file;
            info.name = parsed.getProperty("name", file.getFileNameWithoutExtension()).toString();
            
            juce::String cat = parsed.getProperty("category", "").toString();
            if (file.hasFileExtension(".vsts"))
            {
                cat = "STATES";
            }
            else if (cat.isEmpty())
            {
                cat = "FX";
            }
            info.category = cat;

            juce::String folderBank = file.getParentDirectory().getFileName();
            if (folderBank.equalsIgnoreCase("Presets"))
                folderBank = "User";

            info.bank = parsed.getProperty("bank", folderBank).toString();
            info.sampleFileName = parsed.getProperty("sample", "").toString();
            info.isFavorite = (bool)parsed.getProperty("isFavorite", false);
            info.isFactory = (bool)parsed.getProperty("isFactory", info.bank.equalsIgnoreCase("Factory"));
            info.lastUsed = (juce::int64)parsed.getProperty("lastUsed", (juce::int64)0);

            cachedPresets.add(info);
        }
    }

    isCacheValid = true;
    return cachedPresets;
}

juce::StringArray PresetManager::getAllBanks() const
{
    juce::StringArray banks;
    banks.add("Factory");
    banks.add("User");

    if (presetsFolder.exists())
    {
        auto subDirs = presetsFolder.findChildFiles(juce::File::TypesOfFileToFind::findDirectories, false);
        for (const auto& dir : subDirs)
        {
            juce::String name = dir.getFileName();
            if (name.startsWith("."))
                continue;

            if (!banks.contains(name, true))
                banks.add(name);
        }
    }

    auto presets = getAllPresets();
    for (const auto& p : presets)
    {
        if (p.bank.isNotEmpty() && !p.bank.startsWith(".") && !banks.contains(p.bank, true))
            banks.add(p.bank);
    }

    return banks;
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

bool PresetManager::createBank(const juce::String& bankName)
{
    juce::String clean = juce::File::createLegalFileName(bankName.trim());
    if (clean.isEmpty())
        return false;

    ensureDirectoriesExist();
    auto dir = presetsFolder.getChildFile(clean);
    if (!dir.exists())
    {
        dir.createDirectory();
    }

    return dir.isDirectory() && dir.exists();
}

bool PresetManager::renameBank(const juce::String& oldBankName, const juce::String& newBankName)
{
    if (oldBankName.equalsIgnoreCase("Factory") || newBankName.trim().isEmpty())
        return false;

    juce::String cleanOld = juce::File::createLegalFileName(oldBankName.trim());
    juce::String cleanNew = juce::File::createLegalFileName(newBankName.trim());

    auto oldDir = presetsFolder.getChildFile(cleanOld);
    auto newDir = presetsFolder.getChildFile(cleanNew);

    if (!oldDir.exists())
        return false;

    if (oldDir.moveFileTo(newDir))
    {
        auto files = newDir.findChildFiles(juce::File::TypesOfFileToFind::findFiles, true, "*.json");
        for (const auto& file : files)
        {
            juce::var parsed = juce::JSON::parse(file.loadFileAsString());
            if (parsed.isObject())
            {
                parsed.getDynamicObject()->setProperty("bank", cleanNew);
                file.replaceWithText(juce::JSON::toString(parsed));
            }
        }
        return true;
    }
    return false;
}

bool PresetManager::deleteBank(const juce::String& bankName)
{
    if (bankName.equalsIgnoreCase("Factory") || bankName.equalsIgnoreCase("User"))
        return false;

    juce::String clean = juce::File::createLegalFileName(bankName.trim());
    auto dir = presetsFolder.getChildFile(clean);
    if (dir.exists())
    {
        return dir.deleteRecursively();
    }
    return false;
}

bool PresetManager::importBankPackage(const juce::File& fileOrFolder)
{
    ensureDirectoriesExist();

    if (!fileOrFolder.exists())
        return false;

    if (fileOrFolder.isDirectory())
    {
        juce::String bankName = fileOrFolder.getFileName();
        createBank(bankName);
        auto targetBankDir = presetsFolder.getChildFile(bankName);

        auto jsonFiles = fileOrFolder.findChildFiles(juce::File::TypesOfFileToFind::findFiles, true, "*.vsts;*.vsfx;*.json");
        for (const auto& src : jsonFiles)
        {
            juce::String srcExt = src.getFileExtension();
            if (srcExt.isEmpty()) srcExt = ".vsfx";
            juce::File dest = targetBankDir.getChildFile(src.getFileNameWithoutExtension() + srcExt);
            if (src.copyFileTo(dest))
            {
                juce::var parsed = juce::JSON::parse(dest.loadFileAsString());
                if (parsed.isObject())
                {
                    parsed.getDynamicObject()->setProperty("bank", bankName);
                    parsed.getDynamicObject()->setProperty("isFactory", false);
                    dest.replaceWithText(juce::JSON::toString(parsed));
                }
            }
        }

        auto audioFiles = fileOrFolder.findChildFiles(juce::File::TypesOfFileToFind::findFiles, true, "*.wav;*.mp3;*.flac;*.aiff;*.ogg;*.m4a");
        for (const auto& audio : audioFiles)
        {
            importSample(audio);
        }
        return true;
    }
    else if (fileOrFolder.hasFileExtension(".zip"))
    {
        juce::ZipFile zip(fileOrFolder);
        juce::String bankName = fileOrFolder.getFileNameWithoutExtension();
        createBank(bankName);
        auto targetBankDir = presetsFolder.getChildFile(bankName);

        for (int i = 0; i < zip.getNumEntries(); ++i)
        {
            auto* entry = zip.getEntry(i);
            if (entry != nullptr && !entry->filename.endsWithChar('/') && !entry->filename.endsWithChar('\\'))
            {
                juce::String entryName = entry->filename;
                if (entryName.endsWithIgnoreCase(".vsts") || entryName.endsWithIgnoreCase(".vsfx") || entryName.endsWithIgnoreCase(".json"))
                {
                    juce::String entryExt = juce::File(entryName).getFileExtension();
                    if (entryExt.isEmpty()) entryExt = ".vsfx";
                    juce::File dest = targetBankDir.getChildFile(juce::File(entryName).getFileNameWithoutExtension() + entryExt);
                    zip.uncompressEntry(i, dest);
                    juce::var parsed = juce::JSON::parse(dest.loadFileAsString());
                    if (parsed.isObject())
                    {
                        parsed.getDynamicObject()->setProperty("bank", bankName);
                        parsed.getDynamicObject()->setProperty("isFactory", false);
                        dest.replaceWithText(juce::JSON::toString(parsed));
                    }
                }
                else if (entryName.endsWithIgnoreCase(".wav") || entryName.endsWithIgnoreCase(".mp3") ||
                         entryName.endsWithIgnoreCase(".flac") || entryName.endsWithIgnoreCase(".aiff") ||
                         entryName.endsWithIgnoreCase(".ogg") || entryName.endsWithIgnoreCase(".m4a"))
                {
                    juce::File dest = samplesFolder.getChildFile(juce::File(entryName).getFileName());
                    zip.uncompressEntry(i, dest);
                }
            }
        }
        return true;
    }
    else if (fileOrFolder.hasFileExtension(".vsts") || fileOrFolder.hasFileExtension(".vsfx") || fileOrFolder.hasFileExtension(".json"))
    {
        juce::String bankName = "User";
        juce::var parsed = juce::JSON::parse(fileOrFolder.loadFileAsString());
        if (parsed.isObject())
        {
            bankName = parsed.getProperty("bank", "User").toString();
            if (bankName.equalsIgnoreCase("Factory"))
                bankName = "User";
        }
        createBank(bankName);
        auto targetBankDir = presetsFolder.getChildFile(juce::File::createLegalFileName(bankName));
        juce::String fileExt = fileOrFolder.getFileExtension();
        if (fileExt.isEmpty()) fileExt = ".vsfx";
        juce::File dest = targetBankDir.getChildFile(fileOrFolder.getFileNameWithoutExtension() + fileExt);
        if (fileOrFolder.copyFileTo(dest))
        {
            juce::var destParsed = juce::JSON::parse(dest.loadFileAsString());
            if (destParsed.isObject())
            {
                destParsed.getDynamicObject()->setProperty("bank", bankName);
                destParsed.getDynamicObject()->setProperty("isFactory", false);
                dest.replaceWithText(juce::JSON::toString(destParsed));
            }
            return dest.existsAsFile();
        }
    }

    return false;
}

bool PresetManager::toggleFavorite(const juce::File& presetFile)
{
    if (!presetFile.existsAsFile())
        return false;

    juce::var parsed = juce::JSON::parse(presetFile.loadFileAsString());
    if (parsed.isObject())
    {
        bool curFav = (bool)parsed.getProperty("isFavorite", false);
        parsed.getDynamicObject()->setProperty("isFavorite", !curFav);
        bool ok = presetFile.replaceWithText(juce::JSON::toString(parsed));
        if (ok) invalidateCache();
        return ok;
    }
    return false;
}

juce::String PresetManager::audioBufferToBase64Wav(const juce::AudioBuffer<float>& buffer, double sampleRate)
{
    if (buffer.getNumSamples() == 0 || buffer.getNumChannels() == 0)
        return {};

    auto stream = std::make_unique<juce::MemoryOutputStream>();
    auto* rawStream = stream.get();
    std::unique_ptr<juce::OutputStream> outStream(std::move(stream));
    juce::WavAudioFormat wavFormat;
    std::unique_ptr<juce::AudioFormatWriter> writer(wavFormat.createWriterFor(
        outStream,
        juce::AudioFormatWriterOptions()
            .withSampleRate(sampleRate)
            .withNumChannels((unsigned int)buffer.getNumChannels())
            .withBitsPerSample(16)));

    if (writer != nullptr)
    {
        writer->writeFromAudioSampleBuffer(buffer, 0, buffer.getNumSamples());
        writer->flush();
        juce::String base64 = juce::Base64::toBase64(rawStream->getData(), rawStream->getDataSize());
        writer.reset();
        return base64;
    }

    return {};
}

bool PresetManager::base64WavToAudioBuffer(const juce::String& base64Str, juce::AudioBuffer<float>& outBuffer, double& outSampleRate)
{
    if (base64Str.isEmpty())
        return false;

    juce::MemoryBlock block;
    juce::MemoryOutputStream decodeStream(block, false);

    if (!juce::Base64::convertFromBase64(decodeStream, base64Str))
        return false;

    if (block.getSize() == 0)
        return false;

    auto* input = new juce::MemoryInputStream(block, false);
    juce::WavAudioFormat wavFormat;
    std::unique_ptr<juce::AudioFormatReader> reader(wavFormat.createReaderFor(input, true));

    if (reader != nullptr && reader->lengthInSamples > 0 && reader->numChannels > 0)
    {
        outBuffer.setSize((int)reader->numChannels, (int)reader->lengthInSamples);
        reader->read(&outBuffer, 0, (int)reader->lengthInSamples, 0, true, true);
        outSampleRate = reader->sampleRate;
        return true;
    }

    return false;
}

bool PresetManager::savePreset(const juce::String& presetName,
                                const juce::String& category,
                                const juce::String& bankName,
                                const juce::String& sampleFileName,
                                juce::AudioProcessorValueTreeState& apvts,
                                float startRegion,
                                float endRegion,
                                const juce::var& selectionsVar,
                                bool isFavorite,
                                bool loopEnabled,
                                const juce::AudioBuffer<float>* sampleAudioBuffer,
                                double sampleRate)
{
    ensureDirectoriesExist();

    juce::String finalName = presetName.trim();
    if (finalName.isEmpty())
    {
        auto existing = getAllPresets();
        finalName = "Preset " + juce::String(existing.size() + 1);
    }

    juce::String finalBank = bankName.trim();
    if (finalBank.isEmpty())
        finalBank = "User";

    if (finalBank.equalsIgnoreCase("Factory"))
        return false;

    juce::String cleanBank = juce::File::createLegalFileName(finalBank);
    auto targetDir = presetsFolder.getChildFile(cleanBank);
    targetDir.createDirectory();

    juce::String cleanName = juce::File::createLegalFileName(finalName);
    if (cleanName.isEmpty())
        cleanName = "Preset_1";

    juce::String ext = (category.equalsIgnoreCase("STATES") || (sampleAudioBuffer != nullptr && sampleAudioBuffer->getNumSamples() > 0)) ? ".vsts" : ".vsfx";
    juce::File presetFile = targetDir.getChildFile(cleanName + ext);

    auto* obj = new juce::DynamicObject();
    obj->setProperty("name", finalName);
    obj->setProperty("category", category.trim().isEmpty() ? "User" : category.trim());
    obj->setProperty("bank", finalBank);
    obj->setProperty("sample", sampleFileName);
    obj->setProperty("isFavorite", isFavorite);
    obj->setProperty("isFactory", false);
    obj->setProperty("lastUsed", (juce::int64)juce::Time::currentTimeMillis());

    auto* specObj = new juce::DynamicObject();
    specObj->setProperty("startRegion", startRegion);
    specObj->setProperty("endRegion", endRegion);
    specObj->setProperty("loopEnabled", loopEnabled);
    specObj->setProperty("selections", selectionsVar);
    obj->setProperty("spectrogram", specObj);

    if (sampleAudioBuffer != nullptr && sampleAudioBuffer->getNumSamples() > 0)
    {
        juce::String b64Wav = audioBufferToBase64Wav(*sampleAudioBuffer, sampleRate);
        if (b64Wav.isNotEmpty())
        {
            obj->setProperty("audioData", b64Wav);
        }
    }

    auto* paramsObj = new juce::DynamicObject();

    const char* paramIDs[] = {
        "AMP_ATTACK", "AMP_DECAY", "AMP_SUSTAIN", "AMP_RELEASE",
        "PLAYBACK_MODE", "PITCH_MODE", "PITCH_SEMITONES",
        "TIMBRE_DRIFT", "EXCITER", "POLY_MODE", "GLIDE", "GAIN",
        "FX_SIDECHAIN_ENABLE", "FX_SIDECHAIN_MIX", "FX_SIDECHAIN_RATE",
        "FX_CHORUS_ENABLE", "FX_CHORUS_AMOUNT", "FX_CHORUS_RATE",
        "FX_PHASER_ENABLE", "FX_PHASER_AMOUNT", "FX_PHASER_RATE",
        "FX_DELAY_ENABLE", "FX_DELAY_AMOUNT", "FX_DELAY_TIME", "FX_DELAY_FEEDBACK",
        "FX_DRIVE_ENABLE", "FX_DRIVE_AMOUNT", "FX_DRIVE_TONE"
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

    DBG("[VanceSpectral SaveInstrumentation] Target File Path: " << presetFile.getFullPathName());
    DBG("[VanceSpectral SaveInstrumentation] Target Directory Exists: " << (targetDir.exists() ? "YES" : "NO"));

    juce::FileOutputStream stream(presetFile);
    if (stream.openedOk())
    {
        stream.setPosition(0);
        stream.truncate();
        stream.writeText(jsonString, false, false, nullptr);
        stream.flush();

        bool verified = (presetFile.existsAsFile() && presetFile.getSize() > 0);
        DBG("[VanceSpectral SaveInstrumentation] Write Result: " << (verified ? "SUCCESS" : "FAILED")
            << " | Written Bytes: " << presetFile.getSize());
        if (verified) invalidateCache();
        return verified;
    }

    DBG("[VanceSpectral SaveInstrumentation] Failed to open FileOutputStream for path: " << presetFile.getFullPathName());
    return false;
}

bool PresetManager::loadPreset(const juce::File& presetFile,
                                juce::AudioProcessorValueTreeState& apvts,
                                juce::String& loadedSampleFileName,
                                float& loadedStartRegion,
                                float& loadedEndRegion,
                                juce::var& loadedSelectionsVar,
                                bool& loadedLoopEnabled,
                                juce::AudioBuffer<float>* outAudioBuffer,
                                double* outSampleRate)
{
    if (!presetFile.existsAsFile())
        return false;

    juce::var parsed = juce::JSON::parse(presetFile.loadFileAsString());
    if (!parsed.isObject())
        return false;

    loadedSampleFileName = parsed.getProperty("sample", "").toString();
    loadedStartRegion = 0.0f;
    loadedEndRegion = 1.0f;
    loadedLoopEnabled = false;
    loadedSelectionsVar = juce::var();

    auto specVar = parsed.getProperty("spectrogram", juce::var());
    if (specVar.isObject())
    {
        loadedStartRegion = (float)(double)specVar.getProperty("startRegion", 0.0);
        loadedEndRegion = (float)(double)specVar.getProperty("endRegion", 1.0);
        loadedLoopEnabled = (bool)specVar.getProperty("loopEnabled", false);
        loadedSelectionsVar = specVar.getProperty("selections", juce::var());
    }

    if (outAudioBuffer != nullptr)
    {
        juce::String audioDataStr = parsed.getProperty("audioData", "").toString();
        if (audioDataStr.isNotEmpty())
        {
            double sr = 44100.0;
            if (base64WavToAudioBuffer(audioDataStr, *outAudioBuffer, sr))
            {
                if (outSampleRate != nullptr)
                    *outSampleRate = sr;
            }
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

    // Update lastUsed timestamp
    parsed.getDynamicObject()->setProperty("lastUsed", (juce::int64)juce::Time::currentTimeMillis());
    presetFile.replaceWithText(juce::JSON::toString(parsed));

    return true;
}

bool PresetManager::deletePreset(const juce::File& presetFile)
{
    if (!presetFile.existsAsFile())
        return false;

    juce::var parsed = juce::JSON::parse(presetFile.loadFileAsString());
    if (parsed.isObject())
    {
        bool isFactory = (bool)parsed.getProperty("isFactory", false);
        juce::String bank = parsed.getProperty("bank", "").toString();
        if (isFactory || bank.equalsIgnoreCase("Factory"))
            return false; // Locked factory preset!
    }

    bool deleted = presetFile.deleteFile();
    if (deleted) invalidateCache();
    return deleted;
}

juce::File PresetManager::findMatchingSample(const juce::File& sourceFile) const
{
    if (!sourceFile.existsAsFile() || !samplesFolder.exists())
        return {};

    juce::String sourceName = sourceFile.getFileName();
    auto existingSamples = getAllSamples();

    for (const auto& sample : existingSamples)
    {
        if (sample == sourceFile)
            continue;

        if (sample.getFileName().equalsIgnoreCase(sourceName) || sourceFile.hasIdenticalContentTo(sample))
        {
            return sample;
        }
    }

    return {};
}

juce::File PresetManager::importSample(const juce::File& sourceFile, bool overwriteExisting, const juce::File& sampleToReplace)
{
    ensureDirectoriesExist();

    if (!sourceFile.existsAsFile())
        return {};

    juce::File targetFile = samplesFolder.getChildFile(sourceFile.getFileName());

    // If sourceFile is already inside samplesFolder and not replacing another sample with a different name
    if (sourceFile.getParentDirectory() == samplesFolder && sourceFile.existsAsFile())
    {
        if (!sampleToReplace.existsAsFile() || sampleToReplace == sourceFile)
            return sourceFile;
    }

    if (overwriteExisting)
    {
        // If there's an explicit sampleToReplace with a different file name, delete it first to prevent duplicates
        if (sampleToReplace.existsAsFile() && sampleToReplace != targetFile)
        {
            sampleToReplace.deleteFile();
        }

        // Delete targetFile first so copyFileTo doesn't fail or create conflicts
        if (targetFile.existsAsFile())
        {
            targetFile.deleteFile();
        }

        if (sourceFile.copyFileTo(targetFile))
            return targetFile;

        return {};
    }

    if (targetFile.existsAsFile())
    {
        // Not overwriting: if targetFile exists with exact same name, return existing targetFile
        return targetFile;
    }

    if (sourceFile.copyFileTo(targetFile))
    {
        return targetFile;
    }

    return {};
}

bool PresetManager::renameSample(const juce::File& sampleFile, const juce::String& newName)
{
    if (!sampleFile.existsAsFile() || newName.trim().isEmpty())
        return false;

    juce::String clean = juce::File::createLegalFileName(newName.trim());
    if (!clean.contains("."))
        clean += sampleFile.getFileExtension();

    juce::File targetFile = samplesFolder.getChildFile(clean);
    if (targetFile == sampleFile)
        return true;

    return sampleFile.moveFileTo(targetFile);
}

bool PresetManager::deleteSample(const juce::File& sampleFile)
{
    if (sampleFile.existsAsFile())
    {
        return sampleFile.deleteFile();
    }
    return false;
}

void PresetManager::createDefaultFactoryPresets(juce::AudioProcessorValueTreeState& apvts)
{
    auto existing = getAllPresets();
    if (!existing.isEmpty())
        return;

    savePreset("Cold Synth", "Synth", "Factory", "", apvts, false);
    savePreset("Spectral Lead", "Lead", "Factory", "", apvts, false);
    savePreset("Cyber Bass", "Bass", "Factory", "", apvts, false);
    savePreset("Neon Pad", "Pad", "Factory", "", apvts, false);
    savePreset("Vibe Synth", "Synth", "Factory", "", apvts, false);
    savePreset("Glitch Pulse", "FX", "Factory", "", apvts, false);
}

