# Standardized Deployment Script for Bronco Controls
# This script MUST be used for all deployments to maintain version consistency
# AI AGENTS: Always use this script - never manually edit version files or manifests

param(
    [string]$VersionBump = "patch",  # patch, minor, or major
    [string]$Changelog = "",
    [switch]$SkipBuild,
    [switch]$SkipOTA,
    [switch]$SkipGit,
    [switch]$LocalOnly
)

$ErrorActionPreference = "Stop"

Write-Host "`n=== Bronco Controls Standardized Deployment ===" -ForegroundColor Cyan
Write-Host "This ensures consistent versioning across all updates`n" -ForegroundColor Yellow

# Step 1: Get current version from version_auto.h
$versionFile = "src/version_auto.h"
$content = Get-Content $versionFile -Raw
if ($content -match 'APP_VERSION = "(\d+)\.(\d+)\.(\d+)"') {
    $major = [int]$matches[1]
    $minor = [int]$matches[2]
    $patch = [int]$matches[3]
    $currentVersion = "$major.$minor.$patch"
    Write-Host "Current version: $currentVersion" -ForegroundColor White
} else {
    Write-Host "ERROR: Could not parse current version from $versionFile" -ForegroundColor Red
    exit 1
}

# Step 2: Bump version
switch ($VersionBump) {
    "major" { $major++; $minor = 0; $patch = 0 }
    "minor" { $minor++; $patch = 0 }
    "patch" { $patch++ }
    default { 
        Write-Host "ERROR: Invalid version bump type. Use: patch, minor, or major" -ForegroundColor Red
        exit 1
    }
}

$newVersion = "$major.$minor.$patch"
Write-Host "New version: $newVersion" -ForegroundColor Green

# Step 3: Update version_auto.h with proper formatting
$timestamp = (Get-Date).ToUniversalTime().ToString("yyyy-MM-ddTHH:mm:ss.ffffffZ")
$newVersionContent = @"
#pragma once
// Auto-generated on $timestamp
constexpr const char* APP_VERSION = "$newVersion";

"@

Set-Content -Path $versionFile -Value $newVersionContent -NoNewline -Encoding UTF8
Write-Host "Updated $versionFile" -ForegroundColor Green

# Step 4: Build firmware
if (-not $SkipBuild) {
    Write-Host "`nBuilding firmware..." -ForegroundColor Yellow
    $buildResult = pio run -e waveshare_7in 2>&1
    if ($LASTEXITCODE -ne 0) {
        Write-Host "Build failed!" -ForegroundColor Red
        Write-Host $buildResult
        exit 1
    }
    Write-Host "Build successful!" -ForegroundColor Green
}

# Step 5: Create OTA release directory
if (-not $SkipOTA -and -not $LocalOnly) {
    $releaseDir = "ota_functions/releases/$newVersion"
    New-Item -ItemType Directory -Path $releaseDir -Force | Out-Null
    Write-Host "Created release directory: $releaseDir" -ForegroundColor Green

    # Step 6: Copy firmware binary
    $firmwarePath = ".pio/build/waveshare_7in/firmware.bin"
    if (-not (Test-Path $firmwarePath)) {
        Write-Host "ERROR: Firmware binary not found at $firmwarePath" -ForegroundColor Red
        exit 1
    }

    Copy-Item $firmwarePath "$releaseDir/firmware.bin" -Force
    $firmwareFile = Get-Item "$releaseDir/firmware.bin"
    $md5Hash = (Get-FileHash -Algorithm MD5 -Path $firmwareFile.FullName).Hash.ToLower()
    $size = $firmwareFile.Length

    Write-Host "Firmware: $size bytes, MD5: $md5Hash" -ForegroundColor White

    # Step 7: Create properly formatted manifest.json (NO PowerShell ConvertTo-Json!)
    # This prevents the formatting issues that break the OTA server
    if ([string]::IsNullOrWhiteSpace($Changelog)) {
        $Changelog = "Version $newVersion update"
    }

    $manifestContent = @"
{
  "version": "$newVersion",
  "channel": "stable",
  "changelog": "$Changelog",
  "min_version": "1.0.0",
  "firmware": {
    "url": "https://image-optimizer-still-flower-1282.fly.dev/ota/releases/$newVersion/firmware.bin",
    "md5": "$md5Hash",
    "size": $size
  }
}
"@

    Set-Content -Path "$releaseDir/manifest.json" -Value $manifestContent -NoNewline -Encoding UTF8
    Write-Host "Created manifest.json" -ForegroundColor Green

    # Step 8: Update main manifest.json
    $mainManifestContent = @"
{
  "version": "$newVersion",
  "channel": "stable",
  "released_at": "$timestamp",
  "md5": "$md5Hash",
  "url": "https://image-optimizer-still-flower-1282.fly.dev/ota/releases/$newVersion/firmware.bin",
  "size": $size
}
"@

    Set-Content -Path "ota_functions/manifest.json" -Value $mainManifestContent -NoNewline -Encoding UTF8
    Write-Host "Updated main manifest.json" -ForegroundColor Green
}

# Step 9: Git commit and push
if (-not $SkipGit) {
    Write-Host "`nCommitting to git..." -ForegroundColor Yellow
    
    git add src/version_auto.h
    if (-not $SkipOTA -and -not $LocalOnly) {
        git add "ota_functions/releases/$newVersion/"
        git add "ota_functions/manifest.json"
    }
    
    $commitMessage = if ($Changelog) { "v$newVersion - $Changelog" } else { "v$newVersion" }
    git commit -m $commitMessage
    
    if ($LASTEXITCODE -eq 0) {
        Write-Host "Committed changes" -ForegroundColor Green
        
        if (-not $LocalOnly) {
            git push origin main
            if ($LASTEXITCODE -eq 0) {
                Write-Host "Pushed to GitHub" -ForegroundColor Green
            } else {
                Write-Host "WARNING: Git push failed" -ForegroundColor Yellow
            }
        }
    } else {
        Write-Host "Nothing to commit" -ForegroundColor Yellow
    }
}

# Step 10: Deploy to Fly.io
if (-not $SkipOTA -and -not $LocalOnly) {
    Write-Host "`nDeploying to Fly.io..." -ForegroundColor Yellow
    
    # Force cache bust
    Get-Date -Format "yyyyMMddHHmmss" | Out-File "ota_functions/.cachebust" -NoNewline -Encoding UTF8
    
    Push-Location ota_functions
    try {
        $deployResult = flyctl deploy --remote-only -a image-optimizer-still-flower-1282 2>&1
        if ($deployResult -match "deployed|Visit") {
            Write-Host "Deployed to Fly.io successfully!" -ForegroundColor Green
        } else {
            Write-Host "Deploy may have issues. Check output above." -ForegroundColor Yellow
        }
    } finally {
        Pop-Location
    }
}

# Step 11: Summary
Write-Host "`n=== Deployment Complete ===" -ForegroundColor Cyan
Write-Host "Version: $currentVersion → $newVersion" -ForegroundColor White
Write-Host "Changelog: $Changelog" -ForegroundColor White

if (-not $LocalOnly) {
    Write-Host "`nOTA Update:" -ForegroundColor Yellow
    Write-Host "  Manifest: https://image-optimizer-still-flower-1282.fly.dev/ota/manifest" -ForegroundColor White
    Write-Host "`nTo trigger update on device:" -ForegroundColor Yellow
    Write-Host "  Invoke-RestMethod -Uri 'http://192.168.7.116/api/ota/update' -Method POST" -ForegroundColor White
}

Write-Host "`n"
