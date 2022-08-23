// Colony tile. Only used as a marker for visualisation.
// Matt Young, 2022
#pragma once
#include <cstdint>

namespace ants {
    struct ColonyTile {
        /// Colour to paint the tile
        RGBColour colour{};

        ColonyTile() = default;

        ColonyTile(uint8_t r, uint8_t g, uint8_t b) {
            colour.r = r;
            colour.g = g;
            colour.b = b;
        }
    };
};
