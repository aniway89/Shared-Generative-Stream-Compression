# script2_stream_generator_fixed.py
import os
import hashlib
import struct

PTIbin_DIR = "PTIbin"
OUTPUT_STREAM = "universal_lib.bin"
NEW_DATA_SIZE = 10 * 1024 * 1024   # 10 MB (change as needed)

# ------------------------------------------------------------
# Step 1: read & concatenate all .bin files
# ------------------------------------------------------------
all_data = bytearray()
for bin_file in sorted(os.listdir(PTIbin_DIR)):
    if bin_file.endswith(".bin"):
        with open(os.path.join(PTIbin_DIR, bin_file), "rb") as f:
            all_data.extend(f.read())

print(f"Original PTIbin total: {len(all_data)} bytes")

with open(OUTPUT_STREAM, "wb") as f:
    f.write(all_data)

# ------------------------------------------------------------
# Step 2: deterministic RNG seeded by training data
# ------------------------------------------------------------
seed = hashlib.sha256(all_data).digest()
state = struct.unpack("<Q", seed[:8])[0]

def next_byte():
    global state
    state ^= state >> 12
    state ^= state << 25
    state ^= state >> 27
    # Take lowest 8 bits, ensure 0-255
    return (state * 0x2545F4914F6CDD1D) & 0xFF

generated = bytearray(next_byte() for _ in range(NEW_DATA_SIZE))

# Append to stream
with open(OUTPUT_STREAM, "ab") as f:
    f.write(generated)

print(f"Generated {len(generated)} bytes")
print(f"Final universal_lib.bin size: {len(all_data) + len(generated)} bytes")