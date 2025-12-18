# Logo Preparation Tool for Bronco Controls

This Python script prepares logo images for optimal display on your ESP32-S3 device.

## Installation

1. Install Python 3.7 or newer
2. Install Pillow library:
```bash
pip install Pillow
```

## Usage

### Basic Usage
```bash
python prepare_logo.py your_logo.png
```

This will:
- Resize to 48×36 pixels (maintains aspect ratio)
- Remove white background (makes transparent)
- Optimize file size
- Generate PNG file and base64 text file

### Output Files
- `your_logo_prepared.png` - Optimized PNG image
- `your_logo_prepared_base64.txt` - Base64 data URI (paste this into web interface)

### Advanced Options

**Keep original background:**
```bash
python prepare_logo.py logo.png --no-remove-bg
```

**Custom dimensions:**
```bash
python prepare_logo.py logo.png --width 40 --height 30
```

**Adjust background removal sensitivity:**
```bash
python prepare_logo.py logo.png --tolerance 50
```
(Higher = removes more colors, Lower = only pure white)

**Specify output file:**
```bash
python prepare_logo.py logo.png -o output.png
```

## Tips for Best Results

1. **Start with a simple logo** - Complex designs with gradients create large files
2. **Use solid colors** - Fewer colors = smaller file size
3. **SVG to PNG first** - Convert vector logos to PNG before processing
4. **Check the output** - The script warns if file is too large (>10KB)

## Examples

### Company logo with white background:
```bash
python prepare_logo.py company_logo.jpg
```

### Icon that should keep its background:
```bash
python prepare_logo.py icon.png --no-remove-bg
```

### Very small logo for minimal size:
```bash
python prepare_logo.py logo.png --width 32 --height 24
```

## Using the Output

1. Run the script on your logo
2. Open the `*_base64.txt` file
3. Copy the entire contents
4. In the web interface, paste into the logo upload field
5. Save configuration

The prepared logo will display properly on any background color!

## Troubleshooting

**"Logo too large" warning:**
- Try reducing dimensions: `--width 40 --height 30`
- Simplify the logo design
- Use fewer colors

**Background not removed:**
- Increase tolerance: `--tolerance 60`
- Manually edit image to have pure white background first

**Logo looks pixelated:**
- Start with a higher resolution source image
- Increase max dimensions slightly (but watch file size)
