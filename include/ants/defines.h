// Hardcoded configuration for the simulator
// Matt Young, 2022
#pragma once

/// If true, use OpenMP for acceleration.
#define USE_OMP 1

/// If true, use MPI for acceleration. If USE_OMP is 0, ant updates will be serial (only colonies are
/// distributed). If USE_OMP is 1, then colonies are distributed using MPI, and colonies are distributed
/// using OMP.
#define USE_MPI 1
