// World class. Defines the grid in the world.
// Matt Young, 2022, UQRacing
#pragma once

#include <cstdint>
#include <array>
#include <vector>
#include "ant_simulation/tile.h"
#include "ant_simulation/empty.h"
#include "microtar/microtar.h"

namespace ants {
    typedef std::vector<std::vector<std::vector<Tile>>> ant_grid_t;

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
         */
        void setupRecording(const std::string &prefix);

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

    private:
        /// 3D array containing the grid (height, width, entities on this tile)
        ant_grid_t grid{};

        uint32_t width{}, height{};
        uint32_t tickCount{};

        /// List of colony data structures. Each colony has both a portion of grid tiles it owns, and
        /// an entry in this list to keep track of it.
        // TODO
        std::vector<int> colonies{};

        /// Buffer of PNGs waiting to be written to disk
        std::vector<ant_grid_t> pngBuffer{};

        mtar_t tarfile{};
        bool tarfileOk = false;
    };
};