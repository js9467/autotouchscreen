# Deploy v1.3.69 with CAN receive capability
Write-Host "`n=== DELETING OLD VERSION TO PREVENT LOOP ===" -ForegroundColor Red
Remove-Item "ota_functions/releases/1.3.68" -Recurse -Force -ErrorAction SilentlyContinue
Write-Host "Deleted v1.3.68" -ForegroundColor Yellow

Write-Host "`n=== BUILDING v1.3.69 ===" -ForegroundColor Cyan

# Set version
@"
#pragma once
// Auto-generated
constexpr const char* APP_VERSION = "1.3.69";
"@ | Out-File "src/version_auto.h" -Encoding utf8NoBOM

# Build
Write-Host "Building..." -ForegroundColor Yellow
pio run -e waveshare_7in

if ($LASTEXITCODE -ne 0) {
    Write-Host "`nBUILD FAILED!" -ForegroundColor Red
    exit 1
}

# Get firmware file
$firmwareFile = Get-Item ".pio/build/waveshare_7in/firmware.bin"
$size = $firmwareFile.Length
$md5 = (Get-FileHash -Algorithm MD5 -Path $firmwareFile.FullName).Hash.ToLower()

Write-Host "`nBuild SUCCESS!" -ForegroundColor Green
Write-Host "Size: $size bytes" -ForegroundColor Gray
Write-Host "MD5: $md5" -ForegroundColor Gray

# Create release directory
New-Item -ItemType Directory -Path "ota_functions/releases/1.3.69" -Force | Out-Null

# Copy firmware
Copy-Item $firmwareFile.FullName "ota_functions/releases/1.3.69/firmware.bin" -Force

# Create manifest
@"
{
  "version": "1.3.69",
  "firmware": {
    "url": "firmware.bin",
    "size": $size,
    "md5": "$md5"
  },
  "releaseNotes": "CAN receive capability - receiveMessage() and /api/can/receive endpoint"
}
"@ | Out-File "ota_functions/releases/1.3.69/manifest.json" -Encoding utf8NoBOM

Write-Host "`n=== DEPLOYING TO FLY.IO ===" -ForegroundColor Cyan
Set-Location ota_functions
flyctl deploy --remote-only -a image-optimizer-still-flower-1282

Write-Host "`n=== DONE ===" -ForegroundColor Green
Write-Host "Device will auto-update to v1.3.69 within ~1 minute" -ForegroundColor Yellow
Write-Host "`nTest CAN receive with:" -ForegroundColor Cyan
Write-Host '  Invoke-RestMethod -Uri "http://192.168.7.116/api/can/receive?timeout=1000"' -ForegroundColor Gray
