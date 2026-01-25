# InfinityBox PowerCell J1939 Test Instructions

## Current Status
- Firmware 1.3.61 is built and ready with `/api/can/send` endpoint
- Firmware is located at: `.pio/build/waveshare_7in/firmware.bin`
- Test script ready: `test_infinitybox.ps1`

## Upload Firmware (Choose ONE method)

### Method 1: USB Upload (Fastest - Recommended)
1. Connect ESP32 to computer via USB
2. Close any serial monitors
3. Run:
```powershell
pio run -e waveshare_7in --target upload --upload-port COM6
```
*If COM6 doesn't work, check Device Manager for the correct COM port*

### Method 2: OTA Update (Not working currently due to manifest parsing issue)
The OTA server is having trouble parsing the 1.3.61 manifest. Skip this method for now.

## Run InfinityBox Test

Once firmware 1.3.61 is uploaded:

```powershell
# Verify firmware version
Invoke-RestMethod -Uri "http://192.168.7.116/api/status" | Select firmware_version

# Should show: 1.3.61
```

```powershell
# Run comprehensive test
.\test_infinitybox.ps1
```

## What the Test Does

The script sends J1939 CAN frames to your InfinityBox PowerCell on all addresses (A1-A8) according to the manual:

1. **Address Scan**: Sends test frames to PGNs 0xFF01 through 0xFF08 (addresses A1-A8)
2. **Pattern Test**: Cycles through different output patterns on address A1:
   - All outputs OFF
   - Individual outputs ON (1-8)
   - Multiple outputs simultaneously
   - Alternating patterns
3. **Rapid Fire**: 20 quick frames to test response
4. **Multi-Address**: Different patterns to addresses A1-A4 simultaneously
5. **Cleanup**: Turns all outputs OFF

## What to Look For

On the InfinityBox PowerCell:
- **Blue LED blinking** = CAN traffic is being received
- **Output activity** on configured channels (check connectors A & B)

On ESP32 Serial Monitor (115200 baud):
```
[CanManager] Sent PGN 0xFF01 with priority 6
[CanManager] Sent PGN 0xFF02 with priority 6
...
```

## Troubleshooting

### No Response from InfinityBox

1. **Check CAN wiring**:
   - CAN_H and CAN_L properly connected
   - 120Ω termination resistors at both ends of bus
   
2. **Verify baud rate**: Both devices must be 250kbps (this is default)

3. **Check PowerCell address configuration**: 
   - PowerCell must be set to one of the test addresses (A1-A8)
   - Refer to PowerCell manual section on address DIP switches

4. **Test continuity**: Use multimeter to verify CAN bus wiring

### API Endpoint Not Found (404 Error)

Firmware needs to be updated to 1.3.61. Check current version:
```powershell
Invoke-RestMethod -Uri "http://192.168.7.116/api/status" | Select firmware_version
```

## Manual CAN Frame Test

If you want to send a single test frame manually:

```powershell
$testFrame = @{
    pgn = 0xFF01          # PowerCell address A1
    priority = 6
    source = 0xF9
    destination = 0xFF    # Broadcast
    data = @(0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00)  # Turn on output 1
} | ConvertTo-Json

Invoke-RestMethod -Uri "http://192.168.7.116/api/can/send" -Method POST -Body $testFrame -ContentType "application/json"
```

## Frame Format Reference (from InfinityBox Manual)

### PGN to Address Mapping
- 0xFF01 = Address A1
- 0xFF02 = Address A2  
- 0xFF03 = Address A3
- 0xFF04 = Address A4
- 0xFF05 = Address A5
- 0xFF06 = Address A6
- 0xFF07 = Address A7
- 0xFF08 = Address A8

### Data Byte Mapping
Each bit in the 8 data bytes controls a specific output:

**Byte 1 (bits 0-7)**: Outputs 1-8 (Track personality)
**Byte 2 (bits 0-3)**: Outputs 9-10 (Track), 11-20 (Soft-start 2sec)
**Byte 3-8**: Additional PWM and pattern outputs

Example: `data = @(0x01, 0, 0, 0, 0, 0, 0, 0)` turns on Output 1

## Next Steps

1. Upload firmware 1.3.61
2. Run test script: `.\test_infinitybox.ps1`
3. Observe InfinityBox blue LED and output behavior
4. Check ESP32 serial monitor for CAN transmission logs
5. Report results!
