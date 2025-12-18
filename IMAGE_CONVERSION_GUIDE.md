# Image Conversion Guide for LVGL

## Step 1: Resize Images (use Paint.NET, GIMP, or Photoshop)

From `D:\Software\Bronco-Controls-4\assets\`:

- **broncobackground.png** → 800×480 (exact display size)
- **home_icon.PNG** → 192×192 (or 256×256)
- **windows_icon.PNG** → 192×192
- **locks_icon.PNG** → 192×192
- **runningboards_icon.PNG** → 192×192
- **bronco_logo.jpg** → ~200px wide (keep aspect ratio)

Save resized versions in `assets/resized/` folder.

## Step 2: Convert to LVGL C Arrays

### Use LVGL Image Converter Online:
**https://lvgl.io/tools/imageconverter**

### For each icon PNG (home, windows, locks, runningboards):
1. Upload the resized PNG
2. **Output format:** C array
3. **Color format:** True color with alpha (CF_TRUE_COLOR_ALPHA)
4. **Name:**
   - home_icon.PNG → `img_home_icon`
   - windows_icon.PNG → `img_windows_icon`
   - locks_icon.PNG → `img_locks_icon`
   - runningboards_icon.PNG → `img_runningboards_icon`
5. Click "Convert"
6. Download the `.c` file

### For background (broncobackground.png):
1. Upload the 800×480 PNG
2. **Output format:** C array
3. **Color format:** True color (CF_TRUE_COLOR) - no alpha needed
4. **Name:** `img_background`
5. Download the `.c` file

### For logo (bronco_logo.jpg):
1. Upload the resized logo
2. **Output format:** C array
3. **Color format:** True color (CF_TRUE_COLOR)
4. **Name:** `img_bronco_logo`
5. Download the `.c` file

## Step 3: Place Converted Files

Move all downloaded `.c` files to:
```
D:\Software\Bronco-Controls-4\src\assets\
```

You should have:
- `img_home_icon.c`
- `img_windows_icon.c`
- `img_locks_icon.c`
- `img_runningboards_icon.c`
- `img_background.c`
- `img_bronco_logo.c`

## Step 4: Build Project

The project is already set up with `images.h` header. Just place the `.c` files and build!

```
pio run --target upload
```

## Notes:
- **True color with alpha** is needed for icons with transparency/glow
- **True color** (no alpha) is fine for solid background
- Smaller images = faster compilation & less flash usage
- If images are too large, reduce resolution in Step 1
