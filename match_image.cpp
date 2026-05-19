// ============================================================
// FILE: match_image.cpp
// PURPOSE:
// Match COMP_IMG images against stream using hash index
// ============================================================

#include <iostream>
#include <vector>
#include <unordered_map>
#include <fstream>
#include <filesystem>
#include <opencv2/opencv.hpp>
#include "xxhash.h"
#include "logger.h"

namespace fs = std::filesystem;

constexpr int TILE_SIZE = 4;

struct Tile {
    uint8_t data[TILE_SIZE * TILE_SIZE * 3];
};

struct MatchRect {
    uint32_t stream_offset;
    uint16_t x;
    uint16_t y;
    uint16_t w;
    uint16_t h;
};

std::unordered_map<uint64_t, std::vector<uint32_t>> hash_index;

uint64_t hash_tile(const Tile& t) {
    return XXH64(t.data, sizeof(t.data), 0);
}

void load_index() {

    std::ifstream in("STREAM/hash_index.bin", std::ios::binary);

    uint64_t entries;

    in.read((char*)&entries, sizeof(entries));

    for (uint64_t i = 0; i < entries; i++) {

        uint64_t hash;
        uint32_t count;

        in.read((char*)&hash, sizeof(hash));
        in.read((char*)&count, sizeof(count));

        std::vector<uint32_t> offsets(count);

        in.read((char*)offsets.data(), count * sizeof(uint32_t));

        hash_index[hash] = offsets;
    }

    in.close();
}

Tile extract_tile(cv::Mat& img, int x, int y) {

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

    return t;
}

void process_image(const std::string& path) {

    BenchmarkTimer timer;
    timer.tic();

    cv::Mat img = cv::imread(path);

    std::vector<MatchRect> matches;

    for (int y = 0; y <= img.rows - TILE_SIZE; y += TILE_SIZE) {

        for (int x = 0; x <= img.cols - TILE_SIZE; x += TILE_SIZE) {

            Tile t = extract_tile(img, x, y);

            uint64_t h = hash_tile(t);

            auto it = hash_index.find(h);

            if (it == hash_index.end())
                continue;

            MatchRect r;

            r.stream_offset = it->second[0];
            r.x = x;
            r.y = y;
            r.w = TILE_SIZE;
            r.h = TILE_SIZE;

            matches.push_back(r);
        }
    }

    std::ofstream out("MATCH/image.map", std::ios::binary);

    uint32_t count = matches.size();

    out.write((char*)&count, sizeof(count));

    out.write((char*)matches.data(), count * sizeof(MatchRect));

    out.close();

    double elapsed = timer.toc();

    uint64_t coord_size =
        file_size_bytes("MATCH/image.map");

    uint64_t original_size =
        file_size_bytes(path);

    double ratio =
        (double)original_size /
        (double)coord_size;

    std::cout << "\n========== MATCH RESULTS ==========\n";

    print_stat(
        "Original File Size",
        mb(original_size),
        "MB"
    );

    print_stat(
        "Coordinate File Size",
        mb(coord_size),
        "MB"
    );

    print_stat(
        "Compression Ratio",
        ratio,
        "x"
    );

    print_stat(
        "Matching Time",
        elapsed,
        "sec"
    );

    append_log("========== MATCH RESULTS ==========");
    append_log("Image: " + path);
    append_log("Original MB: " + std::to_string(mb(original_size)));
    append_log("Coord MB: " + std::to_string(mb(coord_size)));
    append_log("Compression Ratio: " + std::to_string(ratio));
    append_log("Matching Time Sec: " + std::to_string(elapsed));
    append_log("");

    std::cout << "Matches: " << matches.size() << "\n";
}

int main() {

    fs::create_directory("MATCH");

    load_index();

    for (auto& p : fs::directory_iterator("COMP_IMG")) {
        process_image(p.path().string());
    }

    return 0;
}