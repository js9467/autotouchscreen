# Emergency Recovery - CAN Working Version

## 🆘 Quick Restore Commands

If CAN stops working, restore the last known good version:

```powershell
cd D:\Software\Bronco-Controls-4
git fetch origin
git checkout v1.3.78-can-working
pio run -e waveshare_7in --target upload --upload-port COM5
```

Wait for upload to complete, then test by pressing the "Windows" button.

## 🔍 How to Verify It's Working

1. **Serial Monitor** (optional):
   ```powershell
   python simple_serial_monitor.py COM5
   ```
   Look for: `[CanManager] ✓ CAN transceiver enabled (USB_SEL=HIGH)`

2. **Physical Test**:
   - Press "Windows" button on touchscreen
   - InfinityBox LED should light up
   - Output 9 should activate

3. **CAN Bus Test** (with canagain):
   ```powershell
   cd D:\Software\canagain
   python quick_test.py
   ```
   Should see CAN messages being received

## 🎯 The Critical Fix

**Problem:** CAN transceiver was unpowered  
**Root Cause:** CH422G I/O expander USB_SEL pin not set HIGH  
**Solution:** Write 0x2A to I2C register 0x38 before TWAI init  

## 📌 Git Information

**Tag:** `v1.3.78-can-working`  
**Commit:** `ffc56b2`  
**Remote:** https://github.com/js9467/autotouchscreen.git  
**Date:** January 28, 2026

## ⚠️ Don't Forget

- I2C register 0x38 (NOT 0x24!)
- Value 0x2A to enable USB_SEL
- CAN init AFTER panel->begin()
- GPIO20=TX, GPIO19=RX (NOT swapped)

---

**Keep this file handy for emergencies!**
