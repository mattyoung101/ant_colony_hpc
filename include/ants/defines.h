// Copyright (c) 2022 Matt Young. All rights reserved.
//
// This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
// If a copy of the MPL was not distributed with this file, You can obtain one at
// http://mozilla.org/MPL/2.0/.
#pragma once

// Hardcoded configuration for the simulator

/// If true, use OpenMP for acceleration.
#define USE_OMP 1

/// If true, use MPI for acceleration.
#define USE_MPI 0

#if USE_MPI && USE_OMP
#error "Sorry, due to time constraints, the OMP and MPI combination is not available at this time"
#endif
