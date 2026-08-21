VanceSpectral is a high-quality spectral synthesizer plugin with advanced waveform manipulation.

INSTALLATION INSTRUCTIONS

Please copy the appropriate plugin files for your operating system and format to the correct folder on your computer.

Windows VST3: copy VanceSpectral.vst3 to C:\Program Files\Common Files\VST3\
Windows CLAP: copy VanceSpectral.clap to C:\Program Files\Common Files\CLAP\
Windows AAX: copy VanceSpectral.aaxplugin to C:\Program Files\Common Files\Avid\Audio\Plug-Ins\

macOS VST3: copy VanceSpectral.vst3 to /Library/Audio/Plug-Ins/VST3/
macOS CLAP: copy VanceSpectral.clap to /Library/Audio/Plug-Ins/CLAP/
macOS AAX: copy VanceSpectral.aaxplugin to /Library/Application Support/Avid/Audio/Plug-Ins/
macOS AU: copy VanceSpectral.component to /Library/Audio/Plug-Ins/Components/

NOTES & TROUBLESHOOTING

- Rescan Plugins: After copying the files to the designated folders, be sure to rescan your plugins inside your DAW, or simply restart the DAW so it can detect the new instrument.
- Security Warnings: These builds are currently unsigned. On Windows, SmartScreen may show a security warning—this is expected. On macOS, Gatekeeper may prevent the plugin from running initially. To bypass this, either allow the plugin via System Settings -> Privacy & Security, or run the following command in Terminal to clear the quarantine flag: `xattr -cr /Library/Audio/Plug-Ins/Components/VanceSpectral.component` (replace path with your installed format).
- Dependencies: These plugins are statically linked and fully self-contained. No external visual C++ redistributables or frameworks should be needed beyond a standard installation.
