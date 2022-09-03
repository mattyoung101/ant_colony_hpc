// Pheromone tile in the grid world
// Matt Young, 2022
#pragma once
#include <vector>

namespace ants {
    enum PheromoneDirection {
        TO_COLONY = 0,
        TO_FOOD,
    };

    struct PheromoneStrength {
        /// Strength of this pheromone to food
        double toColony{};
        /// Strength of this pheromone to colony
        double toFood{};

        PheromoneStrength() = default;

        PheromoneStrength(double toColony, double toFood) : toColony(toColony), toFood(toFood) {}
    };

    struct Pheromone {
        /// Mapping between (colony -> {to food strength, to colony strength})
        /// Index is the unique colony ID, value is the PheromoneStrength instance
        std::vector<PheromoneStrength> values;

        /// Returns a value to represent the colour of this pheromone in the PNG TAR
        [[nodiscard]] double getColourValue() const;
    };
};