===================================================================
VanceSpectral Professional Spectral Audio Instrument - Installation Guide
===================================================================

VanceSpectral is a high-performance spectral synthesizer and sample manipulation instrument featuring interactive frequency rectangle selection, granular spectral resynthesis, dynamic pitch shifting, and direct drag-and-drop sample export to host DAWs.

===================================================================
MANUAL INSTALLATION INSTRUCTIONS (DRAG & DROP)
===================================================================

WINDOWS:
--------
- VST3 (FL Studio, Ableton Live, Reaper, Cubase, Studio One, Bitwig):
  Copy 'Windows\VST3\VanceSpectral.vst3' to:
  C:\Program Files\Common Files\VST3\

- CLAP (Bitwig, Reaper, FL Studio):
  Copy 'Windows\CLAP\VanceSpectral.clap' to:
  C:\Program Files\Common Files\CLAP\

- AAX (Pro Tools 2021.3+):
  Copy 'Windows\AAX\VanceSpectral.aaxplugin' to:
  C:\Program Files\Common Files\Avid\Audio\Plug-Ins\


macOS (Universal Binary - Intel & Apple Silicon M1/M2/M3):
---------------------------------------------------------
- VST3 (Ableton Live, Cubase, Reaper, Bitwig, Studio One):
  Copy 'macOS/VST3/VanceSpectral.vst3' to:
  /Library/Audio/Plug-Ins/VST3/

- CLAP (Bitwig, Reaper, FL Studio):
  Copy 'macOS/CLAP/VanceSpectral.clap' to:
  /Library/Audio/Plug-Ins/CLAP/

- AAX (Pro Tools 2021.3+):
  Copy 'macOS/AAX/VanceSpectral.aaxplugin' to:
  /Library/Application Support/Avid/Audio/Plug-Ins/

- Audio Unit / AU (Logic Pro X / 11, GarageBand, MainStage):
  Copy 'macOS/AU/VanceSpectral.component' to:
  /Library/Audio/Plug-Ins/Components/

===================================================================
IMPORTANT SECURITY & CODE SIGNING NOTES
===================================================================

- Windows SmartScreen / Security Warning:
  If Windows SmartScreen displays a warning when copying or scanning the files, click "More Info" -> "Run Anyway". This occurs because the development build is unsigned.

- macOS Gatekeeper Security Notice:
  These development builds are currently unsigned. On first launch in Logic Pro or macOS, Gatekeeper may prevent opening the plugin.
  Workaround: Right-click the .component / .vst3 file -> Open, or go to System Settings -> Privacy & Security -> General, and click "Allow Anyway".

===================================================================
DAW RESCAN
===================================================================
After copying the plugin files to your system plugin directory, open your DAW and perform a plugin rescan (or restart your DAW). VanceSpectral will appear under Instruments / Synths.
