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
├── ThirdParty/
│   └── SoundTouch/
│
├── Builds/
│   └── VisualStudio2022/
│
├── VanceSpectral.jucer
├── THIRD_PARTY_LICENSES.md
├── .gitignore
└── README.md
```

## Third-Party Dependencies

- **[SoundTouch Audio Processing Library](https://codeberg.org/soundtouch/soundtouch)**: Used for time-stretching and pitch-shifting processing. Integrated statically under `ThirdParty/SoundTouch`. See [`THIRD_PARTY_LICENSES.md`](file:///c:/Users/Ansh%20Srivastava/OneDrive/Documents/GitHub/VanceSpectral/THIRD_PARTY_LICENSES.md) and [`ThirdParty/SoundTouch/INTEGRATION_NOTES.md`](file:///c:/Users/Ansh%20Srivastava/OneDrive/Documents/GitHub/VanceSpectral/ThirdParty/SoundTouch/INTEGRATION_NOTES.md) for licensing and integration details.
