#!/usr/bin/env python3
"""
Update the application logo with the refined version.
Copies betterangle_refined_logo.png to logo.png and generates ICO files.
"""
import os
from PIL import Image

def convert_png_to_ico(png_path, ico_path):
    """Convert PNG to ICO with multiple sizes, preserving transparency."""
    if not os.path.exists(png_path):
        print(f"Error: Source image not found at {png_path}")
        return False
    
    try:
        img = Image.open(png_path).convert("RGBA")
        
        # ICO sizes (Windows expects these standard sizes)
        ico_sizes = [(16, 16), (32, 32), (48, 48), (64, 64), (128, 128), (256, 256)]
        
        # Save as ICO
        img.save(ico_path, format="ICO", sizes=ico_sizes)
        print(f"Generated {ico_path}")
        return True
    except Exception as e:
        print(f"Error converting to ICO: {e}")
        return False

def copy_png(src_path, dst_path):
    """Copy PNG file, preserving transparency."""
    if not os.path.exists(src_path):
        print(f"Error: Source image not found at {src_path}")
        return False
    
    try:
        img = Image.open(src_path).convert("RGBA")
        img.save(dst_path, "PNG")
        print(f"Copied to {dst_path}")
        return True
    except Exception as e:
        print(f"Error copying PNG: {e}")
        return False

def main():
    assets_dir = "assets"
    refined_logo = os.path.join(assets_dir, "betterangle_refined_logo.png")
    
    if not os.path.exists(refined_logo):
        print(f"Error: Refined logo not found at {refined_logo}")
        return
    
    # 1. Update logo.png (used in QML UI)
    logo_png = os.path.join(assets_dir, "logo.png")
    if copy_png(refined_logo, logo_png):
        print("✓ Updated logo.png")
    
    # 2. Generate new ICO files
    # Update the main application icon (BetterAngle_v162.ico - increment version)
    ico_v162 = os.path.join(assets_dir, "BetterAngle_v162.ico")
    if convert_png_to_ico(refined_logo, ico_v162):
        print("✓ Generated BetterAngle_v162.ico")
    
    # Also update icon.ico (used by some scripts)
    icon_ico = os.path.join(assets_dir, "icon.ico")
    if convert_png_to_ico(refined_logo, icon_ico):
        print("✓ Updated icon.ico")
    
    print("\nLogo update complete. Next steps:")
    print("1. Update installer.iss to reference BetterAngle_v162.ico")
    print("2. Update assets/resource.rc to reference BetterAngle_v162.ico")
    print("3. Commit and push changes")

if __name__ == "__main__":
    main()