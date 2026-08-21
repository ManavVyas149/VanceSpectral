$ErrorActionPreference = 'Stop'
$destDir = "C:\VanceSpectral\VanceSpectral-Plugin"

# Remove existing folder if it exists
if (Test-Path $destDir) {
    Remove-Item -Recurse -Force $destDir
}

# Create folder structure
$folders = @(
    "$destDir\Windows\VST3",
    "$destDir\Windows\CLAP",
    "$destDir\Windows\AAX",
    "$destDir\macOS\VST3",
    "$destDir\macOS\CLAP",
    "$destDir\macOS\AAX",
    "$destDir\macOS\AU"
)

foreach ($folder in $folders) {
    New-Item -ItemType Directory -Force -Path $folder | Out-Null
}

# Copy Windows VST3
$vst3Source = "C:\VanceSpectral\Builds\VisualStudio2022\x64\Release\VST3\VanceSpectral.vst3"
if (Test-Path $vst3Source) {
    Copy-Item -Recurse -Force $vst3Source "$destDir\Windows\VST3\"
}

# Copy Windows CLAP
$clapSource = "C:\VanceSpectral\Builds\VisualStudio2022\x64\Release\CLAP\VanceSpectral.clap"
if (Test-Path $clapSource) {
    Copy-Item -Recurse -Force $clapSource "$destDir\Windows\CLAP\"
}

# Copy Windows AAX
$aaxSource = "C:\VanceSpectral\Builds\VisualStudio2022\x64\Release\AAX\VanceSpectral.aaxplugin"
if (Test-Path $aaxSource) {
    Copy-Item -Recurse -Force $aaxSource "$destDir\Windows\AAX\"
}

# NOTE: macOS binaries cannot be built natively on Windows using Visual Studio/MSBuild.
# They will need to be added manually after building on a macOS machine.

# Copy README
Copy-Item -Force "C:\VanceSpectral\README-Dist.txt" "$destDir\README.txt"

# Create Zip
$zipPath = "C:\VanceSpectral\VanceSpectral-Plugin.zip"
if (Test-Path $zipPath) {
    Remove-Item -Force $zipPath
}
Compress-Archive -Path $destDir -DestinationPath $zipPath

Write-Host "Packaging complete: $zipPath"
