# VanceSpectral

VanceSpectral is a JUCE-based audio plugin project focused on sample playback, waveform visualization, spectral analysis, and audio processing.

## Current Features

- Audio sample loading via drag and drop
- WAV audio support
- MP3 audio support
- AIFF audio support
- FLAC audio support
- Waveform visualization
- Sample playback
- Loop playback
- Start and end region selection
- Stereo audio output
- JUCE-based audio processing

## Project Structure

```text
VanceSpectral/
│
├── Source/
│   ├── PluginProcessor.cpp
│   ├── PluginProcessor.h
│   ├── PluginEditor.cpp
│   ├── PluginEditor.h
│   ├── SampleEngine.cpp
│   ├── SampleEngine.h
│   ├── SpectrogramComponent.cpp
│   └── SpectrogramComponent.h
│
├── JuceLibraryCode/
│
├── Builds/
│   └── VisualStudio2022/
│
├── VanceSpectral.jucer
├── .gitignore
└── README.md
