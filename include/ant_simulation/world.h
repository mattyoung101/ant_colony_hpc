// World class. Defines the grid in the world.
// Matt Young, 2022, UQRacing
#pragma once
#include <cstdint>
#include <array>
#include <vector>
#include "ant_simulation/tile.h"
#include "ant_simulation/empty.h"

namespace ants {
  class World {
  public:
      /// Instantiates an empty world
      World(uint32_t width, uint32_t height);

      /// Instantiates a world from the given PNG file as per specifications
      explicit World(const std::string& filename);

      /// Updates the world for one time step
      void update();

  private:
      /// 3D array containing the grid (height, width, MAX_ENTITIES_CELL)
      Tile *grid{};

      uint32_t width{}, height{};

      /// List of colony data structures. Each colony has both a portion of grid tiles it owns, and
      /// an entry in this list to keep track of it.
      // TODO
      std::vector<int> colonies{};
  };
};