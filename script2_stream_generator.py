# script2_stream_generator.py
import os
import struct
from collections import defaultdict

PTIbin_DIR = "PTIbin"
OUTPUT_STREAM = "universal_lib.bin"
NEW_DATA_SIZE = 100 * 1024 * 1024   # 100 MB

# ------------------------------------------------------------
# Step 1: read & concatenate all .bin files from PTIbin/
# ------------------------------------------------------------
all_data = bytearray()
for bin_file in sorted(os.listdir(PTIbin_DIR)):
    if bin_file.endswith(".bin"):
        with open(os.path.join(PTIbin_DIR, bin_file), "rb") as f:
            all_data.extend(f.read())

print(f"Original PTIbin total: {len(all_data)} bytes")

with open(OUTPUT_STREAM, "wb") as f:
    f.write(all_data)          # exact copy first

# ------------------------------------------------------------
# Step 2: build Markov model (order-4) on the original data
# ------------------------------------------------------------
if len(all_data) < 5:
    raise RuntimeError("Not enough original data to build model (need at least 5 bytes)")

model = defaultdict(list)      # context (4 bytes) -> list of next bytes
for i in range(len(all_data) - 4):
    ctx = bytes(all_data[i:i+4])
    nxt = all_data[i+4]
    model[ctx].append(nxt)

# convert lists to probability distribution (deterministic: keep order)
# we will cycle through the list for the same context
pos_in_model = {ctx: 0 for ctx in model}

# ------------------------------------------------------------
# Step 3: generate new data deterministically
# ------------------------------------------------------------
# initial context = last 4 bytes of original data
ctx = bytes(all_data[-4:])
generated = bytearray()
while len(generated) < NEW_DATA_SIZE:
    if ctx not in model:
        # fallback: repeat original data bytes cyclically
        nxt = all_data[len(generated) % len(all_data)]
    else:
        lst = model[ctx]
        idx = pos_in_model[ctx]
        nxt = lst[idx]
        pos_in_model[ctx] = (idx + 1) % len(lst)
    generated.append(nxt)
    ctx = bytes(ctx[1:] + bytes([nxt]))

print(f"Generated {len(generated)} bytes of new binary data")

# ------------------------------------------------------------
# Step 4: append to universal_lib.bin
# ------------------------------------------------------------
with open(OUTPUT_STREAM, "ab") as f:
    f.write(generated)

print(f"Final universal_lib.bin size: {len(all_data) + len(generated)} bytes")