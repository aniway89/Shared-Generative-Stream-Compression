# script1_convert_to_bin.py
import os
from PIL import Image
import struct

INPUT_DIR = "PretrainImage"
OUTPUT_DIR = "PTIbin"
os.makedirs(OUTPUT_DIR, exist_ok=True)

def image_to_rgba_bin(image_path, output_path):
    img = Image.open(image_path).convert("RGBA")
    w, h = img.size
    pixels = list(img.getdata())   # list of (R,G,B,A)
    with open(output_path, "wb") as f:
        f.write(struct.pack("<II", w, h))          # width, height
        for r, g, b, a in pixels:
            f.write(bytes([r, g, b, a]))           # 4 bytes per pixel

for fname in os.listdir(INPUT_DIR):
    if fname.lower().endswith((".png", ".jpg", ".jpeg", ".bmp", ".tiff")):
        in_path = os.path.join(INPUT_DIR, fname)
        out_name = os.path.splitext(fname)[0] + ".bin"
        out_path = os.path.join(OUTPUT_DIR, out_name)
        image_to_rgba_bin(in_path, out_path)
        print(f"Converted: {fname} -> {out_path}")