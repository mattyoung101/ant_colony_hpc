// Copyright (c) 2022 Matt Young. All rights reserved.
//
// This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
// If a copy of the MPL was not distributed with this file, You can obtain one at
// http://mozilla.org/MPL/2.0/.
#include <random>
#include "pcg/pcg_random.hpp"
#include "log/log.h"

// This file is used to dump a bunch of pcg32_fast numbers with a seed to a binary file that
// can be loaded later.
// Usage: ./dump_random <seed> <width> <height>
// Will produce width * height numbers
// This is used in World::decayPheromones to efficiently generate a huge amount of numbers.

int main(int argc, char *argv[]) {
    log_set_level(LOG_TRACE);
    if (argc < 4) {
        log_error("Usage: ./dump_random <seed> <width> <height>");
        exit(1);
    }

    auto seed = std::stol(argv[1]);
    auto width = std::stoi(argv[2]);
    auto height = std::stoi(argv[3]);
    log_info("Seed: %ld, width: %d, height: %d", seed, width, height);

    FILE *out = fopen("random.bin", "wb");
    pcg32_fast rng{};
    std::uniform_real_distribution<double> dist(-1.0, 1.0);
    rng.seed(seed);

    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            auto n = dist(rng);
            fwrite(&n, 1, sizeof(n), out);
        }
    }
    fclose(out);
    log_info("Done");
}