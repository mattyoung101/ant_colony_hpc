// Record of a colony in the simulation. Stores info about colony data.
// Matt Young, 2022
#pragma once
#include <cstdint>
#include "ants/utils.h"
#include "ant.h"

namespace ants {
    struct Colony {
        /// Current colony hunger
        double hunger = 1.0;
        /// Colour of the ant
        RGBColour colour{};
        /// Position of the colony in the world
        Vector2i pos{};
        /// Ants in the colony
        std::vector<Ant> ants{};
    };
};