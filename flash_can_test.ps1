Write-Host "`n========================================" -ForegroundColor Cyan
Write-Host "   ESP32-S3 Bootloader Mode Flash" -ForegroundColor Cyan
Write-Host "========================================`n" -ForegroundColor Cyan

Write-Host "STEP 1: Put device in bootloader mode" -ForegroundColor Yellow
Write-Host "  1. Locate BOOT and RESET buttons on your board" -ForegroundColor White
Write-Host "  2. Press and HOLD the BOOT button" -ForegroundColor White
Write-Host "  3. While holding BOOT, press and release RESET" -ForegroundColor White
Write-Host "  4. Release the BOOT button" -ForegroundColor White
Write-Host "`nYour device is now in bootloader mode!`n" -ForegroundColor Green

Write-Host "Press ENTER to start flashing..." -ForegroundColor Yellow
Read-Host

Write-Host "`nSTEP 2: Flashing firmware..." -ForegroundColor Yellow
pio run --target upload --upload-port COM5 -e can_test

if ($LASTEXITCODE -eq 0) {
    Write-Host "`n========================================" -ForegroundColor Green
    Write-Host "   FLASH SUCCESSFUL!" -ForegroundColor Green
    Write-Host "========================================`n" -ForegroundColor Green
    
    Write-Host "Opening serial monitor in 3 seconds..." -ForegroundColor Cyan
    Start-Sleep 3
    
    Write-Host "`nSerial Monitor - Type 'help' for commands" -ForegroundColor Yellow
    Write-Host "Press Ctrl+C to exit`n" -ForegroundColor Gray
    
    pio device monitor --port COM5 --baud 115200
} else {
    Write-Host "`n========================================" -ForegroundColor Red
    Write-Host "   FLASH FAILED" -ForegroundColor Red
    Write-Host "========================================`n" -ForegroundColor Red
    Write-Host "Try again or check connections" -ForegroundColor Yellow
}
