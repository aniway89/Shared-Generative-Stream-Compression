# ============================================================
# FILE: build.sh
# ============================================================

echo "========== BUILD START =========="

mkdir -p STREAM
mkdir -p MATCH
mkdir -p OUT

echo "[1/3] Building generate_stream..."

g++ generate_stream.cpp -o generate_stream \
`pkg-config --cflags --libs opencv4` \
-O3 -march=native

echo "[2/3] Building match_image..."

g++ match_image.cpp -o match_image \
`pkg-config --cflags --libs opencv4` \
-O3 -march=native

echo "[3/3] Building reconstruct..."

g++ reconstruct.cpp -o reconstruct \
`pkg-config --cflags --libs opencv4` \
-O3 -march=native

echo "========== BUILD COMPLETE =========="