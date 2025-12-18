# Complete Setup Instructions for Bronco Controls

## Overview

This document explains how to add the required hardware libraries that are NOT available via PlatformIO's library manager. These libraries are from Waveshare and must be added manually.

## Required Libraries

The project needs two Waveshare-specific libraries:

1. **ESP32_Display_Panel** - Display and touch driver
2. **ESP32_IO_Expander** - IO expander driver for the CH422G chip

## Download and Installation

### Option 1: Download from Waveshare (Recommended for Exact Hardware Match)

1. **Download the Waveshare library package**:
   - Visit: https://www.waveshare.com/wiki/ESP32-S3-Touch-LCD-4.3
   - Download: [S3-4.3-libraries.zip](https://files.waveshare.com/wiki/ESP32-S3-Touch-LCD-4.3/S3-4.3-libraries.zip)

2. **Extract the archive** and locate:
   - `ESP32_Display_Panel/` folder
   - `ESP32_IO_Expander/` folder

3. **Copy to project lib folder**:
   ```
   Bronco-Controls-4/
   └── lib/
       ├── ESP32_Display_Panel/  <-- Copy here
       ├── ESP32_IO_Expander/    <-- Copy here
       ├── ESP_Panel_Conf.h
       └── lv_conf.h
   ```

### Option 2: Clone from GitHub Reference

Alternatively, use the reference repository that has these libraries:

```bash
# In a temporary directory
git clone https://github.com/istvank/Waveshare-ESP32-S3-Touch-LCD-4.3 temp_ref

# Copy the library folders
cp -r temp_ref/lib/ESP32_Display_Panel  d:/Software/Bronco-Controls-4/lib/
cp -r temp_ref/lib/ESP32_IO_Expander    d:/Software/Bronco-Controls-4/lib/

# Clean up
rm -rf temp_ref
```

## Final Project Structure

After adding the libraries, your project should look like this:

```
Bronco-Controls-4/
├── platformio.ini
├── README.md
├── COMPLETE_SETUP.md          # This file
├── lib/
│   ├── ESP_Panel_Conf.h       # ✓ Already created
│   ├── lv_conf.h              # ✓ Already created
│   ├── ESP32_Display_Panel/   # ← ADD THIS (from Waveshare)
│   │   ├── src/
│   │   │   ├── ESP_Panel.cpp
│   │   │   ├── ESP_Panel.h
│   │   │   ├── ESP_PanelLcd.cpp
│   │   │   ├── ESP_PanelLcdTouch.cpp
│   │   │   ├── bus/
│   │   │   ├── lcd/
│   │   │   └── lcd_touch/
│   │   └── ...
│   └── ESP32_IO_Expander/     # ← ADD THIS (from Waveshare)
│       ├── src/
│       │   ├── ESP_IOExpander.cpp
│       │   ├── ESP_IOExpander.h
│       │   ├── chip/
│       │   │   ├── CH422G.cpp
│       │   │   └── CH422G.h
│       └── ...
└── src/
    ├── main.cpp               # ✓ Already created
    ├── ui_theme.h/cpp         # ✓ Already created
    ├── app_state.h/cpp        # ✓ Already created
    └── components/            # Future expansion
```

## Build and Flash

### 1. Open Project in VS Code

```bash
# Open VS Code in the project directory
cd d:/Software/Bronco-Controls-4
code .
```

### 2. Let PlatformIO Download Dependencies

When you open the project, PlatformIO will automatically:
- Download the ESP32 platform (espressif32)
- Download LVGL library (v8.3.11)
- Configure the build system

**This may take several minutes on first build.**

### 3. Build the Project

- Click the **✓ (checkmark)** icon in the PlatformIO toolbar
- Or open PlatformIO menu → Project Tasks → Build
- Or use CLI: `pio run`

Expected output:
```
Processing esp32s3box (platform: espressif32; board: esp32s3box; framework: arduino)
...
Building .pio/build/esp32s3box/firmware.bin
...
[SUCCESS] Took X.XX seconds
```

### 4. Connect the Hardware

1. **Connect** ESP32-S3 board to PC via USB-C cable
2. **Enter bootloader mode** (FIRST FLASH ONLY):
   - Press and **hold BOOT** button
   - Press **RESET** button (while holding BOOT)
   - Release **RESET**
   - Release **BOOT**
3. Board should appear as a COM port

### 5. Upload Firmware

- Click the **→ (right arrow)** icon in PlatformIO toolbar
- Or: PlatformIO menu → Project Tasks → Upload
- Or use CLI: `pio run --target upload`

Expected output:
```
Uploading .pio/build/esp32s3box/firmware.bin
...
Writing at 0x00001000... (XX %)
...
[SUCCESS] Took X.XX seconds
```

### 6. Monitor Serial Output (Optional)

- Click the **🔌 (plug)** icon in PlatformIO toolbar
- Or use CLI: `pio device monitor --baud 115200`

You should see:
```
=================================
Bronco Controls - Automotive HMI
=================================

Initializing LVGL...
Initializing display panel...
Initializing IO expander...
...
Setup complete! Touch to interact
=================================
```

## Troubleshooting

### Library Not Found Errors

**Error**: `fatal error: ESP_Panel_Library.h: No such file or directory`

**Solution**: You haven't copied the Waveshare libraries to the `lib/` folder. Follow Option 1 or 2 above.

### Build Errors in Waveshare Libraries

**Error**: Compilation errors in `ESP32_Display_Panel` or `ESP32_IO_Expander`

**Solution**: 
1. Make sure you downloaded the LATEST libraries from Waveshare
2. Verify the CH422G chip support is included in `ESP32_IO_Expander`
3. If using GitHub clone, try the direct Waveshare download instead

### Upload Failed

**Error**: `Failed to connect to ESP32`

**Solution**:
1. Manually enter bootloader mode (BOOT + RESET sequence)
2. Check that USB cable supports data (not just charging)
3. Install CP210x drivers if needed (Windows)
4. Try different USB port
5. Close Serial Monitor if open

### Display Not Working

**Error**: Display stays black

**Solution**:
1. Check that IO expander is initializing (should see in serial output)
2. Verify `LCD_BL` (backlight) pin is being set HIGH in main.cpp
3. Check `ESP_Panel_Conf.h` RGB pin mappings match hardware
4. Try reducing `ESP_PANEL_LCD_RGB_CLK_HZ` to 14MHz if artifacts appear

### Touch Not Responding

**Error**: Touch input not working

**Solution**:
1. Verify GT911 touch controller is detected (check serial output)
2. Check I2C pins in `ESP_Panel_Conf.h`: SCL=9, SDA=8
3. Uncomment touch debug in `main.cpp` to see raw coordinates
4. Adjust `ESP_PANEL_LCD_TOUCH_MIRROR_X/Y` if coordinates are flipped

### PSRAM Allocation Failed

**Error**: `Failed to allocate LVGL buffer in PSRAM!`

**Solution**:
1. Verify `BOARD_HAS_PSRAM` is defined in `platformio.ini`
2. Check that `board_build.arduino.memory_type = qio_opi` is set
3. This board REQUIRES PSRAM for the large display buffer

## Next Steps

Once you have a working build:

1. **Test Navigation**: Touch the WINDOWS and LOCKS buttons on home screen
2. **Expand Functionality**: 
   - Create complete Windows and Locks screens (see README.md)
   - Add more components (Toggle, Slider, etc.)
   - Implement state persistence
3. **Add CAN Integration**: Use the AppState architecture for J1939 data
4. **Customize Theme**: Edit colors in `ui_theme.cpp`

## Support Resources

- **This Project README**: [README.md](README.md)
- **Waveshare Wiki**: https://www.waveshare.com/wiki/ESP32-S3-Touch-LCD-4.3
- **LVGL Docs**: https://docs.lvgl.io/8.3/
- **PlatformIO Docs**: https://docs.platformio.org/
- **Reference Repo**: https://github.com/istvank/Waveshare-ESP32-S3-Touch-LCD-4.3

---

## Quick Checklist

- [ ] Downloaded/cloned Waveshare libraries
- [ ] Copied `ESP32_Display_Panel` to `lib/`
- [ ] Copied `ESP32_IO_Expander` to `lib/`
- [ ] Opened project in VS Code
- [ ] PlatformIO detected and configured
- [ ] Build completed successfully
- [ ] Board connected and in bootloader mode
- [ ] Upload succeeded
- [ ] Serial monitor shows initialization messages
- [ ] Display shows "BRONCO CONTROLS" home screen
- [ ] Touch responds to button presses

If all boxes are checked: **Congratulations! Your Bronco Controls HMI is running!** 🎉
