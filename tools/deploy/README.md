# Bronco Controls Deployment Tools

This directory contains tools for flashing firmware to ESP32-S3-Box devices and managing firmware releases.

## Quick Start for End Users

### ⚡ **Easiest Method: One-Click Installer (RECOMMENDED)**

**Download and run - that's it!**

1. **Download the installer:** [BroncoFlasher-Installer.zip](https://github.com/js9467/autotouchscreen/raw/main/tools/deploy/BroncoFlasher-Installer.zip)

2. **Extract the ZIP** - you'll get one file: `BroncoFlasher.cmd`

3. **Double-click `BroncoFlasher.cmd`** to flash your device

The installer auto-downloads everything (latest firmware, bootloader files, esptool) and flashes your ESP32 device automatically.

**What it does:**
- Downloads latest firmware from OTA server
- Downloads bootloader files from GitHub  
- Auto-detects your ESP32 device
- Flashes everything automatically
- No PowerShell execution policy issues
- Just one double-click!

**Note:** Chrome/Edge may warn "This type of file can harm your computer" - click "Keep" to download. This is normal for .cmd files.

---

### 🔧 **Advanced: PowerShell Script**

For users who prefer PowerShell or need custom options:

**Download:** Right-click → Save Link As: [BroncoFlasher.ps1](https://github.com/js9467/autotouchscreen/raw/main/tools/deploy/BroncoFlasher.ps1)

```powershell
# List available COM ports
.\BroncoFlasher.ps1 -ListPorts

# Specify COM port manually
.\BroncoFlasher.ps1 -Port COM3

# Offline mode (use cached files)
.\BroncoFlasher.ps1 -OfflineMode
```

---

### 📦 **Alternative: BroncoFlasher.zip (Traditional Method)**

If you prefer offline flashing or the standalone script doesn't work:

1. **Download BroncoFlasher package:**
   ```
   https://github.com/js9467/autotouchscreen/raw/main/tools/deploy/BroncoFlasher.zip
   ```

2. **Extract the ZIP file** to a folder on your computer

3. **Connect your ESP32** device via USB

4. **Ensure internet connection** (script downloads latest firmware automatically)

5. **Double-click `flash_device.cmd`** to start the flashing process

The script will:
- **Auto-download the latest firmware** from the OTA server
- Auto-detect your ESP32 device
- Download esptool if needed
- Flash the firmware automatically
- Show progress and completion status

**Note:** The script always downloads the latest firmware version before flashing, ensuring you get the most up-to-date release even if the ZIP file is old.

### Advanced Options (BroncoFlasher.ps1)

```powershell
# Default: Auto-download latest and flash
.\BroncoFlasher.ps1

# List available COM ports
.\BroncoFlasher.ps1 -ListPorts

# Specify COM port manually
.\BroncoFlasher.ps1 -Port COM3

# Offline mode (use cached files only, no downloads)
.\BroncoFlasher.ps1 -OfflineMode

# Use custom OTA server
.\BroncoFlasher.ps1 -OtaServer "https://custom-server.com"
```

The standalone script caches downloaded files in `%LOCALAPPDATA%\BroncoControls\flash-temp\` for 7 days, so subsequent flashes are faster.

## For Developers

### Files in BroncoFlasher.zip

- **bootloader.bin** - ESP32-S3 bootloader (required)
- **partitions.bin** - Partition table (required)
- **boot_app0.bin** - Boot application (required)
- **firmware.bin** - Fallback firmware (used only if OTA download fails)
- **flash_device.ps1** - PowerShell flashing script (full features)
- **flash_device.cmd** - Simple double-click wrapper for Windows
- **flash_device.sh** - Bash script for Linux/Mac
- **firmware_version.txt** - Last bundled firmware version info

**Important:** The flash script automatically downloads the latest firmware from the OTA server before flashing. The firmware.bin in the ZIP is only used as a fallback if internet is unavailable.

### Update Firmware in BroncoFlasher

Use the `update_firmware.ps1` script to fetch the latest firmware from the OTA server:

```powershell
# Download latest firmware only
.\update_firmware.ps1

# Download and rebuild BroncoFlasher.zip
.\update_firmware.ps1 -RebuildZip

# Use a different OTA server
.\update_firmware.ps1 -OtaServer "https://your-server.com" -RebuildZip
```

This script:
1. Fetches the latest firmware manifest from the OTA server
2. Downloads the firmware binary
3. Verifies MD5 hash
4. Updates firmware.bin in the deploy directory
5. (Optional) Rebuilds BroncoFlasher.zip with the new firmware

### Flash Script Options
Default: Auto-download latest firmware and flash
.\flash_device.ps1

# List available COM ports
.\flash_device.ps1 -ListPorts

# Flash using a specific COM port
.\flash_device.ps1 -Port COM3

# Skip OTA download and use local firmware.bin only
.\flash_device.ps1 -SkipDownload

# Flash with a custom firmware package path
.\flash_device.ps1 -PackagePath "C:\path\to\firmware\folder" -SkipDownload

# Use a custom OTA server
.\flash_device.ps1
# Download from custom OTA server
.\flash_device.ps1 -DownloadLatest -OtaServer "https://custom-server.com"
```

### Build and Deploy Workflow

1. **Build firmware:**
   ```bash
   pio run
   ```

2. **Copy firmware to OTA releases:**
   ```powershell
   Copy-Item .pio\build\esp32s3box\firmware.bin ota_functions\releases\1.2.x\
   ```

3. **Create manifest.json** (see ota_functions/releases/README.md)

4. **Deploy to OTA server:**
   ```bash
   cd ota_functions
   flyctl deploy --remote-only
   ```

5. **Update BroncoFlasher package:**
   ```powershell
   .\tools\deploy\update_firmware.ps1 -RebuildZip
   ```

6. **Upload to GitHub:**
   ```bash
   git add tools/deploy/BroncoFlasher.zip
   git commit -m "Update BroncoFlasher to v1.2.x"
   git push
   ```

## OTA vs USB Flashing

### OTA (Over-The-Air) Updates
- **Use when:** Device is already running Bronco Controls firmware
- **Advantages:** Wireless, no USB cable needed, can update multiple devices remotely
- **How:** Device checks OTA server, downloads firmware, installs automatically
- **URL:** https://image-optimizer-still-flower-1282.fly.dev/ota/manifest

### USB Flashing
- **Use when:** 
  - First-time installation
  - OTA is not working
  - Device is bricked or won't boot
  - No WiFi available
- **Advantages:** Always works, doesn't require existing firmware
- **How:** Connect USB cable, run flash_device script

## Firmware Distribution Strategy

The firmware is distributed in multiple ways:

1. **Git Repository** - Source code and firmware binaries in `ota_functions/releases/`
2. **OTA Server** - Live firmware hosted on Fly.io for wireless updates
3. **BroncoFlasher.zip** - Downloadable package with offline flashing tools

This ensures users can always get firmware even if:
- OTA server is down → Use BroncoFlasher.zip from GitHub
- GitHub is inaccessible → Use OTA server directly
- No internet → Use firmware.bin already in git repository

## Troubleshooting

### "Could not auto-detect the ESP32-S3 serial port"
- Run `.\flash_device.ps1 -ListPorts` to see available ports
- Manually specify port: `.\flash_device.ps1 -Port COM3`
- Check USB cable and drivers

### "Required artifact missing: firmware.bin"
- Ensure BroncoFlasher.zip was extracted completely
- Or download latest: `.\flash_device.ps1 -DownloadLatest`

### "esptool reported exit code"
- Device might be in use by another program (close serial monitors)
- Try a different USB cable or port
- Press and hold BOOT button during flashing

## Advanced Usage

### Backup Device Configuration
```powershell
.\backup_device.ps1 -Port COM3 -OutputPath ".\backup"
```

### Flash Custom Partition Table
```powershell
.\flash_device.ps1 -PackagePath ".\custom_build"
```

## Links

- **OTA Server:** https://image-optimizer-still-flower-1282.fly.dev
- **GitHub Repository:** https://github.com/js9467/autotouchscreen
- **BroncoFlasher Download:** https://github.com/js9467/autotouchscreen/raw/main/tools/deploy/BroncoFlasher.zip
- **OTA Manifest:** https://image-optimizer-still-flower-1282.fly.dev/ota/manifest
