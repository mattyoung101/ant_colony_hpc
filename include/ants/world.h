// Copyright (c) 2022 Matt Young. All rights reserved.
//
// This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
// If a copy of the MPL was not distributed with this file, You can obtain one at
// http://mozilla.org/MPL/2.0/.
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
#include "ants/snapgrid.h"
#include "ants/defines.h"

// World class header. Most of the simulator code is in world.cpp/world.h.

namespace ants {
    typedef enum {
        /// Tag to indicate this message is food grid data
        TAG_FOOD_DATA = 0,
        /// Tag to indicate this message is food grid written or not table
        TAG_FOOD_WRITTEN,
        /// Tag to indicate this message is pheromone grid data
        TAG_PHEROMONES_DATA,
        /// Tag to indicate this message is pheromone grid written or not table
        TAG_PHEROMONES_WRITTEN,
        /// Tag to receive colony add ants
        TAG_COLONY_ADD_ANTS,
    } MPITag_t;

    struct World {
        /// Instantiates a world from the given PNG file as per specifications
        explicit World(const std::string &filename, mINI::INIStructure config);

        World() = default;

        ~World() = default;

        /// Updates the world for one time step
        /// @return true if we should keep iterating the simulation, false if we should quit now
        [[nodiscard]] bool update();

        /**
         * Updates the world for one time step using MPI. Replaces World::update() when MPI is enabled.
         * @return true if we should keep iterating the simulation, false if we should quit now
         */
        [[nodiscard]] bool updateMpi();

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

        size_t maxAntsLastTick{};
        int32_t width{};
        int32_t height{};
#if USE_MPI
        int32_t mpiWorldSize{};
        int32_t mpiRank{};
#endif
    private:
        /// Returns a random movement vector for the specified ant
        Vector2i randomMovementVector(const Ant &ant, pcg32_fast &localRng) const;

        /// Decays pheromones in the grid
        void decayPheromones();

        /**
         * Calculates the strongest direction vector, and its strength, based on the pheromones
         * surrounding the ant.
         * The output vector will depend on the mode of the ant (i.e. to food or to colony).
         * @param colony colony the ant belongs to
         * @param ant the ant to consider
         * @return pair: first value is the strongest direction, second value is the strength
         */
        [[nodiscard]] std::pair<Vector2i, double> computePheromoneVector(const Colony &colony, const Ant &ant) const;

        /**
         * Updates a single ant in the world
         * @param ant ant to update
         * @param colony pointer to colony being updated
         * @param localRng local pcg32 instance
         * @returns true if the colony should add more ants, false otherwise
         */
        bool updateAnt(Ant *ant, Colony *colony, pcg32_fast &localRng);

#if USE_MPI
        /**
         * Updates the selected colonies and their ants, for use in MPI
         * @param colonyWorkIdx indices of colonies to update, array of length mpiColoniesPerWorker
         * @param colonyAddAnts for each ant in colonyWorkIdx, -1 if we should not add more ants, otherwise
         * the colony index (we should add more ants)
         * @param seed seed to initialise pcg32_fast rng with
         */
        void updateColoniesMpi(int *colonyWorkIdx, int *colonyAddAnts, uint64_t seed);

        /**
         * Packs the pheromone grid into a continuous data structure suitable for transmitting using
         * MPI
         * @return unique_ptr to allocated region of doubles
         */
        std::unique_ptr<double[]> packPheromoneGrid();

        /// Unpacks a packed pheromone grid into the pheromone grid of this class
        void unpackPheromoneGrid(const std::shared_ptr<double[]>& packedData);
#endif

        /// Renders a pheromone to a colour value. Returns the colour value between 0.0 and 1.0.
        [[nodiscard]] double pheromoneToColour(int32_t x, int32_t y) const;

        /// MPI master update function
        [[nodiscard]] bool updateMpiMaster();

        /// MPI worker update function
        [[nodiscard]] bool updateMpiWorker();


        SnapGrid2D<bool> foodGrid{};
        /// indexes are x, y, colony
        SnapGrid3D<PheromoneStrength> pheromoneGrid{};
        SnapGrid2D<bool> obstacleGrid{};

        /// List of colonies
        std::vector<Colony> colonies{};

        /// PRNG: we use PCG, and pcg64_fast, which doesn't say it has any worse statistical quality
        /// than pcg64, and has plenty large state for our use case
        pcg64_fast rng{};
        /// Buffer of random values used in World::decayPheromones
        std::vector<double> randomBuffer{};

        /// INI values
        double pheromoneDecayFactor{};
        double pheromoneGainFactor{};
        double pheromoneFuzzFactor{};

        double antMoveRightChance{}, antUsePheromone{};
        int32_t antKillNotUseful{};

        double colonyHungerDrain{}, colonyHungerReplenish{};
        int32_t colonyAntsPerTick{}, colonyReturnDist{};

        /// PNG TAR output file
        mtar_t tarfile{};
        /// true if PNG TAR recording initialised successfully
        bool tarfileOk = false;
        /// Path to where the recording is saved
        std::string recordingPath{};

#if USE_MPI
        // number of colonies per MPI worker
        int32_t mpiColoniesPerWorker{};
#endif
    };
};