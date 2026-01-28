$firmwarePath = ".pio/build/waveshare_7in/firmware.bin"
$uri = "http://192.168.7.116/update"

Write-Host "`n=== Web OTA Upload ===" -ForegroundColor Yellow
Write-Host "Firmware: $firmwarePath" -ForegroundColor Cyan
Write-Host "Target: $uri`n" -ForegroundColor Cyan

if (!(Test-Path $firmwarePath)) {
    Write-Host "ERROR: Firmware file not found!" -ForegroundColor Red
    exit 1
}

$fileInfo = Get-Item $firmwarePath
Write-Host "File size: $($fileInfo.Length) bytes" -ForegroundColor White

Write-Host "`nUploading..." -ForegroundColor Green
try {
    $fileBytes = [System.IO.File]::ReadAllBytes((Resolve-Path $firmwarePath).Path)
    $response = Invoke-WebRequest -Uri $uri -Method Post -Body $fileBytes -ContentType "application/octet-stream" -TimeoutSec 120
    Write-Host "Upload complete: $($response.StatusCode) - $($response.StatusDescription)" -ForegroundColor Green
    Write-Host "`nWaiting 15 seconds for device to restart..." -ForegroundColor Yellow
    Start-Sleep 15
    
    Write-Host "`nChecking new version..." -ForegroundColor Cyan
    $status = Invoke-RestMethod -Uri "http://192.168.7.116/api/status" -TimeoutSec 5
    Write-Host "Current version: $($status.firmware_version)" -ForegroundColor Green
    Write-Host "Uptime: $($status.uptime_ms) ms`n" -ForegroundColor White
} catch {
    Write-Host "Upload failed: $_" -ForegroundColor Red
    exit 1
}
