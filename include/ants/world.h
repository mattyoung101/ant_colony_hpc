// World class. Defines the grid in the world.
// Matt Young, 2022
#pragma once
#include <cstdint>
#include <array>
#include <vector>
#include "microtar/microtar.h"
#include "ants/colony.h"
#include "ants/food.h"
#include "ants/colony.h"
#include "ants/pheromone.h"
#include "ants/food.h"
#include "ants/colony_tile.h"

namespace ants {
    class World {
    public:
        /// Instantiates a world from the given PNG file as per specifications
        explicit World(const std::string &filename);

        ~World();

        /// Updates the world for one time step
        void update();

        /**
         * Sets up PNG TAR disk output. PNG files are written to a TAR file that can then be downloaded
         * later. For performance, a certain amount are buffered in memory first, before being written
         * to disk.
         * @param prefix prefix to write before the
         * @param recordInterval flush buffer every this many ticks
         */
        void setupRecording(const std::string &prefix, uint32_t recordInterval);

        /**
         * Writes statistics about the recording to a file called "stats.txt" in the TAR file
         * @param numTicks number of ticks done
         * @param numMs total number of milliseconds recording took
         * @param ticksPerSecond number of ticks per second
         */
        void writeRecordingStatistics(uint32_t numTicks, long numMs, double ticksPerSecond);

        // TODO when PNGs are written to the tar, do it async (on another thread)

        /// Flushes current in-memory buffered PNGs to the disk TAR file
        void flushRecording();

        /// Allocates and creates a clone of the grid. Expensive in complexity and memory.
        // TODO

    private:
        /// Grid of food tiles
        Food ***foodGrid;
        /// Grid of pheromone tiles
        Pheromone ***pheromoneGrid;
        /// Grid of colony tiles, only used for debugging
        ColonyTile ***colonyGrid;
        /// Grid of obstacles
        bool **obstacleGrid;
        /// List of colonies
        std::vector<Colony> colonies{};

        /// Grid sizes
        uint32_t width{}, height{};
        /// Number of ticks performed
        uint32_t tickCount{};
        /// approximate grid size in bytes
        size_t approxGridSize{};

        /// Buffer of grid data waiting to be encoded into a PNG
        // TODO don't buffer this, and instead just do it on another thread
        // TODO data structure for this? maybe we should just do array of worlds?

        mtar_t tarfile{};
        bool tarfileOk = false;
    };
};