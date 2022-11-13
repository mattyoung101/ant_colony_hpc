// Hardcoded configuration for the simulator
// Matt Young, 2022
#pragma once

/// If true, use OpenMP for acceleration.
#define USE_OMP 0

/// If true, use MPI for acceleration.
#define USE_MPI 1

#if USE_MPI && USE_OMP
#error "Sorry, due to time constraints, the OMP and MPI combination is not available at this time"
#endif
