// ============================================================
// FILE: reconstruct.cpp
// ============================================================

#include <iostream>
#include <fstream>
#include <vector>
#include <filesystem>
#include <opencv2/opencv.hpp>

#include "logger.h"

namespace fs = std::filesystem;


struct SegmentMatch {

    uint64_t stream_offset;

    uint32_t target_offset;

    uint32_t length;
};

struct FileHeader {

    uint32_t width;
    uint32_t height;

    uint32_t segment_count;
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

bool verify_pixels(
    const std::string& original_path,
    const std::string& reconstructed_path
) {

    cv::Mat original =
        cv::imread(original_path);

    cv::Mat reconstructed =
        cv::imread(reconstructed_path);

    if (
        original.empty() ||
        reconstructed.empty()
    )
        return false;

    if (
        original.rows != reconstructed.rows ||
        original.cols != reconstructed.cols
    )
        return false;

    cv::Mat diff;

    cv::absdiff(
        original,
        reconstructed,
        diff
    );

    int non_zero =
        cv::countNonZero(
            diff.reshape(1)
        );

    return non_zero == 0;
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

    if (!in.is_open()) {

        std::cout
            << "Failed to open map file\n";

        return;
    }

    // ========================================================
    // READ HEADER
    // ========================================================

    FileHeader header;

    in.read(
        (char*)&header,
        sizeof(header)
    );

    // ========================================================
    // READ SEGMENTS
    // ========================================================

    std::vector<SegmentMatch> matches(
        header.segment_count
    );

    in.read(
        (char*)matches.data(),
        sizeof(SegmentMatch)
        *
        header.segment_count
    );

    in.close();

    // ========================================================
    // CREATE OUTPUT BYTE BUFFER
    // ========================================================

    uint64_t total_bytes =
        (uint64_t)header.width
        *
        header.height
        *
        3;

    std::vector<uint8_t> reconstructed(
        total_bytes,
        0
    );

    // ========================================================
    // REBUILD IMAGE
    // ========================================================

    for (const auto& m : matches) {

        if (
            m.stream_offset + m.length
            > stream_data.size()
        )
            continue;

        if (
            m.target_offset + m.length
            > reconstructed.size()
        )
            continue;

        memcpy(

            reconstructed.data()
            + m.target_offset,

            stream_data.data()
            + m.stream_offset,

            m.length
        );
    }

    // ========================================================
    // BYTE ARRAY → IMAGE
    // ========================================================

    cv::Mat output(
        header.height,
        header.width,
        CV_8UC3
    );

    size_t idx = 0;

    for (
        int y = 0;
        y < output.rows;
        y++
    ) {

        for (
            int x = 0;
            x < output.cols;
            x++
        ) {

            cv::Vec3b& p =
                output.at<cv::Vec3b>(y, x);

            p[0] = reconstructed[idx++];
            p[1] = reconstructed[idx++];
            p[2] = reconstructed[idx++];
        }
    }

    // ========================================================
    // SAVE OUTPUT
    // ========================================================

    std::string name =
        fs::path(map_path)
        .stem()
        .string();

    std::string out_path =
        "OUT/" +
        name +
        ".png";

    cv::imwrite(
        out_path,
        output
    );

    double elapsed =
        timer.toc();

    std::cout
        << "\n========== RECONSTRUCTION ==========\n";

    print_stat(
        "Segments Used",
        matches.size()
    );

    print_stat(
        "Reconstruction Time",
        elapsed,
        "sec"
    );

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