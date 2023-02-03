// Copyright (c) 2022 Matt Young. All rights reserved.
//
// This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
// If a copy of the MPL was not distributed with this file, You can obtain one at
// http://mozilla.org/MPL/2.0/
#pragma once
#include <cstdint>
#include "ants/utils.h"
#include "ant.h"

// Record of a colony in the simulation. Stores info about colony data.

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
        /// Unique colony ID
        uint32_t id{};
        /// If true, this colony is dead
        bool isDead{};

        bool operator==(const Colony &rhs) const {
            return id == rhs.id;
        }

        bool operator!=(const Colony &rhs) const {
            return !(rhs == *this);
        }

        // for cereal
        friend class cereal::access;
        template<class Archive>
        void serialize(Archive & archive) {
            archive(CEREAL_NVP(hunger), CEREAL_NVP(colour), CEREAL_NVP(pos), CEREAL_NVP(ants),
                    CEREAL_NVP(id), CEREAL_NVP(isDead));
        }
    };
};