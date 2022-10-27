// Hardcoded configuration for the simulator
// Matt Young, 2022
#pragma once

// TODO make these CMake parameters??

/// If true, use OpenMP for acceleration
#define USE_OMP 1

/// If true, use CUDA for acceleration
#define USE_CUDA 0

#if USE_OMP && USE_CUDA
#error "Invalid configuration, USE_OMP and USE_CUDA are mutually exclusive"
#endif