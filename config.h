// ============================================================
// FILE: config.h
// ============================================================

#pragma once

// ============================================================
// STREAM MODES
// ============================================================

// false = exact-only deterministic stream
// true  = exact + generated pixel variants
constexpr bool ENABLE_GENERATIVE_MODE = true;

// ============================================================
// GENERATION PARAMETERS
// ============================================================

// how many synthetic variants per pixel
constexpr int GENERATIVE_REPEAT_COUNT = 2;

// separator bytes between images
constexpr int STREAM_SEPARATOR_SIZE = 64;