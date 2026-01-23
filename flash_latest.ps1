# Bronco Controls ESP32 Flash Installer
# Downloads latest firmware and flashes to device

param(
    [string]$Port = "",
    [switch]$EraseFirst = $false,
    [switch]$BuildLocal = $false
)

Write-Host "==================================" -ForegroundColor Cyan
Write-Host "Bronco Controls Flash Installer" -ForegroundColor Cyan
Write-Host "==================================" -ForegroundColor Cyan
Write-Host ""

# Configuration
$OTA_BASE_URL = "https://bronco-ota.fly.dev"
$LOCAL_FIRMWARE_PATH = ".\.pio\build\waveshare_7in\firmware.bin"
$TEMP_DIR = ".\temp_flash"

# Function to detect COM port
function Get-ESP32Port {
    Write-Host "Detecting ESP32 device..." -ForegroundColor Yellow
    $ports = [System.IO.Ports.SerialPort]::getportnames()
    if ($ports.Count -eq 0) {
        Write-Host "ERROR: No COM ports detected!" -ForegroundColor Red
        Write-Host "Please ensure:" -ForegroundColor Yellow
        Write-Host "  1. ESP32 is connected via USB" -ForegroundColor Yellow
        Write-Host "  2. CH343/CH340 drivers are installed" -ForegroundColor Yellow
        Write-Host "  3. Device shows up in Device Manager" -ForegroundColor Yellow
        return $null
    }
    
    if ($ports.Count -eq 1) {
        Write-Host "Found device on $($ports[0])" -ForegroundColor Green
        return $ports[0]
    }
    
    Write-Host "Multiple COM ports detected:" -ForegroundColor Yellow
    for ($i = 0; $i -lt $ports.Count; $i++) {
        Write-Host "  [$i] $($ports[$i])" -ForegroundColor White
    }
    
    $selection = Read-Host "Select port number (0-$($ports.Count - 1))"
    return $ports[[int]$selection]
}

# Function to get latest version info
function Get-LatestVersion {
    try {
        Write-Host "Checking latest firmware version..." -ForegroundColor Yellow
        $manifest = Invoke-RestMethod -Uri "$OTA_BASE_URL/manifest.json" -TimeoutSec 10
        Write-Host "Latest version: $($manifest.version)" -ForegroundColor Green
        return $manifest
    } catch {
        Write-Host "WARNING: Could not reach OTA server: $_" -ForegroundColor Yellow
        return $null
    }
}

# Function to download firmware
function Download-Firmware {
    param([string]$Version)
    
    Write-Host "Downloading firmware version $Version..." -ForegroundColor Yellow
    
    New-Item -ItemType Directory -Force -Path $TEMP_DIR | Out-Null
    $firmwarePath = "$TEMP_DIR\firmware_$Version.bin"
    
    try {
        Invoke-WebRequest -Uri "$OTA_BASE_URL/firmware/firmware.bin" -OutFile $firmwarePath -TimeoutSec 30
        Write-Host "Download complete: $firmwarePath" -ForegroundColor Green
        return $firmwarePath
    } catch {
        Write-Host "ERROR: Download failed: $_" -ForegroundColor Red
        return $null
    }
}

# Function to build local firmware
function Build-LocalFirmware {
    Write-Host "Building local firmware..." -ForegroundColor Yellow
    
    $buildResult = & pio run -e waveshare_7in 2>&1
    
    if ($LASTEXITCODE -eq 0 -and (Test-Path $LOCAL_FIRMWARE_PATH)) {
        Write-Host "Build successful!" -ForegroundColor Green
        return $LOCAL_FIRMWARE_PATH
    } else {
        Write-Host "ERROR: Build failed!" -ForegroundColor Red
        return $null
    }
}

# Function to erase flash
function Erase-Flash {
    param([string]$ComPort)
    
    Write-Host ""
    Write-Host "ERASING DEVICE FLASH..." -ForegroundColor Red
    Write-Host "This will completely wipe the device." -ForegroundColor Yellow
    
    Write-Host ""
    Write-Host "Put device in BOOT MODE:" -ForegroundColor Cyan
    Write-Host "  1. Hold BOOT button" -ForegroundColor White
    Write-Host "  2. Press and release RESET button" -ForegroundColor White
    Write-Host "  3. Release BOOT button" -ForegroundColor White
    Write-Host ""
    Read-Host "Press ENTER when device is in boot mode"
    
    & pio run -e waveshare_7in -t erase --upload-port $ComPort
    
    if ($LASTEXITCODE -eq 0) {
        Write-Host "Flash erased successfully!" -ForegroundColor Green
        return $true
    } else {
        Write-Host "WARNING: Erase may have failed" -ForegroundColor Yellow
        return $false
    }
}

# Function to flash firmware
function Flash-Firmware {
    param(
        [string]$FirmwarePath,
        [string]$ComPort
    )
    
    Write-Host ""
    Write-Host "FLASHING FIRMWARE..." -ForegroundColor Cyan
    Write-Host "Firmware: $FirmwarePath" -ForegroundColor White
    Write-Host "Port: $ComPort" -ForegroundColor White
    Write-Host ""
    
    Write-Host "Put device in BOOT MODE if auto-reset fails:" -ForegroundColor Yellow
    Write-Host "  1. Hold BOOT button" -ForegroundColor White
    Write-Host "  2. Press and release RESET button" -ForegroundColor White
    Write-Host "  3. Release BOOT button" -ForegroundColor White
    Write-Host ""
    Read-Host "Press ENTER to start flashing"
    
    & pio run -e waveshare_7in -t upload --upload-port $ComPort
    
    if ($LASTEXITCODE -eq 0) {
        Write-Host ""
        Write-Host "FLASH SUCCESSFUL!" -ForegroundColor Green
        Write-Host ""
        Write-Host "The device should now restart automatically." -ForegroundColor Green
        Write-Host "If the screen is blank, press the RESET button." -ForegroundColor Yellow
        return $true
    } else {
        Write-Host ""
        Write-Host "ERROR: Flash failed!" -ForegroundColor Red
        return $false
    }
}

# Main execution
Write-Host "Starting flash process..." -ForegroundColor White
Write-Host ""

# Detect or use specified port
if ($Port -eq "") {
    $Port = Get-ESP32Port
    if ($Port -eq $null) {
        exit 1
    }
} else {
    Write-Host "Using specified port: $Port" -ForegroundColor Green
}

# Erase flash if requested
if ($EraseFirst) {
    $eraseSuccess = Erase-Flash -ComPort $Port
    if (-not $eraseSuccess) {
        $continue = Read-Host "Erase may have failed. Continue anyway? (y/n)"
        if ($continue -ne "y") {
            exit 1
        }
    }
    Write-Host ""
    Write-Host "Waiting 3 seconds..." -ForegroundColor Yellow
    Start-Sleep -Seconds 3
}

# Get firmware
$firmwarePath = $null

if ($BuildLocal) {
    # Build locally
    $firmwarePath = Build-LocalFirmware
} else {
    # Try to download latest
    $manifest = Get-LatestVersion
    if ($manifest -ne $null) {
        $firmwarePath = Download-Firmware -Version $manifest.version
    }
    
    # Fall back to local build if download fails
    if ($firmwarePath -eq $null) {
        Write-Host ""
        Write-Host "Download failed. Building locally instead..." -ForegroundColor Yellow
        $firmwarePath = Build-LocalFirmware
    }
}

if ($firmwarePath -eq $null) {
    Write-Host ""
    Write-Host "ERROR: Could not obtain firmware!" -ForegroundColor Red
    exit 1
}

# Flash the firmware
$flashSuccess = Flash-Firmware -FirmwarePath $firmwarePath -ComPort $Port

# Cleanup
if (Test-Path $TEMP_DIR) {
    Write-Host ""
    Write-Host "Cleaning up temporary files..." -ForegroundColor Yellow
    Remove-Item -Recurse -Force $TEMP_DIR
}

# Final status
Write-Host ""
Write-Host "==================================" -ForegroundColor Cyan
if ($flashSuccess) {
    Write-Host "INSTALLATION COMPLETE!" -ForegroundColor Green
    Write-Host ""
    Write-Host "Your Bronco Controls device is ready!" -ForegroundColor Green
    exit 0
} else {
    Write-Host "INSTALLATION FAILED" -ForegroundColor Red
    Write-Host ""
    Write-Host "Please check the errors above." -ForegroundColor Yellow
    exit 1
}
