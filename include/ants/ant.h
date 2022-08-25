// Ant class
// Matt Young, 2022
#pragma once

namespace ants {
    struct Ant {
        /// Current position
        Vector2i pos{};

        /// True if the ant is holding food
        bool holdingFood{};
    };
}