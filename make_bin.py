import sys
import os
from PIL import Image
import struct

def main():
    # Look for first image in Search_image folder
    search_dir = "Search_image"
    if not os.path.isdir(search_dir):
        print(f"ERROR: Folder '{search_dir}' not found.")
        sys.exit(1)
    
    images = [f for f in os.listdir(search_dir) if f.lower().endswith(('.png','.jpg','.jpeg','.bmp'))]
    if not images:
        print(f"No image files found in {search_dir}")
        sys.exit(1)
    
    input_path = os.path.join(search_dir, images[0])
    output_path = "search_image.bin"
    
    print(f"Reading: {input_path}")
    try:
        img = Image.open(input_path).convert("RGBA")
    except Exception as e:
        print(f"Failed to open image: {e}")
        sys.exit(1)
    
    w, h = img.size
    print(f"Image size: {w} x {h}")
    
    # Build raw RGBA binary: header (width, height as uint32 little-endian) + pixels
    data = bytearray()
    data.extend(struct.pack("<II", w, h))
    # img.getdata() returns sequence of (R,G,B,A)
    for pixel in img.getdata():
        data.extend(pixel)  # pixel is a tuple of 4 ints
    
    with open(output_path, "wb") as f:
        f.write(data)
    
    print(f"Successfully wrote {len(data)} bytes to {output_path}")
    print("File size: {:.2f} MB".format(len(data) / (1024*1024)))

if __name__ == "__main__":
    main()