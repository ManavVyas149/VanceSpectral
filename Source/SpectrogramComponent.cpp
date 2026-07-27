#include "SpectrogramComponent.h"
#include "PluginProcessor.h"

SpectrogramComponent::SpectrogramComponent(VancespectralAudioProcessor &p)
    : processor(p) {
  formatManager.registerBasicFormats();

  addAndMakeVisible(loopButton);

  loopButton.onClick = [this]() {
    loopEnabled = !loopEnabled;

    processor.setLoop(loopEnabled);

    loopButton.setButtonText(loopEnabled ? "LOOP ON" : "LOOP");
    ;
  };
}

void SpectrogramComponent::loadAudioFile(const juce::File &file) {
  reader.reset(formatManager.createReaderFor(file));

  if (reader == nullptr)
    return;

  audioBuffer.setSize((int)reader->numChannels, (int)reader->lengthInSamples);

  reader->read(&audioBuffer, 0, (int)reader->lengthInSamples, 0, true, true);

  fileLoaded = true;

  // Send the loaded sample to the SampleEngine
  processor.loadSample(audioBuffer);

  repaint();
}

bool SpectrogramComponent::isInterestedInFileDrag(
    const juce::StringArray &files) {
  for (auto &file : files) {
    if (file.endsWithIgnoreCase(".wav"))
      return true;

    if (file.endsWithIgnoreCase(".aiff"))
      return true;

    if (file.endsWithIgnoreCase(".flac"))
      return true;

    if (file.endsWithIgnoreCase(".mp3"))
      return true;

    if (file.endsWithIgnoreCase(".ogg"))
      return true;

    if (file.endsWithIgnoreCase(".m4a"))
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

void SpectrogramComponent::mouseUp(const juce::MouseEvent &) {
  draggingStart = false;
  draggingEnd = false;
}

void SpectrogramComponent::changeListenerCallback(juce::ChangeBroadcaster *) {
  repaint();
}

void SpectrogramComponent::mouseDown(const juce::MouseEvent &e) {
  auto area = getLocalBounds().reduced(20);

  float startX = area.getX() + area.getWidth() * startPosition;

  float endX = area.getX() + area.getWidth() * endPosition;

  if (std::abs(e.position.x - startX) < 10) {
    draggingStart = true;
    return;
  }

  if (std::abs(e.position.x - endX) < 10) {
    draggingEnd = true;
    return;
  }

  if (isPlaying) {
    processor.stopSample();
    isPlaying = false;
  } else {
    processor.playSample();
    isPlaying = true;
  }
}

void SpectrogramComponent::mouseDrag(const juce::MouseEvent &e) {
  auto area = getLocalBounds().reduced(20);

  float mousePos = (float)(e.position.x - area.getX()) / area.getWidth();

  mousePos = juce::jlimit(0.0f, 1.0f, mousePos);

  if (draggingStart)
    startPosition = mousePos;

  if (draggingEnd)
    endPosition = mousePos;

  processor.setRegion(startPosition, endPosition);

  repaint();
}

void SpectrogramComponent::drawWaveform(juce::Graphics &g) {
  if (!fileLoaded)
    return;

  auto area = getLocalBounds().reduced(20);

  auto *samples = audioBuffer.getReadPointer(0);

  int numSamples = audioBuffer.getNumSamples();

  float width = (float)area.getWidth();
  float height = (float)area.getHeight();

  juce::Path waveform;

  float centreY = area.getCentreY();

  int samplesPerPixel = juce::jmax(1, numSamples / area.getWidth());

  waveform.startNewSubPath(area.getX(), centreY);

  for (int x = 0; x < width; x++) {
    int samplePosition = x * samplesPerPixel;

    float min = 1.0f;
    float max = -1.0f;

    for (int i = 0; i < samplesPerPixel; i++) {
      if (samplePosition + i < numSamples) {
        float sample = samples[samplePosition + i];

        min = juce::jmin(min, sample);
        max = juce::jmax(max, sample);
      }
    }

    float y1 = centreY - (max * height * 0.45f);
    float y2 = centreY - (min * height * 0.45f);

    waveform.lineTo(area.getX() + x, y1);

    waveform.lineTo(area.getX() + x, y2);
  }

  g.setColour(juce::Colours::cyan);

  g.strokePath(waveform, juce::PathStrokeType(1.5f));
}

void SpectrogramComponent::paint(juce::Graphics &g) {
  g.fillAll(juce::Colour(18, 18, 18));

  if (dragActive)
    g.setColour(juce::Colours::deepskyblue);
  else
    g.setColour(juce::Colour(55, 55, 55));
  g.drawRoundedRectangle(getLocalBounds().toFloat(), 8.0f, 2.0f);

  g.setColour(juce::Colour(35, 35, 35));

  const int spacing = 40;

  for (int x = spacing; x < getWidth(); x += spacing)
    g.drawVerticalLine(x, 0.0f, (float)getHeight());

  for (int y = spacing; y < getHeight(); y += spacing)
    g.drawHorizontalLine(y, 0.0f, (float)getWidth());

  g.setColour(juce::Colours::white);
  g.setFont(20.0f);

  g.drawText("Drag Sample Here", getLocalBounds(),
             juce::Justification::centred);
  if (loadedFile.existsAsFile()) {
    g.setColour(juce::Colours::white);
    g.setFont(18.0f);
    g.drawText("Loaded: " + loadedFile.getFileName(),
               getLocalBounds().reduced(20),
               juce::Justification::centredBottom);
  }
  if (fileLoaded) {
    drawWaveform(g);
    g.setColour(juce::Colours::lime);

    g.drawText("Samples: " + juce::String(audioBuffer.getNumSamples()), 20, 20,
               300, 30, juce::Justification::left);
  }
  auto area = getLocalBounds().reduced(20);

  float startX = area.getX() + area.getWidth() * startPosition;

  float endX = area.getX() + area.getWidth() * endPosition;

  g.setColour(juce::Colours::yellow);

  g.drawVerticalLine(startX, area.getY(), area.getBottom());

  g.drawVerticalLine(endX, area.getY(), area.getBottom());
}

void SpectrogramComponent::resized() {
  loopButton.setBounds(20, getHeight() - 40, 100, 25);
}