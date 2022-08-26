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
#include "xoroshiro/XoroshiroCpp.h"

namespace ants {
    struct World {
        /// Grid sizes
        uint32_t width{};
        /// Grid sizes
        uint32_t height{};
    public:
        /// Instantiates a world from the given PNG file as per specifications
        explicit World(const std::string &filename);

        World() = default;

        ~World();

        /// Updates the world for one time step
        void update() noexcept;

        /**
         * Sets up PNG TAR disk output. PNG files are written to a TAR file that can then be downloaded
         * later.
         * @param prefix prefix to write before the
         */
        void setupRecording(const std::string &prefix);

        /**
         * Writes statistics about the recording to a file called "stats.txt" in the TAR file
         * @param numTicks number of ticks done
         * @param numMs total number of milliseconds recording took
         * @param ticksPerSecond number of ticks per second
         */
        void writeRecordingStatistics(uint32_t numTicks, double numMs, double ticksPerSecond);

        /**
         * Writes the data to the PNG TAR, if recording is enabled
         * @param filename name of the file with extension to write to
         * @param data byte buffer of data
         * @param len length of byte buffer in bytes
         */
        void writeToTar(const std::string &filename, uint8_t *data, size_t len);

        /// Finalises the PNG TAR recording. Must be called before program exit.
        void finaliseRecording();

        /// Renders the world to an uncompressed RGB pixel buffer
        std::vector<uint8_t> renderWorldUncompressed() const;

    private:
        /// Grid of food tiles
        Food ***foodGrid{};
        /// Grid of pheromone tiles
        Pheromone ***pheromoneGrid{};
        /// Grid of obstacles
        bool **obstacleGrid{};
        /// List of colonies
        std::vector<Colony> colonies{};

        /// Number of ticks performed
        uint32_t tickCount{};
        /// approximate grid size in bytes
        size_t approxGridSize{};

        /// PSRNG
        XoshiroCpp::Xoshiro256StarStar rng{};

        /// PNG TAR output file
        mtar_t tarfile{};
        /// true if PNG TAR recording initialised successfully
        bool tarfileOk = false;
    };
};