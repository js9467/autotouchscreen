# Image Management Guide for Bronco Controls

## Overview

The Bronco Controls system now includes a comprehensive image management system that automatically resizes and optimizes images for use on your ESP32-S3 display. This guide covers everything you need to know about uploading and managing images.

## Supported Image Types

The system supports **4 different image types**, each with specific dimensions and purposes:

### 1. Header Logo
- **Purpose**: Displayed in the top header bar of your interface
- **Max Dimensions**: 48×36 pixels
- **Format**: PNG with transparency (alpha channel)
- **Max File Size**: 30 KB
- **Best for**: Small brand logos, icons

### 2. Splash Screen Logo
- **Purpose**: Shown during startup/boot sequence
- **Max Dimensions**: 400×300 pixels
- **Format**: PNG with transparency (alpha channel)
- **Max File Size**: 200 KB
- **Best for**: Larger branding, welcome screens

### 3. Background Image
- **Purpose**: Full-screen background for your interface
- **Max Dimensions**: 800×480 pixels (exact display size)
- **Format**: JPEG (for smaller file size)
- **Max File Size**: 300 KB
- **Best for**: Backgrounds, photos, textured surfaces

### 4. Sleep Overlay Logo
- **Purpose**: Displayed when the screen goes to sleep after timeout
- **Max Dimensions**: 200×150 pixels
- **Format**: PNG with transparency (alpha channel)
- **Max File Size**: 50 KB
- **Best for**: Sleep indicators, brand marks

## Automatic Image Processing

### What Happens When You Upload

The system automatically processes your images in **two stages**:

#### Stage 1: Client-Side Processing (Web Browser)
When you upload an image through the web interface:

1. **Format Detection**: Identifies your image format (PNG, JPG, BMP, etc.)
2. **Dimension Check**: Compares to maximum allowed dimensions
3. **Automatic Resizing**: Scales down while maintaining aspect ratio
4. **Format Conversion**: Converts to optimal format (PNG for logos, JPEG for backgrounds)
5. **Compression**: Applies smart compression to reduce file size
6. **Size Validation**: Ensures final file is within limits
7. **Base64 Encoding**: Converts to text format for upload

**Result**: You can upload ANY image format and size - the system handles it!

#### Stage 2: Device Processing (ESP32)
When the configuration is saved:

1. **Base64 Decoding**: Converts back to binary image data
2. **LVGL Integration**: Prepares image for display library
3. **Memory Optimization**: Loads efficiently to avoid memory issues

### Why This Matters

- **No More Size Errors**: Images are automatically resized to fit
- **No Manual Editing**: Upload directly from your camera or computer
- **Optimal Quality**: Smart compression maintains visual quality
- **Memory Safe**: System ensures images won't crash your device

## Using the Web Interface

### Accessing Image Upload

1. Connect to your device's web interface
2. Navigate to the **"Builder"** tab
3. Scroll down to the **"🖼️ Image Assets"** card

### Uploading Images

For each image type:

1. Click **"Choose File"** next to the image type
2. Select your image file (any format: PNG, JPG, BMP, GIF, etc.)
3. The system will automatically:
   - Show a processing message
   - Resize and optimize your image
   - Display a preview with dimensions and file size
   - Store the image in configuration

4. You'll see:
   - ✅ Success message if upload worked
   - ⚠️ Warning if image is too large (with suggestions)
   - ❌ Error message if there's a problem

### Previewing Images

After upload, you'll see:
- **Preview**: Actual image as it will appear
- **Dimensions**: Final width×height in pixels
- **File Size**: Final size in KB

### Clearing Images

To remove an uploaded image:
1. Click the **"Clear"** button next to the image type
2. The image is removed from configuration
3. System will use default or no image

### Saving Changes

After uploading all desired images:
1. Click **"Save Configuration"** at the top
2. Wait for confirmation message
3. **Reboot device** to see new images displayed

## Using the Python Tool

For offline processing or batch operations, use the included Python utility.

### Installation

```bash
# Install required library
pip install Pillow
```

### Basic Usage

#### Process a Single Image

```bash
# Process header logo
python image_manager.py my_logo.png header

# Process with background removal
python image_manager.py logo.png header --remove-bg

# Process splash screen
python image_manager.py splash.png splash --remove-bg

# Process background image
python image_manager.py photo.jpg background

# Process sleep logo
python image_manager.py sleep_icon.png sleep
```

#### Batch Process Multiple Images

Place all images in a folder with descriptive names:
- `my_header_logo.png`
- `company_splash.png`
- `background_image.jpg`
- `sleep_overlay.png`

Then run:

```bash
python image_manager.py --batch ./my_images/
```

The tool will automatically detect the type from filenames!

#### Advanced Options

```bash
# Custom output location
python image_manager.py logo.png header -o custom_output.png

# Adjust background removal tolerance
python image_manager.py logo.png header --remove-bg --tolerance 50

# Custom JPEG quality (1-100)
python image_manager.py photo.jpg background --quality 80

# Batch with custom output directory
python image_manager.py --batch ./input/ --output-dir ./processed/
```

### Python Tool Output

The tool generates:
1. **Optimized image file** (e.g., `logo_header.png`)
2. **Base64 text file** (e.g., `logo_header_base64.txt`)
3. **Processing report** with dimensions and file sizes

You can either:
- Upload the optimized image through the web interface
- Copy the base64 data URI and paste into your configuration

## Tips for Best Results

### Header Logo
✅ **Do:**
- Use simple, clean logos
- Ensure logo is legible at small size
- Use transparent background
- Test visibility on dark backgrounds

❌ **Don't:**
- Use photos or complex images
- Include fine text details
- Use very light colors on light backgrounds

### Splash Logo
✅ **Do:**
- Use high-contrast designs
- Center important elements
- Consider it will be centered on screen
- Use bold, clear graphics

❌ **Don't:**
- Fill entire area (leave breathing room)
- Use gradients that compress poorly
- Include critical text at edges

### Background Image
✅ **Do:**
- Use subtle, low-contrast backgrounds
- Consider readability of UI elements on top
- Use patterns or textures
- Test with your UI colors

❌ **Don't:**
- Use busy, high-contrast images
- Include important details that UI will cover
- Use very bright images (hard on eyes)
- Forget about button/text visibility

### Sleep Logo
✅ **Do:**
- Use simple, recognizable symbols
- Consider it appears on dark overlay
- Use white or light colors
- Make it obviously a "sleep" indicator

❌ **Don't:**
- Use detailed graphics
- Use dark colors (won't show well)
- Make it too large or distracting

## Troubleshooting

### "Image too large" Error

**Problem**: Image exceeds maximum file size after compression

**Solutions**:
1. **Use simpler image**: Remove gradients, reduce colors
2. **Lower quality**: Use `--quality 70` in Python tool
3. **Use correct format**: JPEG for photos, PNG for logos
4. **Reduce dimensions**: Start with smaller source image
5. **For backgrounds**: Use solid colors or simple patterns

### "Failed to process image" Error

**Problem**: Browser can't process the image

**Solutions**:
1. **Try different image**: File may be corrupted
2. **Convert format**: Save as PNG or JPEG in image editor
3. **Reduce size**: Make image smaller before uploading
4. **Use Python tool**: Process offline then upload result

### Image Not Displaying

**Problem**: Image uploaded but not showing on device

**Solutions**:
1. **Reboot device**: New images require restart
2. **Check enable flags**: Ensure "Show Logo" is checked
3. **Check memory**: Very large configs may have issues
4. **Verify upload**: Re-upload and save configuration

### Image Quality Poor

**Problem**: Image looks pixelated or blurry

**Solutions**:
1. **Use higher quality source**: Start with better original
2. **Match dimensions**: Use source close to target size
3. **Adjust quality**: Increase quality setting in Python tool
4. **Use PNG**: Better quality for logos/graphics

### Background Removal Not Working

**Problem**: White background still visible

**Solutions**:
1. **Adjust tolerance**: Use `--tolerance 50` or higher
2. **Use proper format**: Ensure PNG with alpha channel
3. **Pre-process**: Remove background in image editor first
4. **Check image**: May have off-white background

## Image Size Reference

| Type | Max Dimensions | Max File Size | Format | Has Alpha |
|------|---------------|---------------|--------|-----------|
| Header | 48×36 | 30 KB | PNG | Yes |
| Splash | 400×300 | 200 KB | PNG | Yes |
| Background | 800×480 | 300 KB | JPEG | No |
| Sleep | 200×150 | 50 KB | PNG | Yes |

## Memory Considerations

### Total Image Budget

The ESP32-S3 has limited memory. Guidelines:
- **Keep total image size under 500 KB**
- **Use 2-3 image types maximum**
- **Prefer smaller images when possible**
- **Monitor "heap" value in status API**

### What Uses Memory

- **Base64 in config**: ~1.33× original file size
- **Decoded image data**: ~1× original file size
- **Display buffer**: Additional overhead

### Optimization Tips

1. **Don't use all 4 types**: Pick 2-3 most important
2. **Background OR splash**: Usually not both
3. **Compress aggressively**: Lower quality for non-critical images
4. **Test on device**: Upload, reboot, check stability

## Frequently Asked Questions

### Can I use animated GIFs?
No, only static images are supported. Use first frame.

### Can I use vector graphics (SVG)?
No, must convert to raster format (PNG/JPG) first.

### Do I need to resize images myself?
No! The system does this automatically.

### Can I upload from my phone?
Yes! Use the web interface from any device with a browser.

### Will large images crash the device?
No, the system validates and rejects too-large images before saving.

### Can I use the same image for multiple types?
Yes, upload the same source and it will be optimized differently for each use.

### How do I restore default logos?
Click "Clear" for all uploaded images and save configuration.

### Do images persist across reboots?
Yes, images are saved in configuration file.

### Can I edit images on the device?
No, upload new images through web interface or Python tool.

### What's the fastest way to test images?
1. Upload through web interface
2. Save configuration
3. Reboot device
4. Check display

## Examples

### Example 1: Complete Brand Setup

```bash
# Prepare all images in one batch
cd my_brand/
ls -l
# header_logo.png
# splash_screen.png
# dashboard_background.jpg

# Process all at once
python ../image_manager.py --batch ./ --output-dir ./optimized/

# Upload through web interface:
# 1. Open web interface
# 2. Go to Builder tab
# 3. Upload each optimized image
# 4. Save configuration
# 5. Reboot device
```

### Example 2: Quick Header Logo Update

```bash
# Process logo with background removal
python image_manager.py new_logo.png header --remove-bg

# Output:
# new_logo_header.png (optimized)
# new_logo_header_base64.txt (for pasting)

# Option A: Upload the PNG through web interface
# Option B: Copy content of base64.txt and paste in manual config
```

### Example 3: Custom Background

```bash
# High-quality background with custom quality
python image_manager.py photo.jpg background --quality 90

# If too large, retry with lower quality
python image_manager.py photo.jpg background --quality 70

# Upload through web interface when size is acceptable
```

## Support

If you encounter issues:

1. **Check file sizes**: Ensure within limits
2. **Verify format**: PNG for logos, JPEG for backgrounds
3. **Test with simple image**: Validate system works
4. **Check device logs**: Serial output shows processing
5. **Review this guide**: Common solutions documented above

## Version History

- **v2.0** (2024-12): Complete image management system
  - Four image types supported
  - Automatic client-side resizing
  - Python batch processing tool
  - Memory-safe implementation

---

**Need help?** Check the troubleshooting section or consult device documentation.
