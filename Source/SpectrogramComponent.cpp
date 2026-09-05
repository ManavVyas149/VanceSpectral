#include "SpectrogramComponent.h"
#include "PluginProcessor.h"
#include "PresetManager.h"
#include "AudioResampler.h"

SpectrogramComponent::SpectrogramComponent(VancespectralAudioProcessor &p)
    : processor(p) {
  formatManager.registerBasicFormats();
  setWantsKeyboardFocus(true);

  addAndMakeVisible(loopButton);
  loopButton.setClickingTogglesState(true);
  loopButton.onClick = [this]() {
    loopEnabled = loopButton.getToggleState();
    processor.setLoop(loopEnabled);
    repaint();
  };

  startTimerHz(60);
}

SpectrogramComponent::~SpectrogramComponent() { stopTimer(); }

void SpectrogramComponent::timerCallback() {
  if (processor.isPlaying() || isDrawing || dragState != DragState::None) {
    repaint();
  }
}

void SpectrogramComponent::loadAudioFile(const juce::File &file, bool isPartOfPresetLoad) {
  if (!file.existsAsFile() || file.getSize() == 0) {
    auto* dialog = new juce::AlertWindow(
        "Invalid Sample File",
        "The selected audio file is empty or missing: " + file.getFileName(),
        juce::AlertWindow::WarningIcon);
    dialog->addButton("OK", 1, juce::KeyPress(juce::KeyPress::returnKey));
    dialog->enterModalState(true, nullptr, true);
    return;
  }

  isLoadingSample = true;
  repaint();

  juce::Thread::launch([this, file, isPartOfPresetLoad]() {
    juce::AudioFormatManager localFmt;
    localFmt.registerBasicFormats();

    std::unique_ptr<juce::AudioFormatReader> localReader(localFmt.createReaderFor(file));
    if (localReader == nullptr || localReader->lengthInSamples == 0 || localReader->numChannels == 0) {
      juce::MessageManager::callAsync([this, file]() {
        isLoadingSample = false;
        repaint();
        auto* dialog = new juce::AlertWindow(
            "Unsupported Sample Format",
            "Could not decode audio from file: " + file.getFileName() + "\nPlease ensure it is a valid WAV, MP3, FLAC, AIFF, or OGG file.",
            juce::AlertWindow::WarningIcon);
        dialog->addButton("OK", 1, juce::KeyPress(juce::KeyPress::returnKey));
        dialog->enterModalState(true, nullptr, true);
      });
      return;
    }

    juce::AudioBuffer<float> rawBuffer((int)localReader->numChannels, (int)localReader->lengthInSamples);
    localReader->read(&rawBuffer, 0, (int)localReader->lengthInSamples, 0, true, true);
    double sourceSr = localReader->sampleRate;
    localReader.reset();

    // Resample at import time via libsamplerate (SRC_SINC_BEST_QUALITY) if sample rate differs from plugin session rate
    double targetSr = processor.getSampleRate() > 0.0 ? processor.getSampleRate() : 44100.0;
    juce::AudioBuffer<float> tempBuffer = AudioResampler::resampleIfNeeded(rawBuffer, sourceSr, targetSr);
    double sr = targetSr;

    PresetManager pm;
    juce::File imported = pm.importSample(file);
    juce::File fileToUse = imported.existsAsFile() ? imported : file;

    int imgWidth = juce::jmax(100, getWidth() > 50 ? getWidth() - 44 : 560);
    int imgHeight = juce::jmax(100, getHeight() > 0 ? getHeight() : 350);
    juce::Image tempImg(juce::Image::RGB, imgWidth, imgHeight, false);

    const auto* samples = tempBuffer.getReadPointer(0);
    int totalSamples = tempBuffer.getNumSamples();
    int samplesPerPixel = juce::jmax(1, totalSamples / imgWidth);

    {
      juce::Image::BitmapData bitmapData(tempImg, juce::Image::BitmapData::writeOnly);
      for (int x = 0; x < imgWidth; ++x) {
        int startSample = x * samplesPerPixel;
        if (startSample >= totalSamples) break;
        int numToScan = juce::jmin(samplesPerPixel, totalSamples - startSample);
        float lowEnergy = 0.0f;
        float midEnergy = 0.0f;
        float highEnergy = 0.0f;

        for (int i = 0; i < numToScan - 1; ++i) {
          float s = std::abs(samples[startSample + i]);
          float diff = std::abs(samples[startSample + i + 1] - samples[startSample + i]);
          lowEnergy += s;
          midEnergy += diff * 1.5f;
          highEnergy += diff * diff * 3.0f;
        }

        lowEnergy /= (float)numToScan;
        midEnergy /= (float)numToScan;
        highEnergy /= (float)numToScan;

        for (int y = 0; y < imgHeight; ++y) {
          float normY = 1.0f - ((float)y / (float)imgHeight);
          float mag = 0.0f;
          if (normY < 0.35f)
            mag = lowEnergy * (1.0f - normY / 0.35f) + midEnergy * 0.3f;
          else if (normY < 0.7f)
            mag = midEnergy * (1.0f - std::abs(normY - 0.5f) / 0.2f);
          else
            mag = highEnergy * ((normY - 0.7f) / 0.3f) + midEnergy * 0.2f;

          mag = juce::jlimit(0.0f, 1.0f, mag * 3.5f);
          bitmapData.setPixelColour(x, y, getSpectrogramColor(mag));
        }
      }
    }

    juce::MessageManager::callAsync([this, file, fileToUse, tempBuffer, tempImg, sr, isPartOfPresetLoad]() mutable {
      audioBuffer = tempBuffer;
      spectrogramImage = tempImg;
      loadedFile = file;
      fileLoaded = true;
      isLoadingSample = false;

      if (!isPartOfPresetLoad) {
        processor.resetParametersToDefault();
        startPosition = 0.0f;
        endPosition = 1.0f;
        selections.clear();
        activeSelectionIndex = -1;
        processor.setCurrentPresetName("Custom / Unsaved");
        if (onManualSampleLoaded)
          onManualSampleLoaded();
      }

      processor.setLoadedSample(fileToUse, audioBuffer, sr);
      processor.setRegion(startPosition, endPosition);
      processor.setLoop(loopEnabled);
      updateFrequencyFilterFromSelections();

      if (onFileLoadedStateChanged)
        onFileLoadedStateChanged(true);

      repaint();
    });
  });
}

void SpectrogramComponent::loadDirectAudioBuffer(const juce::AudioBuffer<float>& buffer, double sampleRate, const juce::String& fileName, bool isLooping) {
  if (buffer.getNumSamples() == 0)
    return;

  double targetSr = processor.getSampleRate() > 0.0 ? processor.getSampleRate() : 44100.0;
  audioBuffer = AudioResampler::resampleIfNeeded(buffer, sampleRate, targetSr);
  loadedFile = juce::File();
  fileLoaded = true;

  loopEnabled = isLooping;
  loopButton.setToggleState(loopEnabled, juce::dontSendNotification);

  processor.setLoadedSample(juce::File(), audioBuffer, targetSr);
  processor.setCurrentPresetName(fileName.isNotEmpty() ? fileName : "Custom / Unsaved");
  processor.setRegion(startPosition, endPosition);
  processor.setLoop(loopEnabled);
  updateFrequencyFilterFromSelections();
  generateSpectrogramImage();

  if (onFileLoadedStateChanged)
    onFileLoadedStateChanged(true);

  repaint();
}

void SpectrogramComponent::setLoopEnabled(bool loop) {
  loopEnabled = loop;
  loopButton.setToggleState(loopEnabled, juce::dontSendNotification);
  processor.setLoop(loopEnabled);
  repaint();
}

void SpectrogramComponent::restoreFromProcessorState() {
  if (processor.isSampleLoaded()) {
    if (processor.getLoadedSampleBuffer().getNumSamples() > 0) {
      audioBuffer = processor.getLoadedSampleBuffer();
      loadedFile = processor.getLoadedSampleFile();
      fileLoaded = true;
    } else {
      loadedFile = processor.getLoadedSampleFile();
      if (loadedFile.existsAsFile()) {
        reader.reset(formatManager.createReaderFor(loadedFile));
        if (reader != nullptr) {
          juce::AudioBuffer<float> rawBuf((int)reader->numChannels, (int)reader->lengthInSamples);
          reader->read(&rawBuf, 0, (int)reader->lengthInSamples, 0, true, true);
          double sourceSr = reader->sampleRate;
          reader.reset();

          double targetSr = processor.getSampleRate() > 0.0 ? processor.getSampleRate() : 44100.0;
          audioBuffer = AudioResampler::resampleIfNeeded(rawBuf, sourceSr, targetSr);
          fileLoaded = true;
        }
      }
    }
  }

  startPosition = processor.getRegionStartNormalized();
  endPosition = processor.getRegionEndNormalized();
  loopEnabled = processor.getLoopEnabled();
  loopButton.setToggleState(loopEnabled, juce::dontSendNotification);

  selections.clear();
  activeSelectionIndex = -1;

  juce::var selVar = processor.getSelectionsVar();
  if (selVar.isArray()) {
    auto* arr = selVar.getArray();
    for (const auto& item : *arr) {
      if (item.isObject()) {
        auto* obj = item.getDynamicObject();
        float x = (float)(double)obj->getProperty("x");
        float y = (float)(double)obj->getProperty("y");
        float w = (float)(double)obj->getProperty("w");
        float h = (float)(double)obj->getProperty("h");

        SelectionRegion reg;
        reg.id = nextSelectionId++;
        reg.type = ToolType::RectangleSelect;
        reg.normalizedBounds = juce::Rectangle<float>(x, y, w, h);
        reg.normalizedPath.addRectangle(reg.normalizedBounds);
        reg.isSelected = false;

        selections.add(reg);
      }
    }
    if (!selections.isEmpty())
      activeSelectionIndex = selections.size() - 1;
  }

  if (fileLoaded) {
    generateSpectrogramImage();
    if (onFileLoadedStateChanged)
      onFileLoadedStateChanged(true);
  }

  updateFrequencyFilterFromSelections();
  repaint();
}

void SpectrogramComponent::restorePresetSnapshot(float startRegion, float endRegion, const juce::var& selectionsVar) {
  startPosition = juce::jlimit(0.0f, 1.0f, startRegion);
  endPosition = juce::jlimit(0.0f, 1.0f, endRegion);
  if (endPosition <= startPosition)
    endPosition = juce::jmin(1.0f, startPosition + 0.05f);

  processor.setRegion(startPosition, endPosition);

  selections.clear();
  activeSelectionIndex = -1;

  if (selectionsVar.isArray()) {
    auto* arr = selectionsVar.getArray();
    for (const auto& item : *arr) {
      if (item.isObject()) {
        auto* obj = item.getDynamicObject();
        float x = (float)(double)obj->getProperty("x");
        float y = (float)(double)obj->getProperty("y");
        float w = (float)(double)obj->getProperty("w");
        float h = (float)(double)obj->getProperty("h");

        SelectionRegion reg;
        reg.id = nextSelectionId++;
        reg.type = ToolType::RectangleSelect;
        reg.normalizedBounds = juce::Rectangle<float>(x, y, w, h);
        reg.normalizedPath.addRectangle(reg.normalizedBounds);
        reg.isSelected = false;

        selections.add(reg);
      }
    }
    if (!selections.isEmpty())
      activeSelectionIndex = selections.size() - 1;
  }

  updateFrequencyFilterFromSelections();
  repaint();
}

juce::var SpectrogramComponent::getSelectionsAsVar() const {
  juce::Array<juce::var> selArr;
  for (const auto& reg : selections) {
    auto* obj = new juce::DynamicObject();
    obj->setProperty("x", reg.normalizedBounds.getX());
    obj->setProperty("y", reg.normalizedBounds.getY());
    obj->setProperty("w", reg.normalizedBounds.getWidth());
    obj->setProperty("h", reg.normalizedBounds.getHeight());
    selArr.add(juce::var(obj));
  }
  return juce::var(selArr);
}

float SpectrogramComponent::yToFrequency(float normY) {
  normY = juce::jlimit(0.0f, 1.0f, normY);
  return 20.0f * std::pow(1000.0f, 1.0f - normY);
}

float SpectrogramComponent::frequencyToY(float hz) {
  hz = juce::jlimit(20.0f, 20000.0f, hz);
  return 1.0f - (std::log(hz / 20.0f) / std::log(1000.0f));
}

juce::String SpectrogramComponent::formatFrequency(float hz) {
  if (hz >= 1000.0f)
    return juce::String(hz / 1000.0f, 1) + "kHz";
  return juce::String((int)hz) + "Hz";
}

juce::Rectangle<float> SpectrogramComponent::getAxisBounds() const {
  auto bounds = getLocalBounds().toFloat();
  if (bounds.isEmpty())
    return {};
  return bounds.removeFromLeft(44.0f).reduced(0.0f, 10.0f);
}

juce::Rectangle<float> SpectrogramComponent::getGraphBounds() const {
  auto bounds = getLocalBounds().toFloat();
  if (bounds.isEmpty())
    return {};
  bounds.removeFromLeft(44.0f);
  return bounds.reduced(0.0f, 10.0f);
}

void SpectrogramComponent::generateRandomSelections() {
  if (!fileLoaded)
    return;

  selections.clear();
  int numRegions = juce::Random::getSystemRandom().nextBool() ? 1 : 2;

  for (int k = 0; k < numRegions; ++k) {
    float w = 0.12f + juce::Random::getSystemRandom().nextFloat() * 0.33f;
    float x = juce::Random::getSystemRandom().nextFloat() * (1.0f - w);
    float h = 0.15f + juce::Random::getSystemRandom().nextFloat() * 0.35f;
    float y = juce::Random::getSystemRandom().nextFloat() * (1.0f - h);

    SelectionRegion reg;
    reg.id = nextSelectionId++;
    reg.type = ToolType::RectangleSelect;
    reg.normalizedBounds = juce::Rectangle<float>(x, y, w, h);
    reg.isSelected = (k == 0);
    selections.add(reg);
  }

  activeSelectionIndex = selections.isEmpty() ? -1 : 0;
  updateFrequencyFilterFromSelections();
  repaint();
}

void SpectrogramComponent::updateFrequencyFilterFromSelections() {
  if (!fileLoaded || selections.isEmpty()) {
    processor.setSpectralRegions(juce::Array<SpectralRegion>());
    processor.setSelectionsVar(juce::var());
    startPosition = 0.0f;
    endPosition = 1.0f;
    processor.setRegion(startPosition, endPosition);
    return;
  }

  juce::Array<SpectralRegion> regions;
  float minTimeStart = 1.0f;
  float maxTimeEnd = 0.0f;

  for (int i = 0; i < selections.size(); ++i) {
    const auto &reg = selections.getReference(i);
    auto bounds = reg.normalizedBounds;
    if (!bounds.isEmpty()) {
      SpectralRegion sr;
      sr.id = reg.id;
      sr.startNorm = bounds.getX();
      sr.endNorm = bounds.getRight();

      minTimeStart = juce::jmin(minTimeStart, sr.startNorm);
      maxTimeEnd = juce::jmax(maxTimeEnd, sr.endNorm);

      float yTop = bounds.getY();
      float yBottom = bounds.getBottom();

      float maxFreq = yToFrequency(yTop);
      float minFreq = yToFrequency(yBottom);

      if (minFreq > maxFreq)
        std::swap(minFreq, maxFreq);

      sr.minFreq = minFreq;
      sr.maxFreq = maxFreq;

      regions.add(sr);
    }
  }

  processor.setSpectralRegions(regions);
  processor.setSelectionsVar(getSelectionsAsVar());

  // Constrain playback range to the combined time span of active selections
  if (minTimeStart < maxTimeEnd) {
    startPosition = minTimeStart;
    endPosition = maxTimeEnd;
    processor.setRegion(startPosition, endPosition);
  }
}

juce::Colour
SpectrogramComponent::getSpectrogramColor(float magnitudeNormalized) {
  magnitudeNormalized = juce::jlimit(0.0f, 1.0f, magnitudeNormalized);

  if (magnitudeNormalized < 0.05f) {
    return SpectralUILookAndFeel::graphBgColour;
  } else if (magnitudeNormalized < 0.35f) {
    float t = (magnitudeNormalized - 0.05f) / 0.30f;
    return juce::Colour::fromRGB(0x0C, 0x0D, 0x12)
        .interpolatedWith(juce::Colour::fromRGB(0x24, 0x18, 0x30), t);
  } else if (magnitudeNormalized < 0.65f) {
    float t = (magnitudeNormalized - 0.35f) / 0.30f;
    return juce::Colour::fromRGB(0x24, 0x18, 0x30)
        .interpolatedWith(juce::Colour::fromRGB(0x60, 0x28, 0x72), t);
  } else if (magnitudeNormalized < 0.88f) {
    float t = (magnitudeNormalized - 0.65f) / 0.23f;
    return juce::Colour::fromRGB(0x60, 0x28, 0x72)
        .interpolatedWith(juce::Colour::fromRGB(0xB8, 0x4D, 0xC4), t);
  } else {
    float t = (magnitudeNormalized - 0.88f) / 0.12f;
    return juce::Colour::fromRGB(0xB8, 0x4D, 0xC4)
        .interpolatedWith(juce::Colour::fromRGB(0xF2, 0xB8, 0xFF), t);
  }
}

void SpectrogramComponent::generateSpectrogramImage() {
  if (!fileLoaded || audioBuffer.getNumSamples() == 0)
    return;

  int imgWidth = juce::jmax(100, getWidth() > 50 ? getWidth() - 44 : 560);
  int imgHeight = juce::jmax(100, getHeight() > 0 ? getHeight() : 350);

  if (!spectrogramImage.isNull() && spectrogramImage.getWidth() == imgWidth && spectrogramImage.getHeight() == imgHeight)
    return;

  spectrogramImage = juce::Image(juce::Image::RGB, imgWidth, imgHeight, false);

  const auto *samples = audioBuffer.getReadPointer(0);
  int totalSamples = audioBuffer.getNumSamples();
  int samplesPerPixel = juce::jmax(1, totalSamples / imgWidth);

  {
    juce::Image::BitmapData bitmapData(spectrogramImage,
                                       juce::Image::BitmapData::writeOnly);

    for (int x = 0; x < imgWidth; ++x) {
      int startSample = x * samplesPerPixel;
      if (startSample >= totalSamples)
        break;

      int numToScan = juce::jmin(samplesPerPixel, totalSamples - startSample);

      float lowEnergy = 0.0f;
      float midEnergy = 0.0f;
      float highEnergy = 0.0f;

      for (int i = 0; i < numToScan - 1; ++i) {
        float s = std::abs(samples[startSample + i]);
        float diff =
            std::abs(samples[startSample + i + 1] - samples[startSample + i]);

        lowEnergy += s;
        midEnergy += diff * 1.5f;
        highEnergy += diff * diff * 3.0f;
      }

      lowEnergy /= (float)numToScan;
      midEnergy /= (float)numToScan;
      highEnergy /= (float)numToScan;

      for (int y = 0; y < imgHeight; ++y) {
        float normY =
            1.0f - ((float)y / (float)imgHeight); // 20Hz at bottom, 20kHz at top

        float mag = 0.0f;
        if (normY < 0.35f)
          mag = lowEnergy * (1.0f - normY / 0.35f) + midEnergy * 0.3f;
        else if (normY < 0.7f)
          mag = midEnergy * (1.0f - std::abs(normY - 0.5f) / 0.2f);
        else
          mag = highEnergy * ((normY - 0.7f) / 0.3f) + midEnergy * 0.2f;

        mag = juce::jlimit(0.0f, 1.0f, mag * 3.5f);
        bitmapData.setPixelColour(x, y, getSpectrogramColor(mag));
      }
    }
  }
}

void SpectrogramComponent::drawFrequencyAxis(
    juce::Graphics &g, juce::Rectangle<float> axisBounds) {
  g.setFont(SpectralUILookAndFeel::getMonospaceFont(8.5f));

  struct ScalePoint {
    float freq;
    const char *label;
  };

  ScalePoint points[] = {
      {20000.0f, "20kHz"},
      {10000.0f, "10kHz"},
      {5000.0f,  "5kHz"},
      {2000.0f,  "2kHz"},
      {1000.0f,  "1kHz"},
      {500.0f,   "500Hz"},
      {200.0f,   "200Hz"},
      {100.0f,   "100Hz"},
      {20.0f,    "20Hz"}
  };

  float rightEdge = axisBounds.getRight() - 3.0f;
  float parentBottom = getLocalBounds().toFloat().getBottom();

  for (const auto &pt : points) {
    float normY = frequencyToY(pt.freq);
    float y = axisBounds.getY() + normY * axisBounds.getHeight();

    // Draw tick mark line into graph boundary
    g.setColour(SpectralUILookAndFeel::dividerColour.withAlpha(0.6f));
    g.drawLine(rightEdge - 3.0f, y, rightEdge, y, 1.0f);

    // Keep label text within component canvas boundaries
    float labelY = juce::jlimit(2.0f, parentBottom - 15.0f, y - 7.0f);
    juce::Rectangle<int> labelRect((int)axisBounds.getX() + 2, (int)labelY,
                                   (int)(rightEdge - axisBounds.getX() - 5.0f),
                                   14);
    g.setColour(SpectralUILookAndFeel::textMutedColour);
    g.drawText(pt.label, labelRect, juce::Justification::centredRight, false);
  }

  // Draw axis hairline separator line
  g.setColour(SpectralUILookAndFeel::dividerColour);
  g.drawVerticalLine((int)rightEdge, axisBounds.getY() - 4.0f,
                     axisBounds.getBottom() + 4.0f);
}

void SpectrogramComponent::drawWaveform(juce::Graphics &g,
                                        juce::Rectangle<float> bounds) {
  if (!fileLoaded || audioBuffer.getNumSamples() == 0)
    return;

  const auto *samples = audioBuffer.getReadPointer(0);
  int numSamples = audioBuffer.getNumSamples();
  float width = bounds.getWidth();
  float height = bounds.getHeight();
  float centreY = bounds.getCentreY();

  int samplesPerPixel = juce::jmax(1, numSamples / (int)width);

  // Center zero-crossing hairline in translucent violet accent
  juce::Colour waveCol = SpectralUILookAndFeel::accentColour;
  g.setColour(waveCol.withAlpha(0.20f));
  g.drawHorizontalLine((int)centreY, bounds.getX(), bounds.getRight());

  juce::Path waveformEnvelope;
  juce::Path waveformTop;
  juce::Path waveformBottom;

  waveformTop.startNewSubPath(bounds.getX(), centreY);
  waveformBottom.startNewSubPath(bounds.getX(), centreY);
  waveformEnvelope.startNewSubPath(bounds.getX(), centreY);

  for (int x = 0; x < (int)width; ++x) {
    int samplePos = x * samplesPerPixel;
    float minS = 1.0f;
    float maxS = -1.0f;

    for (int i = 0; i < samplesPerPixel; ++i) {
      if (samplePos + i < numSamples) {
        float s = samples[samplePos + i];
        minS = juce::jmin(minS, s);
        maxS = juce::jmax(maxS, s);
      }
    }

    float px = bounds.getX() + (float)x;
    float yMax = centreY - (maxS * height * 0.42f);
    float yMin = centreY - (minS * height * 0.42f);

    waveformTop.lineTo(px, yMax);
    waveformBottom.lineTo(px, yMin);
  }

  // Construct closed silhouette envelope for filled glow
  waveformEnvelope.addPath(waveformTop);
  for (int x = (int)width - 1; x >= 0; --x) {
    int samplePos = x * samplesPerPixel;
    float minS = 1.0f;
    for (int i = 0; i < samplesPerPixel; ++i) {
      if (samplePos + i < numSamples)
        minS = juce::jmin(minS, samples[samplePos + i]);
    }
    float px = bounds.getX() + (float)x;
    float yMin = centreY - (minS * height * 0.42f);
    waveformEnvelope.lineTo(px, yMin);
  }
  waveformEnvelope.closeSubPath();

  // Translucent burple waveform fill (~14% alpha)
  g.setColour(waveCol.withAlpha(0.14f));
  g.fillPath(waveformEnvelope);

  // Crisp top & bottom burple/bright waveform outlines (~90% alpha)
  g.setColour(SpectralUILookAndFeel::accentBright.withAlpha(0.90f));
  g.strokePath(waveformTop, juce::PathStrokeType(1.2f));
  g.setColour(waveCol.withAlpha(0.70f));
  g.strokePath(waveformBottom, juce::PathStrokeType(1.2f));
}

bool SpectrogramComponent::isInterestedInFileDrag(
    const juce::StringArray &files) {
  for (auto &file : files) {
    if (file.endsWithIgnoreCase(".wav") || file.endsWithIgnoreCase(".aiff") ||
        file.endsWithIgnoreCase(".flac") || file.endsWithIgnoreCase(".mp3") ||
        file.endsWithIgnoreCase(".ogg") || file.endsWithIgnoreCase(".m4a"))
      return true;
  }
  return false;
}

void SpectrogramComponent::fileDragEnter(const juce::StringArray &, int, int) {
  dragActive = true;
  repaint();
}

void SpectrogramComponent::fileDragMove(const juce::StringArray &, int, int) {}

void SpectrogramComponent::fileDragExit(const juce::StringArray &) {
  dragActive = false;
  repaint();
}

void SpectrogramComponent::filesDropped(const juce::StringArray &files, int,
                                        int) {
  dragActive = false;
  if (files.size() > 0) {
    loadedFile = juce::File(files[0]);
    loadAudioFile(loadedFile);
  }
  repaint();
}

void SpectrogramComponent::changeListenerCallback(juce::ChangeBroadcaster *) {
  repaint();
}

juce::File SpectrogramComponent::createTempWavForExport(bool exportSelectionOnly) {
  if (!fileLoaded || audioBuffer.getNumSamples() == 0)
    return {};

  juce::File tempDir = juce::File::getSpecialLocation(juce::File::userDocumentsDirectory)
                          .getChildFile("VanceSpectral")
                          .getChildFile("TempExports");
  if (!tempDir.exists())
    tempDir.createDirectory();

  // Clean old exports older than 1 hour
  auto oldFiles = tempDir.findChildFiles(juce::File::TypesOfFileToFind::findFiles, false, "*.wav");
  for (const auto& f : oldFiles) {
    if (f.getLastModificationTime() < juce::Time::getCurrentTime() - juce::RelativeTime::hours(1))
      f.deleteFile();
  }

  juce::String baseName = loadedFile.getFileNameWithoutExtension();
  while (baseName.endsWithIgnoreCase("_Export") || baseName.endsWithIgnoreCase("_Rendered_Region")) {
    if (baseName.endsWithIgnoreCase("_Export"))
      baseName = baseName.dropLastCharacters(7);
    else if (baseName.endsWithIgnoreCase("_Rendered_Region"))
      baseName = baseName.dropLastCharacters(16);
  }

  if (baseName.isEmpty())
    baseName = "Sample";

  juce::String exportFileName = exportSelectionOnly 
      ? baseName + "_Rendered_Region.wav" 
      : baseName + "_Export.wav";

  juce::File exportFile = tempDir.getChildFile(exportFileName);
  if (exportFile.existsAsFile())
    exportFile.deleteFile();

  int totalSamples = audioBuffer.getNumSamples();
  int startSample = 0;
  int numSamples = totalSamples;

  if (exportSelectionOnly) {
    startSample = juce::jlimit(0, totalSamples - 1, (int)(startPosition * (float)totalSamples));
    int endSample = juce::jlimit(startSample + 1, totalSamples, (int)(endPosition * (float)totalSamples));
    numSamples = endSample - startSample;
  }

  if (numSamples <= 0)
    return loadedFile;

  juce::WavAudioFormat wavFormat;
  auto fileStream = std::make_unique<juce::FileOutputStream>(exportFile);
  if (fileStream == nullptr || fileStream->failedToOpen())
    return loadedFile;

  std::unique_ptr<juce::OutputStream> outStream(std::move(fileStream));
  double sr = reader != nullptr && reader->sampleRate > 0.0 ? reader->sampleRate : 44100.0;
  std::unique_ptr<juce::AudioFormatWriter> writer(wavFormat.createWriterFor(
      outStream,
      juce::AudioFormatWriterOptions()
          .withSampleRate(sr)
          .withNumChannels((unsigned int)audioBuffer.getNumChannels())
          .withBitsPerSample(24)));

  if (writer != nullptr) {
    juce::AudioBuffer<float> exportBuffer(audioBuffer.getNumChannels(), numSamples);
    for (int ch = 0; ch < audioBuffer.getNumChannels(); ++ch) {
      exportBuffer.copyFrom(ch, 0, audioBuffer, ch, startSample, numSamples);
    }

    writer->writeFromAudioSampleBuffer(exportBuffer, 0, numSamples);
    writer.reset();
    return exportFile;
  }

  return loadedFile;
}

void SpectrogramComponent::startExternalDrag(const juce::MouseEvent &e) {
  if (!fileLoaded || isExternalDragging)
    return;

  isExternalDragging = true;
  repaint();

  bool shiftHeld = e.mods.isShiftDown();
  bool trimmed = (startPosition > 0.001f || endPosition < 0.999f || !selections.isEmpty());
  bool exportSelectionOnly = shiftHeld || trimmed;

  juce::File exportFile = createTempWavForExport(exportSelectionOnly);
  if (!exportFile.existsAsFile())
    exportFile = loadedFile;

  if (exportFile.existsAsFile()) {
    if (auto* dragContainer = juce::DragAndDropContainer::findParentDragContainerFor(this)) {
      juce::StringArray files;
      files.add(exportFile.getFullPathName());
      dragContainer->performExternalDragDropOfFiles(files, false);
    }
  }

  isExternalDragging = false;
  repaint();
}

void SpectrogramComponent::mouseMove(const juce::MouseEvent &) {}

void SpectrogramComponent::mouseDown(const juce::MouseEvent &e) {
  grabKeyboardFocus();
  mouseDownPos = e.position;

  auto graphBounds = getGraphBounds();
  if (graphBounds.isEmpty())
    return;

  float mouseX = e.position.x;
  float startX = graphBounds.getX() + graphBounds.getWidth() * startPosition;
  float endX = graphBounds.getX() + graphBounds.getWidth() * endPosition;

  if (std::abs(mouseX - startX) <= 12.0f) {
    dragState = DragState::DraggingStartMarker;
    return;
  }

  if (std::abs(mouseX - endX) <= 12.0f) {
    dragState = DragState::DraggingEndMarker;
    return;
  }

  if (!graphBounds.contains(e.position) || !fileLoaded)
    return;

  float normX = juce::jlimit(0.0f, 1.0f, (e.position.x - graphBounds.getX()) / graphBounds.getWidth());
  float normY = juce::jlimit(0.0f, 1.0f, (e.position.y - graphBounds.getY()) / graphBounds.getHeight());

  // 1. Double click toggles Play/Pause
  if (e.getNumberOfClicks() >= 2) {
    if (processor.isPlaying())
      processor.stopSample();
    else
      processor.playSample();
    return;
  }

  // 2. Check if clicking on tag 'x' delete button on any selection region
  for (int i = selections.size() - 1; i >= 0; --i) {
    auto b = selections.getReference(i).normalizedBounds;
    float rx = graphBounds.getX() + b.getX() * graphBounds.getWidth();
    float ry = graphBounds.getY() + b.getY() * graphBounds.getHeight();
    juce::Rectangle<float> deleteBtnRect(rx + 138.0f, ry + 2.0f, 14.0f, 14.0f);

    if (deleteBtnRect.contains(e.position)) {
      selections.remove(i);
      activeSelectionIndex = selections.isEmpty() ? -1 : selections.size() - 1;
      updateFrequencyFilterFromSelections();
      repaint();
      return;
    }
  }

  // 3. Handle Erase Tool
  if (currentTool == ToolType::Erase) {
    for (int i = selections.size() - 1; i >= 0; --i) {
      if (selections.getReference(i).normalizedBounds.contains(normX, normY)) {
        selections.remove(i);
        activeSelectionIndex = selections.isEmpty() ? -1 : selections.size() - 1;
        updateFrequencyFilterFromSelections();
        repaint();
        return;
      }
    }
    return;
  }

  // 4. Handle Copy Tool
  if (currentTool == ToolType::Copy) {
    for (int i = selections.size() - 1; i >= 0; --i) {
      if (selections.getReference(i).normalizedBounds.contains(normX, normY)) {
        SelectionRegion dup = selections.getReference(i);
        dup.id = nextSelectionId++;
        dup.normalizedBounds.translate(0.04f, 0.04f);
        selections.add(dup);
        activeSelectionIndex = selections.size() - 1;
        updateFrequencyFilterFromSelections();
        repaint();
        return;
      }
    }
    return;
  }

  // 5. Check if clicking on active selection corner handles (for resizing)
  if (activeSelectionIndex >= 0 && activeSelectionIndex < selections.size()) {
    auto b = selections.getReference(activeSelectionIndex).normalizedBounds;
    float rx = graphBounds.getX() + b.getX() * graphBounds.getWidth();
    float ry = graphBounds.getY() + b.getY() * graphBounds.getHeight();
    float rw = b.getWidth() * graphBounds.getWidth();
    float rh = b.getHeight() * graphBounds.getHeight();

    float hs = 12.0f;
    juce::Rectangle<float> tl(rx - hs * 0.5f, ry - hs * 0.5f, hs, hs);
    juce::Rectangle<float> tr(rx + rw - hs * 0.5f, ry - hs * 0.5f, hs, hs);
    juce::Rectangle<float> bl(rx - hs * 0.5f, ry + rh - hs * 0.5f, hs, hs);
    juce::Rectangle<float> br(rx + rw - hs * 0.5f, ry + rh - hs * 0.5f, hs, hs);

    if (tl.contains(e.position)) {
      dragState = DragState::ResizingTopLeft;
      initialSelectionBoundsNormalized = b;
      dragStartMousePosNormalized = juce::Point<float>(normX, normY);
      return;
    }
    if (tr.contains(e.position)) {
      dragState = DragState::ResizingTopRight;
      initialSelectionBoundsNormalized = b;
      dragStartMousePosNormalized = juce::Point<float>(normX, normY);
      return;
    }
    if (bl.contains(e.position)) {
      dragState = DragState::ResizingBottomLeft;
      initialSelectionBoundsNormalized = b;
      dragStartMousePosNormalized = juce::Point<float>(normX, normY);
      return;
    }
    if (br.contains(e.position)) {
      dragState = DragState::ResizingBottomRight;
      initialSelectionBoundsNormalized = b;
      dragStartMousePosNormalized = juce::Point<float>(normX, normY);
      return;
    }
  }

  // 6. Check if clicking inside ANY existing selection (for selection & moving)
  for (int i = selections.size() - 1; i >= 0; --i) {
    if (selections.getReference(i).normalizedBounds.contains(normX, normY)) {
      activeSelectionIndex = i;
      dragState = DragState::MovingSelection;
      initialSelectionBoundsNormalized = selections.getReference(i).normalizedBounds;
      dragStartMousePosNormalized = juce::Point<float>(normX, normY);
      updateFrequencyFilterFromSelections();
      repaint();
      return;
    }
  }

  // 7. Otherwise, start drawing a new selection region (additive)
  if (currentTool != ToolType::None) {
    if (selections.size() >= 2) {
      // Maximum 2 selections allowed: block drawing a 3rd selection!
      return;
    }

    isDrawing = true;
    dragState = DragState::DrawingNew;
    dragStartPosNormalized = juce::Point<float>(normX, normY);
    dragCurrentPosNormalized = dragStartPosNormalized;

    if (currentTool == ToolType::Freehand) {
      currentDrawingPathNormalized.clear();
      currentDrawingPathNormalized.startNewSubPath(dragStartPosNormalized);
    }
  }
}

void SpectrogramComponent::mouseDrag(const juce::MouseEvent &e) {
  auto graphBounds = getGraphBounds();
  if (graphBounds.isEmpty() || !fileLoaded)
    return;

  if (!isExternalDragging && e.getDistanceFromDragStart() > 14) {
    if (e.mods.isShiftDown() || e.mods.isAltDown() || !graphBounds.contains(e.position)) {
      startExternalDrag(e);
      return;
    }
  }
  float normX = juce::jlimit(0.0f, 1.0f, (e.position.x - graphBounds.getX()) / graphBounds.getWidth());
  float normY = juce::jlimit(0.0f, 1.0f, (e.position.y - graphBounds.getY()) / graphBounds.getHeight());

  if (dragState == DragState::DraggingStartMarker) {
    startPosition = juce::jlimit(0.0f, endPosition - 0.01f, normX);
    processor.setRegion(startPosition, endPosition);
    repaint();
    return;
  }

  if (dragState == DragState::DraggingEndMarker) {
    endPosition = juce::jlimit(startPosition + 0.01f, 1.0f, normX);
    processor.setRegion(startPosition, endPosition);
    repaint();
    return;
  }

  if (dragState == DragState::MovingSelection && activeSelectionIndex >= 0 && activeSelectionIndex < selections.size()) {
    float dx = normX - dragStartMousePosNormalized.x;
    float dy = normY - dragStartMousePosNormalized.y;
    auto b = initialSelectionBoundsNormalized.translated(dx, dy);

    if (b.getX() < 0.0f) b.setX(0.0f);
    if (b.getY() < 0.0f) b.setY(0.0f);
    if (b.getRight() > 1.0f) b.setX(1.0f - b.getWidth());
    if (b.getBottom() > 1.0f) b.setY(1.0f - b.getHeight());

    selections.getReference(activeSelectionIndex).normalizedBounds = b;
    repaint();
    return;
  }

  if ((dragState == DragState::ResizingTopLeft || dragState == DragState::ResizingTopRight ||
       dragState == DragState::ResizingBottomLeft || dragState == DragState::ResizingBottomRight) &&
      activeSelectionIndex >= 0 && activeSelectionIndex < selections.size()) {

    auto initB = initialSelectionBoundsNormalized;
    float left = initB.getX();
    float top = initB.getY();
    float right = initB.getRight();
    float bottom = initB.getBottom();

    if (dragState == DragState::ResizingTopLeft) {
      left = juce::jmin(normX, right - 0.01f);
      top = juce::jmin(normY, bottom - 0.01f);
    } else if (dragState == DragState::ResizingTopRight) {
      right = juce::jmax(normX, left + 0.01f);
      top = juce::jmin(normY, bottom - 0.01f);
    } else if (dragState == DragState::ResizingBottomLeft) {
      left = juce::jmin(normX, right - 0.01f);
      bottom = juce::jmax(normY, top + 0.01f);
    } else if (dragState == DragState::ResizingBottomRight) {
      right = juce::jmax(normX, left + 0.01f);
      bottom = juce::jmax(normY, top + 0.01f);
    }

    selections.getReference(activeSelectionIndex).normalizedBounds =
        juce::Rectangle<float>(left, top, right - left, bottom - top);
    repaint();
    return;
  }

  if (dragState == DragState::DrawingNew) {
    dragCurrentPosNormalized = juce::Point<float>(normX, normY);

    if (currentTool == ToolType::Freehand) {
      currentDrawingPathNormalized.lineTo(dragCurrentPosNormalized);
    }
    repaint();
  }
}

void SpectrogramComponent::mouseUp(const juce::MouseEvent &) {
  if (dragState == DragState::DraggingStartMarker || dragState == DragState::DraggingEndMarker) {
    dragState = DragState::None;
    return;
  }

  if (dragState == DragState::MovingSelection || dragState == DragState::ResizingTopLeft ||
      dragState == DragState::ResizingTopRight || dragState == DragState::ResizingBottomLeft ||
      dragState == DragState::ResizingBottomRight) {
    dragState = DragState::None;
    updateFrequencyFilterFromSelections();
    repaint();
    return;
  }

  if (dragState == DragState::DrawingNew) {
    dragState = DragState::None;
    isDrawing = false;

    if (selections.size() < 2) {
      SelectionRegion newRegion;
      newRegion.id = nextSelectionId++;
      newRegion.type = currentTool;

      if (currentTool == ToolType::RectangleSelect) {
        float x1 = juce::jmin(dragStartPosNormalized.x, dragCurrentPosNormalized.x);
        float y1 = juce::jmin(dragStartPosNormalized.y, dragCurrentPosNormalized.y);
        float w = std::abs(dragCurrentPosNormalized.x - dragStartPosNormalized.x);
        float h = std::abs(dragCurrentPosNormalized.y - dragStartPosNormalized.y);

        if (w > 0.01f && h > 0.01f) {
          newRegion.normalizedBounds = juce::Rectangle<float>(x1, y1, w, h);
          newRegion.isSelected = true;
          selections.add(newRegion);
          activeSelectionIndex = selections.size() - 1;
        }
      } else if (currentTool == ToolType::Freehand) {
        currentDrawingPathNormalized.closeSubPath();
        newRegion.normalizedPath = currentDrawingPathNormalized;
        newRegion.normalizedBounds = currentDrawingPathNormalized.getBounds();
        newRegion.isSelected = true;

        if (!newRegion.normalizedBounds.isEmpty()) {
          selections.add(newRegion);
          activeSelectionIndex = selections.size() - 1;
        }
      }
    }

    currentDrawingPathNormalized.clear();
    updateFrequencyFilterFromSelections();
    repaint();
  }
}

bool SpectrogramComponent::keyPressed(const juce::KeyPress &key) {
  if (key == juce::KeyPress::spaceKey) {
    if (processor.isPlaying())
      processor.stopSample();
    else
      processor.playSample();
    return true;
  }

  if (key == juce::KeyPress::deleteKey || key == juce::KeyPress::backspaceKey) {
    if (activeSelectionIndex >= 0 && activeSelectionIndex < selections.size()) {
      selections.remove(activeSelectionIndex);
      activeSelectionIndex = selections.isEmpty() ? -1 : selections.size() - 1;
      updateFrequencyFilterFromSelections();
      repaint();
      return true;
    }
  }
  return false;
}

void SpectrogramComponent::paint(juce::Graphics &g) {
  auto bounds = getLocalBounds().toFloat();

  // Panel background
  g.setColour(SpectralUILookAndFeel::graphBgColour);
  g.fillRoundedRectangle(bounds, 8.0f);

  // Left Frequency Axis Margin (44px width, 20kHz at Top -> 20Hz at Bottom)
  auto axisBounds = getAxisBounds();
  drawFrequencyAxis(g, axisBounds);

  auto graphBounds = getGraphBounds();

  if (fileLoaded && !spectrogramImage.isNull()) {
    // 1. Draw Spectrogram Image
    g.drawImage(spectrogramImage, graphBounds,
                juce::RectanglePlacement::stretchToFit);

    // 2. Draw Waveform Overlay
    drawWaveform(g, graphBounds);

    // 3. Darken out-of-region areas
    float startX = graphBounds.getX() + graphBounds.getWidth() * startPosition;
    float endX = graphBounds.getX() + graphBounds.getWidth() * endPosition;

    if (startPosition > 0.001f) {
      g.setColour(juce::Colours::black.withAlpha(0.45f));
      g.fillRect(graphBounds.getX(), graphBounds.getY(),
                 startX - graphBounds.getX(), graphBounds.getHeight());
    }

    if (endPosition < 0.999f) {
      g.setColour(juce::Colours::black.withAlpha(0.45f));
      g.fillRect(endX, graphBounds.getY(), graphBounds.getRight() - endX,
                 graphBounds.getHeight());
    }

    // 4. Draw Start Position Marker Line & Dual Top/Bottom Drag Handles
    g.setColour(SpectralUILookAndFeel::accentColour);
    g.drawVerticalLine((int)startX, graphBounds.getY(),
                       graphBounds.getBottom());

    juce::Rectangle<float> startTopFlag(
        startX - 2.0f, graphBounds.getY() + 4.0f, 42.0f, 16.0f);
    g.setColour(SpectralUILookAndFeel::accentColour);
    g.fillRoundedRectangle(startTopFlag, 3.0f);
    g.setFont(SpectralUILookAndFeel::getGeometricFont(9.0f, true));
    g.setColour(juce::Colours::black);
    g.drawText("START >", startTopFlag.toNearestInt(),
               juce::Justification::centred, false);

    juce::Rectangle<float> startBottomGrip(
        startX - 6.0f, graphBounds.getBottom() - 14.0f, 12.0f, 10.0f);
    g.setColour(SpectralUILookAndFeel::accentColour);
    g.fillRoundedRectangle(startBottomGrip, 2.0f);

    // 5. Draw End Position Marker Line & Dual Top/Bottom Drag Handles
    g.setColour(SpectralUILookAndFeel::accentColour);
    g.drawVerticalLine((int)endX, graphBounds.getY(), graphBounds.getBottom());

    juce::Rectangle<float> endTopFlag(endX - 40.0f, graphBounds.getY() + 4.0f,
                                      42.0f, 16.0f);
    g.setColour(SpectralUILookAndFeel::accentColour);
    g.fillRoundedRectangle(endTopFlag, 3.0f);
    g.setFont(SpectralUILookAndFeel::getGeometricFont(9.0f, true));
    g.setColour(juce::Colours::black);
    g.drawText("< END", endTopFlag.toNearestInt(), juce::Justification::centred,
               false);

    juce::Rectangle<float> endBottomGrip(
        endX - 6.0f, graphBounds.getBottom() - 14.0f, 12.0f, 10.0f);
    g.setColour(SpectralUILookAndFeel::accentColour);
    g.fillRoundedRectangle(endBottomGrip, 2.0f);

  } else {
    // Empty state prompt
    g.setColour(SpectralUILookAndFeel::dividerColour);

    juce::Path promptBox;
    auto promptRect = graphBounds.reduced(graphBounds.getWidth() * 0.25f,
                                          graphBounds.getHeight() * 0.25f);
    promptBox.addRoundedRectangle(promptRect, 6.0f);

    juce::PathStrokeType stroke(1.2f);
    float dashes[] = {4.0f, 4.0f};
    stroke.createDashedStroke(promptBox, promptBox, dashes, 2);
    g.strokePath(promptBox, stroke);

    g.setFont(SpectralUILookAndFeel::getGeometricFont(12.0f, false));
    g.setColour(SpectralUILookAndFeel::textMutedColour);
    g.drawText("drop sample", graphBounds.toNearestInt(),
               juce::Justification::centred, false);
  }

  // Draw dragged selection in progress
  if (isLoadingSample) {
    g.setColour(juce::Colours::black.withAlpha(0.75f));
    g.fillRoundedRectangle(graphBounds, 6.0f);
    g.setColour(SpectralUILookAndFeel::accentColour);
    g.drawRoundedRectangle(graphBounds.reduced(1.0f), 6.0f, 1.5f);
    g.setFont(SpectralUILookAndFeel::getGeometricFont(13.0f, true));
    g.drawText("ANALYZING & LOADING SAMPLE...", graphBounds.toNearestInt(), juce::Justification::centred, false);
  }

  if (isDrawing && fileLoaded && currentTool != ToolType::None) {
    g.setColour(SpectralUILookAndFeel::accentColour.withAlpha(0.12f));
    if (currentTool == ToolType::RectangleSelect) {
      float x1 = graphBounds.getX() + juce::jmin(dragStartPosNormalized.x,
                                                 dragCurrentPosNormalized.x) *
                                          graphBounds.getWidth();
      float y1 = graphBounds.getY() + juce::jmin(dragStartPosNormalized.y,
                                                 dragCurrentPosNormalized.y) *
                                          graphBounds.getHeight();
      float w =
          std::abs(dragCurrentPosNormalized.x - dragStartPosNormalized.x) *
          graphBounds.getWidth();
      float h =
          std::abs(dragCurrentPosNormalized.y - dragStartPosNormalized.y) *
          graphBounds.getHeight();

      juce::Rectangle<float> rect(x1, y1, w, h);
      g.fillRect(rect);
      g.setColour(SpectralUILookAndFeel::accentColour);
      g.drawRect(rect, 1.2f);
    } else if (currentTool == ToolType::Freehand) {
      juce::Path scaledPath = currentDrawingPathNormalized;
      scaledPath.applyTransform(
          juce::AffineTransform::scale(graphBounds.getWidth(),
                                       graphBounds.getHeight())
              .translated(graphBounds.getX(), graphBounds.getY()));
      g.fillPath(scaledPath);
      g.setColour(SpectralUILookAndFeel::accentColour);
      g.strokePath(scaledPath, juce::PathStrokeType(1.2f));
    }
  }

  // Render stored selection regions with ID tags and handles
  if (fileLoaded) {
    for (int i = 0; i < selections.size(); ++i) {
      const auto &region = selections.getReference(i);
      bool isActive = (i == activeSelectionIndex);

      juce::Rectangle<float> rect(
          graphBounds.getX() +
              region.normalizedBounds.getX() * graphBounds.getWidth(),
          graphBounds.getY() +
              region.normalizedBounds.getY() * graphBounds.getHeight(),
          region.normalizedBounds.getWidth() * graphBounds.getWidth(),
          region.normalizedBounds.getHeight() * graphBounds.getHeight());

      g.setColour(SpectralUILookAndFeel::accentColour.withAlpha(
          isActive ? 0.15f : 0.08f));

      if (region.type == ToolType::Freehand &&
          !region.normalizedPath.isEmpty()) {
        juce::Path p = region.normalizedPath;
        p.applyTransform(
            juce::AffineTransform::scale(graphBounds.getWidth(),
                                         graphBounds.getHeight())
                .translated(graphBounds.getX(), graphBounds.getY()));
        g.fillPath(p);
        g.setColour(SpectralUILookAndFeel::accentColour.withAlpha(
            isActive ? 1.0f : 0.6f));
        g.strokePath(p, juce::PathStrokeType(1.2f));
      } else {
        g.fillRect(rect);
        g.setColour(SpectralUILookAndFeel::accentColour.withAlpha(
            isActive ? 1.0f : 0.6f));
        g.drawRect(rect, 1.2f);

        if (isActive) {
          float hs = 4.0f;
          g.setColour(SpectralUILookAndFeel::accentColour);
          g.fillRect(rect.getX() - hs * 0.5f, rect.getY() - hs * 0.5f, hs, hs);
          g.fillRect(rect.getRight() - hs * 0.5f, rect.getY() - hs * 0.5f, hs,
                     hs);
          g.fillRect(rect.getX() - hs * 0.5f, rect.getBottom() - hs * 0.5f, hs,
                     hs);
          g.fillRect(rect.getRight() - hs * 0.5f, rect.getBottom() - hs * 0.5f,
                     hs, hs);
        }
      }

      float yTop = region.normalizedBounds.getY();
      float yBottom = region.normalizedBounds.getBottom();
      float maxF = yToFrequency(yTop);
      float minF = yToFrequency(yBottom);
      if (minF > maxF)
        std::swap(minF, maxF);

      juce::String tag = "#0" + juce::String(region.id) + " [" +
                         formatFrequency(minF) + " - " +
                         formatFrequency(maxF) + "]";

      juce::Rectangle<float> tagBg(rect.getX() + 2.0f, rect.getY() + 2.0f, 156.0f, 16.0f);
      g.setColour(juce::Colour::fromRGB(0x10, 0x11, 0x14).withAlpha(0.90f));
      g.fillRoundedRectangle(tagBg, 3.0f);

      g.setFont(SpectralUILookAndFeel::getMonospaceFont(9.0f));
      g.setColour(SpectralUILookAndFeel::accentColour);
      g.drawText(tag, tagBg.toNearestInt().withTrimmedLeft(4).withTrimmedRight(16),
                 juce::Justification::centredLeft, false);

      juce::Rectangle<float> deleteBtn(tagBg.getRight() - 15.0f, tagBg.getY() + 1.0f, 14.0f, 14.0f);
      g.setColour(SpectralUILookAndFeel::accentColour);
      g.drawText("x", deleteBtn.toNearestInt(), juce::Justification::centred, false);
    }
  }

  // Playhead line sweep during playback (Multi-cursor in Poly mode, Single-cursor in Mono mode)
  if (processor.isPlaying()) {
    bool isPoly = processor.isPolyMode();
    auto voicePositions = processor.getActiveVoicePositions();

    if (!isPoly || voicePositions.size() <= 1) {
      float playheadPosNorm = (voicePositions.size() > 0) ? voicePositions[0] : (float)processor.getPlayheadPosition();
      float playheadX = graphBounds.getX() + graphBounds.getWidth() * playheadPosNorm;

      g.setColour(SpectralUILookAndFeel::accentColour);
      g.drawVerticalLine((int)playheadX, graphBounds.getY(), graphBounds.getBottom());
    } else {
      int numVoices = voicePositions.size();
      float baseAlpha = juce::jlimit(0.55f, 0.90f, 1.4f / std::sqrt((float)numVoices));

      for (int i = 0; i < numVoices; ++i) {
        float posNorm = voicePositions[i];
        float playheadX = graphBounds.getX() + graphBounds.getWidth() * posNorm;

        g.setColour(SpectralUILookAndFeel::accentColour.withAlpha(baseAlpha));
        g.drawVerticalLine((int)playheadX, graphBounds.getY(), graphBounds.getBottom());
      }
    }
  }

  // Outer panel hairline border
  g.setColour(dragActive ? SpectralUILookAndFeel::accentColour
                         : SpectralUILookAndFeel::dividerColour);
  g.drawRoundedRectangle(getLocalBounds().toFloat(), 8.0f, 1.0f);
}

void SpectrogramComponent::resized() {
  loopButton.setBounds(getWidth() - 84, 8, 76, 22);

  if (fileLoaded)
    generateSpectrogramImage();
}