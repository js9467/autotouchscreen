[CmdletBinding()]
param(
    [string]$PackagePath,
    [string]$Port,
    [switch]$ListPorts,
    [string]$EsptoolVersion = "v4.7.0"
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"
$RequiredArtifacts = @("bootloader.bin", "partitions.bin", "boot_app0.bin", "firmware.bin")

function Write-Info {
    param([string]$Message)
    Write-Host "[flash] $Message"
}

function Resolve-PackagePath {
    param([string]$InputPath)

    if ($InputPath) {
        return (Resolve-Path -LiteralPath $InputPath -ErrorAction Stop).Path
    }

    $embeddedArtifacts = $RequiredArtifacts |
        ForEach-Object { Join-Path $PSScriptRoot $_ }

    [array]$missingEmbedded = $embeddedArtifacts | Where-Object { -not (Test-Path $_) }
    if ($missingEmbedded.Count -eq 0) {
        return $PSScriptRoot
    }

    $localDist = Join-Path $PSScriptRoot "dist"
    $latestDir = Join-Path $localDist "latest"

    if (Test-Path $latestDir) {
        return (Resolve-Path -LiteralPath $latestDir).Path
    }

    if (Test-Path $localDist) {
        # Grab the highest-looking version folder that contains all required binaries.
        $candidates = Get-ChildItem -LiteralPath $localDist -Directory |
            Sort-Object Name -Descending

        foreach ($dir in $candidates) {
            $required = $RequiredArtifacts |
                ForEach-Object { Join-Path $dir.FullName $_ }

            [array]$missing = $required | Where-Object { -not (Test-Path $_) }
            if ($missing.Count -gt 0) {
                continue
            }

            return $dir.FullName
        }
    }

    throw "Could not locate firmware package. Provide -PackagePath or include dist/<version> with binaries."
}

function Get-SerialCandidates {
    try {
        return @(Get-CimInstance Win32_SerialPort |
            Select-Object DeviceID, Description, Manufacturer, PNPDeviceID |
            Sort-Object DeviceID)
    } catch {
        Write-Warning "Unable to enumerate serial ports: $_"
        return @()
    }
}

function Detect-Port {
    param([array]$Candidates)
    [array]$Candidates = $Candidates
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

$candidates = @(Get-SerialCandidates)
if ($ListPorts) {
    if ($candidates.Count -eq 0) {
        Write-Info "No serial ports found"
    } else {
        Write-Info "Available ports:"
        $candidates | Format-Table -AutoSize
    }
    return
}

$resolvedPackage = Resolve-PackagePath -InputPath $PackagePath
$artifactMap = @{}
foreach ($name in $RequiredArtifacts) {
    $artifactMap[$name] = Join-Path $resolvedPackage $name
    if (-not (Test-Path $artifactMap[$name])) {
        throw "Required artifact missing: $($artifactMap[$name])"
    }
}

$bootloader = $artifactMap["bootloader.bin"]
$partitions = $artifactMap["partitions.bin"]
$bootApp0 = $artifactMap["boot_app0.bin"]
$firmware = $artifactMap["firmware.bin"]

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
