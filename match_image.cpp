// ============================================================
// FILE: match_image.cpp
//
// TWO-ZONE STREAM DESIGN:
//   Zone 1 — training data (indexed with rolling hash)
//   Zone 2 — universal palette suffix (NOT indexed)
//             palette starts at a known offset stored in the
//             stream header so we can compute any pixel's
//             position by pure arithmetic, zero search needed.
//
// PALETTE LOOKUP FORMULA:
//   Given pixel (B, G, R):
//   offset = palette_start + (B*65536 + G*256 + R) * 3
//
// This means:
//   - index stays small (training data only)
//   - every pixel is guaranteed a match via palette
//   - no OOM from indexing 48M palette entries
// ============================================================

#include <iostream>
#include <fstream>
#include <vector>
#include <unordered_map>
#include <filesystem>
#include <opencv2/opencv.hpp>
#include "logger.h"
#include "config.h"

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
std::unordered_map<uint64_t, std::vector<uint64_t>> chunk_index;

// Offset where the universal palette begins inside stream.bin.
// 0 means no palette was appended.
uint64_t palette_start_offset = 0;

// ============================================================
// MODULAR ARITHMETIC
// ============================================================

inline uint64_t mod_mul(uint64_t a, uint64_t b) {
    return (__uint128_t)a * b % RK_MOD;
}

inline uint64_t mod_add(uint64_t a, uint64_t b) {
    a += b;
    if (a >= RK_MOD) a -= RK_MOD;
    return a;
}

inline uint64_t compute_base_pow(uint32_t len) {
    uint64_t bp = 1;
    for (uint32_t i = 0; i < len - 1; i++)
        bp = mod_mul(bp, RK_BASE);
    return bp;
}

inline uint64_t compute_hash(const std::vector<uint8_t>& buf,
                              size_t start, size_t len) {
    uint64_t h = 0;
    for (size_t i = 0; i < len; i++)
        h = mod_add(mod_mul(h, RK_BASE), buf[start + i]);
    return h;
}

inline uint64_t roll_hash(uint64_t h, uint64_t base_pow,
                           uint8_t old_byte, uint8_t new_byte) {
    h = mod_add(mod_mul(h, RK_BASE), new_byte);
    uint64_t sub = mod_mul(mod_mul(base_pow, RK_BASE), old_byte);
    h = (h + RK_MOD - sub % RK_MOD) % RK_MOD;
    return h;
}

// ============================================================
// DETECT PALETTE BOUNDARY
//
// The palette is preceded by STREAM_SEPARATOR_SIZE 0x00 bytes.
// We scan from the end of the stream backwards to find them.
// If found, palette_start_offset is set to the first byte of
// the actual BGR data (after the separator).
// ============================================================

void detect_palette_boundary() {

    if (!ENABLE_UNIVERSAL_PALETTE) return;

    // Palette is exactly 256*256*256*3 bytes
    const size_t PALETTE_BYTES = (size_t)256 * 256 * 256 * 3;
    const size_t SEP           = STREAM_SEPARATOR_SIZE;

    if (stream_data.size() < PALETTE_BYTES + SEP) return;

    // Expected start of separator
    size_t sep_start = stream_data.size() - PALETTE_BYTES - SEP;

    // Verify the separator bytes are all 0x00
    bool found = true;
    for (size_t i = 0; i < SEP; i++) {
        if (stream_data[sep_start + i] != 0x00) {
            found = false;
            break;
        }
    }

    if (found) {
        palette_start_offset = sep_start + SEP;
        std::cout << "Palette detected at stream offset "
                  << palette_start_offset << "\n";
    } else {
        std::cout << "No palette suffix found — using training data only.\n";
    }
}

// ============================================================
// PALETTE LOOKUP — O(1), no search
//
// Given a BGR pixel, return its exact offset in the palette.
// Layout: for b in 0..255, g in 0..255, r in 0..255 → BGR
// ============================================================

inline uint64_t palette_offset_for(uint8_t b, uint8_t g, uint8_t r) {
    return palette_start_offset
         + ((uint64_t)b * 65536 + (uint64_t)g * 256 + (uint64_t)r) * 3;
}

// ============================================================
// LOAD STREAM + BUILD CHUNK INDEX (training zone only)
// ============================================================

void load_stream() {

    std::ifstream in("STREAM/stream.bin", std::ios::binary);
    in.seekg(0, std::ios::end);
    size_t size = in.tellg();
    in.seekg(0);
    stream_data.resize(size);
    in.read((char*)stream_data.data(), size);
    in.close();

    std::cout << "Loaded stream: " << human_size(size) << "\n";

    // Detect where palette starts so we don't index it
    detect_palette_boundary();

    // Index only the training zone
    size_t index_end = (palette_start_offset > 0)
                       ? (palette_start_offset - STREAM_SEPARATOR_SIZE)
                       : size;

    std::cout << "Building chunk index (chunk=" << INDEX_CHUNK
              << ") over " << human_size(index_end) << " training data...\n";

    if (index_end < INDEX_CHUNK) return;

    uint64_t base_pow = compute_base_pow(INDEX_CHUNK);
    uint64_t h        = compute_hash(stream_data, 0, INDEX_CHUNK);
    chunk_index[h].push_back(0);

    for (size_t i = 1; i + INDEX_CHUNK <= index_end; i++) {
        h = roll_hash(h, base_pow,
                      stream_data[i - 1],
                      stream_data[i + INDEX_CHUNK - 1]);
        chunk_index[h].push_back(i);
    }

    std::cout << "Index entries: " << chunk_index.size() << "\n";
}

// ============================================================
// IMAGE → BYTE ARRAY (BGR row-major)
// ============================================================

std::vector<uint8_t> image_to_bytes(const std::string& path,
                                     uint32_t& width,
                                     uint32_t& height) {
    cv::Mat img = cv::imread(path);
    std::vector<uint8_t> bytes;
    if (img.empty()) {
        std::cout << "FAILED to load: " << path << "\n";
        return bytes;
    }
    width  = img.cols;
    height = img.rows;
    bytes.reserve((size_t)width * height * 3);
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
// FULL IMAGE MATCH — Rabin-Karp O(N+M)
// ============================================================

bool full_match_search(const std::vector<uint8_t>& target,
                       uint64_t& found_offset) {
    size_t M = target.size();
    size_t N = stream_data.size();
    if (M > N) return false;

    uint64_t base_pow    = compute_base_pow(M);
    uint64_t target_hash = compute_hash(target, 0, M);

    auto verify = [&](size_t off) -> bool {
        for (size_t j = 0; j < M; j++)
            if (stream_data[off + j] != target[j]) return false;
        return true;
    };

    uint64_t h = compute_hash(stream_data, 0, M);
    if (h == target_hash && verify(0)) { found_offset = 0; return true; }

    for (size_t i = 1; i + M <= N; i++) {
        h = roll_hash(h, base_pow, stream_data[i-1], stream_data[i+M-1]);
        if (h == target_hash && verify(i)) { found_offset = i; return true; }
    }
    return false;
}

// ============================================================
// EXTEND MATCH
// ============================================================

uint32_t extend_match(const std::vector<uint8_t>& target,
                      uint32_t  target_anchor,
                      uint64_t  stream_anchor,
                      uint64_t& best_stream_off,
                      uint32_t& best_target_off) {
    size_t Tsz = target.size();
    size_t Ssz = stream_data.size();

    uint32_t fwd = INDEX_CHUNK;
    while (target_anchor + fwd < Tsz &&
           stream_anchor + fwd < Ssz &&
           target[target_anchor + fwd] == stream_data[stream_anchor + fwd])
        fwd++;

    uint32_t bwd = 0;
    while (bwd < target_anchor &&
           bwd < stream_anchor &&
           target[target_anchor - bwd - 1] == stream_data[stream_anchor - bwd - 1])
        bwd++;

    best_stream_off = stream_anchor - bwd;
    best_target_off = target_anchor - bwd;
    return fwd + bwd;
}

// ============================================================
// SEGMENTED MATCH — training zone only
// ============================================================

std::vector<SegmentMatch> segmented_match(
    const std::vector<uint8_t>& target) {

    std::vector<SegmentMatch> matches;
    size_t Tsz = target.size();
    if (Tsz < INDEX_CHUNK) return matches;

    uint64_t base_pow = compute_base_pow(INDEX_CHUNK);
    uint32_t pos = 0;
    uint64_t h   = compute_hash(target, 0, INDEX_CHUNK);

    while (pos + INDEX_CHUNK <= Tsz) {

        auto it = chunk_index.find(h);

        if (it != chunk_index.end()) {
            SegmentMatch best; best.length = 0;

            for (uint64_t soff : it->second) {
                if (soff + INDEX_CHUNK > stream_data.size()) continue;

                bool ok = true;
                for (uint32_t k = 0; k < INDEX_CHUNK; k++) {
                    if (target[pos + k] != stream_data[soff + k]) {
                        ok = false; break;
                    }
                }
                if (!ok) continue;

                uint64_t ext_soff; uint32_t ext_toff;
                uint32_t len = extend_match(target, pos, soff,
                                            ext_soff, ext_toff);
                if (len > best.length) {
                    best.length        = len;
                    best.stream_offset = ext_soff;
                    best.target_offset = ext_toff;
                }
            }

            if (best.length >= MIN_MATCH_LEN) {
                matches.push_back(best);
                pos += best.length;
                if (pos + INDEX_CHUNK <= Tsz)
                    h = compute_hash(target, pos, INDEX_CHUNK);
                continue;
            }
        }

        if (pos + INDEX_CHUNK < Tsz)
            h = roll_hash(h, base_pow, target[pos], target[pos + INDEX_CHUNK]);
        pos++;
    }

    return matches;
}

// ============================================================
// PALETTE FALLBACK
//
// After segmented_match, any byte positions still uncovered
// (not in any segment) get a 3-byte palette segment each.
// Each segment is exactly one pixel (3 bytes) pointing to
// the mathematically-computed palette offset.
// ============================================================

void add_palette_fallback(
    const std::vector<uint8_t>& target,
    std::vector<SegmentMatch>& matches) {

    if (palette_start_offset == 0) return;

    size_t Tsz = target.size();

    // Build covered map
    std::vector<bool> covered(Tsz, false);
    for (const auto& m : matches)
        for (uint32_t i = m.target_offset;
             i < m.target_offset + m.length && i < Tsz; i++)
            covered[i] = true;

    // For every uncovered pixel (step by 3 = one BGR triplet)
    size_t palette_hits = 0;
    for (size_t i = 0; i + 2 < Tsz; i += 3) {
        if (covered[i]) continue;

        uint8_t b = target[i];
        uint8_t g = target[i+1];
        uint8_t r = target[i+2];

        SegmentMatch m;
        m.stream_offset = palette_offset_for(b, g, r);
        m.target_offset = (uint32_t)i;
        m.length        = 3;
        matches.push_back(m);
        palette_hits++;
    }

    if (palette_hits > 0)
        std::cout << "Palette fallback covered "
                  << palette_hits << " pixels ("
                  << palette_hits * 3 << " bytes)\n";
}

// ============================================================
// COVERAGE LOG
// ============================================================

void log_coverage(const std::vector<SegmentMatch>& matches,
                  size_t total_bytes,
                  const std::string& name) {
    std::vector<bool> covered(total_bytes, false);
    for (const auto& m : matches) {
        size_t end = std::min((size_t)m.target_offset + m.length, total_bytes);
        for (size_t i = m.target_offset; i < end; i++)
            covered[i] = true;
    }
    size_t uncovered = 0;
    for (bool b : covered) if (!b) uncovered++;
    double pct = 100.0 * (double)(total_bytes - uncovered) / (double)total_bytes;
    std::cout << "Coverage: " << pct << "%  ("
              << (total_bytes - uncovered) << "/" << total_bytes << " bytes)\n";
    if (uncovered > 0)
        std::cout << "WARNING: " << uncovered << " bytes still unmatched.\n";
    append_log("Coverage: " + std::to_string(pct) + "% for " + name);
}

// ============================================================
// SAVE MATCH FILE
// ============================================================

void save_match_file(const std::string& out_path,
                     uint32_t width, uint32_t height,
                     const std::vector<SegmentMatch>& matches) {
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

    // Stage 1: full image match
    uint64_t full_offset = 0;
    bool full_found = full_match_search(target, full_offset);

    std::vector<SegmentMatch> matches;

    if (full_found) {
        std::cout << "\nFULL MATCH at offset " << full_offset << "\n";
        SegmentMatch m;
        m.stream_offset = full_offset;
        m.target_offset = 0;
        m.length        = (uint32_t)target.size();
        matches.push_back(m);
    } else {
        std::cout << "\nNo full match — segment search...\n";
        matches = segmented_match(target);
        std::cout << "Training segments found: " << matches.size() << "\n";

        // Stage 3: palette fallback for anything still uncovered
        add_palette_fallback(target, matches);

        log_coverage(matches, target.size(), name);
    }

    save_match_file(out_path, width, height, matches);

    double   elapsed         = timer.toc();
    uint64_t original_size   = file_size_bytes(path);
    uint64_t compressed_size = file_size_bytes(out_path);
    double   ratio           = (double)original_size / (double)compressed_size;

    std::cout << "\n========== MATCH RESULTS ==========\n";
    print_stat("Image",             path);
    print_stat("Original Size",     human_size(original_size));
    print_stat("Compressed Size",   human_size(compressed_size));
    print_stat("Compression Ratio", ratio, "x");
    print_stat("Segment Count",     matches.size());
    print_stat("Matching Time",     elapsed, "sec");

    append_log("========== MATCH RESULTS ==========");
    append_log("Image: "              + path);
    append_log("Compression Ratio: " + std::to_string(ratio));
    append_log("Segment Count: "     + std::to_string(matches.size()));
    append_log("Matching Time: "     + std::to_string(elapsed));
    append_log("");
}

// ============================================================
// MAIN
// ============================================================

int main() {
    fs::create_directory("MATCH");
    load_stream();

    for (auto& p : fs::directory_iterator("COMP_IMG"))
        process_image(p.path().string());

    return 0;
}