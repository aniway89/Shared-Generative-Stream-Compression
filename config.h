// ============================================================
// FILE: config.h
// ============================================================

#pragma once
#include <string>
#include <fstream>
#include <filesystem>

namespace fs = std::filesystem;

// ============================================================
// STREAM GENERATION
// ============================================================

static constexpr bool ENABLE_GENERATIVE_MODE  = true;
static constexpr int  GENERATIVE_REPEAT_COUNT = 2;
static constexpr int  STREAM_SEPARATOR_SIZE   = 16;

// ============================================================
// ACTIVE LEARNING (update_stream.cpp)
// ============================================================

static constexpr bool ENABLE_ACTIVE_LEARNING = true;
static constexpr int  ACTIVE_LEARN_DELTA     = 2;

// ============================================================
// UNIVERSAL PALETTE SUFFIX (generate_stream.cpp)
// ============================================================

static constexpr bool ENABLE_UNIVERSAL_PALETTE = true;

// ============================================================
// MATCHING
// IMPORTANT: delete any local constexpr definitions of these
// inside match_image.cpp or they will shadow these values.
// ============================================================

static constexpr uint32_t MIN_MATCH_LEN = 3;
static constexpr uint32_t INDEX_CHUNK   = 16;

// Rabin-Karp hash parameters
static constexpr uint64_t RK_BASE = 257;
static constexpr uint64_t RK_MOD  = (1ULL << 61) - 1;

// NOTE: get_attempt_number() is defined in logger.h only.
// Do NOT define it here — causes duplicate definition error.