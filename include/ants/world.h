// World class. Defines the grid in the world.
// Matt Young, 2022
#pragma once
#include <cstdint>
#include <array>
#include <vector>
#include "microtar/microtar.h"
#include "ants/colony.h"
#include "ants/colony.h"
#include "ants/pheromone.h"
#include "mini/ini.h"
#include "tinycolor/tinycolormap.hpp"
#include "pcg/pcg_random.hpp"

namespace ants {
    struct World {
        /// Grid sizes
        int32_t width{};
        /// Grid sizes
        int32_t height{};

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

        /// Renders the current world to an uncompressed RGB pixel buffer.
        [[nodiscard]] std::vector<uint8_t> renderWorldUncompressed() const;

        /**
         * Finds the distance to the nearest food tile, from pos. Distance metric is Cheybshev
         * distance.
         * @param pos input position
         * @return pair: distance, position of food tile
         */
        [[nodiscard]] std::pair<double, Vector2i> findNearestFood(const Vector2i &pos) const;

        /// Returns a random vector between (-1,1)
        Vector2i randomMovementVector();

        /// Decays pheromones in the grid
        void decayPheromones();

        /**
         * Calculates the strongest direction vector, and its strength, based on the pheromones
         * surrounding the ant.
         * The output vector will depend on the mode of the ant (i.e. to food or to colony).
         * @param searchWidth width on each side of a square to search for pheromones in; see antconfig.ini
         * and docs/phermone_search_width_2.jpg for an explanation.
         * @param colony colony the ant belongs to
         * @param ant the ant to consider
         * @return a pair: first value is the strongest direction, second value is the strength
         */
        [[nodiscard]] std::pair<Vector2i, double>
        computePheromoneVector(const Colony &colony, const Ant &ant) const;

    private:
        /// Grid of food tiles
        bool **foodGrid{};
        /// Grid of pheromone tiles
        Pheromone ***pheromoneGrid{};
        /// Grid of obstacles
        bool **obstacleGrid{};
        /// List of colonies
        std::vector<Colony> colonies{};

        /// PRNG: we use PCG, and pcg64_fast, which doesn't say it has any worse statistical quality
        /// than pcg64, and has plenty large state for our use case
        pcg64_fast rng{};

        /// INI values
        double pheromoneDecayFactor{};
        double pheromoneGainFactor{};

        double antMoveRightChance{};
        int32_t antKillNotUseful{};

        /// PNG TAR output file
        mtar_t tarfile{};
        /// true if PNG TAR recording initialised successfully
        bool tarfileOk = false;
        /// Path to where the recording is saved
        std::string recordingPath{};
    };
};