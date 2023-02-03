// Copyright (c) 2022 Matt Young. All rights reserved.
//
// This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
// If a copy of the MPL was not distributed with this file, You can obtain one at
// http://mozilla.org/MPL/2.0/.
#pragma once
#include <vector>

//  Pheromone tile in the grid world

namespace ants {
    struct PheromoneStrength {
        /// Strength of this pheromone to food
        double toColony{};
        /// Strength of this pheromone to colony
        double toFood{};

        PheromoneStrength() = default;

        PheromoneStrength(double toColony, double toFood) : toColony(toColony), toFood(toFood) {}
    };
};