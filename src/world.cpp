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
        std::uniform_int_distribution<int> indexDist(0, 7);
        for (int i = 0; i < numAnts; i++) {
            Ant ant{};
            ant.isFirstGeneration = true;
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

std::pair<Vector2i, double>
World::computePheromoneVector(const Colony &colony, const Ant &ant) const {
    // FIXME don't search with searchWidth, just check ant neighbouring bounds

    std::unordered_map<Vector2i, double> strengths{};
    for (const auto &direction : directions) {
        int x = ant.pos.x + direction.x;
        int y = ant.pos.y + direction.y;
        if (x < 0 || y < 0 || x >= width || y >= height || obstacleGrid[y][x]) {
            // out of bounds (same check as in World::update)
            continue;
        }
        // TODO
    }

//    // carefully construct bounds: make sure we don't access outside the grid
//    auto pos = ant.pos;
//    int minY = std::clamp(pos.y - searchWidth, 0, height);
//    int maxY = std::clamp(pos.y + searchWidth, 0, height);
//    int minX = std::clamp(pos.x - searchWidth, 0, width);
//    int maxX = std::clamp(pos.x + searchWidth, 0, width);
//
//    // FIXME could this be replaced with a convolution?
//    // could also be weighted average, but I'm worried the diagonals will be weighted too highly then
//
//    std::vector<std::pair<Vector2i, PheromoneStrength>> out{};
//    // maybe change this for loop to be a constant lookup on the diagonals and stuff
//    for (int y = minY; y < maxY; y++) {
//        for (int x = minX; x < maxX; x++) {
//            // skip over the ant itself
//            if (x == pos.x && y == pos.y) {
//                continue;
//            }
//        }
//    }

    return {};
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

void World::update() noexcept {
    // uniform distribution between 0 and 1, currently used for ant move chance
    std::uniform_real_distribution<double> uniformDistribution(0.0, 1.0);
    // could pre-calculate this to save like 1 nanosecond
    double maxDist = static_cast<double>(Vector2i(0, 0).distance(Vector2i(width, height)));

    // decay pheromones not in use
    decayPheromones();

    // update the ants
    for (auto colony = colonies.begin(); colony != colonies.end() ; ) {
        for (auto &ant: colony->ants) {
            // position the ant might move to
            auto newX = ant.pos.x;
            auto newY = ant.pos.y;

            // TODO we should kill off ants that haven't touched food in like >n ticks

            if (!ant.holdingFood) {
                // ant is not holding food: we want to get to food

                // first, see what pheromones are around the ant
                auto [phVector, phStrength] = computePheromoneVector(*colony, ant);

                // steer ant towards food (with a bit of randomness)
                // see ant_navigation.md for full implementation details
                auto [foodDist, foodPos] = findNearestFood(ant.pos);

                // probability of moving in the right direction (towards the nearest food)
                auto probability = antMoveRightChance;
                // movement vector that corresponds with a compass direction (e.g. (1,1) = NE)
                Vector2i movement{};

                if (uniformDistribution(rng) <= probability) {
                    // we're in luck, move in the right direction
                    movement = (foodPos - ant.pos).norm();
                } else {
                    // bad luck, move in a noisy direction
                    movement = randomMovementVector();
                }
                // apply movement vector
                newX += movement.x;
                newY += movement.y;
            } else {
                // ant is holding food: we want to go back to the colony
                // TODO
            }

            // only move the ant if it wouldn't intersect an obstacle, and is in bounds
            if (newX < 0 || newY < 0 || newX >= width || newY >= height || obstacleGrid[newY][newX]) {
                continue;
            }

            // checks passed, so update the ant data
            ant.pos.x = newX;
            ant.pos.y = newY;

            // update world
            // FIXME this should depend on ant mode (if it's looking for food or going home)
            pheromoneGrid[ant.pos.y][ant.pos.x]->values[colony->id].toFood += pheromoneGainFactor;

            // update ant state
            if (!ant.holdingFood && foodGrid[ant.pos.y][ant.pos.x]) {
                // we're on food now!
                log_debug("Ant in colony %d just found food at %d,%d", colony->id, ant.pos.x, ant.pos.y);
                ant.holdingFood = true;
                // "use up" this resource in the world
                foodGrid[ant.pos.y][ant.pos.x] = false;
            } else if (ant.holdingFood && ant.pos.x == colony->pos.x && ant.pos.y == colony->pos.y) {
                // got our food and returned home (on top of the colony tile)
                log_debug("Ant in colony %d just returned home with food", colony->id);
                ant.holdingFood = false;
                // TODO update colony stats (hunger, have it make more ants, etc)
            }
        }

        // update colony hunger
        if (colony->hunger <= 0) {
            log_debug("Colony id %d has died!", colony->id);
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