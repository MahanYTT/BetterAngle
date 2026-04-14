import os
from PIL import Image, ImageDraw

def generate_icons(source_path, output_png, output_ico):
    # Load source
    img = Image.open(source_path).convert("RGBA")
    width, height = img.size
    
    # Create a circular mask to remove the checkerboard background
    # (Checking the image, it's a centered circle)
    mask = Image.new("L", (width, height), 0)
    draw = ImageDraw.Draw(mask)
    
    # Define the circle boundary (padding slightly to avoid edges)
    # The generated image has a circular button
    # I'll use a circle that covers about 95% of the frame
    padding = 10
    draw.ellipse((padding, padding, width - padding, height - padding), fill=255)
    
    # Apply mask
    result = Image.new("RGBA", (width, height), (0,0,0,0))
    result.paste(img, (0,0), mask=mask)
    
    # Save PNG
    result.save(output_png)
    print(f"Saved {output_png}")
    
    # Save ICO with multiple resolutions
    sizes = [(16,16), (32,32), (48,48), (64,64), (128,128), (256,256)]
    result.save(output_ico, format="ICO", sizes=sizes)
    print(f"Saved {output_ico}")

if __name__ == "__main__":
    source = "/Users/kierenpatel/.gemini/antigravity/brain/f53cffab-f06d-4312-bc77-9f7aa62de87e/betterangle_new_logo_master_1776134747858.png"
    assets_dir = "/Users/kierenpatel/.gemini/antigravity/scratch/BetterAngle/assets"
    
    generate_icons(source, 
                   os.path.join(assets_dir, "logo.png"), 
                   os.path.join(assets_dir, "icon.ico"))
