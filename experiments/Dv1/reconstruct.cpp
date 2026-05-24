// ============================================================
// FILE: reconstruct.cpp
// ============================================================

#include <iostream>
#include <fstream>
#include <vector>
#include <filesystem>
#include <opencv2/opencv.hpp>

namespace fs = std::filesystem;

// ============================================================
// STRUCTS
// ============================================================

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

// ============================================================
// GLOBALS
// ============================================================

std::vector<uint8_t> stream_data;

// ============================================================
// LOAD STREAM
// ============================================================

void load_stream() {

    std::ifstream in(
        "STREAM/stream.bin",
        std::ios::binary
    );

    in.seekg(0, std::ios::end);

    size_t size =
        in.tellg();

    in.seekg(0);

    stream_data.resize(size);

    in.read(
        (char*)stream_data.data(),
        size
    );

    in.close();
}

// ============================================================
// RECONSTRUCT
// ============================================================

void reconstruct_file(
    const std::string& map_path
) {

    std::ifstream in(
        map_path,
        std::ios::binary
    );

    FileHeader header;

    in.read(
        (char*)&header,
        sizeof(header)
    );

    SegmentMatch match;

    in.read(
        (char*)&match,
        sizeof(match)
    );

    in.close();

    std::vector<uint8_t> bytes(

        stream_data.begin()
        +
        match.stream_offset,

        stream_data.begin()
        +
        match.stream_offset
        +
        match.length
    );

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

            p[0] = bytes[idx++];
            p[1] = bytes[idx++];
            p[2] = bytes[idx++];
        }
    }

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

    std::cout
        << "Reconstructed: "
        << out_path
        << "\n";
}

// ============================================================
// MAIN
// ============================================================

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
