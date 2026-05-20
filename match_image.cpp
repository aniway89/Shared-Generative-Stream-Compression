// ============================================================
// FILE: match_image.cpp
// PURPOSE:
// Find longest exact byte sequence inside universal stream
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

// ============================================================
// LOAD UNIVERSAL STREAM
// ============================================================

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

    std::cout
        << "Loaded Stream: "
        << human_size(size)
        << "\n";
}

// ============================================================
// IMAGE → RAW BYTE ARRAY
// ============================================================

std::vector<uint8_t> image_to_bytes(
    const std::string& path,
    uint32_t& width,
    uint32_t& height
) {

    cv::Mat img =
        cv::imread(path);

    std::vector<uint8_t> bytes;

    if (img.empty()) {

        std::cout
            << "Failed loading: "
            << path
            << "\n";

        return bytes;
    }

    width = img.cols;
    height = img.rows;

    bytes.reserve(
        width * height * 3
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

// ============================================================
// BRUTE FORCE STREAM SEARCH
// ============================================================

bool find_exact_match(
    const std::vector<uint8_t>& target,
    uint64_t& found_offset
) {

    if (
        target.empty() ||
        stream_data.empty()
    )
        return false;

    size_t target_size =
        target.size();

    size_t stream_size =
        stream_data.size();

    for (
        size_t i = 0;
        i <= stream_size - target_size;
        i++
    ) {

        bool match = true;

        for (
            size_t j = 0;
            j < target_size;
            j++
        ) {

            if (
                stream_data[i + j]
                != target[j]
            ) {

                match = false;
                break;
            }
        }

        if (match) {

            found_offset = i;
            return true;
        }
    }

    return false;
}

// ============================================================
// PROCESS IMAGE
// ============================================================

void process_image(
    const std::string& path
) {

    BenchmarkTimer timer;

    timer.tic();

    uint32_t width;
    uint32_t height;

    std::vector<uint8_t> target =
        image_to_bytes(
            path,
            width,
            height
        );

    uint64_t found_offset = 0;

    bool found =
        find_exact_match(
            target,
            found_offset
        );

    if (!found) {

        std::cout
            << "\nNO MATCH FOUND\n";

        append_log(
            "NO MATCH FOUND: " +
            path
        );

        return;
    }

    StreamMatch match;

    match.stream_offset =
        found_offset;

    match.length =
        target.size();

    match.width =
        width;

    match.height =
        height;

    std::string name =
        fs::path(path)
        .stem()
        .string();

    std::string out_path =
        "MATCH/" +
        name +
        ".bin";

    std::ofstream out(
        out_path,
        std::ios::binary
    );

    out.write(
        (char*)&match,
        sizeof(match)
    );

    out.close();

    double elapsed =
        timer.toc();

    uint64_t original_size =
        file_size_bytes(path);

    uint64_t coord_size =
        file_size_bytes(out_path);

    double ratio =
        (double)original_size /
        (double)coord_size;

    std::cout
        << "\n========== MATCH RESULTS ==========\n";

    print_stat(
        "Image",
        path
    );

    print_stat(
        "Match Found",
        "YES"
    );

    print_stat(
        "Stream Offset",
        std::to_string(found_offset)
    );

    print_stat(
        "Original Size",
        human_size(original_size)
    );

    print_stat(
        "Coordinate File Size",
        human_size(coord_size)
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

    append_log(
        "========== MATCH RESULTS =========="
    );

    append_log(
        "Image: " + path
    );

    append_log(
        "Match Found: YES"
    );

    append_log(
        "Stream Offset: " +
        std::to_string(found_offset)
    );

    append_log(
        "Original Size: " +
        human_size(original_size)
    );

    append_log(
        "Coordinate File Size: " +
        human_size(coord_size)
    );

    append_log(
        "Compression Ratio: " +
        std::to_string(ratio)
    );

    append_log(
        "Matching Time: " +
        std::to_string(elapsed)
    );

    append_log("");
}

// ============================================================
// MAIN
// ============================================================

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