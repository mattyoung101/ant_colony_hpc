// An ant colony in the grid world
// Matt Young, 2022
#pragma once
#include <memory>
#include "ant_simulation/tile.h"

namespace ants {
    struct ColonyTile : public Tile {
    public:
        std::shared_ptr<ColonyRecord> owner{};
    };
};