// World class. Defines the grid in the world.
// Matt Young, 2022, UQRacing
#pragma once
#include <cstdint>
#include <array>
#include "lodepng/lodepng.h"
#include "ant_simulation/entity.h"

/// Maximum number of entities that can occupy a single grid cell
#define MAX_ENTITIES_CELL 8

namespace ants {
  class World {
  public:
      /// Instantiates an empty world
      World(uint32_t width, uint32_t height);

      /// Instantiates a world from the given PNG file as per specifications
      World(std::string filename);

      /// Updates the world for one time step
      void update();

  private:
      /// 3D array containing the grid (height, width, MAX_ENTITIES_CELL)
      Entity *grid;
  };
};