import struct
import os
from PIL import Image

LOC_FILE = "comImgLoc.txt"
OUTPUT_IMAGE = "reconstructed.png"

def raw_to_image(data):
    w = struct.unpack("<I", data[0:4])[0]
    h = struct.unpack("<I", data[4:8])[0]
    raw = data[8:8 + w*h*4]
    return Image.frombytes("RGBA", (w, h), raw)

# Determine canvas size from search image (need original dimensions)
# For simplicity, we'll read the original search image dimensions directly:
orig = Image.open("Search_image/test_image.png")
w_orig, h_orig = orig.size
result = Image.new("RGBA", (w_orig, h_orig))

with open(LOC_FILE) as f:
    for line in f:
        img_file, tx, ty, sx, sy = line.strip().split()
        tx, ty, sx, sy = map(int, [tx, ty, sx, sy])
        with open(os.path.join("PTIbin", img_file), "rb") as fbin:
            data = fbin.read()
        img = raw_to_image(data)
        tile = img.crop((tx, ty, tx+16, ty+16))
        result.paste(tile, (sx, sy))

result.save(OUTPUT_IMAGE)
print("Reconstructed image saved")