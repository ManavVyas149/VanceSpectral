================================================================================
                    VANCESPECTRAL SYNTHESIZER & SAMPLER
                               Version 1.0.0
================================================================================

ABOUT VANCESPECTRAL
-------------------
VanceSpectral is a polyphonic spectral synthesizer and sample-manipulation
instrument featuring multi-resolution STFT visualization, logarithmic frequency
remapping, pitch processing (Stretch and Resample modes), and a 5-stage master
effects chain (Sidechain -> Chorus -> Phaser -> Delay -> Drive).

NOTE: VanceSpectral is an INSTRUMENT / SYNTH, NOT an audio effect plugin. In
your DAW's plugin browser, look for VanceSpectral under "Instruments" or "Synths".


DIRECTORY STRUCTURE
-------------------
VanceSpectral-Plugin/
├── Windows/
│   ├── VST3/
│   │   └── VanceSpectral.vst3
│   ├── CLAP/
│   │   └── (Reserved for CLAP binary)
│   └── AAX/
│       └── VanceSpectral.aaxplugin
├── macOS/
│   ├── VST3/
│   ├── CLAP/
│   ├── AAX/
│   └── AU/
└── README.txt


MANUAL INSTALLATION INSTRUCTIONS
--------------------------------
Copy the appropriate plugin bundle for your operating system and desired format
into the designated plugin directory on your system:

Windows (64-bit):
  • VST3: Copy "VanceSpectral.vst3" to:
          C:\Program Files\Common Files\VST3\
  • CLAP: Copy "VanceSpectral.clap" to:
          C:\Program Files\Common Files\CLAP\
  • AAX:  Copy "VanceSpectral.aaxplugin" to:
          C:\Program Files\Common Files\Avid\Audio\Plug-Ins\

macOS (Universal / Apple Silicon & Intel):
  • VST3: Copy "VanceSpectral.vst3" to:
          /Library/Audio/Plug-Ins/VST3/
  • CLAP: Copy "VanceSpectral.clap" to:
          /Library/Audio/Plug-Ins/CLAP/
  • AAX:  Copy "VanceSpectral.aaxplugin" to:
          /Library/Application Support/Avid/Audio/Plug-Ins/
  • AU:   Copy "VanceSpectral.component" to:
          /Library/Audio/Plug-Ins/Components/


AFTER INSTALLING: RESCAN PLUGINS
--------------------------------
After copying the files into the destination directory:
1. Open your DAW (Digital Audio Workstation) or host software.
2. Trigger a plugin rescan in your DAW's preferences, or restart the DAW.
3. Add a new Instrument / MIDI track.
4. VanceSpectral will appear in your instrument list under:
   Vendor: yourcompany / Category: Synth / Instrument.


CODE SIGNING & SECURITY NOTICES
-------------------------------
These initial distribution builds are currently unsigned:

• Windows (SmartScreen):
  If Windows Defender SmartScreen displays a blue "Windows protected your PC"
  dialog upon first loading, click "More info" followed by "Run anyway".

• macOS (Gatekeeper):
  If macOS displays a warning that the plugin cannot be opened because the
  developer cannot be verified:
  1. Open System Settings -> Privacy & Security.
  2. Scroll to Security and click "Open Anyway" next to VanceSpectral.
  Alternatively, clear the quarantine attribute via Terminal:
    sudo xattr -cr /Library/Audio/Plug-Ins/Components/VanceSpectral.component
    (replace the path with your installed format)


THIRD-PARTY LICENSES & LINKING
------------------------------
VanceSpectral incorporates high-performance open-source audio libraries:
  • SoundTouch (LGPL v2.1) - Pitch and tempo stretch engine. In this release,
    SoundTouch is statically linked into the plugin binary. LGPL v2.1 Section 6
    relinking instructions and full source code are preserved in the repository
    (ThirdParty/SoundTouch/). If dynamically linked SoundTouch.dll is distributed
    in future updates, it must remain placed alongside the plugin binary.
  • libsamplerate (BSD-2-Clause) - Sample rate conversion.
  • chowdsp_utils (BSD-3-Clause / GPLv3) - DSP mathematical and SIMD utilities.
  • Airwindows (MIT License) - Spiral saturation and ChorusEnsemble algorithms.
Detailed license declarations are available in THIRD_PARTY_LICENSES.md.


HOST & FORMAT COMPATIBILITY
---------------------------
• VST3 (Windows x64): Fully validated with zero non-system dependencies.
• Standalone (Windows x64): Tested for direct host-free execution.
• AAX (Windows x64): Built against the AAX SDK. Requires verification on an
  Avid Pro Tools workstation.
• macOS / CLAP: Formats are reserved in the package structure for corresponding
  platform builds.
================================================================================
