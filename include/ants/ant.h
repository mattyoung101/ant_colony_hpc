// Ant class
// Matt Young, 2022
#pragma once
#include "ants/utils.h"

namespace ants {
    enum AntState {
        /// Ant is looking for food
        ANT_FIND_FOOD,
        /// Ant has found food, and is bringing it home
        ANT_RETURN_HOME,
    };

    struct Ant {
        /// Current position
        Vector2i pos{};

        /// True if the ant is holding food
        bool holdingFood{};
    };
}