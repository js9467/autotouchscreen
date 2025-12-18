#!/usr/bin/env python3
"""
Convert PNG/JPG images to LVGL C arrays
Generates C files compatible with LVGL v8.3
"""

import sys
from PIL import Image
import struct
import os

def resize_image(img, target_size):
    """Resize image while maintaining aspect ratio, then crop to exact size"""
    img.thumbnail(target_size, Image.Resampling.LANCZOS)
    
    # Create a new image with the target size and paste the resized image centered
    new_img = Image.new('RGBA', target_size, (0, 0, 0, 0))
    paste_x = (target_size[0] - img.width) // 2
    paste_y = (target_size[1] - img.height) // 2
    new_img.paste(img, (paste_x, paste_y))
    
    return new_img

def convert_to_lvgl_c(img, var_name, has_alpha=False):
    """Convert PIL Image to LVGL C array format"""
    width = img.width
    height = img.height
    
    # Always convert to RGBA for processing (alpha will be discarded for RGB565)
    if img.mode != 'RGBA':
        img = img.convert('RGBA')
    
    pixels = img.load()
    
    # Generate C array
    c_code = f"""/**
 * @file {var_name}.c
 * LVGL image data - auto-generated
 */

#include <lvgl.h>

#ifndef LV_ATTRIBUTE_MEM_ALIGN
#define LV_ATTRIBUTE_MEM_ALIGN
#endif

"""
    
    if has_alpha:
        # True color with alpha (ARGB8888 / CF_TRUE_COLOR_ALPHA)
        c_code += f"""#ifndef LV_ATTRIBUTE_IMG_{var_name.upper()}
#define LV_ATTRIBUTE_IMG_{var_name.upper()}
#endif

const LV_ATTRIBUTE_MEM_ALIGN LV_ATTRIBUTE_IMG_{var_name.upper()} uint8_t {var_name}_map[] = {{
"""
        
        # Write pixel data (ARGB8888 format: Alpha, Red, Green, Blue)
        for y in range(height):
            c_code += "  "
            for x in range(width):
                r, g, b, a = pixels[x, y]
                # LVGL expects: 0xAARRGGBB but stores as bytes in memory
                c_code += f"0x{b:02x},0x{g:02x},0x{r:02x},0x{a:02x}, "
                
                if (x + 1) % 4 == 0:  # Line break every 4 pixels for readability
                    c_code += "\n  "
            c_code += "\n"
        
        c_code += "};\n\n"
        
        # Create image descriptor
        c_code += f"""const lv_img_dsc_t {var_name} = {{
  .header.cf = LV_IMG_CF_TRUE_COLOR_ALPHA,
  .header.always_zero = 0,
  .header.reserved = 0,
  .header.w = {width},
  .header.h = {height},
  .data_size = {width * height * 4},
  .data = {var_name}_map,
}};
"""
    else:
        # True color without alpha (RGB565 / CF_TRUE_COLOR)
        c_code += f"""#ifndef LV_ATTRIBUTE_IMG_{var_name.upper()}
#define LV_ATTRIBUTE_IMG_{var_name.upper()}
#endif

const LV_ATTRIBUTE_MEM_ALIGN LV_ATTRIBUTE_IMG_{var_name.upper()} uint8_t {var_name}_map[] = {{
"""
        
        # Write pixel data (RGB565 format: 5 bits R, 6 bits G, 5 bits B)
        for y in range(height):
            c_code += "  "
            for x in range(width):
                r, g, b, _ = pixels[x, y]
                # Convert to RGB565
                r5 = (r >> 3) & 0x1F
                g6 = (g >> 2) & 0x3F
                b5 = (b >> 3) & 0x1F
                rgb565 = (r5 << 11) | (g6 << 5) | b5
                
                # Store as little-endian bytes
                c_code += f"0x{rgb565 & 0xFF:02x},0x{(rgb565 >> 8) & 0xFF:02x}, "
                
                if (x + 1) % 8 == 0:  # Line break every 8 pixels
                    c_code += "\n  "
            c_code += "\n"
        
        c_code += "};\n\n"
        
        # Create image descriptor
        c_code += f"""const lv_img_dsc_t {var_name} = {{
  .header.cf = LV_IMG_CF_TRUE_COLOR,
  .header.always_zero = 0,
  .header.reserved = 0,
  .header.w = {width},
  .header.h = {height},
  .data_size = {width * height * 2},
  .data = {var_name}_map,
}};
"""
    
    return c_code

def main():
    """Convert all assets to LVGL format"""
    
    assets_dir = "assets"
    output_dir = "src/assets"
    
    # Ensure output directory exists
    os.makedirs(output_dir, exist_ok=True)
    
    # Image specifications
    conversions = [
        # (input_file, output_var_name, target_size, has_alpha)
        ("home_icon.PNG", "img_home_icon", (80, 80), False),
        ("windows_icon.PNG", "img_windows_icon", (80, 80), False),
        ("locks_icon.PNG", "img_locks_icon", (80, 80), False),
        ("runningboards_icon.PNG", "img_runningboards_icon", (80, 80), False),
        # Skip background - too large!
        # ("broncobackground.png", "img_background", (800, 480), False),
        ("bronco_logo.jpg", "img_bronco_logo", (120, 80), False),
    ]
    
    print("🎨 Converting images to LVGL C arrays...\n")
    
    for input_file, var_name, target_size, has_alpha in conversions:
        input_path = os.path.join(assets_dir, input_file)
        output_path = os.path.join(output_dir, f"{var_name}.c")
        
        print(f"📸 Processing {input_file}...")
        
        try:
            # Load image
            img = Image.open(input_path)
            print(f"   Original size: {img.width}×{img.height}")
            
            # Resize
            img_resized = resize_image(img, target_size)
            print(f"   Resized to: {img_resized.width}×{img_resized.height}")
            
            # Convert to LVGL C array
            c_code = convert_to_lvgl_c(img_resized, var_name, has_alpha)
            
            # Write C file
            with open(output_path, 'w') as f:
                f.write(c_code)
            
            file_size_kb = len(c_code) / 1024
            print(f"   ✅ Generated {output_path} ({file_size_kb:.1f} KB)\n")
            
        except Exception as e:
            print(f"   ❌ Error: {e}\n")
            continue
    
    print("🎉 Conversion complete!")
    print(f"\n📁 Generated files in: {output_dir}/")
    print("🔧 Next step: Build and upload firmware")

if __name__ == "__main__":
    try:
        main()
    except KeyboardInterrupt:
        print("\n\n⚠️  Conversion cancelled by user")
        sys.exit(1)
