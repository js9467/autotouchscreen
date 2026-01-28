# POWERCELL NGX CAN Test - Quick Deploy Script
# This script builds and uploads the minimal CAN test firmware

Write-Host "`n=== POWERCELL NGX CAN Communication Test ===" -ForegroundColor Cyan
Write-Host "Building minimal firmware for testing..." -ForegroundColor Yellow
Write-Host ""

# Clean build to ensure fresh compilation
Write-Host "Step 1: Cleaning previous build..." -ForegroundColor Green
if (Test-Path ".pio\build\powercell_test") {
    Remove-Item -Recurse -Force ".pio\build\powercell_test"
    Write-Host "  OK Build directory cleaned" -ForegroundColor Gray
}

# Build the firmware
Write-Host "`nStep 2: Building firmware..." -ForegroundColor Green
$buildResult = pio run -c platformio_test.ini -e powercell_test 2>&1

if ($LASTEXITCODE -ne 0) {
    Write-Host "`nX Build failed!" -ForegroundColor Red
    Write-Host $buildResult
    exit 1
}

Write-Host "  OK Build successful!" -ForegroundColor Gray

# Find the COM port
Write-Host "`nStep 3: Looking for ESP32 device..." -ForegroundColor Green
$ports = Get-CimInstance -ClassName Win32_PnPEntity | Where-Object { 
    $_.Caption -match "USB-SERIAL|CP210|CH340|USB Serial|UART" 
}

if ($ports) {
    Write-Host "  Available serial ports:" -ForegroundColor Gray
    foreach ($port in $ports) {
        if ($port.Caption -match "COM(\d+)") {
            $comPort = $matches[0]
            Write-Host "    - $comPort : $($port.Caption)" -ForegroundColor Gray
        }
    }
} else {
    Write-Host "  No serial ports detected" -ForegroundColor Yellow
}

# Prompt for COM port
$port = Read-Host "`nEnter COM port (e.g., COM5)"
if (-not $port) {
    $port = "COM5"
    Write-Host "  Using default: $port" -ForegroundColor Gray
}

# Upload firmware
Write-Host "`nStep 4: Uploading to $port..." -ForegroundColor Green
Write-Host "  If upload fails, hold BOOT button and press RESET" -ForegroundColor Yellow

$uploadResult = pio run -c platformio_test.ini -e powercell_test --target upload --upload-port $port 2>&1

if ($LASTEXITCODE -eq 0) {
    Write-Host "`nOK Upload successful!" -ForegroundColor Green
    
    # Open serial monitor
    Write-Host "`nStep 5: Opening serial monitor..." -ForegroundColor Green
    Write-Host "`nCommands available:" -ForegroundColor Cyan
    Write-Host "  P - Send poll request to POWERCELL" -ForegroundColor White
    Write-Host "  C - Send configuration to POWERCELL" -ForegroundColor White
    Write-Host "  H - Show help" -ForegroundColor White
    Write-Host "`nPress Ctrl+C to exit monitor`n" -ForegroundColor Yellow
    
    Start-Sleep -Seconds 2
    pio device monitor --port $port --baud 115200 --echo --eol LF
    
} else {
    Write-Host "`nX Upload failed!" -ForegroundColor Red
    Write-Host $uploadResult
    
    Write-Host "`nTroubleshooting:" -ForegroundColor Yellow
    Write-Host "  1. Hold BOOT button on device" -ForegroundColor White
    Write-Host "  2. While holding BOOT, press RESET" -ForegroundColor White
    Write-Host "  3. Release BOOT" -ForegroundColor White
    Write-Host "  4. Run this script again" -ForegroundColor White
    exit 1
}
