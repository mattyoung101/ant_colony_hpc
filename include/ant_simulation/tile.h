// Entity in the grid world
// Matt Young, 2022
#pragma once

namespace ants {
    /// An cell (aka tile) in the grid world
    struct Tile {
    public:
        /// Called on each entity to update its state
        virtual void update();
    };
};