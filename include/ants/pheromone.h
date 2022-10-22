// Pheromone tile in the grid world
// Matt Young, 2022
#pragma once
#include <vector>

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