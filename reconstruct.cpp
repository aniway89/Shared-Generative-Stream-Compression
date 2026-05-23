// ============================================================
// FILE: reconstruct.cpp
//
// FIX: Added segment validation and debug output to diagnose
//      black image issues (zero segment count, bad offsets).
// FIX: Pixel write loop uses cv::Mat::data directly (row-major
//      BGR flat copy) instead of manual at<Vec3b> indexing,
//      matching exactly how image_to_bytes() serialised the data.
// FIX: total_bytes uses size_t (not uint64_t cast) consistently
//      so the bounds checks never silently truncate.
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

// ============================================================
// LOAD STREAM
// ============================================================

void load_stream() {

    std::ifstream in("STREAM/stream.bin", std::ios::binary);

    if (!in.is_open()) {
        std::cerr << "ERROR: Cannot open STREAM/stream.bin\n";
        return;
    }

    in.seekg(0, std::ios::end);
    size_t size = in.tellg();
    in.seekg(0);

    stream_data.resize(size);
    in.read((char*)stream_data.data(), size);
    in.close();

    std::cout << "Stream loaded: " << human_size(size) << "\n";
}

// ============================================================
// PIXEL VERIFY
// ============================================================

bool verify_pixels(
    const std::string& original_path,
    const std::string& reconstructed_path
) {
    cv::Mat original     = cv::imread(original_path);
    cv::Mat reconstructed = cv::imread(reconstructed_path);

    if (original.empty() || reconstructed.empty())
        return false;

    if (
        original.rows != reconstructed.rows ||
        original.cols != reconstructed.cols
    )
        return false;

    cv::Mat diff;
    cv::absdiff(original, reconstructed, diff);

    int non_zero = cv::countNonZero(diff.reshape(1));
    return non_zero == 0;
}

// ============================================================
// RECONSTRUCT FILE
// ============================================================

void reconstruct_file(const std::string& map_path) {

    BenchmarkTimer timer;
    timer.tic();

    std::ifstream in(map_path, std::ios::binary);

    if (!in.is_open()) {
        std::cerr << "Failed to open map file: " << map_path << "\n";
        return;
    }

    // ========================================================
    // READ HEADER
    // ========================================================

    FileHeader header;
    in.read((char*)&header, sizeof(header));

    std::cout << "\n--- Reconstructing: " << map_path << " ---\n";
    std::cout << "  Image size : " << header.width << " x " << header.height << "\n";
    std::cout << "  Segments   : " << header.segment_count << "\n";

    if (header.width == 0 || header.height == 0) {
        std::cerr << "ERROR: Invalid image dimensions in header.\n";
        in.close();
        return;
    }

    if (header.segment_count == 0) {
        std::cerr << "ERROR: No segments in match file — "
                     "image will be entirely black.\n"
                     "Re-run match_image with lower MIN_MATCH_LEN / INDEX_CHUNK.\n";
        in.close();
        return;
    }

    // ========================================================
    // READ SEGMENTS
    // ========================================================

    std::vector<SegmentMatch> matches(header.segment_count);
    in.read(
        (char*)matches.data(),
        sizeof(SegmentMatch) * header.segment_count
    );
    in.close();

    // Print first few segments for debugging
    size_t preview = std::min((size_t)5, matches.size());
    for (size_t i = 0; i < preview; i++) {
        std::cout << "  seg[" << i << "]"
                  << "  stream_off=" << matches[i].stream_offset
                  << "  target_off=" << matches[i].target_offset
                  << "  len="        << matches[i].length << "\n";
    }

    // ========================================================
    // CREATE OUTPUT BYTE BUFFER
    // ========================================================

    // Use size_t throughout — avoids silent truncation on large images
    size_t total_bytes =
        (size_t)header.width * (size_t)header.height * 3;

    std::vector<uint8_t> reconstructed(total_bytes, 0);

    // ========================================================
    // COVERAGE TRACKING
    // ========================================================

    size_t bytes_written = 0;
    size_t segments_used = 0;
    size_t segments_skipped = 0;

    // ========================================================
    // REBUILD IMAGE — copy each segment from stream into buffer
    // ========================================================

    for (const auto& m : matches) {

        // Validate stream bounds
        if (m.stream_offset + m.length > stream_data.size()) {
            std::cerr << "  SKIP seg: stream OOB ("
                      << m.stream_offset << "+" << m.length
                      << " > " << stream_data.size() << ")\n";
            segments_skipped++;
            continue;
        }

        // Validate target bounds
        if ((size_t)m.target_offset + m.length > total_bytes) {
            std::cerr << "  SKIP seg: target OOB ("
                      << m.target_offset << "+" << m.length
                      << " > " << total_bytes << ")\n";
            segments_skipped++;
            continue;
        }

        memcpy(
            reconstructed.data() + m.target_offset,
            stream_data.data()   + m.stream_offset,
            m.length
        );

        bytes_written += m.length;
        segments_used++;
    }

    // Coverage report
    double coverage = 100.0 * (double)bytes_written / (double)total_bytes;
    std::cout << "  Coverage   : " << coverage << "%"
              << "  (" << bytes_written << "/" << total_bytes << " bytes)\n";
    std::cout << "  Segs used  : " << segments_used
              << "  skipped: "    << segments_skipped << "\n";

    if (coverage < 99.9) {
        std::cerr << "  WARNING: " << (total_bytes - bytes_written)
                  << " bytes unmatched — partial black areas expected.\n";
    }

    // ========================================================
    // BYTE ARRAY → cv::Mat
    //
    // image_to_bytes() in match_image.cpp writes pixels in
    // BGR row-major order (exactly what OpenCV stores natively).
    // We copy the flat buffer directly into Mat::data instead of
    // going pixel-by-pixel, which is both faster and correct.
    // ========================================================

    cv::Mat output(header.height, header.width, CV_8UC3);

    // Mat::data is a contiguous BGR buffer when isContinuous() == true
    if (output.isContinuous()) {
        memcpy(output.data, reconstructed.data(), total_bytes);
    } else {
        // Fallback: row-by-row copy (handles non-contiguous Mat)
        size_t row_bytes = (size_t)header.width * 3;
        for (int y = 0; y < output.rows; y++) {
            memcpy(
                output.ptr(y),
                reconstructed.data() + (size_t)y * row_bytes,
                row_bytes
            );
        }
    }

    // ========================================================
    // SAVE OUTPUT
    // ========================================================

    std::string name     = fs::path(map_path).stem().string();
    std::string out_path = "OUT/" + name + ".png";

    bool saved = cv::imwrite(out_path, output);
    if (!saved) {
        std::cerr << "ERROR: cv::imwrite failed for " << out_path << "\n";
        return;
    }

    double elapsed = timer.toc();

    std::cout << "\n========== RECONSTRUCTION ==========\n";
    print_stat("Map File",           map_path);
    print_stat("Output",             out_path);
    print_stat("Segments Used",      segments_used);
    print_stat("Coverage",           coverage, "%");
    print_stat("Reconstruction Time", elapsed, "sec");
    std::cout << "Reconstruction Complete\n";

    append_log("========== RECONSTRUCTION ==========");
    append_log("Map File: "             + map_path);
    append_log("Output: "              + out_path);
    append_log("Segments Used: "       + std::to_string(segments_used));
    append_log("Coverage: "            + std::to_string(coverage) + "%");
    append_log("Reconstruction Time: " + std::to_string(elapsed));
    append_log("");
}

// ============================================================
// MAIN
// ============================================================

int main() {

    fs::create_directory("OUT");

    load_stream();

    if (stream_data.empty()) {
        std::cerr << "FATAL: Stream data is empty. Aborting.\n";
        return 1;
    }

    for (auto& p : fs::directory_iterator("MATCH")) {
        reconstruct_file(p.path().string());
    }

    return 0;
}