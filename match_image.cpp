// ============================================================
// FILE: match_image.cpp
// UPDATED ARCHITECTURE
//
// STAGE 1:
// Try FULL IMAGE MATCH
//
// STAGE 2:
// If failed -> segmented longest raw-data matching
//
// OUTPUT:
// compressed coordinate map
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
// GLOBAL STREAM
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
// IMAGE → BYTE ARRAY
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
            << "FAILED: "
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
// FULL IMAGE MATCH
// ============================================================

bool full_match_search(
    const std::vector<uint8_t>& target,
    uint64_t& found_offset
) {

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
// LONGEST SEGMENT MATCH
// ============================================================

SegmentMatch longest_match_from_position(
    const std::vector<uint8_t>& target,
    uint32_t start
) {

    SegmentMatch best;

    best.length = 0;

    size_t stream_size =
        stream_data.size();

    size_t target_size =
        target.size();

    for (
        size_t s = 0;
        s < stream_size;
        s++
    ) {

        uint32_t len = 0;

        while (

            start + len < target_size &&
            s + len < stream_size &&

            target[start + len]
            ==
            stream_data[s + len]

        ) {

            len++;
        }

        if (len > best.length) {

            best.length = len;

            best.stream_offset = s;

            best.target_offset = start;
        }
    }

    return best;
}

// ============================================================
// SEGMENTED MATCHING
// ============================================================

std::vector<SegmentMatch> segmented_match(
    const std::vector<uint8_t>& target
) {

    std::vector<SegmentMatch> matches;

    uint32_t pos = 0;

    while (pos < target.size()) {

        SegmentMatch best =
            longest_match_from_position(
                target,
                pos
            );

        // no usable match
        if (best.length < 12) {

            pos++;
            continue;
        }

        matches.push_back(best);

        pos += best.length;
    }

    return matches;
}

// ============================================================
// SAVE MATCH FILE
// ============================================================

void save_match_file(

    const std::string& out_path,

    uint32_t width,
    uint32_t height,

    const std::vector<SegmentMatch>& matches
) {

    FileHeader header;

    header.width = width;
    header.height = height;

    header.segment_count =
        matches.size();

    std::ofstream out(
        out_path,
        std::ios::binary
    );

    out.write(
        (char*)&header,
        sizeof(header)
    );

    out.write(
        (char*)matches.data(),
        matches.size()
        *
        sizeof(SegmentMatch)
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

    std::string name =
        fs::path(path)
        .stem()
        .string();

    std::string out_path =
        "MATCH/" +
        name +
        ".bin";

    // ========================================================
    // TRY FULL MATCH FIRST
    // ========================================================

    uint64_t full_offset = 0;

    bool full_found =
        full_match_search(
            target,
            full_offset
        );

    std::vector<SegmentMatch> matches;

    if (full_found) {

        std::cout
            << "\nFULL IMAGE MATCH FOUND\n";

        SegmentMatch m;

        m.stream_offset =
            full_offset;

        m.target_offset = 0;

        m.length =
            target.size();

        matches.push_back(m);
    }

    else {

        std::cout
            << "\nFULL MATCH FAILED\n";

        std::cout
            << "STARTING SEGMENT SEARCH...\n";

        matches =
            segmented_match(target);

        std::cout
            << "Segments Found: "
            << matches.size()
            << "\n";
    }

    // ========================================================
    // SAVE
    // ========================================================

    save_match_file(

        out_path,

        width,
        height,

        matches
    );

    double elapsed =
        timer.toc();

    uint64_t original_size =
        file_size_bytes(path);

    uint64_t compressed_size =
        file_size_bytes(out_path);

    double ratio =
        (double)original_size /
        (double)compressed_size;

    // ========================================================
    // TERMINAL OUTPUT
    // ========================================================

    std::cout
        << "\n========== MATCH RESULTS ==========\n";

    print_stat(
        "Image",
        path
    );

    print_stat(
        "Original Size",
        human_size(original_size)
    );

    print_stat(
        "Compressed Map Size",
        human_size(compressed_size)
    );

    print_stat(
        "Compression Ratio",
        ratio,
        "x"
    );

    print_stat(
        "Segment Count",
        matches.size()
    );

    print_stat(
        "Matching Time",
        elapsed,
        "sec"
    );

    // ========================================================
    // LOG FILE
    // ========================================================

    append_log(
        "========== MATCH RESULTS =========="
    );

    append_log(
        "Image: " + path
    );

    append_log(
        "Original Size: " +
        human_size(original_size)
    );

    append_log(
        "Compressed Size: " +
        human_size(compressed_size)
    );

    append_log(
        "Compression Ratio: " +
        std::to_string(ratio)
    );

    append_log(
        "Segment Count: " +
        std::to_string(matches.size())
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