# script3_no_index_matcher.py
import os
import struct
from PIL import Image

SEARCH_DIR = "Search_image"
UNIVERSAL_LIB = "universal_lib.bin"
OUTPUT_LOC = "comImgLoc.txt"

MIN_MATCH = 4               # minimum meaningful match length
MAX_SEARCH_LEN = 4096       # limit match search to 4KB for speed

# ------------------------------------------------------------
# Convert image to RGBA binary
# ------------------------------------------------------------
def image_to_bin(path):
    img = Image.open(path).convert("RGBA")
    w, h = img.size
    data = bytearray()
    data.extend(struct.pack("<II", w, h))
    for r, g, b, a in img.getdata():
        data.extend([r, g, b, a])
    return bytes(data)

# Find search image
search_files = [f for f in os.listdir(SEARCH_DIR) 
                if f.lower().endswith(('.png','.jpg','.jpeg','.bmp'))]
if not search_files:
    raise FileNotFoundError(f"No image in {SEARCH_DIR}")
target = image_to_bin(os.path.join(SEARCH_DIR, search_files[0]))
print(f"Target: {search_files[0]}, size = {len(target)} bytes")

# ------------------------------------------------------------
# Load library
# ------------------------------------------------------------
with open(UNIVERSAL_LIB, "rb") as f:
    lib = f.read()
print(f"Library size: {len(lib)} bytes")

# ------------------------------------------------------------
# Greedy match using direct search (C‑optimized)
# ------------------------------------------------------------
matches = []
p = 0
target_len = len(target)

while p < target_len:
    # Try to find longest match by decreasing length from MAX_SEARCH_LEN down to MIN_MATCH
    best_off = -1
    best_len = 1  # fallback to single byte
    
    max_possible_match = min(MAX_SEARCH_LEN, target_len - p)
    for length in range(max_possible_match, MIN_MATCH - 1, -1):
        pattern = target[p:p+length]
        off = lib.find(pattern)
        if off != -1:
            best_off = off
            best_len = length
            break
    
    # If no match of length >= MIN_MATCH, match a single byte
    if best_off == -1:
        off = lib.find(target[p:p+1])
        if off == -1:
            raise RuntimeError("Byte not found in library")
        best_off = off
        best_len = 1
    
    matches.append((best_off, best_len))
    p += best_len
    
    if p % 50000 == 0:
        print(f"Progress: {p}/{target_len} bytes ({100*p/target_len:.1f}%)")

# ------------------------------------------------------------
# Save results
# ------------------------------------------------------------
with open(OUTPUT_LOC, "w") as f:
    for off, ln in matches:
        f.write(f"{off} {ln}\n")
print(f"Saved {len(matches)} entries to {OUTPUT_LOC}")