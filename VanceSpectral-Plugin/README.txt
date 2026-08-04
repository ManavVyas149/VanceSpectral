===============================================================================
VanceSpectral - Spectral Frequency Sampler Plugin
===============================================================================

VanceSpectral is a real-time spectral frequency sampler and audio instrument designed
for deep sound design, precise spectral filtering, dynamic pitch shifting, and sample manipulation.

-------------------------------------------------------------------------------
INSTALLATION INSTRUCTIONS
-------------------------------------------------------------------------------

Simply copy the appropriate plugin format file(s) for your Operating System into
your DAW's system plugin directory:

--- WINDOWS ---
• VST3: Copy "Windows/VST3/VanceSpectral.vst3" to:
  C:\Program Files\Common Files\VST3\

• CLAP: Copy "Windows/CLAP/VanceSpectral.clap" to:
  C:\Program Files\Common Files\CLAP\

• AAX:  Copy "Windows/AAX/VanceSpectral.aaxplugin" to:
  C:\Program Files\Common Files\Avid\Audio\Plug-Ins\

--- macOS ---
• VST3: Copy "macOS/VST3/VanceSpectral.vst3" to:
  /Library/Audio/Plug-Ins/VST3/

• CLAP: Copy "macOS/CLAP/VanceSpectral.clap" to:
  /Library/Audio/Plug-Ins/CLAP/

• AAX:  Copy "macOS/AAX/VanceSpectral.aaxplugin" to:
  /Library/Application Support/Avid/Audio/Plug-Ins/

• AU:   Copy "macOS/AU/VanceSpectral.component" to:
  /Library/Audio/Plug-Ins/Components/

-------------------------------------------------------------------------------
DEPENDENCIES & CODE SIGNING NOTE
-------------------------------------------------------------------------------
• Self-Contained Builds: All builds are statically linked and fully self-contained.
  No external frameworks, dylibs, runtime installer packages, or build tools are required.

• Security Warnings: These pre-release builds are currently unsigned.
  - On macOS: Gatekeeper may display a security alert on first load. To bypass,
    right-click (or Control-click) the plugin file -> select "Open", or approve it under
    System Settings -> Privacy & Security.
  - On Windows: Windows Defender / SmartScreen may display an unrecognized app notice.
    Click "More info" -> "Run anyway".
  (Official certificate signing will be added in a future production release.)

-------------------------------------------------------------------------------
FINAL STEP
-------------------------------------------------------------------------------
After copying the plugin files, open your DAW (FL Studio, Ableton Live, Logic Pro,
Pro Tools, Reaper, Cubase, etc.) and perform a full plugin rescan (or restart your DAW).
===============================================================================
