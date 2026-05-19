import pickle
from PIL import Image

with open("comImgLoc.txt") as f:
    matches = [line.strip().split() for line in f]

# Determine final image size by max sx+16, sy+16
max_w = max(int(m[4]) for m in matches) + 16
max_h = max(int(m[5]) for m in matches) + 16
result = Image.new("RGBA", (max_w, max_h))

for img_file, tx, ty, sx, sy in matches:
    tx, ty, sx, sy = map(int, [tx, ty, sx, sy])
    # Load the tile from the original PTI image
    with open(os.path.join("PTIbin", img_file), "rb") as f:
        data = f.read()
    w = int.from_bytes(data[0:4], 'little')
    h = int.from_bytes(data[4:8], 'little')
    pixels = data[8:]
    img = Image.frombytes("RGBA", (w, h), pixels)
    tile = img.crop((tx, ty, tx+16, ty+16))
    result.paste(tile, (sx, sy))

result.save("reconstructed.png")
print("Reconstructed image saved")