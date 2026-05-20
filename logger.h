// ============================================================
// FILE: logger.h
// PURPOSE:
// Accurate benchmarking + human-readable sizes
// ============================================================

#pragma once

#include <iostream>
#include <fstream>
#include <chrono>
#include <filesystem>
#include <iomanip>
#include <sstream>

namespace fs = std::filesystem;

// ============================================================
// TIMER
// ============================================================

class BenchmarkTimer {

public:

    std::chrono::high_resolution_clock::time_point start;

    void tic() {
        start =
            std::chrono::high_resolution_clock::now();
    }

    double toc() {

        auto end =
            std::chrono::high_resolution_clock::now();

        std::chrono::duration<double> diff =
            end - start;

        return diff.count();
    }
};

// ============================================================
// FILE SIZE
// ============================================================

inline uint64_t file_size_bytes(
    const std::string& path
) {

    if (!fs::exists(path))
        return 0;

    return fs::file_size(path);
}

// ============================================================
// HUMAN READABLE SIZE
// ============================================================

inline std::string human_size(uint64_t bytes) {

    const double KB = 1024.0;
    const double MB = KB * 1024.0;
    const double GB = MB * 1024.0;

    std::ostringstream ss;

    ss << std::fixed << std::setprecision(2);

    if (bytes < 1024) {

        ss << bytes << " Bytes";
    }
    else if (bytes < MB) {

        ss << (bytes / KB) << " KB";
    }
    else if (bytes < GB) {

        ss << (bytes / MB) << " MB";
    }
    else {

        ss << (bytes / GB) << " GB";
    }

    return ss.str();
}

// ============================================================
// HUMAN READABLE COUNT
// ============================================================

inline std::string human_count(uint64_t n) {

    std::ostringstream ss;

    if (n >= 1000000000ULL) {

        ss << std::fixed
           << std::setprecision(2)
           << (n / 1000000000.0)
           << " Billion";
    }
    else if (n >= 1000000ULL) {

        ss << std::fixed
           << std::setprecision(2)
           << (n / 1000000.0)
           << " Million";
    }
    else if (n >= 1000ULL) {

        ss << std::fixed
           << std::setprecision(2)
           << (n / 1000.0)
           << " Thousand";
    }
    else {

        ss << n;
    }

    return ss.str();
}

// ============================================================
// PRINT STAT STRING
// ============================================================

inline void print_stat(
    const std::string& name,
    const std::string& value
) {

    std::cout
        << std::left
        << std::setw(40)
        << name
        << ": "
        << value
        << "\n";
}

// ============================================================
// PRINT STAT DOUBLE
// ============================================================

inline void print_stat(
    const std::string& name,
    double value,
    const std::string& unit = ""
) {

    std::cout
        << std::left
        << std::setw(40)
        << name
        << ": "
        << std::fixed
        << std::setprecision(6)
        << value
        << " "
        << unit
        << "\n";
}

// ============================================================
// LOGGING
// ============================================================

inline void append_log(
    const std::string& text
) {

    std::ofstream log(
        "benchmark.log",
        std::ios::app
    );

    log << text << "\n";

    log.close();
}