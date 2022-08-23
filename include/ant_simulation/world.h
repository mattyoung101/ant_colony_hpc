// World class. Defines the grid in the world.
// Matt Young, 2022
#pragma once
#include <cstdint>
#include <array>
#include <vector>
#include "ant_simulation/tile.h"
#include "ant_simulation/empty.h"
#include "microtar/microtar.h"
#include "ant_simulation/colony_record.h"

namespace ants {
    /**
     * This is probably the world's ugliest data type, but it's like this
     * height -->
     *      width -->
     *          entities in cell -->
     *              Entity 1 ptr
     *              Entity 2 ptr
     *              ...
     * So, in other words, a 3D array [height][width][cell entities] of Tile pointers
     */
    typedef Tile**** AntGrid_t;

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
        AntGrid_t cloneGrid();

    private:
        /// 3D array containing the grid (height, width, entities on this tile)
        AntGrid_t grid{};

        /// Grid sizes
        uint32_t width{}, height{};
        /// Number of ticks performed
        uint32_t tickCount{};
        /// approximate grid size in bytes
        size_t approxGridSize{};

        /// List of colony data structures. Each colony has both a portion of grid tiles it owns, and
        /// an entry in this list to keep track of it.
        std::vector<ColonyRecord> colonies{};

        /// Buffer of grid data waiting to be encoded into a PNG
        // TODO don't buffer this, and instead just do it on another thread
        std::vector<AntGrid_t> pngBuffer{};

        mtar_t tarfile{};
        bool tarfileOk = false;
    };
};