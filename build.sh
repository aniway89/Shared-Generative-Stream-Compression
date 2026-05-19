# ============================================================
# FILE: build.sh
# PURPOSE:
# Build all components
# ============================================================

mkdir -p STREAM
mkdir -p MATCH
mkdir -p OUT

g++ generate_stream.cpp -o generate_stream \
    `pkg-config --cflags --libs opencv4` \
    -lxxhash -O3 -march=native

g++ match_image.cpp -o match_image \
    `pkg-config --cflags --libs opencv4` \
    -lxxhash -O3 -march=native

g++ reconstruct.cpp -o reconstruct \
    `pkg-config --cflags --libs opencv4` \
    -O3 -march=native