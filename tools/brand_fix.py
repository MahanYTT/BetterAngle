import os
from PIL import Image

def fix_branding():
    input_path = "assets/logo.png"
    output_png = "assets/logo_transparent.png"
    output_ico = "assets/app_icon.ico" # New name to bypass cache

    if not os.path.exists(input_path):
        print(f"Error: {input_path} not found.")
        return

    img = Image.open(input_path).convert("RGBA")
    datas = img.getdata()

    new_data = []
    for item in datas:
        # Higher threshold (45) for transparency to ensure a clean circle.
        if item[0] < 45 and item[1] < 45 and item[2] < 45:
            new_data.append((0, 0, 0, 0))
        else:
            new_data.append(item)

    img.putdata(new_data)
    img.save(output_png, "PNG")
    print(f"Saved transparent logo: {output_png}")

    icon_sizes = [(16, 16), (32, 32), (48, 48), (64, 64), (128, 128), (256, 256)]
    img.save(output_ico, format='ICO', sizes=icon_sizes)
    print(f"Saved application icon: {output_ico}")

if __name__ == "__main__":
    fix_branding()
