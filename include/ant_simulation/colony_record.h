// Record of a colony in the simulation. Stores info about colony data.
// Matt Young, 2022
#pragma once
#include <cstdint>
#include "ant_simulation/utils.h"

namespace ants {
    struct ColonyRecord {
        /// Current colony hunger
        double hunger{};
        /// Number of ants we have
        uint32_t numAnts{};
        /// Colour of the ant
        RGBColour colour{};
    };
};