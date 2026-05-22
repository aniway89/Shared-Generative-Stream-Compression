    # compress_manual.py
import sys

if len(sys.argv) != 6:
    print("Usage: compress_manual.py <image_id> <x> <y> <width> <height>")
    sys.exit(1)

image_id, x, y, w, h = sys.argv[1:6]
with open("comImgLoc.txt", "w") as f:
    f.write(f"{image_id} {x} {y} {w} {h}")
print("Location file saved (tiny!)")