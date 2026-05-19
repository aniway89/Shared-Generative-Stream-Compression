#pragma once

#include <iostream>
#include <fstream>
#include <chrono>
#include <filesystem>
#include <iomanip>

namespace fs = std::filesystem;

class BenchmarkTimer {

public:

    std::chrono::high_resolution_clock::time_point start;

    void tic() {
        start = std::chrono::high_resolution_clock::now();
    }

    double toc() {

        auto end = std::chrono::high_resolution_clock::now();

        std::chrono::duration<double> diff = end - start;

        return diff.count();
    }
};

inline uint64_t file_size_bytes(const std::string& path) {

    if (!fs::exists(path))
        return 0;

    return fs::file_size(path);
}

inline double mb(uint64_t bytes) {
    return bytes / 1024.0 / 1024.0;
}

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
        << value
        << " "
        << unit
        << "\n";
}

inline void append_log(const std::string& text) {

    std::ofstream log("benchmark.log", std::ios::app);

    log << text << "\n";

    log.close();
}