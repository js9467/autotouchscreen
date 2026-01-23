# Image Manager - Quick Reference

Comprehensive image processing utility for Bronco Controls ESP32-S3 display.

## Quick Start

```bash
# Install dependencies
pip install Pillow

# Process a single image
python image_manager.py your_image.png header

# Batch process a folder
python image_manager.py --batch ./images/
```

## Image Types

| Type | Size | Format | Alpha | Max Size | Purpose |
|------|------|--------|-------|----------|---------|
| `header` | 48×36 | PNG | ✓ | 30 KB | Header bar logo |
| `splash` | 400×300 | PNG | ✓ | 200 KB | Startup screen |
| `background` | 800×480 | JPEG | ✗ | 300 KB | Full screen background |
| `sleep` | 200×150 | PNG | ✓ | 50 KB | Sleep overlay |

## Common Commands

### Process Specific Image Types

```bash
# Header logo (with background removal)
python image_manager.py logo.png header --remove-bg

# Splash screen
python image_manager.py splash.png splash --remove-bg

# Background image (JPEG, custom quality)
python image_manager.py photo.jpg background --quality 80

# Sleep overlay
python image_manager.py icon.png sleep --remove-bg
```

### Batch Processing

Automatically detects type from filename keywords:

```bash
# Process all images in directory
python image_manager.py --batch ./my_images/

# With custom output directory
python image_manager.py --batch ./input/ --output-dir ./output/
```

**Filename patterns:**
- Contains `header` → header logo
- Contains `splash` → splash logo
- Contains `background` or `bg` → background image
- Contains `sleep` → sleep logo

### Advanced Options

```bash
# Custom output file
python image_manager.py input.png header -o custom_name.png

# Adjust background removal tolerance (0-255)
python image_manager.py logo.png header --remove-bg --tolerance 50

# Custom JPEG quality (1-100, default 85)
python image_manager.py photo.jpg background --quality 90
```

## Output Files

For input `logo.png` → type `header`:

1. **Optimized image**: `logo_header.png`
2. **Base64 data**: `logo_header_base64.txt`

### Using Output Files

**Option 1: Upload through web interface**
- Use the optimized PNG file
- Upload via "Builder" tab → "Image Assets"

**Option 2: Manual configuration**
- Copy contents of `_base64.txt` file
- Paste into configuration JSON

## Features

✅ **Automatic resizing** - Maintains aspect ratio  
✅ **Smart compression** - Optimizes file size  
✅ **Format conversion** - PNG for logos, JPEG for backgrounds  
✅ **Background removal** - Transparent logo backgrounds  
✅ **Size validation** - Prevents oversized uploads  
✅ **Batch processing** - Process multiple images at once  
✅ **Base64 encoding** - Ready for web upload  

## Examples

### Example 1: Create Complete Image Set

```bash
# Create a folder with your images
mkdir my_images
# Put files: header_logo.png, splash_screen.png, background.jpg, sleep_icon.png

# Process all at once
python image_manager.py --batch ./my_images/

# Upload optimized files through web interface
```

### Example 2: Single Logo with Background Removal

```bash
python image_manager.py company_logo.png header --remove-bg --tolerance 40

# Output shows:
# - Processing steps
# - Final dimensions
# - File sizes
# - Success/warning messages

# Files created:
# - company_logo_header.png (upload this)
# - company_logo_header_base64.txt (or paste this)
```

### Example 3: High-Quality Background

```bash
# Try maximum quality first
python image_manager.py photo.jpg background --quality 95

# If too large (>300KB), reduce quality
python image_manager.py photo.jpg background --quality 75
```

## Troubleshooting

### Image Too Large Error

**Symptom**: "Image too large: XXX KB (max: XXX KB)"

**Solutions**:
- Reduce quality: `--quality 70`
- Use simpler image (fewer colors, less detail)
- For backgrounds, use solid colors or simple patterns
- Start with smaller source image

### Background Removal Not Working

**Symptom**: White background still visible

**Solutions**:
- Increase tolerance: `--tolerance 50` (or higher)
- Verify image has white background (not off-white)
- Pre-process in image editor (GIMP, Photoshop, Paint.NET)

### Module Not Found Error

**Symptom**: "No module named 'PIL'"

**Solution**:
```bash
pip install Pillow
```

## Tips for Best Results

### Header Logo (48×36)
- Use simple, bold designs
- High contrast colors
- Transparent background
- Test legibility at small size

### Splash Logo (400×300)
- Center important elements
- Leave margin around edges
- Use bold graphics
- High contrast

### Background (800×480)
- Subtle, low-contrast preferred
- Consider UI readability
- Patterns/textures work well
- Test with actual UI elements

### Sleep Logo (200×150)
- Simple, recognizable symbols
- Light colors (shows on dark overlay)
- Clear purpose/meaning
- Not too large

## Getting Help

```bash
# Show help message
python image_manager.py --help

# Shows:
# - All available options
# - Usage examples
# - Image type descriptions
```

## Integration with Web Interface

1. **Process images** with this tool
2. **Upload** through web interface:
   - Navigate to "Builder" tab
   - Find "🖼️ Image Assets" card
   - Click "Choose File" for each type
   - Upload optimized PNGs/JPEGs
3. **Save** configuration
4. **Reboot** device to see changes

## Memory Considerations

- ESP32-S3 has **limited memory**
- **Keep total images under 500 KB**
- Use **2-3 image types** (not all 4)
- Prefer **smaller sizes** when possible

## Version

Image Manager v2.0 - Part of Bronco Controls project

---

**For detailed documentation**, see [IMAGE_MANAGEMENT_GUIDE.md](IMAGE_MANAGEMENT_GUIDE.md)
