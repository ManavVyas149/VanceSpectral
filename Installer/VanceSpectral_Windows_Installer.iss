; VanceSpectral Professional Plugin Suite Installer Script (Inno Setup)
; Formats: VST3, CLAP, AAX, Standalone

#define MyAppName "VanceSpectral"
#define MyAppVersion "1.0.0"
#define MyAppPublisher "Vance Audio"
#define MyAppURL "https://github.com/ManavVyas149/VanceSpectral"

[Setup]
AppId={{566E5370-4D61-4E75-866E-53704D616E75}
AppName={#MyAppName}
AppVersion={#MyAppVersion}
AppPublisher={#MyAppPublisher}
AppPublisherURL={#MyAppURL}
DefaultDirName={autopf}\{#MyAppName}
DefaultGroupName={#MyAppName}
OutputDir=..\Dist\Release\Windows
OutputBaseFilename=VanceSpectral_Setup_v1.0.0
Compression=lzma2/max
SolidCompression=yes
WizardStyle=modern
PrivilegesRequired=admin
ArchitecturesInstallIn64BitMode=x64

[Types]
Name: "full"; Description: "Full Installation (VST3, CLAP, AAX & Standalone)"
Name: "custom"; Description: "Custom Installation"; Flags: iscustom

[Components]
Name: "vst3"; Description: "VST3 Plugin (64-bit)"; Types: full custom; Flags: fixed
Name: "clap"; Description: "CLAP Plugin (64-bit)"; Types: full custom
Name: "aax"; Description: "AAX Plugin (Pro Tools 64-bit)"; Types: full custom
Name: "standalone"; Description: "Standalone Application"; Types: full custom

[Files]
; VST3 Plugin
Source: "..\Builds\VisualStudio2022\x64\Release\VST3\VanceSpectral.vst3\*"; DestDir: "{cf64}\VST3\VanceSpectral.vst3"; Flags: recursesubdirs createallsubdirs ignoreversion; Components: vst3

; AAX Plugin
Source: "..\Builds\VisualStudio2022\x64\Release\AAX\VanceSpectral.aaxplugin\*"; DestDir: "{cf64}\Avid\Audio\Plug-Ins\VanceSpectral.aaxplugin"; Flags: recursesubdirs createallsubdirs ignoreversion; Components: aax

; Standalone Application
Source: "..\Builds\VisualStudio2022\x64\Release\Standalone Plugin\VanceSpectral.exe"; DestDir: "{app}"; DestName: "VanceSpectral.exe"; Flags: ignoreversion; Components: standalone

[Icons]
Name: "{group}\{#MyAppName}"; Filename: "{app}\VanceSpectral.exe"; Components: standalone
Name: "{group}\Uninstall {#MyAppName}"; Filename: "{uninstallexe}"

[Run]
Description: "Launch {#MyAppName} Standalone"; Filename: "{app}\VanceSpectral.exe"; Flags: postinstall nowait skipifsilent; Components: standalone
