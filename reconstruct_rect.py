# reconstruct_rect.py
import struct
from PIL import Image
import os

with open("comImgLoc.txt") as f:
    img_file, x, y, w, h = f.read().strip().split()
x, y, w, h = map(int, [x, y, w, h])

# Load the training image from PTIbin
with open(os.path.join("PTIbin", img_file), "rb") as f:
    data = f.read()
    width = int.from_bytes(data[0:4], 'little')
    height = int.from_bytes(data[4:8], 'little')
    pixels = data[8:]
    img = Image.frombytes("RGBA", (width, height), pixels)
    crop = img.crop((x, y, x+w, y+h))
    crop.save("reconstructed.png")
print("Reconstructed crop saved")