from PIL import Image
import os

def generate_ico(png_path, ico_path):
    if not os.path.exists(png_path):
        print(f"Error: {png_path} not found.")
        return

    img = Image.open(png_path)
    # Recommended sizes for Windows icons
    sizes = [(16, 16), (24, 24), (32, 32), (48, 48), (64, 64), (128, 128), (256, 256)]
    img.save(ico_path, format='ICO', sizes=sizes)
    print(f"Successfully generated {ico_path}")

if __name__ == "__main__":
    generate_ico("assets/logo.png", "assets/icon.ico")
