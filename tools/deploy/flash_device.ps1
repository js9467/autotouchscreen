[CmdletBinding()]
param(
    [string]$PackagePath = "$PSScriptRoot/../../dist/latest",
    [string]$Port,
    [switch]$ListPorts,
    [string]$EsptoolVersion = "v4.7.0"
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

function Write-Info {
    param([string]$Message)
    Write-Host "[flash] $Message"
}

function Get-SerialCandidates {
    try {
        return Get-CimInstance Win32_SerialPort |
            Select-Object DeviceID, Description, Manufacturer, PNPDeviceID |
            Sort-Object DeviceID
    } catch {
        Write-Warning "Unable to enumerate serial ports: $_"
        return @()
    }
}

function Detect-Port {
    param([array]$Candidates)
    if (-not $Candidates -or $Candidates.Count -eq 0) {
        return $null
    }

    $patterns = @('CP210', 'Silicon Labs', 'USB Serial', 'USB JTAG', 'ESP32', 'CH910')
    foreach ($pattern in $patterns) {
        $match = $Candidates | Where-Object { $_.Description -like "*$pattern*" }
        if ($match) {
            return $match[0].DeviceID
        }
    }

    return $Candidates[0].DeviceID
}

function Ensure-Esptool {
    param([string]$Version)
    $root = Join-Path $env:LOCALAPPDATA "BroncoControls\esptool-$Version"
    $exe = Join-Path $root "esptool.exe"

    if (Test-Path $exe) {
        return $exe
    }

    Write-Info "Downloading esptool $Version"
    $downloadUrl = "https://github.com/espressif/esptool/releases/download/$Version/esptool-$Version-win64.zip"
    $tempZip = Join-Path ([System.IO.Path]::GetTempPath()) ("esptool-" + [guid]::NewGuid() + ".zip")
    Invoke-WebRequest -Uri $downloadUrl -OutFile $tempZip

    if (Test-Path $root) {
        Remove-Item $root -Recurse -Force
    }
    New-Item -ItemType Directory -Path $root | Out-Null
    Expand-Archive -LiteralPath $tempZip -DestinationPath $root
    Remove-Item $tempZip -Force

    $tool = Get-ChildItem -Path $root -Filter esptool.exe -Recurse | Select-Object -First 1
    if (-not $tool) {
        throw "esptool.exe not found after extraction"
    }

    return $tool.FullName
}

$candidates = Get-SerialCandidates
if ($ListPorts) {
    if ($candidates.Count -eq 0) {
        Write-Info "No serial ports found"
    } else {
        Write-Info "Available ports:"
        $candidates | Format-Table -AutoSize
    }
    return
}

if (-not (Test-Path $PackagePath)) {
    throw "Package directory '$PackagePath' does not exist"
}

$resolvedPackage = (Resolve-Path $PackagePath).Path
$bootloader = Join-Path $resolvedPackage "bootloader.bin"
$partitions = Join-Path $resolvedPackage "partitions.bin"
$bootApp0 = Join-Path $resolvedPackage "boot_app0.bin"
$firmware = Join-Path $resolvedPackage "firmware.bin"

$required = @($bootloader, $partitions, $bootApp0, $firmware)
foreach ($file in $required) {
    if (-not (Test-Path $file)) {
        throw "Required artifact missing: $file"
    }
}

if (-not $Port) {
    $Port = Detect-Port -Candidates $candidates
}

if (-not $Port) {
    throw "Could not auto-detect the ESP32-S3 serial port. Re-run with -Port COMx"
}

Write-Info "Using port $Port"
$esptool = Ensure-Esptool -Version $EsptoolVersion

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

Write-Info "Flashing firmware from $resolvedPackage"
& $esptool @arguments

if ($LASTEXITCODE -ne 0) {
    throw "esptool reported exit code $LASTEXITCODE"
}

Write-Info "Flash complete"
