#!/usr/bin/env python3
"""
Image preparation tool for Bronco Controls
Resizes and optimizes images for ESP32 display
"""

import sys
from pathlib import Path
from PIL import Image
import io

def prepare_image(input_path, output_path, max_width, max_height, quality=85):
    """
    Resize and optimize an image for ESP32 use
    
    Args:
        input_path: Path to input image
        output_path: Path to save optimized image
        max_width: Maximum width in pixels
        max_height: Maximum height in pixels
        quality: JPEG quality (1-95)
    """
    try:
        img = Image.open(input_path)
        
        # Convert to RGB if needed
        if img.mode not in ('RGB', 'L'):
            img = img.convert('RGB')
        
        # Calculate new size maintaining aspect ratio
        img.thumbnail((max_width, max_height), Image.Resampling.LANCZOS)
        
        # Determine output format
        ext = Path(output_path).suffix.lower()
        if ext == '.png':
            # Optimize PNG
            img.save(output_path, 'PNG', optimize=True)
        else:
            # Save as JPEG with quality
            img.save(output_path, 'JPEG', quality=quality, optimize=True)
        
        # Check file size
        size_kb = Path(output_path).stat().st_size / 1024
        print(f"✓ Created {output_path}")
        print(f"  Size: {img.size[0]}x{img.size[1]} pixels, {size_kb:.1f} KB")
        
        if size_kb > 50:
            print(f"  ⚠️  Warning: File is large ({size_kb:.1f} KB). Consider reducing quality or size.")
        
        return True
        
    except Exception as e:
        print(f"✗ Error processing {input_path}: {e}")
        return False


def main():
    if len(sys.argv) < 3:
        print("Usage: python prepare_images.py <input_image> <output_image> [type]")
        print()
        print("Types and recommended sizes:")
        print("  header     - 48x48   Header logo")
        print("  splash     - 240x135 Splash screen logo")
        print("  background - 320x240 Background image")
        print("  sleep      - 64x64   Sleep icon")
        print()
        print("Examples:")
        print("  python prepare_images.py mylogo.png header_logo.png header")
        print("  python prepare_images.py splash.jpg splash_logo.jpg splash")
        return
    
    input_path = sys.argv[1]
    output_path = sys.argv[2]
    image_type = sys.argv[3] if len(sys.argv) > 3 else "custom"
    
    # Recommended sizes for each type
    sizes = {
        'header': (48, 48),
        'splash': (240, 135),
        'background': (320, 240),
        'sleep': (64, 64)
    }
    
    if image_type in sizes:
        max_w, max_h = sizes[image_type]
    else:
        max_w, max_h = 320, 240
    
    print(f"Preparing {image_type} image...")
    print(f"Max size: {max_w}x{max_h}")
    
    if prepare_image(input_path, output_path, max_w, max_h, quality=80):
        print()
        print("Image ready! Upload it through the web interface.")
        print("Go to: Config Page → Image Assets section")
    else:
        sys.exit(1)


if __name__ == "__main__":
    main()
