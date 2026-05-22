import os
from PIL import Image
import struct

# Increase or remove the decompression bomb limit
Image.MAX_IMAGE_PIXELS = None  # Remove limit (or set to a higher number)

INPUT_DIR = "PretrainImage"
OUTPUT_DIR = "PTIbin"
os.makedirs(OUTPUT_DIR, exist_ok=True)

def image_to_rgba_bin(path, out):
    img = Image.open(path).convert("RGBA")
    w, h = img.size
    print(f"Processing {path}: {w}x{h} pixels")
    with open(out, "wb") as f:
        f.write(struct.pack("<II", w, h))
        for r, g, b, a in img.getdata():
            f.write(bytes([r, g, b, a]))

for fname in os.listdir(INPUT_DIR):
    if fname.lower().endswith((".png", ".jpg", ".jpeg", ".bmp", ".tiff")):
        try:
            image_to_rgba_bin(os.path.join(INPUT_DIR, fname),
                              os.path.join(OUTPUT_DIR, fname.rsplit('.',1)[0]+".bin"))
        except Exception as e:
            print(f"Skipping {fname}: {e}")