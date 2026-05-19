import pickle
from PIL import Image

TILE_SIZE = 16
INDEX_FILE = "tile_index.pkl"
SEARCH_IMAGE = "Search_image/test_image.png"
OUTPUT_FILE = "comImgLoc.txt"

with open(INDEX_FILE, "rb") as f:
    tile_index = pickle.load(f)

search = Image.open(SEARCH_IMAGE).convert("RGBA")
w, h = search.size

matches = []  # each: (image_file, tile_x, tile_y, search_x, search_y)

# Greedy tile coverage (simple non‑overlapping grid)
for y in range(0, h, TILE_SIZE):
    for x in range(0, w, TILE_SIZE):
        tile = search.crop((x, y, x+TILE_SIZE, y+TILE_SIZE))
        # If tile is smaller than TILE_SIZE at edges, we can either pad or skip
        if tile.size[0] != TILE_SIZE or tile.size[1] != TILE_SIZE:
            continue  # or handle partial tiles by scaling up; simpler to skip
        hsh = hash(tile.tobytes())
        if hsh in tile_index:
            # Use the first matching tile from index (could be improved)
            img_file, tx, ty = tile_index[hsh][0]
            matches.append((img_file, tx, ty, x, y))
        else:
            # No exact match; could fallback to smaller tiles or store raw.
            # For now we raise an error to indicate incomplete dictionary.
            raise ValueError(f"No tile match at ({x},{y}) – dictionary incomplete")

with open(OUTPUT_FILE, "w") as f:
    for img_file, tx, ty, sx, sy in matches:
        f.write(f"{img_file} {tx} {ty} {sx} {sy}\n")
print(f"Saved {len(matches)} tile references to {OUTPUT_FILE}")