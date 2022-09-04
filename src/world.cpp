// World source file
// Matt Young, 2022
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

/// Macro to assist in freeing grids
#define DELETE_GRID(grid) do { \
for (int y = 0; y < height; y++) { \
    for (int  x = 0; x < width; x++) { \
        delete (grid)[y][x]; \
    } \
    delete[] (grid)[y]; \
} \
delete[] (grid); \
} while (0); \

using namespace ants;

static const Vector2i directions[] = {Vector2i(-1, -1), Vector2i(-1, 0), Vector2i(-1, 1),
                                      Vector2i(0, -1), Vector2i(0, 1),
                                      Vector2i(1, -1), Vector2i(1, 0), Vector2i(1, 1)};
static std::uniform_int_distribution<int> indexDist(0, 7);

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
    foodGrid = new bool*[height]{};
    pheromoneGrid = new Pheromone**[height]{};
    obstacleGrid = new bool*[height]{};

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

    for (int32_t y = 0; y < imgHeight; y++) {
        auto *foodRow = new bool[width]{};
        auto **pheromoneRow = new Pheromone*[width]{};
        auto *obstacleRow = new bool[width]{};

        for (int32_t x = 0; x < imgWidth; x++) {
            // https://www.reddit.com/r/opengl/comments/8gyyb6/comment/dygokra/
            uint8_t *p = image + (channels * (y * imgWidth + x));
            uint8_t r = p[0];
            uint8_t g = p[1];
            uint8_t b = p[2];

            // pheromone always gets added
            pheromoneRow[x] = new Pheromone();

            if (r == 0 && g == 0 && b == 0) {
                // black, empty square, skip
                continue;
            } else if (r == 0 && g == 255 && b == 0) {
                // green, food
                foodRow[x] = true;
            } else if (r == 128 && g == 128 && b == 128) {
                // grey, obstacle
                obstacleRow[x] = true;
            } else {
                // colony; store mapping between its unique colour and position
                uniqueColours[RGBColour(r, g, b)] = Vector2i(x, y);
            }
        }

        // add the row to the grid
        foodGrid[y] = foodRow;
        pheromoneGrid[y] = pheromoneRow; // always empty but add it anyway
        obstacleGrid[y] = obstacleRow;
    }

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
            colony.ants.emplace_back(ant);
        }
        colonies.emplace_back(colony);
    }

    // fix up pheromone grid using knowledge from above: each pheromone needs to have a reference to
    // each colony we have
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            for (size_t colony = 0; colony < uniqueColours.size(); colony++) {
                pheromoneGrid[y][x]->values.emplace_back(PheromoneStrength());
            }
        }
    }

    // load INI values
    pheromoneDecayFactor = std::stod(config["Pheromones"]["decay_factor"]);
    pheromoneGainFactor = std::stod(config["Pheromones"]["gain_factor"]);
    antMoveRightChance = std::stod(config["Ants"]["move_right_chance"]);
    antKillNotUseful = std::stoi(config["Ants"]["kill_not_useful"]);
    antUsePheromone = std::stod(config["Ants"]["use_pheromone"]);
    colonyAntsPerTick = std::stoi(config["Colony"]["ants_per_tick"]);
    colonyHungerDrain = std::stod(config["Colony"]["hunger_drain"]);
    colonyHungerReplenish = std::stod(config["Colony"]["hunger_replenish"]);

    stbi_image_free(image);
}

World::~World() {
    // delete grid
    DELETE_GRID(pheromoneGrid)
    for (int y = 0; y < height; y++) {
        delete[] obstacleGrid[y];
    }
    for (int y = 0; y < height; y++) {
        delete[] foodGrid[y];
    }
    delete[] obstacleGrid;
}

Vector2i World::randomMovementVector(const Ant &ant) {
    // uniform distribution between 0 and 1, currently used for ant move chance
    std::uniform_real_distribution<double> uniformDistribution(0.0, 1.0);
    std::uniform_int_distribution<int> positionDist(-1, 1);

    // generate random movement vector
    auto probability = antMoveRightChance;
    if (uniformDistribution(rng) <= probability) {
        // move in the direction we were spawned with
        return ant.preferredDir;
    } else {
        // bad luck, move in a noisy direction
        return { positionDist(rng), positionDist(rng) };
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
        if (x < 0 || y < 0 || x >= width || y >= height || obstacleGrid[y][x]) {
            continue;
        }
        // check it's not a position we have already visited
        if (ant.visitedPos.find(Vector2i(x,y)) != ant.visitedPos.end()) {
            continue;
        }

        double strength;
        if (ant.holdingFood) {
            // ant has food, use the "to colony" strength
            strength = pheromoneGrid[y][x]->values[colony.id].toColony;
        } else {
            // ant doesn't have food, use the "to food" strength
            strength = pheromoneGrid[y][x]->values[colony.id].toFood;
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
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            for (auto &value : pheromoneGrid[y][x]->values) {
                value.toColony -= pheromoneDecayFactor;
                value.toFood -= pheromoneDecayFactor;

                // clamp so it doesn't go below zero
                value.toColony = std::clamp(value.toColony, 0.0, 1.0);
                value.toFood = std::clamp(value.toFood, 0.0, 1.0);
            }
        }
    }
}

void World::update() {
    // so that we don't kill all the ants at once (which looks weird), add some extra noise to the
    // time we might kill them
    std::uniform_int_distribution<int> antKillNoise(0, 75);

    // decay pheromones not in use
    decayPheromones();

    // update the ants
    // TODO divide up this function to find more hotspots
    // TODO maybe we could fix some problems by having the ants **only** follow pheromones when going home?
    for (auto colony = colonies.begin(); colony != colonies.end() ; ) {
        bool colonyShouldAddMoreAnts = false;
        for (auto ant = colony->ants.begin(); ant != colony->ants.end(); ) {
            // position the ant might move to
            auto newX = ant->pos.x;
            auto newY = ant->pos.y;

            // see what pheromones are around teh ant
            auto [phVector, phStrength] = computePheromoneVector(*colony, *ant);
            Vector2i movement{};
            if (phStrength >= antUsePheromone) {
                // strong pheromone, use that
                movement = phVector;
            } else {
                // pheromone not strong enough, move randomly
                movement = randomMovementVector(*ant);
            }

            // apply movement vector
            newX += movement.x;
            newY += movement.y;

            // only move the ant if it wouldn't intersect an obstacle, and is in bounds
            // also don't allow ants to walk on food if they are already holding food
            if (newX < 0 || newY < 0 || newX >= width || newY >= height || obstacleGrid[newY][newX]
                || (ant->holdingFood && foodGrid[newY][newX])) {
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
            if (ant->holdingFood) {
                // holding food, add to the "to food" strength, so we let other ants know where we
                // found food
                pheromoneGrid[ant->pos.y][ant->pos.x]->values[colony->id].toFood += pheromoneGainFactor;
            } else {
                // looking for food, update the "to colony" strength, so other ants know how to get home
                pheromoneGrid[ant->pos.y][ant->pos.x]->values[colony->id].toColony += pheromoneGainFactor;
            }

            // update ant state
            if (!ant->holdingFood && foodGrid[ant->pos.y][ant->pos.x]) {
                // we're on food now!
                log_debug("Ant in colony %d just found food at %d,%d", colony->id, ant->pos.x, ant->pos.y);
                ant->holdingFood = true;
                ant->ticksSinceLastUseful = 0;

                foodGrid[ant->pos.y][ant->pos.x] = false;

                // since the ant has reached food, invert its direction for heading back
                ant->preferredDir.x *= -1;
                ant->preferredDir.y *= -1;

                ant->visitedPos.clear();
            } else if (ant->holdingFood && ant->pos.x == colony->pos.x && ant->pos.y == colony->pos.y) {
                // got our food and returned home (on top of the colony tile)
                log_debug("Ant in colony %d just returned home with food", colony->id);
                ant->holdingFood = false;
                ant->ticksSinceLastUseful = 0;
                ant->visitedPos.clear();

                // boost the colony
                colony->hunger += colonyHungerReplenish;
                // due to the fact that C++ is **by far** the worst programming language known to man,
                // we cannot (yes, physically CANNOT) insert into a vector while iterating over it,
                // no matter what iterator paradigm is used. so, this hack is required.
                // (see below for the rest of it)
                colonyShouldAddMoreAnts = true;
            }

            // update ticks since last useful
            if (!ant->holdingFood) {
                ant->ticksSinceLastUseful++;
            }
            // possibly kill this ant if its time has expired (+ some noise)
            if (ant->ticksSinceLastUseful > antKillNotUseful + antKillNoise(rng)) {
                log_debug("Ant in colony id %d has died", colony->id);
                ant = colony->ants.erase(ant);
            } else {
                ant++;
            }
        }

        // continuing on above from the terrible hack required due to C++
        // while I'm at it, it's like, we should note that most languages don't allow modification
        // while iterating, which is fair enough. but at least languages like Java have a
        // CopyOnWriteArrayList, or libgdx's SnapshotArray, or even just .removeAll() - none of which
        // are remotely provided in C++. in fact, the whole iterator system in C++, I would say, is
        // poorly designed trash that should be yeeted at the next ISO standard meeting, along with
        // the rest of this atrocious, awful, miserable, bloated language
        if (colonyShouldAddMoreAnts) {
            log_debug("Adding more ants to colony id %d", colony->id);
            for (int i = 0; i < colonyAntsPerTick; i++) {
                Ant newAnt{};
                newAnt.holdingFood = false;
                // newAnt starts at the centre of colony
                newAnt.pos = colony->pos;
                // preferred movement direction for when moving randomly
                newAnt.preferredDir = directions[indexDist(rng)];
                colony->ants.emplace_back(newAnt);
            }
        }

        // update colony hunger
        colony->hunger -= colonyHungerDrain;

        // kill the colony if the hunger meter has expired, or all its ants have died
        if (colony->hunger <= 0 || colony->ants.empty()) {
            log_debug("Colony id %d has died! (hunger=%.2f, ants=%zu)", colony->id, colony->hunger,
                      colony->ants.size());
            colony = colonies.erase(colony);
        } else {
            colony++;
        }
    }
}

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
    oss << "\n========== INI config used ==========\n";
    // TODO dump INI config to file as well
    auto str = oss.str();
    mtar_write_file_header(&tarfile, "stats.txt", str.length());
    mtar_write_data(&tarfile, str.c_str(), str.length());
}

void World::finaliseRecording() {
    if (tarfileOk) {
        log_debug("Finalising TAR file in %s", recordingPath.c_str());
        mtar_finalize(&tarfile);
        mtar_close(&tarfile);

        // if logged in as me on my development machine (not on getafix), also copy to clipboard
        // for easy transfer to visualiser.py
        // https://stackoverflow.com/a/8953445/5007892
        uid_t uid = geteuid();
        struct passwd *pw = getpwuid(uid);
        if (pw != nullptr && strcmp(pw->pw_name, "matt") == 0) {
            log_debug("Copied path to clipboard for development");
            clip::set_text(recordingPath);
        } else if (pw == nullptr) {
            log_debug("Failed to getpwuid: %s", strerror(errno));
        }
    } else {
        log_info("PNG TAR recording not initialised, so not being finalised");
    }
}

std::vector<uint8_t> World::renderWorldUncompressed() const {
    // pixel format is {R,G,B,R,G,B,...}
    std::vector<uint8_t> out{};
    out.reserve(width * height * 3);

    // render world
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            if (foodGrid[y][x]) {
                // pixel is food, output green
                out.push_back(0); // R
                out.push_back(255); // G
                out.push_back(0); // B
            } else if (obstacleGrid[y][x]) {
                // pixel is obstacle, output grey
                out.push_back(128);
                out.push_back(128);
                out.push_back(128);
            } else {
                // not a food or obstacle, so we'll juts write the pheromone value in the colour map
                // we tinycolormap and matplotlib's inferno colour map to make the output more
                // visually interesting
                auto pheromone = pheromoneGrid[y][x]->getColourValue();

                auto colour = tinycolormap::GetInfernoColor(pheromone);
                out.push_back(colour.ri());
                out.push_back(colour.gi());
                out.push_back(colour.bi());
            }
        }
    }

    // render ants on top of world
    for (const auto &colony : colonies) {
        for (const auto &ant : colony.ants) {
            // ants are the same colour as their colony
            int channels = 3;
            int y = ant.pos.y;
            int x = ant.pos.x;
            uint8_t *p = out.data() + (channels * (y * width + x));
            p[0] = colony.colour.r;
            p[1] = colony.colour.g;
            p[2] = colony.colour.b;
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