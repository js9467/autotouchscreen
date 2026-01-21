<#
.SYNOPSIS
Bronco Controls - One-Click ESP32 Flasher

.DESCRIPTION
Downloads the latest firmware and flashes your ESP32-S3-Box device.
No extraction needed - just run this script!

.PARAMETER Port
Specify COM port manually (e.g., COM3). Auto-detects if not provided.

.PARAMETER ListPorts
List available serial ports and exit.

.PARAMETER OfflineMode
Use cached files instead of downloading fresh firmware.

.EXAMPLE
.\BroncoFlasher.ps1
Auto-detects device and flashes latest firmware

.EXAMPLE
.\BroncoFlasher.ps1 -Port COM3
Flash using specific COM port

.EXAMPLE
.\BroncoFlasher.ps1 -ListPorts
Show available serial ports
#>

[CmdletBinding()]
param(
    [string]$Port,
    [switch]$ListPorts,
    [switch]$OfflineMode,
    [string]$GitHubRepo = "js9467/autotouchscreen",
    [string]$GitHubBranch = "main",
    [string]$OtaServer = "https://image-optimizer-still-flower-1282.fly.dev",
    [string]$EsptoolVersion = "v4.7.0"
)

$ErrorActionPreference = "Stop"
$ProgressPreference = 'SilentlyContinue'  # Faster downloads

# Colors
function Write-Header { param([string]$Message) Write-Host "`n[BRONCO] $Message" -ForegroundColor Cyan }
function Write-Success { param([string]$Message) Write-Host "  [OK] $Message" -ForegroundColor Green }
function Write-Step { param([string]$Message) Write-Host "  --> $Message" -ForegroundColor Yellow }
function Write-ErrorMsg { param([string]$Message) Write-Host "  [ERROR] $Message" -ForegroundColor Red }

# Setup working directory
$WorkDir = Join-Path $env:LOCALAPPDATA "BroncoControls\flash-temp"
if (-not (Test-Path $WorkDir)) {
    New-Item -ItemType Directory -Path $WorkDir -Force | Out-Null
}

Write-Header "Bronco Controls ESP32 Flasher"
Write-Host "  Workspace: $WorkDir`n" -ForegroundColor DarkGray

# Detect serial ports
function Get-SerialPorts {
    try {
        return @(Get-CimInstance Win32_SerialPort |
            Select-Object DeviceID, Description |
            Sort-Object DeviceID)
    } catch {
        return @()
    }
}

function Detect-ESP32Port {
    param([array]$Ports)
    if ($Ports.Count -eq 0) { return $null }
    
    $patterns = @('CP210', 'Silicon Labs', 'USB Serial', 'USB JTAG', 'ESP32', 'CH910')
    foreach ($pattern in $patterns) {
        $match = $Ports | Where-Object { $_.Description -like "*$pattern*" }
        if ($match) { return $match[0].DeviceID }
    }
    return $Ports[0].DeviceID
}

$ports = @(Get-SerialPorts)
if ($ListPorts) {
    Write-Header "Available Serial Ports"
    if ($ports.Count -eq 0) {
        Write-Host "  No serial ports found`n"
    } else {
        $ports | Format-Table -AutoSize
    }
    exit 0
}

# Download esptool
function Get-Esptool {
    param([string]$Version)
    $esptoolDir = Join-Path $env:LOCALAPPDATA "BroncoControls\esptool-$Version"
    $esptoolExe = Join-Path $esptoolDir "esptool.exe"
    
    if (Test-Path $esptoolExe) {
        return $esptoolExe
    }
    
    # Check for any cached esptool version
    $cachedEsptool = Get-ChildItem -Path "$env:LOCALAPPDATA\BroncoControls" -Filter "esptool-*" -Directory -ErrorAction SilentlyContinue | 
        ForEach-Object { Get-ChildItem -Path $_.FullName -Filter "esptool.exe" -Recurse -ErrorAction SilentlyContinue } | 
        Select-Object -First 1
    
    Write-Step "Downloading esptool $Version..."
    $downloadUrl = "https://github.com/espressif/esptool/releases/download/$Version/esptool-$Version-win64.zip"
    $tempZip = Join-Path $env:TEMP "esptool-$([guid]::NewGuid()).zip"
    
    $maxRetries = 3
    for ($i = 1; $i -le $maxRetries; $i++) {
        try {
            if ($i -gt 1) {
                Write-Step "Retry $i of $maxRetries..."
                Start-Sleep -Seconds 2
            }
            
            Invoke-WebRequest -Uri $downloadUrl -OutFile $tempZip -UseBasicParsing -TimeoutSec 30
            
            if (Test-Path $esptoolDir) {
                Remove-Item $esptoolDir -Recurse -Force
            }
            New-Item -ItemType Directory -Path $esptoolDir -Force | Out-Null
            Expand-Archive -Path $tempZip -DestinationPath $esptoolDir -Force
            Remove-Item $tempZip -Force
            
            $tool = Get-ChildItem -Path $esptoolDir -Filter esptool.exe -Recurse | Select-Object -First 1
            if (-not $tool) {
                throw "esptool.exe not found after extraction"
            }
            
            Write-Success "esptool downloaded"
            return $tool.FullName
            
        } catch {
            if ($i -eq $maxRetries) {
                if ($cachedEsptool) {
                    Write-Step "Using cached esptool (download failed, but cache available)"
                    return $cachedEsptool.FullName
                }
                throw "Failed to download esptool after $maxRetries attempts and no cache available: $_"
            }
        }
    }
}

# Download static binaries from GitHub
function Get-StaticBinary {
    param([string]$Filename)
    
    $localPath = Join-Path $WorkDir $Filename
    $cacheAge = if (Test-Path $localPath) { (Get-Date) - (Get-Item $localPath).LastWriteTime } else { $null }
    
    # Use cached file if less than 7 days old or in offline mode
    if ($OfflineMode -and (Test-Path $localPath)) {
        Write-Step "Using cached $Filename"
        return $localPath
    }
    
    if ($cacheAge -and $cacheAge.TotalDays -lt 7) {
        Write-Step "Using cached $Filename ($(($cacheAge.TotalDays).ToString('0.0')) days old)"
        return $localPath
    }
    
    Write-Step "Downloading $Filename from GitHub..."
    $url = "https://raw.githubusercontent.com/$GitHubRepo/$GitHubBranch/tools/deploy/$Filename"
    
    try {
        Invoke-WebRequest -Uri $url -OutFile $localPath -UseBasicParsing
        Write-Success "$Filename downloaded"
        return $localPath
    } catch {
        if (Test-Path $localPath) {
            Write-Step "Using cached $Filename (download failed, but cache available)"
            return $localPath
        }
        throw "Failed to download $Filename and no cache available: $_"
    }
}

# Download latest firmware from OTA server
function Get-LatestFirmware {
    $firmwarePath = Join-Path $WorkDir "firmware.bin"
    $versionPath = Join-Path $WorkDir "firmware_version.txt"
    
    if ($OfflineMode -and (Test-Path $firmwarePath)) {
        Write-Step "Using cached firmware (offline mode)"
        return $firmwarePath
    }
    
    Write-Step "Fetching latest firmware from OTA server..."
    
    # Force download flag - delete cached firmware
    $forceDownload = $env:BRONCO_FORCE_DOWNLOAD -eq 'true'
    if ($forceDownload) {
        if (Test-Path $firmwarePath) {
            Remove-Item $firmwarePath -Force
            Write-Step "Cleared cached firmware (force download enabled)"
        }
        if (Test-Path $versionPath) {
            Remove-Item $versionPath -Force
        }
    }
    
    try {
        $manifest = Invoke-RestMethod -Uri "$OtaServer/ota/manifest" -Method Get
        $version = $manifest.version
        $firmwareUrl = $manifest.firmware.url
        $expectedMd5 = $manifest.firmware.md5.ToLower()
        
        # Check if we have the latest version cached
        $needsDownload = $true
        
        if ((Test-Path $versionPath)) {
            $cachedInfo = Get-Content $versionPath -Raw
            if ($cachedInfo -match "Firmware Version: (.+)") {
                $cachedVersion = $matches[1]
                if ($cachedVersion -eq $version -and (Test-Path $firmwarePath)) {
                    Write-Step "Latest firmware v$version already cached"
                    $needsDownload = $false
                }
            }
        }
        
        if ($needsDownload) {
            Write-Header "Latest Firmware: v$version"
            Write-Step "Downloading firmware.bin ($(($manifest.firmware.size / 1MB).ToString('0.0')) MB)..."
            
            Invoke-WebRequest -Uri $firmwareUrl -OutFile $firmwarePath -UseBasicParsing
            
            # Verify MD5
            $actualMd5 = (Get-FileHash -Path $firmwarePath -Algorithm MD5).Hash.ToLower()
            if ($actualMd5 -ne $expectedMd5) {
                throw "MD5 mismatch! Expected: $expectedMd5, Got: $actualMd5"
            }
            
            # Save version info
            @"
Firmware Version: $version
Downloaded: $(Get-Date -Format 'yyyy-MM-dd HH:mm:ss')
Source: $OtaServer
MD5: $expectedMd5
Size: $($manifest.firmware.size) bytes
"@ | Set-Content -Path $versionPath
            
            Write-Success "Firmware v$version downloaded and verified"
        }
        
        return $firmwarePath
        
    } catch {
        if (Test-Path $firmwarePath) {
            Write-Step "Using cached firmware (download failed, but cache available)"
            return $firmwarePath
        }
        throw "Failed to download firmware and no cache available: $_"
    }
}

# Main execution
try {
    Write-Header "Preparing Flash Files"
    
    # Download all required files
    $bootloader = Get-StaticBinary "bootloader.bin"
    $partitions = Get-StaticBinary "partitions.bin"
    $bootApp0 = Get-StaticBinary "boot_app0.bin"
    $firmware = Get-LatestFirmware
    $esptool = Get-Esptool -Version $EsptoolVersion
    
    Write-Header "Detecting ESP32 Device"
    
    if (-not $Port) {
        $Port = Detect-ESP32Port -Ports $ports
    }
    
    if (-not $Port) {
        # Check if ESP32 USB device is connected but drivers are missing
        $esp32Device = Get-PnpDevice | Where-Object { 
            ($_.FriendlyName -like '*USB JTAG*' -or $_.FriendlyName -like '*303A*' -or $_.FriendlyName -like '*Composite*') -and 
            $_.Status -eq 'Unknown' -and
            $_.InstanceId -like '*303A*'
        }
        
        if ($esp32Device) {
            Write-Header "ESP32 Device Detected - Installing Drivers"
            Write-Host "  ESP32-S3 found but needs USB drivers..." -ForegroundColor Yellow
            Write-Host "  Installing drivers automatically (requires admin permission)...`n" -ForegroundColor Yellow
            
            try {
                # Get the device instance ID
                $deviceId = $esp32Device | Select-Object -First 1 | Select-Object -ExpandProperty InstanceId
                Write-Step "Device ID: $($deviceId.Substring(0, [Math]::Min(50, $deviceId.Length)))..."
                
                # Download and prepare WinUSB driver
                $driverDir = Join-Path $WorkDir "esp32-driver"
                if (Test-Path $driverDir) {
                    Remove-Item $driverDir -Recurse -Force
                }
                New-Item -ItemType Directory -Path $driverDir -Force | Out-Null
                
                # Create INF file for WinUSB
                $infContent = @"
[Version]
Signature="`$Windows NT`$"
Class=USBDevice
ClassGUID={88BAE032-5A81-49f0-BC3D-A4FF138216D6}
Provider=Espressif
DriverVer=01/21/2026,1.0.0.0
CatalogFile=esp32s3.cat

[Manufacturer]
%ManufacturerName%=Standard,NTamd64

[Standard.NTamd64]
%DeviceName%=USB_Install, USB\VID_303A&PID_1001
%DeviceName2%=USB_Install, USB\VID_303A&PID_1001&MI_02

[USB_Install]
Include=winusb.inf
Needs=WINUSB.NT

[USB_Install.Services]
Include=winusb.inf
Needs=WINUSB.NT.Services

[USB_Install.HW]
AddReg=Dev_AddReg

[Dev_AddReg]
HKR,,DeviceInterfaceGUIDs,0x00010000,"{88BAE032-5A81-49f0-BC3D-A4FF138216D6}"

[Strings]
ManufacturerName="Espressif Systems"
DeviceName="ESP32-S3 USB JTAG/Serial"
DeviceName2="ESP32-S3 USB JTAG/Serial Debug Unit"
"@
                $infPath = Join-Path $driverDir "esp32s3.inf"
                $infContent | Set-Content -Path $infPath -Encoding ASCII
                
                Write-Step "Installing WinUSB driver..."
                Write-Host "  Click 'Yes' when Windows asks for permission`n" -ForegroundColor Cyan
                
                # Install driver using pnputil
                $installResult = Start-Process "pnputil.exe" -ArgumentList "/add-driver `"$infPath`" /install" -Wait -PassThru -Verb RunAs
                
                if ($installResult.ExitCode -eq 0) {
                    Write-Success "Driver installed successfully!"
                    
                    # Try to apply driver to the device
                    Write-Step "Applying driver to device..."
                    try {
                        # Use PowerShell to trigger device refresh
                        $null = Start-Process "pnputil.exe" -ArgumentList "/scan-devices" -Wait -PassThru -NoNewWindow
                    } catch {
                        Write-Host "  Device scan completed" -ForegroundColor DarkGray
                    }
                    
                } else {
                    Write-Warning "pnputil returned code: $($installResult.ExitCode)"
                }
                
                Write-Host "`n  Please UNPLUG and REPLUG your ESP32 device now" -ForegroundColor Yellow
                Write-Host "  (This activates the new driver)`n" -ForegroundColor DarkGray
                Write-Host "  Press any key after replugging..." -ForegroundColor Cyan
                $null = $Host.UI.RawUI.ReadKey('NoEcho,IncludeKeyDown')
                
                Write-Step "Detecting device..."
                Start-Sleep -Seconds 3
                
                # Refresh port list
                $ports = @(Get-SerialPorts)
                $Port = Detect-ESP32Port -Ports $ports
                
                if ($Port) {
                    Write-Success "ESP32 detected on $Port!"
                } else {
                    Write-Warning "Still cannot detect COM port after first replug"
                    Write-Host "`n  Sometimes Windows needs the device replugged twice." -ForegroundColor Yellow
                    Write-Host "  Please unplug and replug ONE MORE TIME...`n" -ForegroundColor Yellow
                    Write-Host "  Press any key after replugging..." -ForegroundColor Cyan
                    $null = $Host.UI.RawUI.ReadKey('NoEcho,IncludeKeyDown')
                    Start-Sleep -Seconds 3
                    $ports = @(Get-SerialPorts)
                    $Port = Detect-ESP32Port -Ports $ports
                    
                    if ($Port) {
                        Write-Success "ESP32 detected on $Port!"
                    }
                }
                
            } catch {
                Write-Warning "Automatic driver installation failed: $_"
                Write-Host "`nTrying alternative driver installation method...`n" -ForegroundColor Yellow
                
                # Fallback: Download official Espressif drivers
                try {
                    Write-Step "Downloading official ESP32 drivers..."
                    $espDriverUrl = "https://dl.espressif.com/dl/idf-driver/idf-driver-esp32-usb-jtag-2021-07-15.zip"
                    $driverZip = Join-Path $WorkDir "esp-driver.zip"
                    
                    Invoke-WebRequest -Uri $espDriverUrl -OutFile $driverZip -UseBasicParsing
                    
                    $driverExtractDir = Join-Path $WorkDir "esp-driver-extracted"
                    if (Test-Path $driverExtractDir) {
                        Remove-Item $driverExtractDir -Recurse -Force
                    }
                    Expand-Archive -Path $driverZip -DestinationPath $driverExtractDir -Force
                    
                    $driverInstaller = Get-ChildItem -Path $driverExtractDir -Filter "*.exe" -Recurse | Select-Object -First 1
                    
                    if ($driverInstaller) {
                        Write-Step "Running official driver installer..."
                        Write-Host "  Click through the installer prompts`n" -ForegroundColor Cyan
                        Start-Process -FilePath $driverInstaller.FullName -Wait -Verb RunAs
                        
                        Write-Host "`n  Please UNPLUG and REPLUG your ESP32 device" -ForegroundColor Yellow
                        Write-Host "  Press any key after replugging..." -ForegroundColor Cyan
                        $null = $Host.UI.RawUI.ReadKey('NoEcho,IncludeKeyDown')
                        Start-Sleep -Seconds 3
                        $ports = @(Get-SerialPorts)
                        $Port = Detect-ESP32Port -Ports $ports
                    }
                } catch {
                    Write-ErrorMsg "All automatic installation methods failed"
                    Write-Host "`nPlease install drivers manually:" -ForegroundColor Yellow
                    Write-Host "  1. Go to Device Manager (Win+X, then M)" -ForegroundColor Cyan
                    Write-Host "  2. Find device with yellow warning icon" -ForegroundColor Cyan
                    Write-Host "  3. Right-click -> Update Driver -> Browse -> Let me pick" -ForegroundColor Cyan
                    Write-Host "  4. Choose 'USB Serial Device' or 'Ports (COM & LPT)'`n" -ForegroundColor Cyan
                }
            }
        }
        
        # Final check
        if (-not $Port) {
            Write-ErrorMsg "Could not auto-detect ESP32 COM port"
            
            # Check if any USB device with ESP32 vendor ID is present
            $anyESP32Device = Get-PnpDevice | Where-Object { 
                $_.InstanceId -like '*303A*'
            }
            
            if ($anyESP32Device) {
                Write-Host "`nESP32 device detected but no COM port found." -ForegroundColor Yellow
                Write-Host "This usually means the USB driver is not installed correctly.`n" -ForegroundColor Yellow
                
                Write-Host "Quick fix options:" -ForegroundColor Cyan
                Write-Host "  1. Unplug and replug the device" -ForegroundColor White
                Write-Host "  2. Try a different USB port (USB 2.0 recommended)" -ForegroundColor White
                Write-Host "  3. Try a different USB cable" -ForegroundColor White
                Write-Host "`nAfter trying above, press R to retry detection" -ForegroundColor Yellow
                Write-Host "Or press any other key to exit`n" -ForegroundColor Yellow
                
                $key = $Host.UI.RawUI.ReadKey('NoEcho,IncludeKeyDown')
                if ($key.Character -eq 'r' -or $key.Character -eq 'R') {
                    Write-Step "Retrying detection..."
                    Start-Sleep -Seconds 2
                    $ports = @(Get-SerialPorts)
                    $Port = Detect-ESP32Port -Ports $ports
                    
                    if ($Port) {
                        Write-Success "ESP32 detected on $Port!"
                    } else {
                        Write-Host "`nStill cannot detect. Please run Install-Drivers.bat first.`n" -ForegroundColor Red
                        exit 1
                    }
                } else {
                    exit 1
                }
            } else {
                Write-Host "`nAvailable ports:" -ForegroundColor Yellow
                if ($ports.Count -eq 0) {
                    Write-Host "  No serial ports found" -ForegroundColor Red
                    Write-Host "`nTroubleshooting:" -ForegroundColor Yellow
                    Write-Host "  1. Make sure ESP32 is connected via USB"
                    Write-Host "  2. Unplug and replug the device"
                    Write-Host "  3. Try a different USB cable or port"
                    Write-Host "  4. Run Install-Drivers.bat to install USB drivers`n"
                } else {
                    $ports | Format-Table -AutoSize
                    Write-Host "Run with -Port COMx to specify manually`n" -ForegroundColor Yellow
                }
                exit 1
            }
        }
    }
    
    Write-Success "Using port: $Port"
    
    Write-Header "Erasing Flash"
    Write-Host "  This ensures a clean installation...`n" -ForegroundColor DarkGray
    
    $eraseArgs = @(
        "--chip", "esp32s3",
        "--port", $Port,
        "--baud", "921600",
        "erase_flash"
    )
    
    & $esptool @eraseArgs
    
    if ($LASTEXITCODE -ne 0) {
        throw "Flash erase failed with exit code $LASTEXITCODE"
    }
    
    Write-Header "Flashing ESP32"
    Write-Host "  This may take 30-60 seconds...`n" -ForegroundColor DarkGray
    
    $arguments = @(
        "--chip", "esp32s3",
        "--port", $Port,
        "--baud", "921600",
        "--before", "default_reset",
        "--after", "hard_reset",
        "write_flash",
        "--flash_mode", "qio",
        "--flash_size", "16MB",
        "0x0", $bootloader,
        "0x8000", $partitions,
        "0xe000", $bootApp0,
        "0x10000", $firmware
    )
    
    & $esptool @arguments
    
    if ($LASTEXITCODE -ne 0) {
        throw "esptool reported exit code $LASTEXITCODE"
    }
    
    Write-Host ""
    Write-Success "Flash complete!"
    
    # Monitor serial for IP address and open browser
    Write-Header "Monitoring Device Startup"
    Write-Step "Waiting for device to boot and connect to WiFi..."
    Write-Host "  (This will timeout after 30 seconds if no WiFi is configured)`n" -ForegroundColor DarkGray
    
    try {
        Start-Sleep -Seconds 2  # Wait for device to reset
        
        $serialPort = New-Object System.IO.Ports.SerialPort $Port, 115200
        $serialPort.Open()
        
        $timeout = 30
        $startTime = Get-Date
        $ipAddress = $null
        
        while (((Get-Date) - $startTime).TotalSeconds -lt $timeout) {
            if ($serialPort.BytesToRead -gt 0) {
                $line = $serialPort.ReadLine()
                Write-Host "  $line" -ForegroundColor DarkGray
                
                # Look for station IP (WiFi connected)
                if ($line -match 'Station connected:\s*(\d{1,3}\.\d{1,3}\.\d{1,3}\.\d{1,3})') {
                    $ipAddress = $matches[1]
                    break
                }
                # Fallback to AP IP if no station connection
                if (!$ipAddress -and $line -match 'AP ready at\s*(\d{1,3}\.\d{1,3}\.\d{1,3}\.\d{1,3})') {
                    $ipAddress = $matches[1]
                }
            }
            Start-Sleep -Milliseconds 100
        }
        
        $serialPort.Close()
        
        if ($ipAddress) {
            Write-Success "Device connected! IP: $ipAddress"
            Write-Host "`n  Opening web interface in browser..." -ForegroundColor Green
            Start-Process "http://$ipAddress"
            Write-Host "`n  Web interface: http://$ipAddress" -ForegroundColor Cyan
        } else {
            Write-Host "`n  No WiFi connection detected (timeout)" -ForegroundColor Yellow
            Write-Host "  Configure WiFi on device, then access web interface at device IP`n" -ForegroundColor Yellow
        }
        
    } catch {
        Write-Host "`n  Could not monitor serial output: $_" -ForegroundColor Yellow
        Write-Host "  Device is ready - configure WiFi to access web interface`n" -ForegroundColor Yellow
    }
    
    Write-Header "Setup Complete!"
    Write-Host "  You may now disconnect the USB cable.`n" -ForegroundColor Green
    
} catch {
    Write-Host ""
    Write-ErrorMsg "Flash failed: $_"
    Write-Host "`nTroubleshooting:" -ForegroundColor Yellow
    Write-Host "  1. Close any programs using the COM port (Arduino IDE, PuTTY, etc.)"
    Write-Host "  2. Try a different USB cable or port"
    Write-Host "  3. Press and hold BOOT button during flashing"
    Write-Host "  4. Run with -Port COM3 to specify port manually`n"
    exit 1
}
