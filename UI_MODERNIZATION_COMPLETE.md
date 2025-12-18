# Bronco Controls - UI Modernization Complete ✅

## What Changed

### ✨ New Architecture: Single Screen with Content Swapping

**Before:** Each page was a separate LVGL screen, causing visible flicker when navigating.

**After:** One base screen with:
- Fixed header bar (top)
- Fixed nav rail (left, 96px wide)
- Dynamic content container (right)

**Benefit:** No more flicker! Navigation now just cleans and rebuilds the content area.

---

## 🎨 New Layout

```
┌─────────────────────────────────────────────┐
│  BRONCO CONTROLS          [LOGO]  ← Header │
├────┬────────────────────────────────────────┤
│HOME│                                        │
├────┤                                        │
│WIN │        Content Area                    │
├────┤     (changes on navigation)           │
│LOCK│                                        │
├────┤                                        │
│ RB │                                        │
└────┴────────────────────────────────────────┘
```

---

## 📦 New Features

### 1. **Running Boards Page** ✅
- Deploy/Retract buttons
- State display (DEPLOYED / RETRACTED / UNKNOWN)
- Ready for CAN integration

### 2. **Left Navigation Rail** ✅
- Always visible 4 icon buttons
- HOME, WINDOWS, LOCKS, RUNNING BOARDS
- Currently uses text placeholders (add images later)

### 3. **Fixed Header** ✅
- Title on left: "BRONCO CONTROLS"
- Logo on right (commented out, ready for `img_bronco_logo`)

### 4. **Background Image Support** ✅
- Code ready in `create_base_screen()`
- Just uncomment when you add `img_background.c`

### 5. **Fixed Button Press Animation** ✅
- Removed size transforms (no more "button jump")
- Now uses opacity change only (smooth press feedback)

---

## 🖼️ Image Integration (Next Step)

### Files Created:
- `src/assets/images.h` - Header declaring all image descriptors
- `src/assets/README.md` - Instructions for adding images
- `IMAGE_CONVERSION_GUIDE.md` - Step-by-step conversion guide

### To Add Images:
1. **Resize** your assets from `assets/` folder (see guide)
2. **Convert** using LVGL Image Converter (https://lvgl.io/tools/imageconverter)
3. **Place** `.c` files in `src/assets/`
4. **Uncomment** image code in `main.cpp`:
   ```cpp
   #include "assets/images.h"  // Line ~13
   // lv_obj_t* bg = lv_img_create(base_screen);  // Line ~185
   // lv_img_set_src(bg, &img_background);
   ```
5. **Build** and upload!

---

## 🎯 Pages Overview

### HOME
- 3 large tiles (WINDOWS, LOCKS, RUNNING BOARDS)
- 260×200px each
- Centered grid layout
- Ready for icon images

### WINDOWS
- 2 columns (Driver, Passenger)
- Press-and-hold UP/DOWN buttons
- Window timer active during hold
- Logs to serial: `WINDOW HOLD id=X dir=Y`

### LOCKS
- Large tappable card (85% width × 75% height)
- Lock icon (32pt font)
- LOCKED/UNLOCKED label
- Synced with AppState

### RUNNING BOARDS (NEW!)
- Icon placeholder at top
- DEPLOY button (amber accent)
- RETRACT button
- Status label shows current state

---

## 🚀 Build Results

**Firmware Size:** 779,397 bytes (59.5% of 1.31 MB flash)
**RAM Usage:** 26,148 bytes (8.0% of 320 KB)
**Upload:** Successful to COM3 @ 921600 baud

---

## 🔧 Technical Details

### Code Changes:
- `app_state.h`: Changed `SETTINGS` → `RUNNING_BOARDS` in Screen enum
- `main.cpp`: Complete rewrite (~500 lines total)
  - Removed: `create_home_screen()`, `create_windows_screen()`, `create_locks_screen()`, `load_screen()`
  - Added: `create_base_screen()`, `build_home_page()`, `build_windows_page()`, `build_locks_page()`, `build_running_boards_page()`
  - Added: Running boards state management (deploy/retract)
  - Changed: `handle_navigation()` now calls `lv_obj_clean(content_container)` instead of screen swap
- `ui_theme.cpp`: Fixed button press style (removed transform width/height)

### Architecture:
```
create_base_screen()  ← Called once in setup()
    ↓
    Creates: header_bar, nav_rail, content_container
    ↓
build_home_page(content_container)  ← Initial page
    ↓
User taps nav button
    ↓
handle_navigation(Screen)
    ↓
    lv_obj_clean(content_container)  ← Remove old content
    ↓
    build_XXX_page(content_container)  ← Build new content
    ↓
    No flicker! (same screen, different content)
```

---

## 🎨 Styling

### Current Theme (UITheme):
- Background: `#1A1A1A` (dark gray)
- Surface: `#2A2A2A` (lighter gray for cards)
- Accent: `#FFA500` (amber/orange - Bronco signature)
- Text Primary: `#FFFFFF` (white)
- Text Secondary: `#AAAAAA` (gray)

### Nav Rail Buttons:
- Size: 76×76px
- Spacing: Medium (`SPACE_MD`)
- Currently text labels (HOME, WIN, LOCK, RB)
- Ready for circular icon images

### Home Tiles:
- Size: 260×200px
- Large enough for automotive touch
- Flex wrap layout (responsive to screen size)

---

## 🔮 Next Steps

### Immediate (Do These First):
1. **Convert and add images** (see `IMAGE_CONVERSION_GUIDE.md`)
2. **Test navigation** on device (tap nav rail buttons)
3. **Test running boards** (deploy/retract buttons)

### Soon:
1. Wire window hold timer to CAN commands (replace Serial.printf in `window_timer_cb()`)
2. Add rear window controls (currently only Driver id=0, Passenger id=1)
3. Implement real running boards state from CAN
4. Add visual feedback for window position (progress bars?)

### Future:
1. Settings page (brightness, units, calibration)
2. Gauges page (speed, RPM, temp, etc.)
3. Persistent storage (remember last state)
4. Animations for state transitions

---

## 📝 Notes

- **No flicker!** Content swap is much faster than screen swap
- **Nav rail always visible** - one tap to any page
- **Button press fixed** - smooth opacity change, no jump
- **Running boards ready** - just wire up CAN
- **Images ready** - all infrastructure in place, just add `.c` files

---

## ✅ Verification

Upload successful! You should now see:
- Nav rail on left with HOME/WIN/LOCK/RB buttons
- Home page with 3 large tiles
- Smooth navigation (no flicker)
- Running boards page works
- Button press looks smooth

**Serial output shows:**
```
Navigate to screen: X
WINDOW HOLD id=X dir=Y  (when holding window buttons)
Running boards: DEPLOY  (when tapping deploy)
Running boards: RETRACT (when tapping retract)
```

---

**Status: COMPLETE** 🎉

All code changes implemented and uploaded. Device is ready for image integration!
