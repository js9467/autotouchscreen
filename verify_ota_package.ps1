# OTA Package Verification Script
param([Parameter(Mandatory=$true)][string]$Version)
$ErrorActionPreference = "Stop"

Write-Host "`n=== OTA Package Verification ===" -ForegroundColor Cyan
Write-Host "Verifying version: $Version`n" -ForegroundColor White

# Check 1: Version state file
Write-Host "[1/5] Checking .version_state.json..." -ForegroundColor Yellow
if (-not (Test-Path ".version_state.json")) {
    Write-Host "FAIL: .version_state.json not found!" -ForegroundColor Red
    exit 1
}
$state = Get-Content ".version_state.json" -Raw | ConvertFrom-Json
$stateVersion = "$($state.major).$($state.minor).$($state.build)"
if ($stateVersion -ne $Version) {
    Write-Host "FAIL: State version ($stateVersion) != expected ($Version)" -ForegroundColor Red
    exit 1
}
Write-Host "PASS: State file shows $stateVersion" -ForegroundColor Green

# Check 2: Version auto header
Write-Host "[2/5] Checking src/version_auto.h..." -ForegroundColor Yellow
if (-not (Test-Path "src/version_auto.h")) {
    Write-Host "FAIL: version_auto.h not found!" -ForegroundColor Red
    exit 1
}
$headerContent = Get-Content "src/version_auto.h" -Raw
if ($headerContent -match "(\d+\.\d+\.\d+)") {
    $headerVersion = $matches[1]
    if ($headerVersion -ne $Version) {
        Write-Host "FAIL: Header version ($headerVersion) != expected ($Version)" -ForegroundColor Red
        exit 1
    }
    Write-Host "PASS: Header shows $headerVersion" -ForegroundColor Green
} else {
    Write-Host "FAIL: Could not parse version from header" -ForegroundColor Red
    exit 1
}

# Check 3: Firmware binary
Write-Host "[3/5] Checking firmware binary..." -ForegroundColor Yellow
$firmwarePath = ".pio/build/waveshare_7in/firmware.bin"
if (-not (Test-Path $firmwarePath)) {
    Write-Host "FAIL: Firmware binary not found" -ForegroundColor Red
    exit 1
}
$firmware = Get-Item $firmwarePath
Write-Host "PASS: Firmware binary exists ($($firmware.Length) bytes)" -ForegroundColor Green

# Check 4: Release directory
Write-Host "[4/5] Checking OTA release directory..." -ForegroundColor Yellow
$releaseDir = "ota_functions/releases/$Version"
if (-not (Test-Path $releaseDir)) {
    Write-Host "FAIL: Release directory not found: $releaseDir" -ForegroundColor Red
    exit 1
}
if (-not (Test-Path "$releaseDir/firmware.bin")) {
    Write-Host "FAIL: Release firmware not found" -ForegroundColor Red
    exit 1
}
if (-not (Test-Path "$releaseDir/manifest.json")) {
    Write-Host "FAIL: Release manifest not found" -ForegroundColor Red
    exit 1
}
Write-Host "PASS: Release directory complete" -ForegroundColor Green

# Check 5: Manifest versions
Write-Host "[5/5] Checking manifest versions..." -ForegroundColor Yellow
$releaseManifest = Get-Content "$releaseDir/manifest.json" -Raw | ConvertFrom-Json
$mainManifest = Get-Content "ota_functions/manifest.json" -Raw | ConvertFrom-Json

if ($releaseManifest.version -ne $Version) {
    Write-Host "FAIL: Release manifest version mismatch" -ForegroundColor Red
    exit 1
}
if ($mainManifest.version -ne $Version) {
    Write-Host "FAIL: Main manifest version mismatch" -ForegroundColor Red
    exit 1
}

$md5 = (Get-FileHash -Algorithm MD5 -Path "$releaseDir/firmware.bin").Hash.ToLower()
if ($releaseManifest.firmware.md5 -ne $md5) {
    Write-Host "FAIL: Release manifest MD5 mismatch!" -ForegroundColor Red
    exit 1
}
if ($mainManifest.md5 -ne $md5) {
    Write-Host "FAIL: Main manifest MD5 mismatch!" -ForegroundColor Red
    exit 1
}
Write-Host "PASS: All manifests consistent" -ForegroundColor Green

Write-Host "`n=== All Checks Passed! ===" -ForegroundColor Green
Write-Host "Version:  $Version" -ForegroundColor White
Write-Host "Firmware: $($firmware.Length) bytes" -ForegroundColor White
Write-Host "MD5:      $md5" -ForegroundColor White
Write-Host "`nSafe to deploy!" -ForegroundColor Green
