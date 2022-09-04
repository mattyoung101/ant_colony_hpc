// Ant class
// Matt Young, 2022
#pragma once
#include "ants/utils.h"
#include <set>

namespace ants {
    struct Ant {
        /// Current position
        Vector2i pos{};
        /// True if the ant is holding food
        bool holdingFood{};
        /// Preferred direction for random movement
        Vector2i preferredDir{};
        /// Number of ticks since this ant last did something useful (touch food, touch colony, etc)
        int32_t ticksSinceLastUseful{};
        /// Set of positions we visited since the last state change
        /// If we don't have this, ants just move back and forth
        std::set<Vector2i> visitedPos{};
    };
}