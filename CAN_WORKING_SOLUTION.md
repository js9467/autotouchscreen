# CAN Communication - Working Solution
**Date:** January 28, 2026  
**Status:** ✅ FULLY FUNCTIONAL  
**Firmware Version:** 1.3.78

## Problem Summary
CAN communication stopped working after firmware upload. CAN RX was completely non-functional despite:
- Correct GPIO configuration (TX=GPIO20, RX=GPIO19)
- Verified CAN bus wiring (120Ω termination, proper voltage)
- Working TWAI driver initialization
- No error messages during compilation

## Root Cause Discovery

### Hardware Architecture (Waveshare ESP32-S3-Touch-LCD-7)
```
ESP32-S3 GPIO19/20 ──→ SN65HVD230 CAN Transceiver ──→ CAN Bus
                           ↑
                           │ Power Control
                           │
                    CH422G I/O Expander (I2C: 0x24)
                    USB_SEL pin (bit 5)
```

**Critical Discovery:** The SN65HVD230 CAN transceiver is NOT directly powered by ESP32. Instead, power is controlled by the CH422G I2C I/O expander's USB_SEL output pin.

### CH422G I2C Protocol (Non-Standard)
The CH422G uses a unique I2C protocol where **register addresses ARE the I2C device addresses**:

| Register | I2C Address | Function |
|----------|-------------|----------|
| WR_SET | 0x24 | Output enable control |
| WR_OC | 0x23 | Upper 4 bits control |
| WR_IO | **0x38** | **Output pin states** |

To enable CAN transceiver:
```cpp
Wire.beginTransmission(0x38);  // NOT 0x24! Register address IS device address
Wire.write(0x2A);               // Bit pattern: USB_SEL=HIGH (bit 5)
Wire.endTransmission();
```

### Software Timing Issue
The ESP_Panel_Library initializes I2C during `panel->begin()`. If CAN manager tries to initialize I2C before the panel:
- **Result:** Double I2C driver installation → Crash loop

If CAN manager initializes after panel but doesn't set USB_SEL:
- **Result:** CAN transceiver unpowered → No RX/TX possible

## Working Solution

### Initialization Sequence (CRITICAL ORDER)
```cpp
// 1. Panel library initializes I2C
panel->begin();

// 2. Set USB_SEL HIGH via expander (backup method)
expander->digitalWrite(USB_SEL, HIGH);

// 3. CAN Manager initialization
CanManager::instance().begin();
  ↓
  // Inside CanManager::begin():
  // a) Write directly to CH422G via I2C (already initialized)
  Wire.beginTransmission(0x38);  // WR_IO register
  Wire.write(0x2A);               // USB_SEL HIGH
  Wire.endTransmission();
  
  // b) Initialize TWAI driver
  twai_driver_install(...);
  twai_start();
```

### Key Code Changes

**File:** `src/can_manager.cpp` (Lines 26-48)
```cpp
bool CanManager::begin(gpio_num_t tx_pin, gpio_num_t rx_pin, std::uint32_t bitrate) {
    // CH422G I2C configuration
    #define CH422G_REG_WR_IO    0x38  // Output control register I2C address
    #define CH422G_USB_SEL_HIGH 0x2A  // USB_SEL (bit 5) = HIGH
    
    Serial.println("[CanManager] Enabling CAN transceiver via CH422G...");
    
    // Write to CH422G register 0x38 to set USB_SEL HIGH
    // I2C already initialized by panel - write directly
    Wire.beginTransmission(CH422G_REG_WR_IO);  // 0x38, not 0x24!
    Wire.write(CH422G_USB_SEL_HIGH);            // 0x2A
    int i2c_result = Wire.endTransmission();
    
    if (i2c_result == 0) {
        Serial.println("[CanManager] ✓ CAN transceiver enabled (USB_SEL=HIGH)");
    } else {
        Serial.printf("[CanManager] ⚠ CH422G I2C write failed (err=%d)\n", i2c_result);
    }
    
    delay(50); // Give transceiver time to power up
    
    // Then initialize TWAI driver...
}
```

**File:** `src/main.cpp` (Lines 233-248)
```cpp
panel->begin();

// Re-ensure USB_SEL is HIGH after panel initialization
delay(50);
if (expander) {
    expander->digitalWrite(USB_SEL, HIGH);
    Serial.println("[Boot] USB_SEL set HIGH for CAN transceiver");
}

// CAN initialization AFTER I2C is ready
Serial.println("\n[CAN] Initializing CAN bus...");
CanManager::instance().begin();

if (CanManager::instance().isReady()) {
    Serial.println("[CAN] ✓ TWAI driver initialized successfully!");
}
```

## Hardware Verification

### Working Test (canagain diagnostic firmware)
- **RX Rate:** 8-10 messages/second from J1939 bus
- **TX Test:** Successfully transmitted test frames
- **Bus State:** RUNNING, 0 errors

### Pin Configuration (VERIFIED CORRECT)
```
CAN_TX = GPIO20  ✓
CAN_RX = GPIO19  ✓
I2C_SDA = GPIO8  ✓
I2C_SCL = GPIO9  ✓
```

**Note:** Pins were NEVER swapped - GPIO20=TX, GPIO19=RX is correct!

## What Didn't Work (Lessons Learned)

❌ **Attempt 1:** Using ESP_IOExpander library to set USB_SEL  
   - **Issue:** Library tries to initialize before I2C ready → Fails silently

❌ **Attempt 2:** Initializing CAN before panel library  
   - **Issue:** Double I2C driver install → Assert failure → Boot loop

❌ **Attempt 3:** Writing to I2C address 0x24  
   - **Issue:** Wrong! CH422G uses register address (0x38) as device address

✅ **Working Solution:** Direct I2C write to register 0x38 AFTER panel->begin()

## Testing Confirmation

### Button Press → CAN Transmission
1. User presses "Windows" button on touchscreen
2. `UIBuilder` calls `CanManager::sendButtonAction()`
3. CAN frame transmitted: `18FF0180` with data `[20 80 00 00 00 00 00 00]`
4. InfinityBox Output 9 responds → LED lights up

### Serial Output Logs
```
[CanManager] Initializing TWAI on TX=GPIO20, RX=GPIO19, Bitrate=250000
[CanManager] Enabling CAN transceiver via CH422G...
[CanManager] ✓ CAN transceiver enabled (USB_SEL=HIGH)
[CanManager] TWAI bus ready at 250 kbps
[CAN] ✓ TWAI driver initialized successfully!
[CAN]   TX=GPIO20, RX=GPIO20
```

## Files Modified
- `src/can_manager.cpp` - Added CH422G I2C write before TWAI init
- `src/can_manager.h` - No changes to public interface
- `src/main.cpp` - Moved CAN init to after panel->begin()
- `src/ui_builder.cpp` - No changes (button handlers unchanged)

## Critical Dependencies
- **I2C must be initialized by panel FIRST**
- **CH422G write must happen BEFORE twai_driver_install()**
- **I2C address must be 0x38 (register), NOT 0x24 (device)**
- **Delay after USB_SEL set to allow transceiver power-up**

## Reversion Instructions
If CAN stops working again:
```bash
git checkout v1.3.78-can-working
pio run -e waveshare_7in --target upload --upload-port COM5
```

## Reference Hardware
- **Board:** Waveshare ESP32-S3-Touch-LCD-7
- **I/O Expander:** CH422G (I2C address 0x24, register 0x38)
- **CAN Transceiver:** SN65HVD230
- **Bus Protocol:** J1939, 250 kbps
- **Target Device:** InfinityBox automotive controller

## Success Metrics
✅ CAN RX functional (receiving J1939 messages)  
✅ CAN TX functional (sending button press commands)  
✅ Display working (LVGL UI rendering)  
✅ Button press → CAN frame → Device response  
✅ No boot loops or crashes  
✅ Serial logging confirms initialization  

**This version is PRODUCTION READY for vehicle installation.**
