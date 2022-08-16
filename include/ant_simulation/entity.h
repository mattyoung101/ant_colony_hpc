// Entity in the grid world
// Matt Young, 2022
#pragma once

namespace ants {
    /// An entity in the grid world
    class Entity {
    public:
        /// Called on each entity to update its state
        virtual void update();
    };
};