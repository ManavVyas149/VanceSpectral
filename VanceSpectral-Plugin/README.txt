VanceSpectral Audio Plugin v1.0.0
=================================

VanceSpectral is a polyphonic spectral sampling instrument and audio plugin featuring sample-accurate pitch glide, frequency-rectangle filtering, and dynamic audio visualization.

INSTALLATION INSTRUCTIONS
-------------------------

Copy the relevant plugin format(s) for your Operating System into your DAW's standard plugin directory:

Windows Installation:
- VST3: Copy 'Windows/VST3/VanceSpectral.vst3' to:
  C:\Program Files\Common Files\VST3\

- CLAP: Copy 'Windows/CLAP/VanceSpectral.clap' to:
  C:\Program Files\Common Files\CLAP\

- AAX: Copy 'Windows/AAX/VanceSpectral.aaxplugin' to:
  C:\Program Files\Common Files\Avid\Audio\Plug-Ins\

macOS Installation:
- VST3: Copy 'macOS/VST3/VanceSpectral.vst3' to:
  /Library/Audio/Plug-Ins/VST3/

- CLAP: Copy 'macOS/CLAP/VanceSpectral.clap' to:
  /Library/Audio/Plug-Ins/CLAP/

- AAX: Copy 'macOS/AAX/VanceSpectral.aaxplugin' to:
  /Library/Application Support/Avid/Audio/Plug-Ins/

- Audio Unit (AU): Copy 'macOS/AU/VanceSpectral.component' to:
  /Library/Audio/Plug-Ins/Components/

NOTE ON SECURITY WARNINGS & CODE SIGNING
-----------------------------------------
These initial builds are currently unsigned. When launching VanceSpectral for the first time:
- macOS: Gatekeeper may display a security prompt. Right-click the plugin file and select 'Open', or go to System Settings -> Privacy & Security -> Allow.
- Windows: SmartScreen may display an unrecognized app warning. Click 'More info' and select 'Run anyway'.
Both warnings are expected until code signing certificates are attached in future releases.

AFTER COPYING FILES
-------------------
Rescan plugins within your Digital Audio Workstation (DAW) or restart your DAW to load VanceSpectral.

SELF-CONTAINED BUILD & DEPENDENCIES
-----------------------------------
All plugin binaries in this package are fully self-contained. JUCE core components, custom DSP engines, and UI graphics are statically linked into the plugin binary. No separately installed frameworks, JUCE runtime libraries, or dev toolchains are required on the host system. Standard OS C++ runtime libraries (MSVC Redistributable on Windows / macOS Core Audio & System Frameworks on macOS) are utilized natively.
