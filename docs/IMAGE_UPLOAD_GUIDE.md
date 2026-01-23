# Image Upload Guide

## The Problem

The ESP32-S3 has limited RAM (320KB) and LVGL cannot decode images from base64 data URLs in memory. This causes uploaded images to not display.

## Simple Solution: Pre-Process Images

Use the provided Python script to resize and optimize images **before** uploading:

### Step 1: Prepare Your Image

```bash
python prepare_images.py mylogo.png output_logo.png header
```

**Image Types and Sizes:**
- `header` - 48×48 pixels (header logo)
- `splash` - 240×135 pixels (splash screen)  
- `background` - 320×240 pixels (full screen background)
- `sleep` - 64×64 pixels (sleep mode icon)

### Step 2: Upload Through Web Interface

1. Open the device configuration page
2. Scroll to **Image Assets** section
3. Click **Choose File** for the image type
4. Select your prepared image
5. Click **Upload**

## Image Requirements

- **Format:** PNG or JPEG
- **Max file size:** 50 KB per image (smaller is better)
- **Color:** RGB or grayscale
- Keep images simple and optimized

## Examples

```bash
# Prepare a header logo (48x48)
python prepare_images.py company_logo.png header_logo.png header

# Prepare a splash screen (240x135)
python prepare_images.py splash_art.jpg splash_logo.jpg splash

# Prepare background (320x240)  
python prepare_images.py background.png bg.png background

# Prepare sleep icon (64x64)
python prepare_images.py moon.png sleep_icon.png sleep
```

## Tips

- **Use simple graphics** - Complex images use more memory
- **Reduce colors** - Fewer colors = smaller files
- **Test on device** - If device crashes, image is too large
- **PNG for logos** - Use PNG for graphics with transparency
- **JPEG for photos** - Use JPEG for photographic images

## Troubleshooting

**Image doesn't show up:**
- Check serial monitor for errors
- Try a smaller/simpler image
- Reduce JPEG quality (edit script, lower `quality=80`)

**Device crashes after upload:**
- Image is too large
- Reduce size further: header to 32×32, sleep to 48×48

**Upload fails:**
- File too large (>50 KB)
- Use the prepare script to optimize

## Technical Details

- Display: 320×240 pixels, 16-bit color
- RAM: 320 KB total, ~100 KB available for images
- LVGL decoders: PNG and JPEG supported via filesystem
- Storage: LittleFS filesystem in flash memory
