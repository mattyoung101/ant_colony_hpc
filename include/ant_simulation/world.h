// World class. Defines the grid in the world.
// Matt Young, 2022, UQRacing
#pragma once
#include <cstdint>
#include "lodepng/lodepng.h"

namespace ants {
  class World {
      /// Instantiates an empty world
      World(uint32_t width, uint32_t height);

      /// Instantiates a world from the given PNG file as per specifications
      World(std::string filename);
  };
};