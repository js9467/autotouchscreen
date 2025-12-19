#!/usr/bin/env python3
"""
Image Manager for Bronco Controls
Comprehensive image processing utility that automatically resizes and optimizes
images for different purposes on the ESP32-S3 device.

Supported image types:
- Header Logo: 48x36px max, PNG with alpha, for display header
- Splash Logo: 400x300px max, PNG with alpha, for startup screen
- Background: 800x480px, JPG or PNG, full screen background
- Sleep Logo: 200x150px max, PNG with alpha, for sleep overlay

Features:
- Auto-resize while maintaining aspect ratio
- Smart compression and optimization
- Base64 encoding for web uploads
- Size validation and warnings
- Batch processing support
"""

import sys
import base64
import argparse
from pathlib import Path
from typing import Tuple, Optional

try:
    from PIL import Image, ImageOps
except ImportError:
    print("Error: Pillow library not found.")
    print("Install it with: pip install Pillow")
    sys.exit(1)


# Image type configurations
IMAGE_CONFIGS = {
    'header': {
        'max_size': (48, 36),
        'format': 'PNG',
        'has_alpha': True,
        'max_file_size': 30 * 1024,  # 30KB
        'description': 'Header logo (appears in top bar)'
    },
    'splash': {
        'max_size': (400, 300),
        'format': 'PNG',
        'has_alpha': True,
        'max_file_size': 200 * 1024,  # 200KB
        'description': 'Splash screen logo (startup screen)'
    },
    'background': {
        'max_size': (800, 480),
        'format': 'JPEG',
        'has_alpha': False,
        'max_file_size': 300 * 1024,  # 300KB
        'description': 'Background image (full screen)'
    },
    'sleep': {
        'max_size': (200, 150),
        'format': 'PNG',
        'has_alpha': True,
        'max_file_size': 50 * 1024,  # 50KB
        'description': 'Sleep overlay logo (idle screen)'
    }
}


def remove_background(img: Image.Image, tolerance: int = 30) -> Image.Image:
    """
    Remove white or near-white backgrounds and make them transparent.
    
    Args:
        img: PIL Image object
        tolerance: Color threshold for background removal (0-255)
    
    Returns:
        Image with transparent background
    """
    img = img.convert("RGBA")
    datas = img.getdata()
    
    new_data = []
    for item in datas:
        # Check if pixel is near white
        if item[0] > 255 - tolerance and item[1] > 255 - tolerance and item[2] > 255 - tolerance:
            new_data.append((255, 255, 255, 0))  # Transparent
        else:
            new_data.append(item)
    
    img.putdata(new_data)
    return img


def resize_with_aspect_ratio(img: Image.Image, max_size: Tuple[int, int]) -> Image.Image:
    """
    Resize image while maintaining aspect ratio.
    
    Args:
        img: PIL Image object
        max_size: Maximum dimensions (width, height)
    
    Returns:
        Resized image
    """
    original_width, original_height = img.size
    max_width, max_height = max_size
    
    # Calculate scaling ratio
    ratio = min(max_width / original_width, max_height / original_height)
    
    # Don't upscale images
    if ratio > 1:
        ratio = 1
    
    new_width = int(original_width * ratio)
    new_height = int(original_height * ratio)
    
    print(f"  Resizing: {original_width}x{original_height} → {new_width}x{new_height}")
    
    # Use high-quality resampling
    return img.resize((new_width, new_height), Image.Resampling.LANCZOS)


def optimize_image(img: Image.Image, image_type: str, quality: int = 85) -> Tuple[bytes, str]:
    """
    Optimize image and return as bytes.
    
    Args:
        img: PIL Image object
        image_type: Type of image (header, splash, background, sleep)
        quality: JPEG quality (1-100)
    
    Returns:
        Tuple of (image_bytes, mime_type)
    """
    config = IMAGE_CONFIGS[image_type]
    format_str = config['format']
    
    from io import BytesIO
    buffer = BytesIO()
    
    if format_str == 'PNG':
        # Optimize PNG
        img.save(buffer, 'PNG', optimize=True, compress_level=9)
        mime_type = 'image/png'
    else:
        # Save as JPEG
        if img.mode == 'RGBA':
            # Convert RGBA to RGB for JPEG
            background = Image.new('RGB', img.size, (255, 255, 255))
            background.paste(img, mask=img.split()[3] if len(img.split()) == 4 else None)
            img = background
        elif img.mode != 'RGB':
            img = img.convert('RGB')
        
        img.save(buffer, 'JPEG', quality=quality, optimize=True)
        mime_type = 'image/jpeg'
    
    return buffer.getvalue(), mime_type


def process_image(
    input_path: str,
    image_type: str,
    output_path: Optional[str] = None,
    remove_bg: bool = False,
    bg_tolerance: int = 30,
    quality: int = 85
) -> dict:
    """
    Process an image for a specific use case.
    
    Args:
        input_path: Path to input image
        image_type: Type of image (header, splash, background, sleep)
        output_path: Optional output path
        remove_bg: Whether to remove white background
        bg_tolerance: Background removal tolerance
        quality: JPEG quality (1-100)
    
    Returns:
        Dictionary with processing results
    """
    if image_type not in IMAGE_CONFIGS:
        raise ValueError(f"Invalid image type: {image_type}. Must be one of: {', '.join(IMAGE_CONFIGS.keys())}")
    
    config = IMAGE_CONFIGS[image_type]
    input_path = Path(input_path)
    
    if not input_path.exists():
        raise FileNotFoundError(f"Input file not found: {input_path}")
    
    print(f"\n{'='*60}")
    print(f"Processing {image_type.upper()} image: {input_path.name}")
    print(f"Target: {config['description']}")
    print(f"Max size: {config['max_size'][0]}x{config['max_size'][1]}")
    print(f"{'='*60}")
    
    # Load image
    print("\n[1/5] Loading image...")
    img = Image.open(input_path)
    print(f"  Original: {img.size[0]}x{img.size[1]}, {img.mode}")
    
    # Convert to appropriate mode
    print("\n[2/5] Converting color mode...")
    if config['has_alpha'] and img.mode != 'RGBA':
        img = img.convert('RGBA')
        print(f"  Converted to RGBA")
    elif not config['has_alpha'] and img.mode not in ['RGB', 'L']:
        if img.mode == 'RGBA':
            # Convert RGBA to RGB
            background = Image.new('RGB', img.size, (255, 255, 255))
            background.paste(img, mask=img.split()[3])
            img = background
        else:
            img = img.convert('RGB')
        print(f"  Converted to RGB")
    
    # Remove background if requested
    if remove_bg and config['has_alpha']:
        print(f"\n[3/5] Removing background (tolerance: {bg_tolerance})...")
        img = remove_background(img, tolerance=bg_tolerance)
    else:
        print(f"\n[3/5] Keeping original background")
    
    # Resize image
    print(f"\n[4/5] Resizing to fit {config['max_size'][0]}x{config['max_size'][1]}...")
    img = resize_with_aspect_ratio(img, config['max_size'])
    
    # Optimize and save
    print(f"\n[5/5] Optimizing and encoding...")
    img_bytes, mime_type = optimize_image(img, image_type, quality)
    file_size = len(img_bytes)
    
    # Check file size
    max_size = config['max_file_size']
    size_ok = file_size <= max_size
    
    print(f"  File size: {file_size:,} bytes ({file_size/1024:.1f} KB)")
    print(f"  Max allowed: {max_size:,} bytes ({max_size/1024:.1f} KB)")
    
    if not size_ok:
        print(f"  ⚠️  WARNING: Image exceeds recommended size!")
        if quality > 70:
            print(f"  Try reducing quality (current: {quality})")
        if image_type == 'background':
            print(f"  Or try using a simpler background image")
    else:
        print(f"  ✅ Size is within limits")
    
    # Save to file if requested
    if output_path:
        output_path = Path(output_path)
        output_path.parent.mkdir(parents=True, exist_ok=True)
        with open(output_path, 'wb') as f:
            f.write(img_bytes)
        print(f"\n  Saved to: {output_path}")
    else:
        # Auto-generate output path
        suffix = f"_{image_type}"
        ext = '.png' if config['format'] == 'PNG' else '.jpg'
        output_path = input_path.parent / f"{input_path.stem}{suffix}{ext}"
        with open(output_path, 'wb') as f:
            f.write(img_bytes)
        print(f"\n  Saved to: {output_path}")
    
    # Generate base64
    b64_data = base64.b64encode(img_bytes).decode('utf-8')
    data_uri = f"data:{mime_type};base64,{b64_data}"
    data_uri_size = len(data_uri)
    
    print(f"\n  Base64 size: {len(b64_data):,} bytes ({len(b64_data)/1024:.1f} KB)")
    print(f"  Data URI size: {data_uri_size:,} bytes ({data_uri_size/1024:.1f} KB)")
    
    # Save base64 to text file
    b64_output = output_path.parent / f"{output_path.stem}_base64.txt"
    with open(b64_output, 'w') as f:
        f.write(data_uri)
    print(f"  Base64 data URI: {b64_output}")
    
    print(f"\n{'='*60}")
    print(f"✅ Processing complete!")
    print(f"{'='*60}\n")
    
    return {
        'output_path': str(output_path),
        'base64_path': str(b64_output),
        'file_size': file_size,
        'base64_size': len(b64_data),
        'data_uri_size': data_uri_size,
        'dimensions': img.size,
        'mime_type': mime_type,
        'size_ok': size_ok
    }


def batch_process(input_dir: str, output_dir: str = None):
    """
    Batch process images from a directory.
    
    Automatically detects image type based on filename patterns:
    - *header*.*     → header
    - *splash*.*     → splash
    - *background*.* → background
    - *sleep*.*      → sleep
    
    Args:
        input_dir: Directory containing images
        output_dir: Optional output directory
    """
    input_path = Path(input_dir)
    if not input_path.is_dir():
        raise ValueError(f"Not a directory: {input_dir}")
    
    output_path = Path(output_dir) if output_dir else input_path / 'processed'
    output_path.mkdir(parents=True, exist_ok=True)
    
    # Supported extensions
    extensions = ('.png', '.jpg', '.jpeg', '.bmp', '.gif')
    
    # Find all images
    images = [f for f in input_path.iterdir() if f.suffix.lower() in extensions]
    
    if not images:
        print(f"No images found in {input_dir}")
        return
    
    print(f"\nFound {len(images)} image(s) to process")
    print(f"Output directory: {output_path}\n")
    
    results = []
    
    for img_file in images:
        # Detect type from filename
        name_lower = img_file.stem.lower()
        
        if 'header' in name_lower:
            img_type = 'header'
        elif 'splash' in name_lower:
            img_type = 'splash'
        elif 'background' in name_lower or 'bg' in name_lower:
            img_type = 'background'
        elif 'sleep' in name_lower:
            img_type = 'sleep'
        else:
            print(f"⚠️  Skipping {img_file.name} - cannot detect type from filename")
            print(f"   (Use keywords: header, splash, background, or sleep)\n")
            continue
        
        try:
            result = process_image(
                str(img_file),
                img_type,
                output_path=str(output_path / f"{img_file.stem}_{img_type}{'.png' if IMAGE_CONFIGS[img_type]['format'] == 'PNG' else '.jpg'}"),
                remove_bg=(img_type in ['header', 'splash', 'sleep']),
                quality=85
            )
            results.append((img_file.name, img_type, result))
        except Exception as e:
            print(f"❌ Error processing {img_file.name}: {e}\n")
    
    # Print summary
    if results:
        print(f"\n{'='*60}")
        print(f"BATCH PROCESSING SUMMARY")
        print(f"{'='*60}")
        for filename, img_type, result in results:
            status = "✅" if result['size_ok'] else "⚠️"
            print(f"{status} {filename}")
            print(f"   Type: {img_type}")
            print(f"   Size: {result['file_size']/1024:.1f} KB")
            print(f"   Dimensions: {result['dimensions'][0]}x{result['dimensions'][1]}")
        print(f"{'='*60}\n")


def main():
    """Command line interface."""
    parser = argparse.ArgumentParser(
        description='Image Manager for Bronco Controls - Process images for ESP32 display',
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Image Types:
  header     - Header logo (48x36px, PNG with alpha, 30KB max)
  splash     - Splash screen logo (400x300px, PNG with alpha, 200KB max)
  background - Background image (800x480px, JPEG, 300KB max)
  sleep      - Sleep overlay logo (200x150px, PNG with alpha, 50KB max)

Examples:
  # Process a header logo
  python image_manager.py my_logo.png header
  
  # Process with background removal
  python image_manager.py my_logo.png header --remove-bg
  
  # Process a background image with custom quality
  python image_manager.py background.jpg background --quality 80
  
  # Batch process a directory
  python image_manager.py --batch ./my_images/
  
  # Process all types from one image (testing)
  python image_manager.py logo.png header --output header_logo.png
  python image_manager.py logo.png splash --output splash_logo.png
        """
    )
    
    parser.add_argument('input', help='Input image file or directory (for batch mode)')
    parser.add_argument('type', nargs='?', choices=['header', 'splash', 'background', 'sleep'],
                        help='Image type to process')
    parser.add_argument('-o', '--output', help='Output file path (optional)')
    parser.add_argument('--remove-bg', action='store_true',
                        help='Remove white background (for PNG images with alpha)')
    parser.add_argument('--tolerance', type=int, default=30,
                        help='Background removal tolerance 0-255 (default: 30)')
    parser.add_argument('--quality', type=int, default=85,
                        help='JPEG quality 1-100 (default: 85)')
    parser.add_argument('--batch', action='store_true',
                        help='Batch process all images in directory')
    parser.add_argument('--output-dir', help='Output directory for batch mode')
    
    args = parser.parse_args()
    
    try:
        if args.batch:
            batch_process(args.input, args.output_dir)
        else:
            if not args.type:
                print("Error: Image type is required (use --batch for batch processing)")
                print("Choose from: header, splash, background, sleep")
                return 1
            
            result = process_image(
                args.input,
                args.type,
                output_path=args.output,
                remove_bg=args.remove_bg,
                bg_tolerance=args.tolerance,
                quality=args.quality
            )
            
            print("📋 You can now:")
            print("   1. Use the optimized image file directly")
            print("   2. Copy the base64 data URI for web upload")
            print("   3. Upload through the web configuration interface")
        
        return 0
    except Exception as e:
        print(f"\n❌ Error: {e}", file=sys.stderr)
        import traceback
        traceback.print_exc()
        return 1


if __name__ == '__main__':
    sys.exit(main())
