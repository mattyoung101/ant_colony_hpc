// Pheromone tile in the grid world, source file
// Matt Young, 2022
#include <cstdio>
#include <vector>
#include <algorithm>
#include "ants/pheromone.h"

#define FACTOR 5.0

double ants::Pheromone::getColourValue() const {
    // just the average but multiplied by a constant
    double sum = 0.0;
    double count = 0.0;
    for (const auto &item : values) {
        sum += item.toFood + item.toColony;
        count += 2.0;
    }
    // TODO make this constant configurable somehow?
    return std::clamp((sum / count) * FACTOR, 0.0, 1.0);
}
