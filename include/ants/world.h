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
#include "xoroshiro/XoroshiroCpp.h"
#include "mini/ini.h"
#include "tinycolor/tinycolormap.hpp"

namespace ants {
    struct World {
        /// Grid sizes
        uint32_t width{};
        /// Grid sizes
        uint32_t height{};
    public:
        /// Instantiates a world from the given PNG file as per specifications
        explicit World(const std::string &filename, mINI::INIStructure config);

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
         * @param wallTime time it took real-time for the simulation to run
         * @param simTime time it took ONLY the simulation part to run (excluding PNG IO)
         */
        void writeRecordingStatistics(uint32_t numTicks, TimeInfo wallTime, TimeInfo simTime);

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
        [[nodiscard]] std::vector<uint8_t> renderWorldUncompressed() const;

    private:
        /// Grid of food tiles
        Food ***foodGrid{};
        /// Grid of pheromone tiles
        Pheromone ***pheromoneGrid{};
        /// Grid of obstacles
        bool **obstacleGrid{};
        /// List of colonies
        std::vector<Colony> colonies{};

        /// PSRNG
        XoshiroCpp::Xoshiro256StarStar rng{};

        /// PNG TAR output file
        mtar_t tarfile{};
        /// true if PNG TAR recording initialised successfully
        bool tarfileOk = false;
    };
};