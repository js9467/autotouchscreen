# POWERCELL NGX CAN Quick Start

## What This Does
Simple, focused firmware to test CAN communication between your Waveshare ESP32-S3-Touch-LCD-7 and the Infinitybox POWERCELL NGX.

## Hardware Setup

### 1. CAN Connections
Connect your ESP32-S3 to the POWERCELL NGX CAN bus:
- **ESP32 TX (GPIO20)** → CAN transceiver TX
- **ESP32 RX (GPIO19)** → CAN transceiver RX  
- **CAN transceiver** → POWERCELL NGX CAN bus (CAN_H and CAN_L)

### 2. CAN Termination
Ensure your CAN bus has proper 120Ω termination resistors at both ends.

## Quick Deploy

Run the deployment script:
```powershell
.\deploy_powercell_test.ps1
```

It will:
1. Build the minimal test firmware
2. Upload to your ESP32
3. Open serial monitor

## Testing

### Serial Commands
Once running, you can send these commands via serial:
- **P** - Send poll request to POWERCELL (gets current configuration)
- **C** - Send configuration message to POWERCELL
- **H** - Show help

### What to Expect

#### On Startup:
```
=== POWERCELL NGX CAN Communication Test ===
Hardware: ESP32-S3-Touch-LCD-7
CAN TX Pin: GPIO20
CAN RX Pin: GPIO19
Bitrate: 250000 bps
POWERCELL Address: 1
==========================================

✓ CAN driver installed
✓ CAN driver started
✓ CAN initialized successfully!
Listening for POWERCELL responses...
```

#### Sending Poll Request (every 5 seconds):
```
→ Sent poll request to POWERCELL
  CAN ID: 0x18FF4163
  Data: 11 00 00 00 00 00 00 00
```

#### Receiving POWERCELL Response:
```
← Received CAN message:
  CAN ID: 0x18FF5101
  Length: 8 bytes
  Data: 01 02 05 BE 01 00 00 55
  ★ POWERCELL RESPONSE DETECTED!
  Major Version: 1
  Minor Version: 2
  ADC Source: 5
  User Config: 190
  Data Rate: 250 kbps
  LOC Timer: 10s
  Report Timer: 250ms
  PWM Freq: 200Hz
```

## Configuration

### POWERCELL Address
Edit `test_powercell_can.cpp` line 16:
```cpp
#define POWERCELL_ADDRESS 1  // Change to your cell address (1-16)
```

### CAN Bitrate  
Edit `test_powercell_can.cpp` line 18:
```cpp
#define CAN_BITRATE 250000  // 250kbps, 500000, or 1000000
```

### Send Configuration
Press **C** in serial monitor to send this configuration:
- J1939 Data Rate: 250 kb/s
- Loss-of-Com Timer: 10 seconds
- Reporting Timer: 250 ms
- PWM Frequency: 200 Hz
- All outputs: Maintain State on LOC
- User config number: 1

**Important:** POWERCELL must be power cycled after configuration!

## Troubleshooting

### No Response from POWERCELL
1. Check CAN wiring (TX, RX, CAN_H, CAN_L)
2. Verify 120Ω termination resistors
3. Confirm POWERCELL is powered
4. Check POWERCELL address matches (default is 1)
5. Verify bitrate matches (default is 250kbps)

### Upload Failed
1. Hold BOOT button on ESP32
2. Press RESET while holding BOOT
3. Release BOOT
4. Try upload again

### CAN Driver Errors
Check serial output for:
- "✓ CAN driver installed" - Driver OK
- "✗ Failed to install CAN driver" - Hardware issue

## CAN ID Reference

For POWERCELL address 1:
- **Configuration message:** 0x18FF4163 (we send this)
- **Response message:** 0x18FF5101 (POWERCELL sends this)

For other addresses (2-16), adjust the last nibble:
- Address 2: 0x18FF4263 / 0x18FF5202
- Address 3: 0x18FF4363 / 0x18FF5303
- etc.

## Next Steps

Once CAN communication is working:
1. Test sending control messages to outputs
2. Monitor POWERCELL status broadcasts
3. Integrate into your main application

## Files
- `test_powercell_can.cpp` - Main test firmware
- `platformio_test.ini` - Build configuration
- `deploy_powercell_test.ps1` - Build & upload script
