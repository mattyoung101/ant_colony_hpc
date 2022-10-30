// Hardcoded configuration for the simulator
// Matt Young, 2022
#pragma once

/// If true, use OpenMP for acceleration
#define USE_OMP 0

#if HAVE_CUDA
/// If true, use CUDA for acceleration
#define USE_CUDA 1
#else
/// No CUDA detected. USE_CUDA is hardcoded to be off.
#define USE_CUDA 0
#endif

#if USE_OMP && USE_CUDA
#error "Invalid configuration, USE_OMP and USE_CUDA are mutually exclusive"
#endif