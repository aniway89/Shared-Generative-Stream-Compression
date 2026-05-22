import pickle
from PIL import Image

TILE_SIZE = 16
INDEX_FILE = "tile_index.pkl"
search_path = "Search_image/test_image.png"
output_loc = "comImgLoc.txt"

with open(INDEX_FILE, "rb") as f:
    tile_index = pickle.load(f)

search_img = Image.open(search_path).convert("RGBA")
w, h = search_img.size

matches = []  # list of (image_file, tile_x, tile_y, search_x, search_y)
covered = [[False]*w for _ in range(h)]

def add_tile(sx, sy):
    tile = search_img.crop((sx, sy, sx+TILE_SIZE, sy+TILE_SIZE))
    tile_bytes = tile.tobytes()
    hsh = hash(tile_bytes)
    if hsh in tile_index:
        candidates = tile_index[hsh]
        # Use the first match (or could be smarter)
        img_file, tx, ty = candidates[0]
        matches.append((img_file, tx, ty, sx, sy))
        # Mark covered pixels
        for dy in range(TILE_SIZE):
            for dx in range(TILE_SIZE):
                covered[sy+dy][sx+dx] = True

# Greedy tile placement: scan search image, cover largest possible tile
for y in range(0, h, TILE_SIZE):
    for x in range(0, w, TILE_SIZE):
        if not covered[y][x]:
            add_tile(x, y)

# Write location file (minimal)
with open(output_loc, "w") as f:
    for img_file, tx, ty, sx, sy in matches:
        f.write(f"{img_file} {tx} {ty} {sx} {sy}\n")
print(f"Saved {len(matches)} tile references")