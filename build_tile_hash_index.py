import os
import pickle
import struct
from PIL import Image
from collections import defaultdict

TILE_SIZE = 16  # pixels
INDEX_FILE = "tile_index.pkl"

def raw_to_image(data):
    """Convert PTIbin .bin file to PIL Image"""
    w = struct.unpack("<I", data[0:4])[0]
    h = struct.unpack("<I", data[4:8])[0]
    raw_pixels = data[8:8 + w*h*4]
    return Image.frombytes("RGBA", (w, h), raw_pixels)

index = defaultdict(list)  # hash -> [(image_filename, tile_x, tile_y), ...]

for bin_file in os.listdir("PTIbin"):
    if not bin_file.endswith(".bin"):
        continue
    path = os.path.join("PTIbin", bin_file)
    with open(path, "rb") as f:
        data = f.read()
    img = raw_to_image(data)
    w, h = img.size
    # Iterate over tiles
    for y in range(0, h - TILE_SIZE + 1, TILE_SIZE):
        for x in range(0, w - TILE_SIZE + 1, TILE_SIZE):
            tile = img.crop((x, y, x+TILE_SIZE, y+TILE_SIZE))
            tile_bytes = tile.tobytes()
            hsh = hash(tile_bytes)  # or hashlib.md5(tile_bytes).digest()
            index[hsh].append((bin_file, x, y))

with open(INDEX_FILE, "wb") as f:
    pickle.dump(dict(index), f)
print(f"Indexed {len(index)} unique tiles from PTIbin")