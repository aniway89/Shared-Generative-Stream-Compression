// ============================================================
// FILE: reconstruct.cpp
// PURPOSE:
// Reconstruct image from stream
// ============================================================

#include <iostream>
#include <fstream>
#include <vector>
#include <filesystem>
#include <opencv2/opencv.hpp>
#include "logger.h"

namespace fs = std::filesystem;

struct StreamMatch {

    uint64_t stream_offset;
    uint32_t length;

    uint32_t width;
    uint32_t height;
};

std::vector<uint8_t> stream_data;

void load_stream() {

    std::ifstream in(
        "STREAM/stream.bin",
        std::ios::binary
    );

    in.seekg(0, std::ios::end);

    size_t size = in.tellg();

    in.seekg(0);

    stream_data.resize(size);

    in.read(
        (char*)stream_data.data(),
        size
    );

    in.close();
}

void reconstruct_file(
    const std::string& map_path
) {

    BenchmarkTimer timer;

    timer.tic();

    std::ifstream in(
        map_path,
        std::ios::binary
    );

    StreamMatch match;

    in.read(
        (char*)&match,
        sizeof(match)
    );

    in.close();

    uint64_t expected =
        match.width *
        match.height *
        3;

    if (
        match.length != expected
    ) {

        std::cout
            << "PARTIAL MATCH DETECTED\n";

        std::cout
            << "Cannot reconstruct exact image\n";

        return;
    }

    std::vector<uint8_t> bytes(
        stream_data.begin() +
        match.stream_offset,

        stream_data.begin() +
        match.stream_offset +
        match.length
    );

    cv::Mat output(
        match.height,
        match.width,
        CV_8UC3
    );

    size_t idx = 0;

    for (int y = 0;
         y < match.height;
         y++) {

        for (int x = 0;
             x < match.width;
             x++) {

            cv::Vec3b& p =
                output.at<cv::Vec3b>(y, x);

            p[0] = bytes[idx++];
            p[1] = bytes[idx++];
            p[2] = bytes[idx++];
        }
    }

    std::string name =
        fs::path(map_path).stem().string();

    cv::imwrite(
        "OUT/" + name + ".png",
        output
    );

    double elapsed = timer.toc();

    uint64_t recon_size =
        file_size_bytes(
            "OUT/" + name + ".png"
        );

    std::cout
        << "\n========== RECONSTRUCTION ==========\n";

    print_stat(
        "Reconstructed File Size",
        mb(recon_size),
        "MB"
    );

    print_stat(
        "Reconstruction Time",
        elapsed,
        "sec"
    );

    append_log(
        "========== RECONSTRUCTION =========="
    );

    append_log(
        "Reconstructed MB: " +
        std::to_string(mb(recon_size))
    );

    append_log(
        "Reconstruction Time Sec: " +
        std::to_string(elapsed)
    );

    append_log("");

    std::cout
        << "Reconstruction Complete\n";
}

int main() {

    fs::create_directory("OUT");

    load_stream();

    for (
        auto& p :
        fs::directory_iterator("MATCH")
    ) {

        reconstruct_file(
            p.path().string()
        );
    }

    return 0;
}