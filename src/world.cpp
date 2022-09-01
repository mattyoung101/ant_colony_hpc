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
#include "ants/world.h"
#include "stb/stb_image.h"
#include "stb/stb_image_write.h"
#include "ants/colony.h"
#include "ants/ant.h"
#include "log/log.h"
#include "tinycolor/tinycolormap.hpp"

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
            // ant starts at the centre of colony
            ant.pos = pos;
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

/// See ant_navigation.md. Normalised distance to chance of moving in the right direction.
static inline constexpr double distanceLookup(double x) {
    // https://www.desmos.com/calculator/jrir9ivnnt
    double g = 1.1;
    double k = 0.2;
    double y = k * exp(g * x);
    return std::clamp(y, 0.0, 1.0);

//    return std::clamp(0.45 * sin(1.0/x) + 0.5, 0.0, 1.0);
}

Vector2i World::randomMovementVector() {
    std::uniform_int_distribution<int> positionDist(-1, 1);
    // we actually get better looking random movement by allowing the ant to do (0,0) sometimes
    // without that, it looks kinda robotic
    return {positionDist(rng), positionDist(rng)};
}

// good target for optimisation
std::pair<double, Vector2i> World::findNearestFood(const Vector2i &pos) const {
    int minDist = INT32_MAX - 1;
    Vector2i minPos{};

    // loop over each food element and check if it's closer to the ant than the previously recorded one
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            if (!foodGrid[y][x]) {
                // no food here
                continue;
            }
            // food here, let's see if it's closer
            auto newPos = Vector2i(x, y);
            int newDist = newPos.distance(pos);

            if (newDist <= minDist) {
                minDist = newDist;
                minPos = newPos;
            }
        }
    }
    return std::make_pair(minDist, minPos);
}

void World::update() noexcept {
    // random angle between 0 and 2pi
    std::uniform_real_distribution<double> moveChance(0.0, 1.0);
    // FIXME should pre-calculate this
    double maxDist = static_cast<double>(Vector2i(0, 0).distance(Vector2i(width, height)));

    // decay pheromones not in use
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

    // update the ants
    for (auto &colony: colonies) {
        for (auto &ant: colony.ants) {
            // steer ant towards food (with a bit of randomness)
            // see ant_navigation.md for full implementation details
            auto [foodDist, foodPos] = findNearestFood(ant.pos);
            auto normalisedDist = static_cast<double>(foodDist) / maxDist;

            // probability of moving in the right direction (towards the nearest food)
            auto probability = distanceLookup(normalisedDist);
            // movement vector that corresponds with a compass direction (e.g. (1,1) = NE)
            Vector2i movement{};

            if (moveChance(rng) <= probability) {
                // we're in luck, move in the right direction
                movement = (foodPos - ant.pos).norm();
            } else {
                // bad luck, move in a noisy direction
                movement = randomMovementVector();
            }

            // apply movement vector
            int32_t newX = ant.pos.x + movement.x;
            int32_t newY = ant.pos.y + movement.y;

            // only move the ant if it wouldn't intersect an obstacle, and is in bounds
            if (newX < 0 || newY < 0 || newX >= static_cast<int32_t>(width) ||
                    newY >= static_cast<int32_t>(height) || obstacleGrid[newY][newX]) {
                continue;
            }

            ant.pos.x = newX;
            ant.pos.y = newY;
            // TODO get strength value from config
            pheromoneGrid[ant.pos.y][ant.pos.x]->values[colony.id].toFood += 0.01;
        }
    }
}

void World::finaliseRecording() {
    if (tarfileOk) {
        log_debug("Finalising TAR file in %s", recordingPath.c_str());
        mtar_finalize(&tarfile);
        mtar_close(&tarfile);
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
        // TODO draw colony as a circle (?)
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
