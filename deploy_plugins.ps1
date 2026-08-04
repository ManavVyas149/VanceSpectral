# VanceSpectral Automated Local Deployment Script
# Copies compiled Release binaries to system & user plugin folders for instant DAW testing

$workspace = "C:\Users\Ansh Srivastava\OneDrive\Documents\GitHub\VanceSpectral"
$releaseDir = "$workspace\Builds\VisualStudio2022\x64\Release"

$userVst3Dir = "$env:LOCALAPPDATA\Programs\Common\VST3"
$userVst3Target = "$userVst3Dir\VanceSpectral.vst3"
$systemVst3Target = "C:\Program Files\Common Files\VST3\VanceSpectral.vst3"

$userAaxDir = "$env:LOCALAPPDATA\Avid\Audio\Plug-Ins"
$userAaxTarget = "$userAaxDir\VanceSpectral.aaxplugin"
$systemAaxTarget = "C:\Program Files\Common Files\Avid\Audio\Plug-Ins\VanceSpectral.aaxplugin"

Write-Host "=== VanceSpectral Plugin Deployment ===" -ForegroundColor Cyan

# 1. Deploy VST3
if (Test-Path "$releaseDir\VST3\NewProject.vst3") {
    # Deploy to User VST3 folder
    if (!(Test-Path $userVst3Dir)) { New-Item -ItemType Directory -Path $userVst3Dir -Force | Out-Null }
    if (Test-Path $userVst3Target) { Remove-Item -Recurse -Force $userVst3Target }
    robocopy "$releaseDir\VST3\NewProject.vst3" $userVst3Target /E /NP | Out-Null
    Write-Host "[+] Deployed VST3 to User Folder: $userVst3Target" -ForegroundColor Green

    # Try System VST3 folder
    try {
        if (Test-Path "C:\Program Files\Common Files\VST3") {
            if (Test-Path $systemVst3Target) { Remove-Item -Recurse -Force $systemVst3Target }
            robocopy "$releaseDir\VST3\NewProject.vst3" $systemVst3Target /E /NP | Out-Null
            Write-Host "[+] Deployed VST3 to System Folder: $systemVst3Target" -ForegroundColor Green
        }
    } catch {
        Write-Host "[!] Could not write to system VST3 directory (requires admin). User VST3 location updated." -ForegroundColor Yellow
    }
} else {
    Write-Host "[!] VST3 release build not found." -ForegroundColor Yellow
}

# 2. Deploy AAX
if (Test-Path "$releaseDir\AAX\NewProject.aaxplugin") {
    if (!(Test-Path $userAaxDir)) { New-Item -ItemType Directory -Path $userAaxDir -Force | Out-Null }
    if (Test-Path $userAaxTarget) { Remove-Item -Recurse -Force $userAaxTarget }
    robocopy "$releaseDir\AAX\NewProject.aaxplugin" $userAaxTarget /E /NP | Out-Null
    Write-Host "[+] Deployed AAX to User Folder: $userAaxTarget" -ForegroundColor Green

    try {
        if (Test-Path "C:\Program Files\Common Files\Avid\Audio\Plug-Ins") {
            if (Test-Path $systemAaxTarget) { Remove-Item -Recurse -Force $systemAaxTarget }
            robocopy "$releaseDir\AAX\NewProject.aaxplugin" $systemAaxTarget /E /NP | Out-Null
            Write-Host "[+] Deployed AAX to System Folder: $systemAaxTarget" -ForegroundColor Green
        }
    } catch {
        Write-Host "[!] Could not write to system AAX directory (requires admin). User AAX location updated." -ForegroundColor Yellow
    }
} else {
    Write-Host "[!] AAX release build not found." -ForegroundColor Yellow
}

Write-Host "[SUCCESS] Deployment Complete! Plugins ready for DAW scanning." -ForegroundColor Green

