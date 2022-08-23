// A food tile in the grid world
// Matt Young, 2022
#pragma once
#include <cstdint>
namespace ants {
    struct Food {
        /// Remaining number of times this food can be used
        // TODO - if it's one use per food, we could just do an array of bools
        int remainingUses{};
    };
};