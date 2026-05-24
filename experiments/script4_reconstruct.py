# reconstruct.py
import struct
from PIL import Image

with open("comImgLoc.txt") as f:
    matches = [tuple(map(int, line.split())) for line in f if line.strip()]

with open("universal_lib.bin", "rb") as f:
    lib = f.read()

recon = bytearray()
for off, ln in matches:
    recon.extend(lib[off:off+ln])

w, h = struct.unpack("<II", recon[:8])
pixels = recon[8:8 + 4*w*h]

img = Image.frombytes("RGBA", (w, h), bytes(pixels))
img.save("reconstructed_image.png")
print("Reconstructed image saved as reconstructed_image.png")