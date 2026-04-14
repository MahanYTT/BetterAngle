from PIL import Image, ImageDraw
import os

def scrub_and_export(source_path, png_path, ico_path):
    print(f"Opening source: {source_path}")
    img = Image.open(source_path).convert("RGBA")
    width, height = img.size
    
    # DALL-E checkerboard usually consists of gray/white squares.
    # However, the most robust way to 'scrub' a circular logo
    # is to apply a mathematical circular mask that forces everything
    # outside the glow to pure transparency.
    
    # Create high-precision mask
    mask = Image.new("L", (width, height), 0)
    draw = ImageDraw.Draw(mask)
    
    # Use a 3% margin to ensure we cut inside the 'spillover' area
    margin = int(width * 0.03)
    draw.ellipse((margin, margin, width - margin, height - margin), fill=255)
    
    # Create final image with pure transparency
    scrubbed = Image.new("RGBA", (width, height), (0, 0, 0, 0))
    scrubbed.paste(img, (0, 0), mask=mask)
    
    # Save optimized PNG
    scrubbed.save(png_path, "PNG", optimize=True)
    print(f"Saved scrubbed PNG: {png_path}")
    
    # Save multi-resolution ICO (Cache-busting name)
    sizes = [(16, 16), (32, 32), (48, 48), (64, 64), (128, 128), (256, 256)]
    scrubbed.save(ico_path, format='ICO', sizes=sizes)
    print(f"Saved cache-busting ICO: {ico_path}")

if __name__ == "__main__":
    # Source from DALL-E (the one with the checkerboard)
    src = "/Users/kierenpatel/.gemini/antigravity/brain/f53cffab-f06d-4312-bc77-9f7aa62de87e/betterangle_master_cyan_orb_v2_1776145305930.png"
    assets_dir = "/Users/kierenpatel/.gemini/antigravity/scratch/BetterAngle/assets"
    
    # Cache-busting name for v153
    png_out = os.path.join(assets_dir, "logo.png")
    ico_out = os.path.join(assets_dir, "BetterAngle_v153.ico")
    
    scrub_and_export(src, png_out, ico_out)
