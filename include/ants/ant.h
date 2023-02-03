// Copyright (c) 2022 Matt Young. All rights reserved.
//
// This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
// If a copy of the MPL was not distributed with this file, You can obtain one at
// http://mozilla.org/MPL/2.0/
#pragma once
#include "ants/utils.h"
#include <set>
#include "cereal/types/vector.hpp"
#include "cereal/types/set.hpp"

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
        /// Globally unique ID for this ant
        uint64_t id{};
        /// If true, this ant is dead
        bool isDead{};

        bool operator==(const Ant &rhs) const {
            return id == rhs.id;
        }

        bool operator!=(const Ant &rhs) const {
            return !(rhs == *this);
        }

        // for cereal
        friend class cereal::access;
        template<class Archive>
        void serialize(Archive & archive) {
            archive(CEREAL_NVP(pos), CEREAL_NVP(holdingFood), CEREAL_NVP(preferredDir),
                    CEREAL_NVP(ticksSinceLastUseful), CEREAL_NVP(visitedPos), CEREAL_NVP(id),
                    CEREAL_NVP(isDead));
        }
    };
}