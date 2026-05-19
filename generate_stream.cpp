// ============================================================
// FILE: generate_stream.cpp
// PURPOSE:
// Build deterministic universal tile stream from training images
// ============================================================

#include <iostream>
#include <vector>
#include <unordered_map>
#include <fstream>
#include <filesystem>
#include <opencv2/opencv.hpp>
#include "xxhash.h"

namespace fs = std::filesystem;

constexpr int TILE_SIZE = 4;
constexpr bool ENABLE_PROCEDURAL = true;

struct Tile {
    uint8_t data[TILE_SIZE * TILE_SIZE * 3];
};

std::vector<Tile> stream_tiles;
std::unordered_map<uint64_t, std::vector<uint32_t>> hash_index;

uint64_t hash_tile(const Tile& t) {
    return XXH64(t.data, sizeof(t.data), 0);
}

void insert_tile(const Tile& tile) {
    uint64_t h = hash_tile(tile);

    uint32_t offset = stream_tiles.size();

    stream_tiles.push_back(tile);
    hash_index[h].push_back(offset);
}

Tile mutate_brightness(const Tile& t, int shift) {
    Tile out = t;

    for (int i = 0; i < sizeof(out.data); i++) {
        int v = out.data[i] + shift;

        if (v < 0) v = 0;
        if (v > 255) v = 255;

        out.data[i] = v;
    }

    return out;
}

void procedural_generate(const Tile& t) {
    static const int shifts[] = {-10, -5, 5, 10};

    for (int s : shifts) {
        Tile m = mutate_brightness(t, s);
        insert_tile(m);
    }
}

void process_image(const std::string& path) {

    cv::Mat img = cv::imread(path);

    if (img.empty()) {
        std::cout << "Failed: " << path << "\n";
        return;
    }

    for (int y = 0; y <= img.rows - TILE_SIZE; y += TILE_SIZE) {

        for (int x = 0; x <= img.cols - TILE_SIZE; x += TILE_SIZE) {

            Tile t;
            int idx = 0;

            for (int ty = 0; ty < TILE_SIZE; ty++) {
                for (int tx = 0; tx < TILE_SIZE; tx++) {

                    cv::Vec3b p = img.at<cv::Vec3b>(y + ty, x + tx);

                    t.data[idx++] = p[0];
                    t.data[idx++] = p[1];
                    t.data[idx++] = p[2];
                }
            }

            insert_tile(t);

            if (ENABLE_PROCEDURAL) {
                procedural_generate(t);
            }
        }
    }

    std::cout << "Processed: " << path << "\n";
}

void save_stream() {

    std::ofstream out("STREAM/tiles.bin", std::ios::binary);

    for (const auto& t : stream_tiles) {
        out.write((char*)t.data, sizeof(t.data));
    }

    out.close();
}

void save_index() {

    std::ofstream out("STREAM/hash_index.bin", std::ios::binary);

    uint64_t entries = hash_index.size();

    out.write((char*)&entries, sizeof(entries));

    for (auto& [hash, offsets] : hash_index) {

        out.write((char*)&hash, sizeof(hash));

        uint32_t count = offsets.size();

        out.write((char*)&count, sizeof(count));

        out.write((char*)offsets.data(), count * sizeof(uint32_t));
    }

    out.close();
}

int main() {

    fs::create_directory("STREAM");

    for (auto& p : fs::directory_iterator("IMG")) {
        process_image(p.path().string());
    }

    save_stream();
    save_index();

    std::cout << "Tiles Stored: " << stream_tiles.size() << "\n";

    return 0;
}