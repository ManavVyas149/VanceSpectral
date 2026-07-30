# VanceSpectral Automated Local Deployment Script
# Copies compiled Release binaries to system plugin folders for instant DAW testing

$ErrorActionPreference = "Stop"

$workspace = "C:\Users\Ansh Srivastava\OneDrive\Documents\GitHub\VanceSpectral"
$releaseDir = "$workspace\Builds\VisualStudio2022\x64\Release"

$vst3Target = "C:\Program Files\Common Files\VST3\VanceSpectral.vst3"
$aaxTarget = "C:\Program Files\Common Files\Avid\Audio\Plug-Ins\VanceSpectral.aaxplugin"

Write-Host "=== VanceSpectral Plugin Deployment ===" -ForegroundColor Cyan

# 1. Deploy VST3
if (Test-Path "$releaseDir\VST3\NewProject.vst3") {
    Write-Host "[+] Deploying VST3 to $vst3Target..." -ForegroundColor Green
    if (Test-Path $vst3Target) { Remove-Item -Recurse -Force $vst3Target }
    Copy-Item -Recurse -Force "$releaseDir\VST3\NewProject.vst3" $vst3Target
} else {
    Write-Host "[!] VST3 release build not found." -ForegroundColor Yellow
}

# 2. Deploy AAX
if (Test-Path "$releaseDir\AAX\NewProject.aaxplugin") {
    Write-Host "[+] Deploying AAX to $aaxTarget..." -ForegroundColor Green
    if (!(Test-Path "C:\Program Files\Common Files\Avid\Audio\Plug-Ins")) {
        New-Item -ItemType Directory -Path "C:\Program Files\Common Files\Avid\Audio\Plug-Ins" -Force
    }
    if (Test-Path $aaxTarget) { Remove-Item -Recurse -Force $aaxTarget }
    Copy-Item -Recurse -Force "$releaseDir\AAX\NewProject.aaxplugin" $aaxTarget
} else {
    Write-Host "[!] AAX release build not found." -ForegroundColor Yellow
}

Write-Host "[SUCCESS] Deployment Complete! Plugins ready for DAW scanning." -ForegroundColor Green
