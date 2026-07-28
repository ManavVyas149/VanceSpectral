#include "SpectrogramComponent.h"
#include "PluginProcessor.h"

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

void SpectrogramComponent::timerCallback() { repaint(); }

void SpectrogramComponent::loadAudioFile(const juce::File &file) {
  reader.reset(formatManager.createReaderFor(file));

  if (reader == nullptr)
    return;

  audioBuffer.setSize((int)reader->numChannels, (int)reader->lengthInSamples);
  reader->read(&audioBuffer, 0, (int)reader->lengthInSamples, 0, true, true);

  loadedFile = file;
  fileLoaded = true;

  startPosition = 0.0f;
  endPosition = 1.0f;

  processor.loadSample(audioBuffer, reader->sampleRate);
  processor.setRegion(startPosition, endPosition);
  processor.setLoop(loopEnabled);
  updateFrequencyFilterFromSelections();
  generateSpectrogramImage();

  if (onFileLoadedStateChanged)
    onFileLoadedStateChanged(true);

  repaint();
}

float SpectrogramComponent::yToFrequency(float normY) {
  normY = juce::jlimit(0.0f, 1.0f, normY);
  return 20.0f * std::pow(1000.0f, 1.0f - normY);
}

juce::String SpectrogramComponent::formatFrequency(float hz) {
  if (hz >= 1000.0f)
    return juce::String(hz / 1000.0f, 1) + "kHz";
  return juce::String((int)hz) + "Hz";
}

void SpectrogramComponent::updateFrequencyFilterFromSelections() {
  if (!fileLoaded || selections.isEmpty()) {
    processor.setFrequencyFilterBands(juce::Array<FrequencyBand>());
    return;
  }

  juce::Array<FrequencyBand> bands;
  for (int i = 0; i < selections.size(); ++i) {
    const auto &reg = selections.getReference(i);
    auto bounds = reg.normalizedBounds;
    if (!bounds.isEmpty()) {
      float yTop = bounds.getY();
      float yBottom = bounds.getBottom();

      float maxFreq = yToFrequency(yTop);
      float minFreq = yToFrequency(yBottom);

      if (minFreq > maxFreq)
        std::swap(minFreq, maxFreq);

      bands.add({minFreq, maxFreq});
    }
  }

  processor.setFrequencyFilterBands(bands);
}

juce::Colour
SpectrogramComponent::getSpectrogramColor(float magnitudeNormalized) {
  magnitudeNormalized = juce::jlimit(0.0f, 1.0f, magnitudeNormalized);

  if (magnitudeNormalized < 0.05f) {
    return SpectralUILookAndFeel::graphBgColour;
  } else if (magnitudeNormalized < 0.35f) {
    float t = (magnitudeNormalized - 0.05f) / 0.30f;
    return juce::Colour::fromRGB(0x0A, 0x0A, 0x0C)
        .interpolatedWith(juce::Colour::fromRGB(0x16, 0x20, 0x2C), t);
  } else if (magnitudeNormalized < 0.65f) {
    float t = (magnitudeNormalized - 0.35f) / 0.30f;
    return juce::Colour::fromRGB(0x16, 0x20, 0x2C)
        .interpolatedWith(juce::Colour::fromRGB(0x2D, 0x3E, 0x50), t);
  } else if (magnitudeNormalized < 0.88f) {
    float t = (magnitudeNormalized - 0.65f) / 0.23f;
    return juce::Colour::fromRGB(0x2D, 0x3E, 0x50)
        .interpolatedWith(juce::Colour::fromRGB(0x78, 0x5A, 0x42), t);
  } else {
    float t = (magnitudeNormalized - 0.88f) / 0.12f;
    return juce::Colour::fromRGB(0x78, 0x5A, 0x42)
        .interpolatedWith(juce::Colour::fromRGB(0xD9, 0x8B, 0x4F), t);
  }
}

void SpectrogramComponent::generateSpectrogramImage() {
  if (!fileLoaded || audioBuffer.getNumSamples() == 0)
    return;

  int imgWidth = juce::jmax(100, getWidth() > 40 ? getWidth() - 38 : 560);
  int imgHeight = juce::jmax(100, getHeight() > 0 ? getHeight() : 350);

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
  g.setColour(SpectralUILookAndFeel::textMutedColour);

  // Standard order: 20kHz at Top -> 20Hz at Bottom
  struct ScalePoint {
    float normY; // 0 at top, 1 at bottom
    const char *label;
  };

  ScalePoint points[] = {{0.04f, "20kHz"}, {0.18f, "10kHz"}, {0.34f, "5kHz"},
                         {0.50f, "1kHz"},  {0.66f, "500Hz"}, {0.82f, "100Hz"},
                         {0.96f, "20Hz"}};

  float rightEdge = axisBounds.getRight() - 4.0f;

  for (const auto &pt : points) {
    float y = axisBounds.getY() + pt.normY * axisBounds.getHeight();

    // Draw tick mark line
    g.setColour(SpectralUILookAndFeel::dividerColour.withAlpha(0.6f));
    g.drawLine(rightEdge - 3.0f, y, rightEdge, y, 1.0f);

    // Draw label text right-aligned
    juce::Rectangle<int> labelRect((int)axisBounds.getX(), (int)(y - 7.0f),
                                   (int)(rightEdge - axisBounds.getX() - 5.0f),
                                   14);
    g.setColour(SpectralUILookAndFeel::textMutedColour);
    g.drawText(pt.label, labelRect, juce::Justification::centredRight, false);
  }

  // Draw axis hairline separator line
  g.setColour(SpectralUILookAndFeel::dividerColour);
  g.drawVerticalLine((int)axisBounds.getRight(), axisBounds.getY(),
                     axisBounds.getBottom());
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

  // Center zero-crossing hairline in translucent Ice Cyan
  juce::Colour cyanColour = juce::Colour::fromRGB(0x38, 0xBD, 0xF8); // #38BDF8 Sky Cyan
  g.setColour(cyanColour.withAlpha(0.20f));
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

  // Translucent Ice Cyan waveform fill (~12% alpha)
  g.setColour(cyanColour.withAlpha(0.12f));
  g.fillPath(waveformEnvelope);

  // Crisp top & bottom Ice Cyan waveform outlines (~85% alpha)
  g.setColour(cyanColour.withAlpha(0.85f));
  g.strokePath(waveformTop, juce::PathStrokeType(1.2f));
  g.setColour(cyanColour.withAlpha(0.65f));
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

void SpectrogramComponent::mouseDown(const juce::MouseEvent &e) {
  grabKeyboardFocus();
  auto fullBounds = getLocalBounds().toFloat();
  if (fullBounds.isEmpty())
    return;

  auto graphBounds = fullBounds.withTrimmedLeft(38.0f);

  float mouseX = e.position.x;
  float startX = graphBounds.getX() + graphBounds.getWidth() * startPosition;
  float endX = graphBounds.getX() + graphBounds.getWidth() * endPosition;

  if (std::abs(mouseX - startX) <= 16.0f) {
    draggingStartMarker = true;
    return;
  }

  if (std::abs(mouseX - endX) <= 16.0f) {
    draggingEndMarker = true;
    return;
  }

  if (!graphBounds.contains(e.position))
    return;

  float normX = juce::jlimit(
      0.0f, 1.0f, (e.position.x - graphBounds.getX()) / graphBounds.getWidth());
  float normY = juce::jlimit(0.0f, 1.0f,
                             (e.position.y - graphBounds.getY()) /
                                 graphBounds.getHeight());

  if (!fileLoaded)
    return;

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

  if (e.getNumberOfClicks() >= 2) {
    if (processor.isPlaying())
      processor.stopSample();
    else
      processor.playSample();
    return;
  }

  for (int i = selections.size() - 1; i >= 0; --i) {
    if (selections.getReference(i).normalizedBounds.contains(normX, normY)) {
      activeSelectionIndex = i;
      updateFrequencyFilterFromSelections();
      repaint();
      break;
    }
  }

  if (currentTool != ToolType::None) {
    isDrawing = true;
    dragStartPosNormalized = juce::Point<float>(normX, normY);
    dragCurrentPosNormalized = dragStartPosNormalized;

    if (currentTool == ToolType::Freehand) {
      currentDrawingPathNormalized.clear();
      currentDrawingPathNormalized.startNewSubPath(dragStartPosNormalized);
    }
  }
}

void SpectrogramComponent::mouseDrag(const juce::MouseEvent &e) {
  auto fullBounds = getLocalBounds().toFloat();
  if (fullBounds.isEmpty())
    return;

  auto graphBounds = fullBounds.withTrimmedLeft(38.0f);
  float normX = juce::jlimit(
      0.0f, 1.0f, (e.position.x - graphBounds.getX()) / graphBounds.getWidth());

  if (draggingStartMarker) {
    startPosition = juce::jlimit(0.0f, endPosition - 0.01f, normX);
    processor.setRegion(startPosition, endPosition);
    repaint();
    return;
  }

  if (draggingEndMarker) {
    endPosition = juce::jlimit(startPosition + 0.01f, 1.0f, normX);
    processor.setRegion(startPosition, endPosition);
    repaint();
    return;
  }

  if (!isDrawing || currentTool == ToolType::None)
    return;

  float normY = juce::jlimit(0.0f, 1.0f,
                             (e.position.y - graphBounds.getY()) /
                                 graphBounds.getHeight());
  dragCurrentPosNormalized = juce::Point<float>(normX, normY);

  if (currentTool == ToolType::Freehand) {
    currentDrawingPathNormalized.lineTo(dragCurrentPosNormalized);
  }

  repaint();
}

void SpectrogramComponent::mouseUp(const juce::MouseEvent &) {
  if (draggingStartMarker || draggingEndMarker) {
    draggingStartMarker = false;
    draggingEndMarker = false;
    return;
  }

  if (!isDrawing || currentTool == ToolType::None)
    return;
  isDrawing = false;

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

  currentDrawingPathNormalized.clear();
  updateFrequencyFilterFromSelections();
  repaint();
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

  // Left Frequency Axis Margin (38px width, 20kHz at Top -> 20Hz at Bottom)
  auto axisBounds = bounds.removeFromLeft(38.0f);
  drawFrequencyAxis(g, axisBounds);

  auto graphBounds = bounds;

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

      // Dark background pill box for text legibility over spectrogram
      juce::Rectangle<float> tagBg(rect.getX() + 2.0f, rect.getY() + 2.0f,
                                   145.0f, 15.0f);
      g.setColour(juce::Colour::fromRGB(0x10, 0x11, 0x14).withAlpha(0.85f));
      g.fillRoundedRectangle(tagBg, 3.0f);

      g.setFont(SpectralUILookAndFeel::getMonospaceFont(9.0f));
      g.setColour(SpectralUILookAndFeel::accentColour);
      g.drawText(tag, tagBg.toNearestInt().withTrimmedLeft(4),
                 juce::Justification::centredLeft, false);
    }
  }

  // Thin Playhead line sweep during playback
  if (processor.isPlaying()) {
    float playheadPosNorm = (float)processor.getPlayheadPosition();
    float playheadX =
        graphBounds.getX() + graphBounds.getWidth() * playheadPosNorm;
    g.setColour(juce::Colours::white.withAlpha(0.9f));
    g.drawVerticalLine((int)playheadX, graphBounds.getY(),
                       graphBounds.getBottom());
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