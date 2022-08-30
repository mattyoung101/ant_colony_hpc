// Pheromone tile in the grid world
// Matt Young, 2022
#pragma once

namespace ants {
    struct Pheromone {
        // TODO hashmap with (colony -> {to food strength, to colony strength})
        double value{};
    };
};