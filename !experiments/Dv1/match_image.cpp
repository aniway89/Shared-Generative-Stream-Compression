
// ============================================================
// FILE: match_image.cpp
// ROW INDEXED CROP MATCHER
// ============================================================

#include <iostream>
#include <fstream>
#include <vector>
#include <filesystem>
#include <opencv2/opencv.hpp>

#include "logger.h"

namespace fs = std::filesystem;

// ============================================================
// STRUCTS
// ============================================================

struct RowIndex {

    uint64_t stream_offset;

    uint32_t image_id;

    uint32_t row;

    uint32_t width;
};

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

std::vector<RowIndex> row_index;

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

    std::cout
        << "Loaded Stream: "
        << human_size(size)
        << "\n";
}

// ============================================================
// LOAD ROW INDEX
// ============================================================

void load_row_index() {

    std::ifstream in(
        "STREAM/row_index.bin",
        std::ios::binary
    );

    uint64_t count;

    in.read(
        (char*)&count,
        sizeof(count)
    );

    row_index.resize(count);

    in.read(
        (char*)row_index.data(),
        count * sizeof(RowIndex)
    );

    in.close();
}

// ============================================================
// IMAGE TO BYTES
// ============================================================

std::vector<uint8_t> image_to_bytes(

    const std::string& path,

    uint32_t& width,
    uint32_t& height
) {

    cv::Mat img =
        cv::imread(path);

    std::vector<uint8_t> bytes;

    if (img.empty())
        return bytes;

    width =
        img.cols;

    height =
        img.rows;

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
// ROW SEARCH
// ============================================================

bool crop_match_search(

    const std::vector<uint8_t>& target,

    uint32_t crop_width,
    uint32_t crop_height,

    uint64_t& found_offset
) {

    uint32_t crop_row_bytes =
        crop_width * 3;

    for (const auto& r : row_index) {

        // full image width
        uint32_t full_width =
            r.width;

        // crop cannot fit
        if (crop_width > full_width)
            continue;

        // horizontal sliding
        for (
            uint32_t x_offset = 0;
            x_offset <= full_width - crop_width;
            x_offset++
        ) {

            bool valid = true;

            for (
                uint32_t y = 0;
                y < crop_height;
                y++
            ) {

                uint64_t stream_pos =

                    r.stream_offset
                    +
                    (uint64_t)y
                    *
                    full_width
                    *
                    3
                    +
                    x_offset * 3;

                uint64_t target_pos =
                    (uint64_t)y
                    *
                    crop_row_bytes;

                if (
                    stream_pos + crop_row_bytes
                    > stream_data.size()
                ) {

                    valid = false;
                    break;
                }

                for (
                    uint32_t i = 0;
                    i < crop_row_bytes;
                    i++
                ) {

                    if (
                        stream_data[stream_pos + i]
                        !=
                        target[target_pos + i]
                    ) {

                        valid = false;
                        break;
                    }
                }

                if (!valid)
                    break;
            }

            if (valid) {

                found_offset =

                    r.stream_offset
                    +
                    x_offset * 3;

                return true;
            }
        }
    }

    return false;
}


// ============================================================
// SAVE MATCH
// ============================================================

void save_match_file(

    const std::string& out_path,

    uint32_t width,
    uint32_t height,

    uint64_t offset,
    uint32_t length
) {

    FileHeader header;

    header.width =
        width;

    header.height =
        height;

    header.segment_count =
        1;

    SegmentMatch match;

    match.stream_offset =
        offset;

    match.target_offset =
        0;

    match.length =
        length;

    std::ofstream out(
        out_path,
        std::ios::binary
    );

    out.write(
        (char*)&header,
        sizeof(header)
    );

    out.write(
        (char*)&match,
        sizeof(match)
    );

    out.close();
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

    uint64_t offset;

    bool found =
        crop_match_search(
            target,
            width,
            height,
            offset
        );

    if (!found) {

        std::cout
            << "\nNO MATCH FOUND\n";

        return;
    }

    std::cout
        << "\nMATCH FOUND\n";

    std::string name =
        fs::path(path)
        .stem()
        .string();

    std::string out_path =
        "MATCH/" +
        name +
        ".bin";

    save_match_file(

        out_path,

        width,
        height,

        offset,

        target.size()
    );

    double elapsed =
        timer.toc();

    std::cout
        << "\n========== MATCH RESULTS ==========\n";

    print_stat(
        "Image",
        path
    );

    print_stat(
        "Matching Time",
        elapsed,
        "sec"
    );
}

// ============================================================
// MAIN
// ============================================================

int main() {

    fs::create_directory("MATCH");

    load_stream();

    load_row_index();

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
