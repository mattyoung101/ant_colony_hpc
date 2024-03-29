// Copyright (c) 2022 Matt Young. All rights reserved.
//
// This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
// If a copy of the MPL was not distributed with this file, You can obtain one at
// http://mozilla.org/MPL/2.0/.
#include <stdexcept>
#include <sstream>
#include <ctime>
#include <iomanip>
#include <set>
#include <unordered_map>
#include <random>
#include <chrono>
#include <unistd.h>
#include <pwd.h>
#include "ants/world.h"
#include "stb/stb_image.h"
#include "stb/stb_image_write.h"
#include "ants/colony.h"
#include "ants/ant.h"
#include "log/log.h"
#include "tinycolor/tinycolormap.hpp"
#include "clip/clip.h"
#include "ants/defines.h"
#include <mpi.h>
#include "cereal/types/map.hpp"
#include "cereal/types/vector.hpp"
#include "cereal/types/string.hpp"
#include "cereal/types/complex.hpp"
#include "cereal/archives/json.hpp"
#include "cereal/archives/binary.hpp"

using namespace ants;

static const Vector2i directions[] = {Vector2i(-1, -1), Vector2i(-1, 0), Vector2i(-1, 1),
                                      Vector2i(0, -1), Vector2i(0, 1),
                                      Vector2i(1, -1), Vector2i(1, 0), Vector2i(1, 1)};
static std::uniform_int_distribution<int> indexDist(0, 7);
static size_t maxAnts = 0;
static uint64_t antId = 0;

World::World(const std::string& filename, mINI::INIStructure config) {
    log_info("Creating world from PNG %s", filename.c_str());

    int32_t imgWidth, imgHeight, channels;
    uint8_t *image = stbi_load(filename.c_str(), &imgWidth, &imgHeight, &channels, 0);
    if (image == nullptr) {
        std::ostringstream oss;
        oss << "Failed to decode file " << filename << ": " << stbi_failure_reason();
        throw std::runtime_error(oss.str());
    }
    width = imgWidth;
    height = imgHeight;

    // construct grids, initialised with nullptr: https://stackoverflow.com/a/2204380/5007892
    foodGrid = SnapGrid2D<bool>(width, height);
    obstacleGrid = SnapGrid2D<bool>(width, height);

    // mapping between each unique colour and its position
    std::unordered_map<RGBColour, Vector2i> uniqueColours{};

    // seed the PCG RNG
    auto rngSeed = std::stol(config["Simulation"]["rng_seed"]);
    if (rngSeed == 0) {
        // as per config file, use an unpredictable source, in this case, time since unix epoch in nanoseconds
        auto now = std::chrono::system_clock::now().time_since_epoch();
        rngSeed = std::chrono::duration_cast<std::chrono::nanoseconds>(now).count();
    }
    log_debug("RNG seed is: %ld", rngSeed);
    rng.seed(rngSeed);

    // acquire random buffer generated with dump_random.cpp, from random.bin file
    log_debug("Attempting to acquire %d doubles from random.bin", width * height);
    FILE *randomBin = fopen("random.bin", "rb");
    for (int i = 0; i < width * height; i++) {
        double n = 0.0;
        if (fread(&n, 1, sizeof(n), randomBin) <= 1) {
            std::ostringstream ostream;
            ostream << "Failed to load double at idx " << i << " from random.bin, file too small?";
            throw std::runtime_error(ostream.str());
        }
        randomBuffer.emplace_back(n);
    }
    fclose(randomBin);

    for (int32_t y = 0; y < imgHeight; y++) {
        for (int32_t x = 0; x < imgWidth; x++) {
            // https://www.reddit.com/r/opengl/comments/8gyyb6/comment/dygokra/
            uint8_t *p = image + (channels * (y * imgWidth + x));
            uint8_t r = p[0];
            uint8_t g = p[1];
            uint8_t b = p[2];

            if (r == 0 && g == 0 && b == 0) {
                // black, empty square, skip
                continue;
            } else if (r == 0 && g == 255 && b == 0) {
                // green, food
                foodGrid.write(x, y, true);
            } else if (r == 128 && g == 128 && b == 128) {
                // grey, obstacle
                obstacleGrid.write(x, y, true);
            } else {
                // colony; store mapping between its unique colour and position
                uniqueColours[RGBColour(r, g, b)] = Vector2i(x, y);
            }
        }
    }
    foodGrid.commit();
    obstacleGrid.commit();
    log_debug("Have %zu unique colours (unique colonies)", uniqueColours.size());

    // setup colonies
    int c = 0;
    int numAnts = std::stoi(config["Colony"]["starting_ants"]);
    for (const auto &pair : uniqueColours) {
        auto [colour, pos] = pair;
        Colony colony{};
        colony.colour = colour;
        colony.pos = pos;
        colony.id = c++;

        log_debug("Colony colour (%d,%d,%d) at %d,%d (id %d)", colour.r, colour.g, colour.b, pos.x,
                  pos.y, colony.id);

        // add starting ants
        for (int i = 0; i < numAnts; i++) {
            Ant ant{};
            ant.holdingFood = false;
            // ant starts at the centre of colony
            ant.pos = pos;
            // preferred movement direction for when moving randomly
            ant.preferredDir = directions[indexDist(rng)];
            ant.id = antId++;
            colony.ants.emplace_back(ant);
        }
        colonies.emplace_back(colony);
    }
    pheromoneGrid = SnapGrid3D<PheromoneStrength>(width, height, static_cast<int>(colonies.size()));

    // initialise MPI
#if USE_MPI
    MPI_Comm_size(MPI_COMM_WORLD, &mpiWorldSize);
    MPI_Comm_rank(MPI_COMM_WORLD, &mpiRank);
    log_info("MPI world size: %d, my rank: %d", mpiWorldSize, mpiRank);

    // because we are using MPI_Scatter, not MPI_Scatterv, number of colonies must be divisible b
    // number of MPI workers
    if (colonies.size() % mpiWorldSize != 0) {
        std::ostringstream oss;
        oss << "Number of colonies (" << colonies.size() << ") is not divisible by number of "
                                                            "MPI workers (" << mpiWorldSize << ")!";
        throw std::runtime_error(oss.str());
    }
    mpiColoniesPerWorker = static_cast<int32_t>(colonies.size()) / mpiWorldSize;
    log_info("MPI will use %d colonies per worker (%zu colonies total, %d workers)", mpiColoniesPerWorker,
             colonies.size(), mpiWorldSize);
#endif

    // load INI values
    pheromoneDecayFactor = std::stod(config["Pheromones"]["decay_factor"]);
    pheromoneGainFactor = std::stod(config["Pheromones"]["gain_factor"]);
    pheromoneFuzzFactor = std::stod(config["Pheromones"]["fuzz_factor"]);
    antMoveRightChance = std::stod(config["Ants"]["move_right_chance"]);
    antKillNotUseful = std::stoi(config["Ants"]["kill_not_useful"]);
    antUsePheromone = std::stod(config["Ants"]["use_pheromone"]);
    colonyAntsPerTick = std::stoi(config["Colony"]["ants_per_tick"]);
    colonyHungerDrain = std::stod(config["Colony"]["hunger_drain"]);
    colonyHungerReplenish = std::stod(config["Colony"]["hunger_replenish"]);
    colonyReturnDist = std::stoi(config["Colony"]["return_distance"]);

    stbi_image_free(image);
}

Vector2i World::randomMovementVector(const Ant &ant, pcg32_fast &localRng) const {
    // uniform distribution between 0 and 1, currently used for ant move chance
    std::uniform_real_distribution<double> uniformDistribution(0.0, 1.0);
    std::uniform_int_distribution<int> positionDist(-1, 1);

    // generate random movement vector
    auto probability = antMoveRightChance;
    if (uniformDistribution(localRng) <= probability) {
        // move in the direction we were spawned with
        return ant.preferredDir;
    } else {
        // bad luck, move in a noisy direction
        return { positionDist(localRng), positionDist(localRng) };
    }
}

std::pair<Vector2i, double>
World::computePheromoneVector(const Colony &colony, const Ant &ant) const {
    Vector2i bestDirection{};
    double bestStrength = INT32_MIN + 1;

    for (const auto &direction : directions) {
        int x = ant.pos.x + direction.x;
        int y = ant.pos.y + direction.y;
        // check if out of bounds (same check as in World::update)
        if (x < 0 || y < 0 || x >= width || y >= height || obstacleGrid.read(x, y)) {
            continue;
        }
        // check it's not a position we have already visited this run
        if (ant.visitedPos.find(Vector2i(x,y)) != ant.visitedPos.end()) {
            continue;
        }

        double strength;
        if (ant.holdingFood) {
            // ant has food, use the "to colony" strength
            strength = pheromoneGrid.read(x, y, colony.id).toColony;
        } else {
            // ant doesn't have food, use the "to food" strength
            strength = pheromoneGrid.read(x, y, colony.id).toFood;
        }

        if (strength >= bestStrength) {
            // new best direction!
            bestStrength = strength;
            bestDirection = direction;
        }
    }
    return {bestDirection, bestStrength};
}

void World::decayPheromones() {
    // decay pheromones at a slightly different rate
    // - this massively slows down the sim (by at least 6x in release build)
    // - improves behaviour significantly
    double fuzz = pheromoneFuzzFactor * pheromoneDecayFactor;

#if USE_OMP
#pragma omp parallel default(none) firstprivate(fuzz)
#endif
    {
        int i = 0;
#if USE_OMP
#pragma omp for
#endif
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                for (int c = 0; c < static_cast<int>(colonies.size()); c++) {
                    // skip dead colonies to save doing extra work
                    if (colonies[c].isDead) {
                        continue;
                    }

                    auto cur = pheromoneGrid.read(x, y, c);
                    if (fabs(fuzz) >= 0.0001) {
                        // fuzz factor is not 0, use RNG
                        // micro-optimisation: compute random value and share it across toColony and
                        // toFood, instead of computing it twice
                        auto randomness = randomBuffer[i++ % randomBuffer.size()] * fuzz;
                        cur.toColony -= pheromoneDecayFactor + randomness;
                        cur.toFood -= pheromoneDecayFactor + randomness;
                    } else {
                        // fuzz factor is 0, don't use RNG
                        cur.toColony -= pheromoneDecayFactor;
                        cur.toFood -= pheromoneDecayFactor;
                    }
                    // clamp so it doesn't go below zero or above 1.0
                    cur.toColony = std::clamp(cur.toColony, 0.0, 1.0);
                    cur.toFood = std::clamp(cur.toFood, 0.0, 1.0);
                    // send it back to the grid
                    pheromoneGrid.write(x, y, c, cur);
                }
            }
        }
    }

    // force a commit, because we want the world to be updated when this routine returns
    pheromoneGrid.commit();
}

bool World::updateAnt(Ant *ant, Colony *colony, pcg32_fast &localRng) {
    bool shouldAddMoreAnts = false;

    // so that we don't kill all the ants at once (which looks weird), add some extra noise to the
    // time we might kill them
    std::uniform_int_distribution<int> antKillNoise(0, 75);

    // position the ant might move to
    auto newX = ant->pos.x;
    auto newY = ant->pos.y;

    // see what pheromones are around the ant
    auto [phVector, phStrength] = computePheromoneVector(*colony, *ant);
    Vector2i movement{};
    if (phStrength >= antUsePheromone) {
        // strong pheromone, use that
        movement = phVector;
    } else {
        // pheromone not strong enough, move randomly
        movement = randomMovementVector(*ant, localRng);
    }
    // apply movement vector
    newX += movement.x;
    newY += movement.y;

    // only move the ant if it wouldn't intersect an obstacle, and is in bounds
    // also don't allow ants to walk on food if they are already holding food
    if (newX < 0 || newY < 0 || newX >= width || newY >= height
        || obstacleGrid.read(newX, newY)
        || (ant->holdingFood && foodGrid.read(newX, newY))) {
        // reached an obstacle, flip our direction ("bounce off" the obstacle)
        ant->preferredDir.x *= -1;
        ant->preferredDir.y *= -1;
        // don't update ant position
    } else {
        // checks passed, so update the ant data
        ant->pos.x = newX;
        ant->pos.y = newY;
        ant->visitedPos.insert(Vector2i(newX, newY));
    }

    // update world
#if USE_OMP
#pragma omp critical
#endif
    {
        if (ant->holdingFood) {
            // holding food, add to the "to food" strength, so we let other ants know where we
            // found food
            auto cur = pheromoneGrid.read(ant->pos.x, ant->pos.y, colony->id);
            cur.toFood += pheromoneGainFactor;
            pheromoneGrid.write(ant->pos.x, ant->pos.y, colony->id, cur);
        } else {
            // looking for food, update the "to colony" strength, so other ants know how to get home
            auto cur = pheromoneGrid.read(ant->pos.x, ant->pos.y, colony->id);
            cur.toColony += pheromoneGainFactor;
            pheromoneGrid.write(ant->pos.x, ant->pos.y, colony->id, cur);
        }
    }

    // update ant state
    if (!ant->holdingFood && foodGrid.read(ant->pos.x, ant->pos.y)) {
        // we're on food now!
        log_trace("Ant id %lu in colony %d just found food at %d,%d", ant->id,
                  colony->id, ant->pos.x, ant->pos.y);
        ant->holdingFood = true;
        ant->ticksSinceLastUseful = 0;
        // since the ant has reached food, invert its direction for heading back
        ant->preferredDir.x *= -1;
        ant->preferredDir.y *= -1;
        // reset the positions the ant has visited for going home
        ant->visitedPos.clear();

        // remove food from the world
#if USE_OMP
#pragma omp critical
#endif
        foodGrid.write(ant->pos.x, ant->pos.y, false);
    } else if (ant->holdingFood && ant->pos.distance(colony->pos) <= colonyReturnDist) {
        // got our food and returned home (near enough to the colony)
        log_trace("Ant id %lu in colony %d just returned home with food", ant->id,
                  colony->id);
        ant->holdingFood = false;
        ant->ticksSinceLastUseful = 0;
        ant->visitedPos.clear();

        // boost the colony
        shouldAddMoreAnts = true;
    } // end update ant state

    // update ticks since last useful for the ant
    if (!ant->holdingFood) {
        ant->ticksSinceLastUseful++;
    }
    // possibly kill this ant if its time has expired (+ some noise, to give it a little extra shot
    // at life)
    if (ant->ticksSinceLastUseful > antKillNotUseful + antKillNoise(rng)) {
        log_trace("Ant id %lu in colony %d has died at %d,%d", ant->id, colony->id,
                  ant->pos.x, ant->pos.y);
        ant->isDead = true;
    }

    return shouldAddMoreAnts;
}

bool World::update() {
    size_t antsAlive = 0;
    bool shouldContinue = true;
    maxAntsLastTick = 0;
    int foodRemaining = 0;
    // when we thread this, we want each thread to have its own RNG. if we didn't do this, then the
    // way the threads access the RNG (which is non-deterministic) would in turn cause the sim
    // results to be non-deterministic. so, what we do is select a unique seed per function call
    // that each "thread local RNG" will be seeded with
    uint64_t seed = rng();

    // decay pheromones not in use
    decayPheromones();

    // colonies that need ants to be added to
    std::vector<Colony*> colonyAddAnts{};

    // update the ants
#if USE_OMP
#pragma omp parallel default(none) shared(colonyAddAnts, maxAnts, antsAlive, seed)
#endif
    {
        // setup thread local RNG
        pcg32_fast localRng{};
        localRng.seed(seed);
        // so that we don't kill all the ants at once (which looks weird), add some extra noise to the
        // time we might kill them
        std::uniform_int_distribution<int> antKillNoise(0, 75);

#if USE_OMP
#pragma omp for
#endif
        for (auto & c : colonies) {
            auto colony = &c;
            // skip dead colonies
            if (colony->isDead) {
                continue;
            }
            // main ant update loop
            for (size_t a = 0; a < colony->ants.size(); a++) {
                auto ant = &colony->ants[a];
                // skip dead ants
                if (ant->isDead) {
                    continue;
                }
                // update the ant
                if (updateAnt(ant, colony, localRng)) {
                    // record that we should add more ants to this colony
                    colonyAddAnts.emplace_back(colony);
                }
            } // end each ant in colony loop
        } // end each colony loop
    } // end OMP block

    // serial code that needs to be done after the loop begins here
    // spawn in new ants for colonies that need it
    for (auto colony : colonyAddAnts) {
        log_trace("Adding more ants to colony id %d", colony->id);
        // boost the colony
        colony->hunger += colonyHungerReplenish;
        // spawn in new ants
        for (int i = 0; i < colonyAntsPerTick; i++) {
            Ant newAnt{};
            newAnt.holdingFood = false;
            // newAnt starts at the centre of colony
            newAnt.pos = colony->pos;
            // preferred movement direction for when moving randomly
            newAnt.preferredDir = directions[indexDist(rng)];
            newAnt.id = antId++;
            colony->ants.emplace_back(newAnt);
        }
    }

    // process colony stats
    for (auto colony = colonies.begin(); colony != colonies.end(); colony++) {
        // update colony hunger
        colony->hunger -= colonyHungerDrain;
        colony->hunger = std::clamp(colony->hunger, 0.0, 1.0);

        // kill the colony if the hunger meter has expired, or all its ants have died
        if (colony->hunger <= 0 || colony->ants.empty()) {
            log_trace("Colony id %d has died! (hunger=%.2f, ants=%zu)", colony->id,
                      colony->hunger,
                      colony->ants.size());
            colony->isDead = true;
        } else {
            // colony has not died, so add to the ants alive count
            antsAlive += colony->ants.size();
        }

        // update max ants statistics based on this colony's data
        if (antsAlive > maxAnts) {
            maxAnts = antsAlive;
        }
        if (antsAlive > maxAntsLastTick) {
            maxAntsLastTick = antsAlive;
        }
    }

    // commit values to snapshot grid
    foodGrid.commit();
    pheromoneGrid.commit();
    //obstacleGrid.commit();

    // count food remaining, to know if we should do early exit
    // would be nice to optimise this using intrinsics (like popcnt), hopefully the compiler is smart
    // enough to do something
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            if (foodGrid.read(x, y)) {
                foodRemaining++;
            }
        }
    }
    // tell main.cpp if we should loop again or not
    if (antsAlive <= 0) {
        log_info("All ants have died");
        shouldContinue = false;
    } else if (foodRemaining <= 0) {
        log_info("All food has been eaten");
        shouldContinue = false;
    }
    return shouldContinue;
}

#if USE_MPI
std::unique_ptr<double[]> World::packPheromoneGrid() {
    int bufSize = pheromoneGrid.width * pheromoneGrid.height * pheromoneGrid.depth * 2;
    // use a unique ptr which will be automatically cleaned up when this function returns
    auto buf = std::make_unique<double[]>(bufSize);
    int bufIdx = 0;
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            for (size_t c = 0; c < colonies.size(); c++) {
                auto pheromone = pheromoneGrid.read(x, y, c);
                buf[bufIdx++] = pheromone.toColony;
                buf[bufIdx++] = pheromone.toFood;
            }
        }
    }
    return buf;
}

void World::unpackPheromoneGrid(const std::shared_ptr<double[]>& packedData) {
    int index = 0;
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            for (size_t c = 0; c < colonies.size(); c++) {
                auto toColony = packedData[index++];
                auto toFood = packedData[index++];
                pheromoneGrid.write(x, y, c, PheromoneStrength(toColony, toFood));
            }
        }
    }
}

bool World::updateMpi() {
    if (mpiRank == 0) {
        return updateMpiMaster();
    } else {
        return updateMpiWorker();
    }
}

void World::updateColoniesMpi(int *colonyWorkIdx, int *colonyAddAnts, uint64_t seed) {
    // setup thread local RNG
    pcg32_fast localRng{};
    localRng.seed(seed);
    // so that we don't kill all the ants at once (which looks weird), add some extra noise to the
    // time we might kill them
    std::uniform_int_distribution<int> antKillNoise(0, 75);
    memset(colonyAddAnts, -1, mpiColoniesPerWorker * sizeof(int));

    for (int c = 0; c < mpiColoniesPerWorker; c++) {
        log_trace("Processing colony index %d (id %d)", c, colonyWorkIdx[c]);
        auto colony = &colonies[colonyWorkIdx[c]];
        // skip dead colonies
        if (colony->isDead) {
            continue;
        }
        // main ant update loop
        for (size_t a = 0; a < colony->ants.size(); a++) {
            auto ant = &colony->ants[a];
            // skip dead ants
            if (ant->isDead) {
                continue;
            }
            // update the ant
            if (updateAnt(ant, colony, localRng)) {
                // record that we should add more ants to this colony
                // colonyAddAnts is a bit different in MPI, because the MPI master needs to know
                // the index of the colony, so put the colony id if we should add more ants, otherwise
                // -1 (since colony id 0 is valid)
                log_trace("updateAnt going to add ants, c = %d, colonyWorkIdx[c] = %d", c, colonyWorkIdx[c]);
                colonyAddAnts[c] = colonyWorkIdx[c];
            }
        } // end each ant in colony loop
    } // end each colony loop
}

bool World::updateMpiMaster() {
    size_t antsAlive = 0;
    bool shouldContinue = true;
    maxAntsLastTick = 0;
    int foodRemaining = 0;

    // first, generate and broadcast the RNG seed to all our workers
    uint64_t seed = rng();
    MPI_Bcast(&seed, 1, MPI_UINT64_T, 0, MPI_COMM_WORLD);
    MPI_Barrier(MPI_COMM_WORLD);
    log_trace("Sent seed to workers: 0x%lX", seed);

    // broadcast SnapGrids to all workers: foodGrid, obstacleGrid, pheromoneGrid
    // only broadcast the dirty grid to save time (remember, dirty == clean at the start of the loop)
    MPI_Bcast(foodGrid.dirty, foodGrid.width * foodGrid.height, MPI_CXX_BOOL,
              0, MPI_COMM_WORLD);
    log_trace("Master obstacle grid hash: 0x%X 0x%X", obstacleGrid.crc32Clean(), obstacleGrid.crc32Dirty());

    // the pheromone grid is harder since MPI can't send classes. what we will do instead is serialise
    // it manually using an array of doubles. we will store it like [toColony, toFood, toColony, toFood, ...]
    int phGridBufSize = pheromoneGrid.width * pheromoneGrid.height * pheromoneGrid.depth * 2;
    log_trace("Master phGridBufSize %d", phGridBufSize);
    // use a unique ptr which will be automatically cleaned up when this function returns
    auto phGridBuf = packPheromoneGrid();
    // transmit the serialised array
    MPI_Bcast(phGridBuf.get(), phGridBufSize, MPI_DOUBLE, 0, MPI_COMM_WORLD);
    MPI_Barrier(MPI_COMM_WORLD);

    log_trace("Sent SnapGrids to workers");
    log_trace("Sent foodGrid dirty hash 0x%X, clean hash 0x%X", foodGrid.crc32Dirty(), foodGrid.crc32Clean());
    log_trace("Sent pheromoneGrid phGridBuf, hash: 0x%X",
              crc32(phGridBuf.get(), pheromoneGrid.width * pheromoneGrid.height * pheromoneGrid.depth * 2 * sizeof(double)));

    // scatter colonies to all workers (this includes ourselves, the master!)
    // we can't broadcast colonies directly, so broadcast colony indices
    int colonyIdx[colonies.size()];
    for (size_t i = 0; i < colonies.size(); i++) {
        colonyIdx[i] = static_cast<int>(i);
    }
    // these are the colonies that we, the master, should work on
    int colonyWorkIdx[mpiColoniesPerWorker];
    MPI_Scatter(colonyIdx, mpiColoniesPerWorker, MPI_INT,
                colonyWorkIdx, mpiColoniesPerWorker, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Barrier(MPI_COMM_WORLD);
    log_trace("Sent scattered colonies");

    // do the work the master is required to do
    int masterColonyAddAnts[mpiColoniesPerWorker];
    memset(masterColonyAddAnts, -1, mpiColoniesPerWorker * sizeof(bool));
    updateColoniesMpi(colonyWorkIdx, masterColonyAddAnts, seed);
    MPI_Barrier(MPI_COMM_WORLD);

    // list of colonies indices we should actually add ants to
    std::vector<Colony*> colonyAddAntsList{};

    // now, receive updated colonies from workers
    // start from the first worker (we don't want to receive from the master!!)
    for (int i = 1; i < mpiWorldSize; i++) {
        log_trace("Attempting to receive from worker %d", i);
        // figure out how large the message they are trying to send is
        MPI_Status status{};
        MPI_Probe(i, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
        int size = 0;
        // note that MPI_Get_count returns the size **in bytes!!!!!**
        MPI_Get_count(&status, MPI_UINT8_T, &size);
        log_trace("Going to receive %d bytes from worker %d", size, i);

        // receive the message. 16 bytes of extra padding just in case.
        auto recvbuf = std::make_unique<uint8_t[]>(size + 16);
        // NOLINTNEXTLINE: because unsigned char == uint8_t
        MPI_Recv(recvbuf.get(), size, MPI_UINT8_T, i, 0, MPI_COMM_WORLD, &status);
        log_trace("Received OK");

        // deserialise it
        std::stringstream ss;
        // this is the best way I can think of to shove the byte array into the stringstream
        for (int j = 0; j < size; j++) {
            ss << recvbuf[j];
        }
        std::vector<Colony> receivedColonies{};
        {
            cereal::BinaryInputArchive ar(ss);
            ar(receivedColonies);
            // once out of scope, cereal will flush the archive (I believe??)
        }
        // store each received colony in our (the master) colonies array
        for (size_t j = 0; j < receivedColonies.size(); j++) {
            colonies[j] = receivedColonies[j];
        }

        // now receive colonyAddAnts
        int colonyAddAnts[mpiColoniesPerWorker];
        memset(colonyAddAnts, -1, mpiColoniesPerWorker * sizeof(int));
        MPI_Recv(colonyAddAnts, mpiColoniesPerWorker, MPI_INT, i,
                 TAG_COLONY_ADD_ANTS, MPI_COMM_WORLD, &status);
        for (int j = 0; j < mpiColoniesPerWorker; j++) {
            int shouldAddAnts = colonyAddAnts[j];
            if (shouldAddAnts != -1) {
                log_trace("Going to add ants to colony id %d", shouldAddAnts);
                colonyAddAntsList.emplace_back(&colonies[shouldAddAnts]);
            }
        }
        log_trace("Received colonyAddAnts from worker %d", i);
    }
    log_trace("Done receiving serialised data from workers");
    MPI_Barrier(MPI_COMM_WORLD);

    // receive grids from workers
    log_trace("Receiving grids from workers");
    for (int i = 1; i < mpiWorldSize; i++) {
        // receive foodGrid data
        auto foodBuf = std::make_unique<bool[]>(foodGrid.width * foodGrid.height);
        MPI_Status status{};
        MPI_Recv(foodBuf.get(), foodGrid.width * foodGrid.height, MPI_CXX_BOOL, i, TAG_FOOD_DATA,
                 MPI_COMM_WORLD, &status);
        log_trace("Received foodBuf from worker %d", i);

        // receive foodGrid written
        auto foodWrittenBuf = std::make_unique<bool[]>(foodGrid.width * foodGrid.height);
        MPI_Recv(foodWrittenBuf.get(), foodGrid.width * foodGrid.height, MPI_CXX_BOOL, i, TAG_FOOD_WRITTEN,
                 MPI_COMM_WORLD, &status);
        log_trace("Received foodWrittenBuf from worker %d", i);

        // merge the foodGrid with the world: copy in all the coordinates which were written by this
        // worker into the buffer
        for (int y = 0; y < foodGrid.height; y++) {
            for (int x = 0; x < foodGrid.width; x++) {
                int j = x + foodGrid.width * y;
                if (foodWrittenBuf[j]) {
                    foodGrid.write(x, y, foodBuf[j]);
                }
            }
        }
        log_trace("Merged food grid");

        // receive pheromoneGrid data
        auto phGridBufRecv = std::make_unique<double[]>(phGridBufSize);
        MPI_Recv(phGridBufRecv.get(), phGridBufSize, MPI_DOUBLE, i, TAG_PHEROMONES_DATA,
                 MPI_COMM_WORLD, &status);
        log_trace("Received phGridBufRecv from worker %d", i);

        // receive pheromoneGrid written
        int phWrittenBufSize = pheromoneGrid.width * pheromoneGrid.height * pheromoneGrid.depth;
        auto phWrittenBufRecv = std::make_unique<bool[]>(phWrittenBufSize);
        MPI_Recv(phWrittenBufRecv.get(), phWrittenBufSize, MPI_CXX_BOOL, i, TAG_PHEROMONES_WRITTEN,
                 MPI_COMM_WORLD, &status);
        log_trace("Received phWrittenBuf from worker %d", i);

        // merge pheromoneGrid
        int index = 0;
        for (int y = 0; y < pheromoneGrid.height; y++) {
            for (int x = 0; x < pheromoneGrid.width; x++) {
                for (int z = 0; z < pheromoneGrid.depth; z++) {
                    int j = x + pheromoneGrid.width * y + pheromoneGrid.width * pheromoneGrid.height * z;
                    auto toColony = phGridBufRecv[index++];
                    auto toFood = phGridBufRecv[index++];
                    if (phWrittenBufRecv[j]) {
                        // so we definitely wrote to this coordinate. we need to pull the indices
                        // out of the phGridBufRecv though now.
                        pheromoneGrid.write(x, y, z, PheromoneStrength(toColony, toFood));
                    }
                }
            }
        }
        log_trace("Merged pheromone grid");

        log_trace("Should be finished processing worker %d this loop", i);
    }
    log_trace("Done updating grids");
    MPI_Barrier(MPI_COMM_WORLD);

    log_trace("Passed all barriers master");

    // serial colony update
    // serial code that needs to be done after the loop begins here
    // spawn in new ants for colonies that need it
    log_trace("Going through colonyAddAntsList");
    for (auto colony : colonyAddAntsList) {
        log_trace("Colony pointer 0x%p", colony);
        log_trace("Adding more ants to colony id %d", colony->id);
        // boost the colony
        colony->hunger += colonyHungerReplenish;
        // spawn in new ants
        for (int i = 0; i < colonyAntsPerTick; i++) {
            Ant newAnt{};
            newAnt.holdingFood = false;
            // newAnt starts at the centre of colony
            newAnt.pos = colony->pos;
            // preferred movement direction for when moving randomly
            newAnt.preferredDir = directions[indexDist(rng)];
            newAnt.id = antId++;
            colony->ants.emplace_back(newAnt);
        }
    }

    // process colony stats
    log_trace("Processing colony stats");
    for (auto colony = colonies.begin(); colony != colonies.end(); colony++) {
        // update colony hunger
        colony->hunger -= colonyHungerDrain;
        colony->hunger = std::clamp(colony->hunger, 0.0, 1.0);

        // kill the colony if the hunger meter has expired, or all its ants have died
        if (colony->hunger <= 0 || colony->ants.empty()) {
            log_trace("Colony id %d has died! (hunger=%.2f, ants=%zu)", colony->id,
                      colony->hunger,
                      colony->ants.size());
            colony->isDead = true;
        } else {
            // colony has not died, so add to the ants alive count
            antsAlive += colony->ants.size();
        }

        // update max ants statistics based on this colony's data
        if (antsAlive > maxAnts) {
            maxAnts = antsAlive;
        }
        if (antsAlive > maxAntsLastTick) {
            maxAntsLastTick = antsAlive;
        }
    }

    // commit values to snapshot grid
    log_trace("Committing grids");
    foodGrid.commit();
    pheromoneGrid.commit();
    //obstacleGrid.commit();

    // count food remaining, to know if we should do early exit
    // would be nice to optimise this using intrinsics (like popcnt), hopefully the compiler is smart
    // enough to do something
    log_trace("Counting remaining food");
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            if (foodGrid.read(x, y)) {
                foodRemaining++;
            }
        }
    }
    // tell main.cpp if we should loop again or not
    log_trace("Returning shouldContinue");
    if (antsAlive <= 0) {
        log_info("All ants have died");
        shouldContinue = false;
    } else if (foodRemaining <= 0) {
        log_info("All food has been eaten");
        shouldContinue = false;
    }
    return shouldContinue;
}

bool World::updateMpiWorker() {
    // receive RNG seed from master
    uint64_t seed = 0;
    MPI_Bcast(&seed, 1, MPI_UINT64_T, 0, MPI_COMM_WORLD);
    MPI_Barrier(MPI_COMM_WORLD);
    log_trace("Received seed from master: 0x%lX", seed);

    // receive SnapGrids from master
    // as explained above we only receive the dirty buffer (because dirty == clean at the start of
    // the loop). we will have to call commit() later to ensure the clean is copied across.
    MPI_Bcast(foodGrid.dirty, foodGrid.width * foodGrid.height, MPI_CXX_BOOL,
              0, MPI_COMM_WORLD);
    log_trace("Worker obstacle grid hash: 0x%X 0x%X", obstacleGrid.crc32Clean(), obstacleGrid.crc32Dirty());

    // now we need to receive pheromoneGrid, as mentioned above it uses a serialisation format, so
    // we'll need to deserialise it
    int phGridBufSize = pheromoneGrid.width * pheromoneGrid.height * pheromoneGrid.depth * 2;
    log_trace("Worker phGridBufSize %d", phGridBufSize);
    // this shared_ptr will be deleted at the end of the function call
    auto tmpBuf = std::make_unique<double[]>(phGridBufSize);
    std::shared_ptr<double[]> phGridBuf = std::move(tmpBuf);
    MPI_Bcast(phGridBuf.get(), phGridBufSize, MPI_DOUBLE, 0, MPI_COMM_WORLD);
    unpackPheromoneGrid(phGridBuf);
    log_trace("Received pheromoneGrid phGridBuf, hash: 0x%X",
              crc32(phGridBuf.get(),pheromoneGrid.width * pheromoneGrid.height
                                    * pheromoneGrid.depth * 2 * sizeof(double)));

    // this will sync the dirty and clean arrays, so that they're equal
    foodGrid.commit();
//    obstacleGrid.commit();
    pheromoneGrid.commit();
    MPI_Barrier(MPI_COMM_WORLD);
    log_trace("Received SnapGrids from master");
    log_trace("Received foodGrid dirty hash 0x%X, clean hash 0x%X", foodGrid.crc32Dirty(), foodGrid.crc32Clean());

    // receive the scattered colonies from the master. these will be the indices of the colonies we
    // are supposed to process
    int colonyWorkIdx[mpiColoniesPerWorker];
    MPI_Scatter(nullptr, 0, MPI_INT, colonyWorkIdx,
                mpiColoniesPerWorker, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Barrier(MPI_COMM_WORLD);
    log_trace("Received scattered colonies");

    // time to process them!!
    int colonyAddAnts[mpiColoniesPerWorker];
    memset(colonyAddAnts, -1, mpiColoniesPerWorker * sizeof(bool));
    updateColoniesMpi(colonyWorkIdx, colonyAddAnts, seed);
    MPI_Barrier(MPI_COMM_WORLD);

    // send master back all the data we generated, so it can replicate our state
    // start with the colony updates. gather a list of colonies we worked on
    log_trace("Worker sending serialised worked on colonies back to master");
    std::vector<Colony> coloniesWorkedOn{};
    for (int i = 0; i < mpiColoniesPerWorker; i++) {
        coloniesWorkedOn.emplace_back(colonies[colonyWorkIdx[i]]);
    }
    // serialise with Cereal
    std::stringstream ss;
    {
        cereal::BinaryOutputArchive ar(ss);
        ar(colonies);
        // once out of scope, cereal will flush the stringstream
    }
    auto serstr = ss.str();
    log_trace("Serialised %zu bytes, sending to master", serstr.size());
    std::vector<uint8_t> tmp(serstr.begin(), serstr.end());
    MPI_Send(&tmp[0], static_cast<int>(serstr.size()), MPI_UINT8_T, 0, 0, MPI_COMM_WORLD);

    // send colony add ants as well to the master (like if we should add colonies or not)
    MPI_Send(colonyAddAnts, mpiColoniesPerWorker, MPI_INT, 0,
             TAG_COLONY_ADD_ANTS, MPI_COMM_WORLD);
    log_trace("Sent colonyAddAnts to master");
    MPI_Barrier(MPI_COMM_WORLD);

    // send grids
    // transmit the snapgrids and the areas we wrote on them back to the master
    // we only have to send pheromone grid and food grid because obstacle grid can't change
    log_trace("Sending grids back to master");
    MPI_Send(foodGrid.dirty, foodGrid.width * foodGrid.height, MPI_CXX_BOOL,
             0, TAG_FOOD_DATA, MPI_COMM_WORLD);
    log_trace("Worker sent foodGrid data");
    MPI_Send(foodGrid.written, foodGrid.width * foodGrid.height, MPI_CXX_BOOL,
             0, TAG_FOOD_WRITTEN, MPI_COMM_WORLD);
    log_trace("Worker sent foodGrid written");

    // this is the updated pheromone grid
    auto phGridBuf2 = std::make_unique<double[]>(phGridBufSize);
    MPI_Send(phGridBuf2.get(), phGridBufSize, MPI_DOUBLE, 0,
             TAG_PHEROMONES_DATA, MPI_COMM_WORLD);
    log_trace("Worker sent pheromoneGrid data");
    MPI_Send(pheromoneGrid.written, pheromoneGrid.width * pheromoneGrid.height * pheromoneGrid.depth,
             MPI_CXX_BOOL, 0, TAG_PHEROMONES_WRITTEN, MPI_COMM_WORLD);
    log_trace("Worker sent pheromoneGrid written");
    log_trace("Done sending grids");

    MPI_Barrier(MPI_COMM_WORLD);
    log_trace("Passed all barriers on worker");

    // commit SnapGrids here for the next time the worker is run
    foodGrid.commit();
    pheromoneGrid.commit();

    // worker always returns true in case there is more work to process
    return true;
}
#endif



// end of simulation code; start of util functions

static std::string generateFileName(const std::string &prefix) {
    // get the current date https://stackoverflow.com/a/16358111/5007892
    auto t = std::time(nullptr);
    auto tm = *std::localtime(&t);
    std::ostringstream oss;
    oss << prefix << std::put_time(&tm, "ants_%d-%m-%Y_%H-%M-%S.tar");
    return oss.str();
}

void World::setupRecording(const std::string &prefix) {
    auto filename = generateFileName(prefix);
    recordingPath = filename;

    int err = mtar_open(&tarfile, filename.c_str(), "w");
    if (err != 0) {
        std::ostringstream oss;
        oss << "Warning: Failed to create PNG TAR recording in " << filename << ": " << mtar_strerror(err) << "\n";
        log_warn("%s", oss.str().c_str());
    }

    log_info("Opened output TAR file %s for writing", filename.c_str());
    tarfileOk = true;
}

void World::writeRecordingStatistics(uint32_t numTicks, TimeInfo wallTime, TimeInfo simTime) {
    if (!tarfileOk)
        return;
    std::ostringstream oss;
    // clang-format off
    oss << "========== Statistics ==========\n"
           "Number of ticks: " << numTicks << "\n" <<
        "Wall time: " << wallTime << "\n"
                                     "Sim time: " << simTime << "\n";
    // clang-format on
    auto str = oss.str();
    mtar_write_file_header(&tarfile, "stats.txt", str.length());
    mtar_write_data(&tarfile, str.c_str(), str.length());
}

void World::finaliseRecording() {
    if (tarfileOk) {
        log_info("Finalising TAR file in %s", recordingPath.c_str());
        mtar_finalize(&tarfile);
        mtar_close(&tarfile);

        // if logged in as me on my development machine (not on getafix), also copy to clipboard
        // for easy transfer to visualiser.py
        // https://stackoverflow.com/a/8953445/5007892
        uid_t uid = geteuid();
        struct passwd *pw = getpwuid(uid);
        if (pw != nullptr && strcmp(pw->pw_name, "matt") == 0) {
            log_info("Copied path to clipboard for development");
            clip::set_text(recordingPath);
        } else if (pw == nullptr) {
            log_info("Failed to getpwuid: %s", strerror(errno));
        }
    } else {
        log_info("PNG TAR recording not initialised, so not being finalised");
    }
    log_info("Surviving colonies: %zu", colonies.size());
    log_info("Max ants alive: %zu", maxAnts);
}

double World::pheromoneToColour(int32_t x, int32_t y) const {
    // max over all the colonies of whichever is higher, to food or to colony
    double bestStrength = -9999.0;
    for (int c = 0; c < static_cast<int>(colonies.size()); c++) {
        double strength = std::max(pheromoneGrid.read(x, y, c).toFood, pheromoneGrid.read(x, y, c).toColony);
        if (strength > bestStrength) {
            bestStrength = strength;
        }
    }
    return bestStrength;
}

std::vector<uint8_t> World::renderWorldUncompressed() const {
    // pixel format is {R,G,B,R,G,B,...}
    std::vector<uint8_t> out{};
    out.reserve(width * height * 3);

    // render world
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            if (foodGrid.read(x, y)) {
                // pixel is food, output green
                out.push_back(0); // R
                out.push_back(255); // G
                out.push_back(0); // B
            } else if (obstacleGrid.read(x, y)) {
                // pixel is obstacle, output grey
                out.push_back(128);
                out.push_back(128);
                out.push_back(128);
            } else {
                // not a food or obstacle, so we'll juts write the pheromone value in the colour map
                // we tinycolormap and matplotlib's inferno colour map to make the output more
                // visually interesting
                auto pheromone = pheromoneToColour(x, y);
                auto colour = tinycolormap::GetInfernoColor(pheromone);
                out.push_back(colour.ri());
                out.push_back(colour.gi());
                out.push_back(colour.bi());
            }
        }
    }

    int channels = 3;
    // render ants and colony on top of world
    for (const auto &colony : colonies) {
        if (colony.isDead) {
            continue;
        }
        for (const auto &ant : colony.ants) {
            if (ant.isDead) {
                continue;
            }
            // ants are the same colour as their colony
            int y = ant.pos.y;
            int x = ant.pos.x;
            uint8_t *p = out.data() + (channels * (y * width + x));
            p[0] = colony.colour.r;
            p[1] = colony.colour.g;
            p[2] = colony.colour.b;
        }

        // draw colony as a square with colour based on hunger
        int h = 2;
        auto colour = colony.colour * colony.hunger;
        for (int y = colony.pos.y - h; y < colony.pos.y + h; y++) {
            for (int x = colony.pos.x - h; x < colony.pos.x + h; x++) {
                if (x < 0 || y < 0 || x >= width || y >= height) {
                    continue;
                }
                uint8_t *p = out.data() + (channels * (y * width + x));
                p[0] = colour.r;
                p[1] = colour.g;
                p[2] = colour.b;
            }
        }
    }

    return out;
}

void World::writeToTar(const std::string &filename, uint8_t *data, size_t len) {
    if (!tarfileOk) {
        return;
    }

    mtar_write_file_header(&tarfile, filename.c_str(), len);
    mtar_write_data(&tarfile, data, len);
}