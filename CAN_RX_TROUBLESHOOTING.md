# CAN RX Troubleshooting - Complete Guide

## Problem Summary

**The Device:** Waveshare ESP32-S3-7" with TWAI (CAN) controller
**The Issue:** CAN RX not working - receiving 0 frames from InfinityBox POWERCELL NGX
**Current Status:** Device online at 192.168.7.116, running firmware 1.3.78
**Confirmed Working:** CAN TX (sends frames successfully)
**Confirmed Broken:** CAN RX (receives 0 frames)

## Root Cause

GPIO 20 (RX pin) is not the correct pin for the CAN transceiver on this Waveshare board variant.

Evidence:
- Test via API confirms 0 frames received
- Historical git commits show multiple pin configurations were tested
- v1.62 production code used GPIO 15/4 successfully

## Solution: Test Alternative GPIO Pins

Code is ready with support for multiple GPIO pin pairs. Build firmware with one of these configurations and test:

### Pin Pairs to Test (in recommended order):

1. **GPIO 15 (TX) / GPIO 4 (RX)** - FIRST CHOICE
   - Used in v1.62 production firmware  
   - Most likely to work

2. **GPIO 4 (TX) / GPIO 5 (RX)** - SECOND CHOICE
   - Already set as default in can_manager.h

3. **GPIO 16 (TX) / GPIO 17 (RX)** - THIRD CHOICE

4. **GPIO 26 (TX) / GPIO 27 (RX)** - FOURTH CHOICE

## How to Build & Test

### Step 1: Build Firmware with Target GPIO Pins

#### Option A: Edit can_manager.h directly

Modify `src/can_manager.h`, change the #else block:

```cpp
#else
    // Default: GPIO 4/5 (alternative pins if 19/20 don't work)
    static constexpr gpio_num_t DEFAULT_TX_PIN = static_cast<gpio_num_t>(4);
    static constexpr gpio_num_t DEFAULT_RX_PIN = static_cast<gpio_num_t>(5);
#endif
```

Change 4→15 and 5→4 to test GPIO 15/4, etc.

#### Option B: Build Command with Preprocessor Flag

```powershell
cd D:\Software\Bronco-Controls-4
# Currently blocked due to file locking - try on different machine
pio run -e waveshare_7in
```

### Step 2: Upload Firmware to Device

#### Method A: Via Serial Bootloader (Preferred)

```powershell
# Requires device in bootloader mode:
# 1. Hold BOOT button
# 2. Press RESET while holding BOOT  
# 3. Release BOOT
python direct_upload.py
```

#### Method B: Via OTA (May Hang)

```powershell
python upload_fw.py
# Or manually via web: PUT /api/ota/update with binary
```

### Step 3: Test CAN RX

```powershell
python can_rx_troubleshoot.py
```

Expected output if working:
```
[STEP 2] Testing current configuration...
  Sending test frame...
    ✓ Frame sent successfully
  Listening for frames...
    📊 Frames received: >0
✅ SUCCESS! CAN RX is working!
```

## Build System Issues (Workaround)

**Problem:** Windows Defender/antivirus file locking prevents compilation

**Solutions:**
1. Build on different machine (Windows 11 without antivirus)
2. Use GitHub Actions for CI/CD (auto-build on push)
3. Disable antivirus completely (not recommended for security)
4. Switch to Linux WSL2 for compilation

**Current Status:** Can't complete build locally due to file locks

## Files Modified/Created

- `src/can_manager.h` - Added GPIO pin flexibility
- `can_rx_troubleshoot.py` - Test script confirming issue
- `direct_upload.py` - Alternative firmware upload method
- `can_rx_troubleshoot.py` - Comprehensive guide and test runner

## Key Code Changes

### src/can_manager.h

Added support for multiple GPIO configurations:

```cpp
#if defined(USE_CAN_PINS_15_4)
    // GPIO 15 (TX) / 4 (RX) - Previous production pins
    static constexpr gpio_num_t DEFAULT_TX_PIN = static_cast<gpio_num_t>(15);
    static constexpr gpio_num_t DEFAULT_RX_PIN = static_cast<gpio_num_t>(4);
#elif defined(USE_CAN_PINS_16_17)
    static constexpr gpio_num_t DEFAULT_TX_PIN = static_cast<gpio_num_t>(16);
    static constexpr gpio_num_t DEFAULT_RX_PIN = static_cast<gpio_num_t>(17);
// ... etc
```

## Debugging Info

Current device status (v1.3.78):
- Firmware version: 1.3.78
- WiFi IP: 192.168.7.116
- CAN bitrate: 250 kbps (J1939 extended format)
- CAN TX pins: GPIO 19/20 (NOT WORKING for RX)
- CAN RX status: **0 frames received**
- Serial interface: Completely unresponsive (USB broken?)
- Web API: Fully functional

## Next Immediate Actions

1. **Get a working build** (currently blocked locally)
   - Try building on different Windows machine
   - Or use cloud build service

2. **Once firmware builds:**
   - Test GPIO 15/4 first (highest chance of success)
   - Then 4/5, 16/17, 26/27 in sequence

3. **Upload and verify**
   - Use bootloader method if OTA fails again
   - Test with can_rx_troubleshoot.py after each upload

4. **Once working:**
   - Lock in the correct GPIO pair
   - Commit to git with version tag
   - Deploy to production

## Reference Information

**Waveshare ESP32-S3-7" Pin Mapping:**
- See `/tools/deploy/ESP32-S3-Touch-LCD-7-Demo/` for original demos
- CAN transceiver (SN65HVD) default pins vary by board revision
- Check PCB silkscreen for actual pin assignments

**Git History of Pin Attempts:**
- v1.62: GPIO 15/4 (WORKED)
- v1.65: TX=20, RX=19 (TESTED)
- v1.79 (current): TX=19, RX=20 (BROKEN)

## Commands Reference

```powershell
# Test device connectivity
python can_rx_troubleshoot.py

# Check current device via API
curl -s http://192.168.7.116/api/status | ConvertFrom-Json

# Send test CAN frame
# See test_can_send.py or web API docs

# Monitor device live
python monitor_can.py

# Troubleshooting serial
python test_serial_device.py
```

## Status: PENDING BUILD

Cannot proceed further until firmware compilation is resolved.

**Required:** Build firmware with alternative GPIO pins and upload to device.

Next action: Try building on different machine or use CI/CD service.
