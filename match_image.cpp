// ============================================================
// FILE: match_image.cpp
// PURPOSE:
// Continuous stream subsequence matching
// ============================================================

#include <iostream>
#include <vector>
#include <fstream>
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

std::vector<uint8_t> image_to_bytes(cv::Mat& img) {

    std::vector<uint8_t> bytes;

    bytes.reserve(
        img.rows * img.cols * 3
    );

    for (int y = 0; y < img.rows; y++) {

        for (int x = 0; x < img.cols; x++) {

            cv::Vec3b p =
                img.at<cv::Vec3b>(y, x);

            bytes.push_back(p[0]);
            bytes.push_back(p[1]);
            bytes.push_back(p[2]);
        }
    }

    return bytes;
}

StreamMatch find_match(
    const std::vector<uint8_t>& img
) {

    StreamMatch best = {0,0,0,0};

    size_t stream_size = stream_data.size();
    size_t img_size = img.size();

    for (size_t s = 0;
         s + 8 < stream_size;
         s++) {

        if (
            *(uint64_t*)&stream_data[s] !=
            *(uint64_t*)&img[0]
        )
            continue;

        size_t matched = 0;

        while (
            s + matched < stream_size &&
            matched < img_size &&
            stream_data[s + matched] ==
            img[matched]
        ) {
            matched++;
        }

        if (matched > best.length) {

            best.stream_offset = s;
            best.length = matched;
        }

        if (matched == img_size)
            break;
    }

    return best;
}

void process_image(
    const std::string& path
) {

    BenchmarkTimer timer;

    timer.tic();

    cv::Mat img = cv::imread(path);

    if (img.empty()) {

        std::cout
            << "Failed: "
            << path
            << "\n";

        return;
    }

    std::vector<uint8_t> img_bytes =
        image_to_bytes(img);

    StreamMatch match =
        find_match(img_bytes);

    match.width = img.cols;
    match.height = img.rows;

    std::string name =
        fs::path(path).stem().string();

    std::ofstream out(
        "MATCH/" + name + ".map",
        std::ios::binary
    );

    out.write(
        (char*)&match,
        sizeof(match)
    );

    out.close();

    double elapsed = timer.toc();

    uint64_t coord_size =
        file_size_bytes(
            "MATCH/" + name + ".map"
        );

    uint64_t original_size =
        file_size_bytes(path);

    uint64_t stream_size =
        file_size_bytes(
            "STREAM/stream.bin"
        );

    double transfer_ratio =
        (double)original_size /
        (double)coord_size;

    double total_ratio =
        (double)original_size /
        (double)(coord_size + stream_size);

    std::cout
        << "\n========== MATCH RESULTS ==========\n";

    print_stat(
        "Original File Size",
        mb(original_size),
        "MB"
    );

    print_stat(
        "Map File Size",
        mb(coord_size),
        "MB"
    );

    print_stat(
        "Transfer Compression",
        transfer_ratio,
        "x"
    );

    print_stat(
        "System Compression",
        total_ratio,
        "x"
    );

    print_stat(
        "Matching Time",
        elapsed,
        "sec"
    );

    print_stat(
        "Matched Length",
        match.length,
        "bytes"
    );

    if (
        match.length ==
        img.rows * img.cols * 3
    ) {

        std::cout
            << "FULL MATCH FOUND\n";
    }
    else {

        std::cout
            << "PARTIAL MATCH FOUND\n";
    }

    append_log(
        "========== MATCH RESULTS =========="
    );

    append_log(
        "Image: " + path
    );

    append_log(
        "Original MB: " +
        std::to_string(mb(original_size))
    );

    append_log(
        "Map MB: " +
        std::to_string(mb(coord_size))
    );

    append_log(
        "Transfer Compression: " +
        std::to_string(transfer_ratio)
    );

    append_log(
        "System Compression: " +
        std::to_string(total_ratio)
    );

    append_log(
        "Matched Length: " +
        std::to_string(match.length)
    );

    append_log(
        "Matching Time Sec: " +
        std::to_string(elapsed)
    );

    append_log("");

    std::cout
        << "Match Offset: "
        << match.stream_offset
        << "\n";
}

int main() {

    fs::create_directory("MATCH");

    load_stream();

    for (
        auto& p :
        fs::directory_iterator("COMP_IMG")
    ) {
        process_image(
            p.path().string()
        );
    }

    return 0;
}