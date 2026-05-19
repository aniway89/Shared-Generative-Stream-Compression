# script3_image_matcher.py
import os
import struct
from PIL import Image

SEARCH_DIR = "Search_image"
UNIVERSAL_LIB = "universal_lib.bin"
OUTPUT_LOC = "comImgLoc.txt"

# ------------------------------------------------------------
# Convert search image to binary (same format)
# ------------------------------------------------------------
def search_image_to_bin(image_path):
    img = Image.open(image_path).convert("RGBA")
    w, h = img.size
    pixels = list(img.getdata())
    data = bytearray()
    data.extend(struct.pack("<II", w, h))
    for r, g, b, a in pixels:
        data.extend(bytes([r, g, b, a]))
    return data

# find first image in SEARCH_DIR
search_files = [f for f in os.listdir(SEARCH_DIR) 
                if f.lower().endswith((".png", ".jpg", ".jpeg", ".bmp", ".tiff"))]
if not search_files:
    raise FileNotFoundError(f"No image found in {SEARCH_DIR}/")
img_path = os.path.join(SEARCH_DIR, search_files[0])
target_data = search_image_to_bin(img_path)
print(f"Search image: {search_files[0]} -> {len(target_data)} bytes")

# ------------------------------------------------------------
# Load universal_lib.bin
# ------------------------------------------------------------
with open(UNIVERSAL_LIB, "rb") as f:
    lib = f.read()
print(f"Universal lib size: {len(lib)} bytes")

# ------------------------------------------------------------
# Build index: all 4‑byte keys -> list of positions (efficient matching)
# ------------------------------------------------------------
print("Indexing universal_lib ...")
index = {}
for pos in range(len(lib) - 3):
    key = lib[pos:pos+4]
    index.setdefault(key, []).append(pos)
print("Indexing done.")

# ------------------------------------------------------------
# Greedy longest match
# ------------------------------------------------------------
matches = []   # list of (offset_in_lib, length)
pos_target = 0
target_len = len(target_data)

while pos_target < target_len:
    best_offset = -1
    best_len = 0
    # try to find longest match starting at pos_target
    # use first 4 bytes as key
    if pos_target + 4 <= target_len:
        key = bytes(target_data[pos_target:pos_target+4])
        if key in index:
            for lib_pos in index[key]:
                # extend match as far as possible
                max_possible = min(target_len - pos_target, len(lib) - lib_pos)
                match_len = 0
                while (match_len < max_possible and 
                       target_data[pos_target + match_len] == lib[lib_pos + match_len]):
                    match_len += 1
                if match_len > best_len:
                    best_len = match_len
                    best_offset = lib_pos
    # if no match found (even 4 bytes), take a single byte fallback
    if best_len == 0:
        # match a single byte at any position (guaranteed to exist)
        byte_val = target_data[pos_target]
        # find first occurrence in lib (slow but rare)
        for i in range(len(lib)):
            if lib[i] == byte_val:
                best_offset = i
                best_len = 1
                break
        if best_len == 0:
            raise RuntimeError("Byte not found in library – impossible")
    matches.append((best_offset, best_len))
    pos_target += best_len
    print(f"Matched {best_len} bytes at offset {best_offset}  (progress {pos_target}/{target_len})")

# ------------------------------------------------------------
# Write location file
# ------------------------------------------------------------
with open(OUTPUT_LOC, "w") as f:
    for off, ln in matches:
        f.write(f"{off} {ln}\n")
print(f"Written {len(matches)} entries to {OUTPUT_LOC}")