#!/usr/bin/env python3
"""
script2_stream_generator_optimized.py

Generates a deterministic stream using an order-4 Markov model trained on PTIbin files.

The script:
1. Reads all .bin files from PTIbin directory
2. Builds a Markov model mapping 4-byte contexts to next bytes
3. Generates a deterministic stream by cycling through transitions
4. Appends generated data to universal_lib.bin

The generation is deterministic because each context cycles through its
possible next bytes in a fixed order, producing the same output each run.

Usage:
    python script2_stream_generator_optimized.py
"""

import os
import struct
from collections import defaultdict

PTI_BIN_DIRECTORY = "PTIbin"
OUTPUT_FILE_NAME = "universal_lib.bin"
NEW_DATA_SIZE = 10 * 1024 * 1024
ORDER = 4  # Markov model order (number of bytes in context)

all_data = bytearray()
with os.scandir(PTI_BIN_DIRECTORY) as it:
    for entry in sorted(it, key=lambda e: e.name):
        if entry.name.endswith(".bin") and entry.is_file():
            with open(entry.path, "rb") as f:
                all_data.extend(f.read())

print(f"Original PTIbin total: {len(all_data)} bytes")

with open(OUTPUT_FILE_NAME, "wb") as f:
    f.write(all_data)

if len(all_data) < ORDER + 1:
    raise RuntimeError("Not enough data")

model = defaultdict(list)
data_view = memoryview(all_data)
max_pos = len(all_data) - ORDER
if max_pos <= 0:
    raise RuntimeError(f"Data too short: need at least {ORDER+1} bytes, got {len(all_data)}")

for i in range(max_pos):
    ctx = (data_view[i] << 24) | (data_view[i+1] << 16) | (data_view[i+2] << 8) | data_view[i+3]
    nxt = data_view[i+4]
    model[ctx].append(nxt)

print(f"Model built with {len(model)} unique contexts")

ctx = (data_view[-4] << 24) | (data_view[-3] << 16) | (data_view[-2] << 8) | data_view[-1]
generated = bytearray()
pos_in_model = {ctx: 0 for ctx in model}

while len(generated) < NEW_DATA_SIZE:
    if ctx in model:
        lst = model[ctx]
        idx = pos_in_model[ctx]
        nxt = lst[idx]
        pos_in_model[ctx] = (idx + 1) % len(lst)
    else:
        nxt = all_data[len(generated) % len(all_data)]
    generated.append(nxt)
    ctx = ((ctx << 8) | nxt) & 0xFFFFFFFF

print(f"Generated {len(generated)} bytes")

with open(OUTPUT_FILE_NAME, "ab") as f:
    f.write(generated)

print(f"Final size: {len(all_data) + len(generated)} bytes")
