import os
import pickle
from PIL import Image
from collections import defaultdict

TILE_SIZE = 16  # pixels
INDEX_FILE = "tile_index.pkl"

tile_index = defaultdict(list)  # hash → list of (image_path, x, y)

for img_file in os.listdir("PTIbin"):
    if not img_file.endswith(".bin"):
        continue
    # Load the raw RGBA binary (no header)
    with open(os.path.join("PTIbin", img_file), "rb") as f:
        data = f.read()
    # Assume header: width (4 bytes), height (4 bytes) – little‑endian
    w = int.from_bytes(data[0:4], 'little')
    h = int.from_bytes(data[4:8], 'little')
    pixels = data[8:]  # raw RGBA, w*h*4 bytes
    # Convert to PIL image for easy tiling
    img = Image.frombytes("RGBA", (w, h), pixels)
    # Iterate over tiles
    for y in range(0, h - TILE_SIZE + 1, TILE_SIZE):
        for x in range(0, w - TILE_SIZE + 1, TILE_SIZE):
            tile = img.crop((x, y, x+TILE_SIZE, y+TILE_SIZE))
            tile_bytes = tile.tobytes()
            hsh = hash(tile_bytes)  # or use hashlib.md5
            tile_index[hsh].append((img_file, x, y))

with open(INDEX_FILE, "wb") as f:
    pickle.dump(tile_index, f)
print(f"Indexed {len(tile_index)} unique tiles")