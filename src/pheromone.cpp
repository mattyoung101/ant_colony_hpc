// Pheromone tile in the grid world, source file
// Matt Young, 2022
#include <cstdio>
#include <vector>
#include <algorithm>
#include "ants/pheromone.h"

#define FACTOR 3.0

double ants::Pheromone::getColourValue() const {
    // average over all the colonies of whichever is higher, to food or to colony
//    double sum = 0.0;
//    double count = 0.0;
//    for (const auto &item : values) {
//        sum += std::max(item.toFood, item.toColony);
//        count += 1.0;
//    }
//    return std::clamp((sum / count) * FACTOR, 0.0, 1.0);

    // max over all the colonies of whichever is higher, to food or to colony
    double bestStrength = -9999.0;
    for (const auto &item : values) {
        double strength = std::max(item.toFood, item.toColony);
        if (strength > bestStrength) {
            bestStrength = strength;
        }
    }
    return bestStrength;
}
