# Navigation Button Color & Font Size Fix

## Issues Fixed

### 1. Inactive Navigation Button Color Not Applying
**Problem:** Inactive navigation buttons were not displaying the correct color set in the theme.

**Root Cause:** The nav button color was only set during creation, but LVGL requires explicit style setting for both DEFAULT and CHECKED states.

**Solution:** 
- Modified `buildNavigation()` in [ui_builder.cpp](d:\Software\Bronco-Controls-4\src\ui_builder.cpp)
- Now explicitly sets `bg_color` for both default state and CHECKED state
- Inactive buttons use `nav_button_color` or per-page `nav_color`
- Active (checked) buttons use `nav_button_active_color`

```cpp
// Set inactive state color
lv_obj_set_style_bg_color(btn, nav_color, 0);
lv_obj_set_style_bg_opa(btn, LV_OPA_COVER, 0);

// Set active state color
lv_color_t active_color = colorFromHex(config_->theme.nav_button_active_color, UITheme::COLOR_ACCENT);
lv_obj_set_style_bg_color(btn, active_color, LV_STATE_CHECKED);
lv_obj_set_style_bg_opa(btn, LV_OPA_COVER, LV_STATE_CHECKED);
```

### 2. Font Size Not Working on Buttons
**Problem:** The `font_size` field (8-72px) was saved correctly but didn't affect the actual button text size.

**Root Cause:** Font selection used a simple conditional that just chose between 3 hardcoded fonts, ignoring the actual font_size value.

**Solution:**
- Improved font size mapping in `buildPage()` 
- Now maps font_size ranges to the 4 available Montserrat fonts:
  - **8-12px** → FONT_CAPTION (Montserrat 14)
  - **13-18px** → FONT_BODY (Montserrat 16)  
  - **19-28px** → FONT_HEADING (Montserrat 24)
  - **29+px** → FONT_TITLE (Montserrat 32)

```cpp
if (button.font_size <= 12) {
    font = UITheme::FONT_CAPTION;
} else if (button.font_size <= 18) {
    font = UITheme::FONT_BODY;
} else if (button.font_size <= 28) {
    font = UITheme::FONT_HEADING;
} else {
    font = UITheme::FONT_TITLE;
}
```

### 3. Font Family Selection Added
**New Feature:** Added font family selection to button configuration.

**Implementation:**
- Added `font_family` field to `ButtonConfig` (default: "montserrat")
- Added font family dropdown in web interface button modal
- Field is saved/loaded with configuration
- Currently only Montserrat is available (more fonts coming in future updates)
- Web interface shows helpful hint: "8-12: Smallest, 13-18: Small, 19-28: Medium, 29+: Large"

## Files Modified

1. **config_types.h**
   - Added `font_family` field to `ButtonConfig`

2. **config_manager.cpp**
   - Added serialization/deserialization for `font_family`

3. **ui_builder.cpp**
   - Fixed nav button color initialization in `buildNavigation()`
   - Improved font size mapping in `buildPage()`

4. **web_interface.h**
   - Added font family dropdown to button modal
   - Added helpful font size range hints
   - Updated JavaScript to save/load `font_family`

## Testing

✅ **Build:** Successful (Exit Code 0)
✅ **Upload:** Successful to ESP32-S3 via COM3
✅ **Memory:** RAM 15.7%, Flash 20.6%

## Usage

### Navigation Button Colors
1. Go to **Theme** tab
2. Set **Nav Button Color** (inactive state)
3. Set **Nav Button Active Color** (active/selected state)
4. Or override per-page in **Pages** tab using **Page Nav Color**

### Button Font Sizing
1. Edit any button in **Pages** tab
2. Set **Font Size** (8-72px)
3. Font will map to closest available size:
   - Small text: 8-12
   - Body text: 13-18
   - Heading: 19-28
   - Large: 29+

### Font Family (Future)
- Dropdown available in button editor
- Currently only Montserrat supported
- More fonts will be added in future updates

## Next Steps
- Add more font families (consider embedded bitmap fonts)
- Allow custom font uploads
- Add font preview in web interface
