// ============================================================
// FILE: match_image.cpp
// FAST ARCHITECTURE — Rabin-Karp rolling hash + suffix-based segment search
//
// STAGE 1: Full image match via Rabin-Karp    O(N + M)
// STAGE 2: Segmented match via hash index     O(N + M * K/chunk)
//
// ============================================================

#include <iostream>
#include <fstream>
#include <vector>
#include <unordered_map>
#include <filesystem>
#include <opencv2/opencv.hpp>
#include "logger.h"

namespace fs = std::filesystem;

// ============================================================
// TUNING CONSTANTS
// ============================================================

// Minimum segment length to bother recording
static constexpr uint32_t MIN_MATCH_LEN = 12;

// Chunk size used when building the hash index for Stage 2.
// Smaller = more granular matches but more memory.
// 64 bytes is a sweet spot for image data.
static constexpr uint32_t INDEX_CHUNK = 64;

// Rabin-Karp rolling hash parameters
static constexpr uint64_t RK_BASE  = 257;
static constexpr uint64_t RK_MOD   = (1ULL << 61) - 1; // Mersenne prime

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

// Hash index: hash of every INDEX_CHUNK-byte window → list of stream offsets
// Built once, reused for every image.
std::unordered_map<uint64_t, std::vector<uint64_t>> chunk_index;

// ============================================================
// MODULAR MULTIPLY (avoids 128-bit overflow for Mersenne prime)
// ============================================================

inline uint64_t mod_mul(uint64_t a, uint64_t b) {
    return (__uint128_t)a * b % RK_MOD;
}

inline uint64_t mod_add(uint64_t a, uint64_t b) {
    a += b;
    if (a >= RK_MOD) a -= RK_MOD;
    return a;
}

// ============================================================
// LOAD STREAM + BUILD CHUNK INDEX
// ============================================================

void load_stream() {

    std::ifstream in("STREAM/stream.bin", std::ios::binary);
    in.seekg(0, std::ios::end);
    size_t size = in.tellg();
    in.seekg(0);
    stream_data.resize(size);
    in.read((char*)stream_data.data(), size);
    in.close();

    std::cout << "Loaded Stream: " << human_size(size) << "\n";
    std::cout << "Building chunk index (chunk=" << INDEX_CHUNK << ")...\n";

    // Rolling hash over every INDEX_CHUNK-byte window in the stream.
    // We store the START offset of each window.
    if (size < INDEX_CHUNK) return;

    // Precompute BASE^(INDEX_CHUNK-1) mod MOD
    uint64_t base_pow = 1;
    for (uint32_t i = 0; i < INDEX_CHUNK - 1; i++)
        base_pow = mod_mul(base_pow, RK_BASE);

    uint64_t h = 0;
    for (uint32_t i = 0; i < INDEX_CHUNK; i++)
        h = mod_add(mod_mul(h, RK_BASE), stream_data[i]);

    chunk_index[h].push_back(0);

    for (size_t i = 1; i + INDEX_CHUNK <= size; i++) {
        // Roll: remove outgoing byte, add incoming byte
        h = mod_add(
            mod_mul(h, RK_BASE),
            stream_data[i + INDEX_CHUNK - 1]
        );
        uint64_t outgoing = mod_mul(
            mod_mul(base_pow, RK_BASE),   // base_pow * BASE
            stream_data[i - 1]
        );
        // h -= outgoing (mod RK_MOD)
        h = (h + RK_MOD - outgoing % RK_MOD) % RK_MOD;

        chunk_index[h].push_back(i);
    }

    std::cout << "Index entries: " << chunk_index.size() << "\n";
}

// ============================================================
// IMAGE → BYTE ARRAY  (BGR row-major, same as before)
// ============================================================

std::vector<uint8_t> image_to_bytes(
    const std::string& path,
    uint32_t& width,
    uint32_t& height
) {
    cv::Mat img = cv::imread(path);
    std::vector<uint8_t> bytes;
    if (img.empty()) {
        std::cout << "FAILED: " << path << "\n";
        return bytes;
    }
    width  = img.cols;
    height = img.rows;
    bytes.reserve(width * height * 3);
    for (int y = 0; y < img.rows; y++)
        for (int x = 0; x < img.cols; x++) {
            cv::Vec3b p = img.at<cv::Vec3b>(y, x);
            bytes.push_back(p[0]);
            bytes.push_back(p[1]);
            bytes.push_back(p[2]);
        }
    return bytes;
}

// ============================================================
// FULL IMAGE MATCH — Rabin-Karp  O(N + M)
// ============================================================

bool full_match_search(
    const std::vector<uint8_t>& target,
    uint64_t& found_offset
) {
    size_t M = target.size();
    size_t N = stream_data.size();
    if (M > N) return false;

    // Precompute BASE^(M-1) mod MOD
    uint64_t base_pow = 1;
    for (size_t i = 0; i < M - 1; i++)
        base_pow = mod_mul(base_pow, RK_BASE);

    // Hash of target
    uint64_t target_hash = 0;
    for (size_t i = 0; i < M; i++)
        target_hash = mod_add(mod_mul(target_hash, RK_BASE), target[i]);

    // Rolling hash over stream
    uint64_t h = 0;
    for (size_t i = 0; i < M; i++)
        h = mod_add(mod_mul(h, RK_BASE), stream_data[i]);

    auto verify = [&](size_t offset) -> bool {
        for (size_t j = 0; j < M; j++)
            if (stream_data[offset + j] != target[j]) return false;
        return true;
    };

    if (h == target_hash && verify(0)) {
        found_offset = 0;
        return true;
    }

    for (size_t i = 1; i + M <= N; i++) {
        uint64_t outgoing = mod_mul(
            mod_mul(base_pow, RK_BASE),
            stream_data[i - 1]
        );
        h = mod_add(
            mod_mul(h, RK_BASE),
            stream_data[i + M - 1]
        );
        h = (h + RK_MOD - outgoing % RK_MOD) % RK_MOD;

        if (h == target_hash && verify(i)) {
            found_offset = i;
            return true;
        }
    }
    return false;
}

// ============================================================
// EXTEND MATCH — given a confirmed anchor, grow it left+right
// ============================================================

uint32_t extend_match(
    const std::vector<uint8_t>& target,
    uint32_t  target_anchor,   // position in target where chunk starts
    uint64_t  stream_anchor,   // position in stream where chunk starts
    uint64_t& best_stream_off,
    uint32_t& best_target_off
) {
    size_t Tsz = target.size();
    size_t Ssz = stream_data.size();

    // Extend forward
    uint32_t fwd = INDEX_CHUNK;
    while (
        target_anchor + fwd < Tsz &&
        stream_anchor + fwd < Ssz &&
        target[target_anchor + fwd] == stream_data[stream_anchor + fwd]
    ) fwd++;

    // Extend backward
    uint32_t bwd = 0;
    while (
        bwd < target_anchor &&
        bwd < stream_anchor &&
        target[target_anchor - bwd - 1] == stream_data[stream_anchor - bwd - 1]
    ) bwd++;

    best_stream_off = stream_anchor - bwd;
    best_target_off = target_anchor - bwd;
    return fwd + bwd;
}

// ============================================================
// SEGMENTED MATCH — hash-indexed, O(N + M * avg_collisions)
// ============================================================

std::vector<SegmentMatch> segmented_match(
    const std::vector<uint8_t>& target
) {
    std::vector<SegmentMatch> matches;
    size_t Tsz = target.size();

    if (Tsz < INDEX_CHUNK) return matches;

    // Precompute BASE^(INDEX_CHUNK-1) mod MOD
    uint64_t base_pow = 1;
    for (uint32_t i = 0; i < INDEX_CHUNK - 1; i++)
        base_pow = mod_mul(base_pow, RK_BASE);

    // Roll a hash over the target in INDEX_CHUNK-wide windows,
    // look each up in the pre-built stream index.
    uint32_t pos = 0;

    // Build initial hash for first window
    uint64_t h = 0;
    for (uint32_t i = 0; i < INDEX_CHUNK && i < Tsz; i++)
        h = mod_add(mod_mul(h, RK_BASE), target[i]);

    while (pos + INDEX_CHUNK <= Tsz) {

        auto it = chunk_index.find(h);

        if (it != chunk_index.end()) {

            // Candidate anchors found — find the longest extension
            SegmentMatch best;
            best.length = 0;

            for (uint64_t soff : it->second) {
                // Quick byte-verify the chunk (hash collision guard)
                bool ok = true;
                for (uint32_t k = 0; k < INDEX_CHUNK; k++) {
                    if (target[pos + k] != stream_data[soff + k]) {
                        ok = false; break;
                    }
                }
                if (!ok) continue;

                uint64_t ext_soff;
                uint32_t ext_toff;
                uint32_t len = extend_match(
                    target, pos, soff,
                    ext_soff, ext_toff
                );

                if (len > best.length) {
                    best.length       = len;
                    best.stream_offset = ext_soff;
                    best.target_offset = ext_toff;
                }
            }

            if (best.length >= MIN_MATCH_LEN) {
                matches.push_back(best);
                // Jump past this match
                uint32_t jump = best.length;
                // Re-roll hash from new position
                pos += jump;
                if (pos + INDEX_CHUNK <= Tsz) {
                    h = 0;
                    for (uint32_t i = 0; i < INDEX_CHUNK; i++)
                        h = mod_add(mod_mul(h, RK_BASE), target[pos + i]);
                }
                continue;
            }
        }

        // Advance by 1, roll hash
        if (pos + INDEX_CHUNK < Tsz) {
            uint64_t outgoing = mod_mul(
                mod_mul(base_pow, RK_BASE),
                target[pos]
            );
            h = mod_add(
                mod_mul(h, RK_BASE),
                target[pos + INDEX_CHUNK]
            );
            h = (h + RK_MOD - outgoing % RK_MOD) % RK_MOD;
        }
        pos++;
    }

    return matches;
}

// ============================================================
// SAVE MATCH FILE  (identical format to original)
// ============================================================

void save_match_file(
    const std::string& out_path,
    uint32_t width,
    uint32_t height,
    const std::vector<SegmentMatch>& matches
) {
    FileHeader header;
    header.width         = width;
    header.height        = height;
    header.segment_count = (uint32_t)matches.size();

    std::ofstream out(out_path, std::ios::binary);
    out.write((char*)&header, sizeof(header));
    out.write((char*)matches.data(), matches.size() * sizeof(SegmentMatch));
    out.close();
}

// ============================================================
// PROCESS IMAGE
// ============================================================

void process_image(const std::string& path) {

    BenchmarkTimer timer;
    timer.tic();

    uint32_t width, height;
    std::vector<uint8_t> target = image_to_bytes(path, width, height);
    if (target.empty()) return;

    std::string name     = fs::path(path).stem().string();
    std::string out_path = "MATCH/" + name + ".bin";

    // --------------------------------------------------------
    // STAGE 1: Full match (Rabin-Karp)
    // --------------------------------------------------------

    uint64_t full_offset = 0;
    bool full_found = full_match_search(target, full_offset);

    std::vector<SegmentMatch> matches;

    if (full_found) {
        std::cout << "\nFULL IMAGE MATCH FOUND at offset " << full_offset << "\n";
        SegmentMatch m;
        m.stream_offset = full_offset;
        m.target_offset = 0;
        m.length        = (uint32_t)target.size();
        matches.push_back(m);
    } else {
        std::cout << "\nFULL MATCH FAILED — starting indexed segment search...\n";
        matches = segmented_match(target);
        std::cout << "Segments Found: " << matches.size() << "\n";
    }

    // --------------------------------------------------------
    // SAVE
    // --------------------------------------------------------

    save_match_file(out_path, width, height, matches);

    double   elapsed         = timer.toc();
    uint64_t original_size   = file_size_bytes(path);
    uint64_t compressed_size = file_size_bytes(out_path);
    double   ratio           = (double)original_size / (double)compressed_size;

    // Terminal
    std::cout << "\n========== MATCH RESULTS ==========\n";
    print_stat("Image",            path);
    print_stat("Original Size",    human_size(original_size));
    print_stat("Compressed Size",  human_size(compressed_size));
    print_stat("Compression Ratio", ratio, "x");
    print_stat("Segment Count",    matches.size());
    print_stat("Matching Time",    elapsed, "sec");

    // Log
    append_log("========== MATCH RESULTS ==========");
    append_log("Image: "            + path);
    append_log("Original Size: "   + human_size(original_size));
    append_log("Compressed Size: " + human_size(compressed_size));
    append_log("Compression Ratio: " + std::to_string(ratio));
    append_log("Segment Count: "   + std::to_string(matches.size()));
    append_log("Matching Time: "   + std::to_string(elapsed));
    append_log("");
}

// ============================================================
// MAIN
// ============================================================

int main() {
    fs::create_directory("MATCH");
    load_stream();   // builds chunk_index here, once

    for (auto& p : fs::directory_iterator("COMP_IMG"))
        process_image(p.path().string());

    return 0;
}