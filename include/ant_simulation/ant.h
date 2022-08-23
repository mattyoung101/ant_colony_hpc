// Ant class
// Matt Young, 2022
#pragma once
#include "ant_simulation/tile.h"

namespace ants {
    class Ant : public Tile {
    private:
        bool holdingFood{};
    };
}