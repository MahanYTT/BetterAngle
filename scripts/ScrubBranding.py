from PIL import Image, ImageDraw
import os

def scrub_and_export(source_path, png_path, ico_path):
    print(f"Opening approved source: {source_path}")
    img = Image.open(source_path).convert("RGBA")
    width, height = img.size
    
    # 6% Margin shave for perfect ring removal
    margin = int(width * 0.06)
    
    mask = Image.new("L", (width, height), 0)
    draw = ImageDraw.Draw(mask)
    draw.ellipse((margin, margin, width - margin, height - margin), fill=255)
    
    pix = img.load()
    res_img = Image.new("RGBA", (width, height), (0, 0, 0, 0))
    res_pix = res_img.load()
    mask_pix = mask.load()
    
    for y in range(height):
        for x in range(width):
            if mask_pix[x, y] == 255:
                r, g, b, a = pix[x, y]
                # Luma threshold to kill black ring artifacts
                if (r + g + b) / 3 > 45:
                    res_pix[x, y] = (r, g, b, a)
                else:
                    res_pix[x, y] = (0, 0, 0, 0)

    res_img.save(png_path, "PNG", optimize=True)
    print(f"Saved FINAL PNG: {png_path}")
    
    # New Cache-Busting name for v161
    sizes = [(16, 16), (32, 32), (48, 48), (64, 64), (128, 128), (256, 256)]
    res_img.save(ico_path, format='ICO', sizes=sizes)
    print(f"Saved FINAL cache-busting ICO: {ico_path}")

if __name__ == "__main__":
    # Source with the '>' symbol
    src = "/Users/kierenpatel/.gemini/antigravity/brain/f53cffab-f06d-4312-bc77-9f7aa62de87e/betterangle_master_logo_with_symbol_resubmit_1776148290360.png"
    assets_dir = "/Users/kierenpatel/.gemini/antigravity/scratch/BetterAngle/assets"
    
    png_out = os.path.join(assets_dir, "logo.png")
    ico_out = os.path.join(assets_dir, "BetterAngle_v161.ico")
    
    scrub_and_export(src, png_out, ico_out)
