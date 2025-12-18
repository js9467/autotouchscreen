#!/usr/bin/env python3
"""
Logo Preparation Script for Bronco Controls
Prepares images for optimal display on the ESP32-S3 device:
- Resizes to fit header (48x36 max)
- Adds transparency/removes background
- Optimizes file size
- Converts to base64 for web upload
"""

import sys
import base64
from pathlib import Path

try:
    from PIL import Image, ImageOps
except ImportError:
    print("Error: Pillow library not found.")
    print("Install it with: pip install Pillow")
    sys.exit(1)


def remove_background(img, tolerance=30):
    """Remove white or near-white backgrounds and make them transparent."""
    img = img.convert("RGBA")
    datas = img.getdata()
    
    new_data = []
    for item in datas:
        # Check if pixel is near white (adjust tolerance as needed)
        if item[0] > 255 - tolerance and item[1] > 255 - tolerance and item[2] > 255 - tolerance:
            # Make it transparent
            new_data.append((255, 255, 255, 0))
        else:
            new_data.append(item)
    
    img.putdata(new_data)
    return img


def prepare_logo(input_path, output_path=None, max_width=48, max_height=36, 
                 remove_bg=True, bg_tolerance=30):
    """
    Prepare a logo image for Bronco Controls.
    
    Args:
        input_path: Path to input image
        output_path: Path to save output (optional, defaults to input_prepared.png)
        max_width: Maximum width in pixels (default 48)
        max_height: Maximum height in pixels (default 36)
        remove_bg: Whether to remove white background (default True)
        bg_tolerance: Tolerance for background removal (0-255, default 30)
    
    Returns:
        Path to output file
    """
    input_path = Path(input_path)
    
    if not input_path.exists():
        raise FileNotFoundError(f"Input file not found: {input_path}")
    
    # Set output path
    if output_path is None:
        output_path = input_path.parent / f"{input_path.stem}_prepared.png"
    else:
        output_path = Path(output_path)
    
    print(f"Loading image: {input_path}")
    img = Image.open(input_path)
    
    # Convert to RGBA if not already
    if img.mode != 'RGBA':
        print(f"Converting from {img.mode} to RGBA")
        img = img.convert('RGBA')
    
    # Remove background if requested
    if remove_bg:
        print(f"Removing white background (tolerance: {bg_tolerance})...")
        img = remove_background(img, tolerance=bg_tolerance)
    
    # Calculate new size maintaining aspect ratio
    original_width, original_height = img.size
    ratio = min(max_width / original_width, max_height / original_height)
    new_width = int(original_width * ratio)
    new_height = int(original_height * ratio)
    
    print(f"Resizing from {original_width}x{original_height} to {new_width}x{new_height}")
    
    # Use high-quality resampling
    img = img.resize((new_width, new_height), Image.Resampling.LANCZOS)
    
    # Optimize and save
    print(f"Saving optimized image: {output_path}")
    img.save(output_path, 'PNG', optimize=True)
    
    # Check file size
    file_size = output_path.stat().st_size
    print(f"Output file size: {file_size:,} bytes")
    
    # Convert to base64 and check size
    with open(output_path, 'rb') as f:
        img_data = f.read()
        b64_data = base64.b64encode(img_data).decode('utf-8')
        b64_size = len(b64_data)
        data_uri = f"data:image/png;base64,{b64_data}"
        data_uri_size = len(data_uri)
    
    print(f"Base64 size: {b64_size:,} bytes")
    print(f"Data URI size: {data_uri_size:,} bytes")
    
    if data_uri_size > 10000:
        print("\n⚠️  WARNING: Data URI is larger than 10KB!")
        print("   This may cause upload failures. Try:")
        print("   1. Reducing max_width/max_height")
        print("   2. Simplifying the logo design")
        print("   3. Using fewer colors")
    else:
        print("\n✅ Image size is good for upload!")
    
    # Save base64 to text file
    b64_output = output_path.parent / f"{output_path.stem}_base64.txt"
    with open(b64_output, 'w') as f:
        f.write(data_uri)
    print(f"\nBase64 data URI saved to: {b64_output}")
    print("You can paste this directly into the web interface.")
    
    return output_path


def main():
    """Command line interface."""
    import argparse
    
    parser = argparse.ArgumentParser(
        description='Prepare logo images for Bronco Controls',
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
  # Basic usage with default settings (48x36, remove white background)
  python prepare_logo.py my_logo.png
  
  # Keep background
  python prepare_logo.py my_logo.png --no-remove-bg
  
  # Custom size
  python prepare_logo.py my_logo.png --width 40 --height 30
  
  # Adjust background removal tolerance
  python prepare_logo.py my_logo.png --tolerance 50
  
  # Specify output file
  python prepare_logo.py my_logo.png -o output.png
        """
    )
    
    parser.add_argument('input', help='Input image file')
    parser.add_argument('-o', '--output', help='Output file path (optional)')
    parser.add_argument('--width', type=int, default=48, 
                        help='Maximum width in pixels (default: 48)')
    parser.add_argument('--height', type=int, default=36,
                        help='Maximum height in pixels (default: 36)')
    parser.add_argument('--no-remove-bg', action='store_true',
                        help='Do not remove white background')
    parser.add_argument('--tolerance', type=int, default=30,
                        help='Background removal tolerance 0-255 (default: 30)')
    
    args = parser.parse_args()
    
    try:
        output_path = prepare_logo(
            args.input,
            output_path=args.output,
            max_width=args.width,
            max_height=args.height,
            remove_bg=not args.no_remove_bg,
            bg_tolerance=args.tolerance
        )
        print(f"\n✅ Success! Logo prepared: {output_path}")
        return 0
    except Exception as e:
        print(f"\n❌ Error: {e}", file=sys.stderr)
        return 1


if __name__ == '__main__':
    sys.exit(main())
