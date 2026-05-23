// ============================================================
// FILE: generate_stream.cpp
//
// RESPONSIBILITY: ONE job only —
//   Read training images from IMG/ and write stream.bin
//
// Does NOT touch COMP_IMG.
// Does NOT read match results.
// Does NOT do active learning.
//
// Run order:
//   1. generate_stream          <- this file
//   2. match_image
//   3. (optional) update_stream <- learns from match failures
//   4. match_image again        <- better coverage
//   5. reconstruct
// ============================================================

#include <iostream>
#include <vector>
#include <fstream>
#include <filesystem>
#include <opencv2/opencv.hpp>

#include "logger.h"
#include "config.h"

namespace fs = std::filesystem;

std::vector<uint8_t> universal_stream;

// ============================================================
// ADD ONE TRAINING IMAGE TO STREAM
// ============================================================

void process_image(const std::string& path) {

    cv::Mat img = cv::imread(path);

    if (img.empty()) {
        std::cout << "Failed to load: " << path << "\n";
        return;
    }

    for (int y = 0; y < img.rows; y++) {
        for (int x = 0; x < img.cols; x++) {

            cv::Vec3b p = img.at<cv::Vec3b>(y, x);

            // Exact pixel BGR
            universal_stream.push_back(p[0]);
            universal_stream.push_back(p[1]);
            universal_stream.push_back(p[2]);

            // Generative variants: +-delta neighbours
            // Helps extend_match grow across colour gradients
            if (ENABLE_GENERATIVE_MODE) {
                for (int d = 1; d <= GENERATIVE_REPEAT_COUNT; d++) {

                    // +d variant
                    universal_stream.push_back((uint8_t)std::min(255, (int)p[0] + d));
                    universal_stream.push_back((uint8_t)std::min(255, (int)p[1] + d));
                    universal_stream.push_back((uint8_t)std::min(255, (int)p[2] + d));

                    // -d variant
                    universal_stream.push_back((uint8_t)std::max(0, (int)p[0] - d));
                    universal_stream.push_back((uint8_t)std::max(0, (int)p[1] - d));
                    universal_stream.push_back((uint8_t)std::max(0, (int)p[2] - d));
                }
            }
        }
    }

    // Boundary separator between images so hash windows
    // don't accidentally bridge across two images
    for (int i = 0; i < STREAM_SEPARATOR_SIZE; i++)
        universal_stream.push_back(255);

    std::cout << "Processed: " << path << "\n";
}

// ============================================================
// UNIVERSAL PALETTE SUFFIX
//
// Appends every possible BGR triplet once.
// 256^3 * 3 = 48 MB added to stream.bin
// Makes black pixels impossible — any pixel value will
// always be found somewhere in the palette section.
// ============================================================

void append_universal_palette() {

    if (!ENABLE_UNIVERSAL_PALETTE) return;

    std::cout << "\n[PALETTE] Appending all 16,777,216 BGR triplets (~48 MB)...\n";

    for (int i = 0; i < STREAM_SEPARATOR_SIZE; i++)
        universal_stream.push_back(0x00);

    universal_stream.reserve(
        universal_stream.size() + (size_t)256 * 256 * 256 * 3
    );

    for (int b = 0; b < 256; b++)
        for (int g = 0; g < 256; g++)
            for (int r = 0; r < 256; r++) {
                universal_stream.push_back((uint8_t)b);
                universal_stream.push_back((uint8_t)g);
                universal_stream.push_back((uint8_t)r);
            }

    std::cout << "[PALETTE] Done. Stream now covers every possible colour.\n";
}

void save_stream() {
    std::ofstream out("STREAM/stream.bin", std::ios::binary);
    out.write((char*)universal_stream.data(), universal_stream.size());
    out.close();
}

int main() {

    int ATTEMPT = get_attempt_number();
    fs::create_directory("STREAM");

    BenchmarkTimer timer;
    timer.tic();

    if (!fs::exists("IMG")) {
        std::cerr << "ERROR: No IMG/ directory found. Put training images there.\n";
        return 1;
    }

    for (auto& p : fs::directory_iterator("IMG"))
        process_image(p.path().string());

    append_universal_palette();

    save_stream();

    double   elapsed     = timer.toc();
    uint64_t stream_size = file_size_bytes("STREAM/stream.bin");

    std::cout << "\n========== STREAM GENERATION ==========\n";
    print_stat("Attempt",           std::to_string(ATTEMPT));
    print_stat("Generative Mode",   ENABLE_GENERATIVE_MODE ? "ON" : "OFF");
    print_stat("Universal Palette", ENABLE_UNIVERSAL_PALETTE ? "ON (+48 MB)" : "OFF");
    print_stat("Stream Size",       human_size(stream_size));
    print_stat("Generation Time",   elapsed, "sec");

    append_log("ATTEMPT #" + std::to_string(ATTEMPT));
    append_log("========== STREAM GENERATION ==========");
    append_log("Stream Size: " + human_size(stream_size));
    append_log("Generation Time: " + std::to_string(elapsed) + " sec");
    append_log("");

    return 0;
}