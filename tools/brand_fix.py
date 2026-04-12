import os
from PIL import Image

def fix_branding():
    input_path = "assets/logo.png"
    output_png = "assets/logo_transparent.png"
    output_ico = "assets/icon.ico"

    if not os.path.exists(input_path):
        print(f"Error: {input_path} not found.")
        return

    img = Image.open(input_path).convert("RGBA")
    datas = img.getdata()

    new_data = []
    for item in datas:
        # If the pixel is pure black or very close to black, make it transparent
        # (Thresholding to remove the black background)
        if item[0] < 30 and item[1] < 30 and item[2] < 30:
            new_data.append((0, 0, 0, 0))
        else:
            new_data.append(item)

    img.putdata(new_data)
    img.save(output_png, "PNG")
    print(f"Saved transparent logo: {output_png}")

    # Generate multi-size ICO for Windows
    icon_sizes = [(16, 16), (32, 32), (48, 48), (64, 64), (128, 128), (256, 256)]
    # Filter out sizes larger than the original image if necessary, but icons usually pad
    img.save(output_ico, format='ICO', sizes=icon_sizes)
    print(f"Saved application icon: {output_ico}")

if __name__ == "__main__":
    fix_branding()
