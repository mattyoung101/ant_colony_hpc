// World source file
// Matt Young, 2022
#include <stdexcept>
#include <sstream>
#include <ctime>
#include <iomanip>
#include <set>
#include <unordered_map>
#include <random>
#include "ants/world.h"
#include "stb/stb_image.h"
#include "stb/stb_image_write.h"
#include "ants/colony.h"
#include "ants/food.h"
#include "ants/ant.h"
#include "log/log.h"
#include "tinycolor/tinycolormap.hpp"

/// Macro to assist in freeing grids
#define DELETE_GRID(grid) do { \
for (uint32_t y = 0; y < height; y++) { \
    for (uint32_t x = 0; x < width; x++) { \
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
    foodGrid = new Food**[height]{};
    pheromoneGrid = new Pheromone**[height]{};
    obstacleGrid = new bool*[height]{};

    // mapping between each unique colour and its position
    std::unordered_map<RGBColour, Vector2i> uniqueColours{};
    rng = XoshiroCpp::Xoshiro256StarStar(std::stoi(config["Simulation"]["rng_seed"]));

    for (int32_t y = 0; y < imgHeight; y++) {
        auto **foodRow = new Food*[width]{};
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
                foodRow[x] = new Food();
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
    for (const auto &pair : uniqueColours) {
        auto [colour, pos] = pair;
        log_debug("Colony colour (%d,%d,%d) at %d,%d", colour.r, colour.g, colour.b, pos.x, pos.y);

        Colony colony{};
        colony.colour = colour;
        colony.pos = pos;

        // add starting ants
        int numAnts = std::stoi(config["Colony"]["starting_ants"]);
        for (int i = 0; i < numAnts; i++) {
            Ant ant{};
            // ant starts at the centre of colony
            ant.pos = pos;
            colony.ants.emplace_back(ant);
        }
        colonies.emplace_back(colony);
    }

    stbi_image_free(image);
}

World::~World() {
    // delete grid
    DELETE_GRID(foodGrid)
    DELETE_GRID(pheromoneGrid)
    for (uint32_t y = 0; y < height; y++) {
        delete[] obstacleGrid[y];
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
    auto str = oss.str();
    mtar_write_file_header(&tarfile, "stats.txt", str.length());
    mtar_write_data(&tarfile, str.c_str(), str.length());
}

template <class T>
static inline constexpr T clamp(T value, T min, T max) {
    if (value > max) {
        return max;
    } else if (value < min) {
        return min;
    } else {
        return value;
    }
}

/// See ant_navigation.md. Normalised distance to food vs. noise/data mix factor.
static inline constexpr double distanceLookup(double x) {
    double g = 2.3;
    double k = 0.2;
    return k * exp(g * x);
}

void World::update() noexcept {
    std::uniform_int_distribution<int> dist(-1, 1);

    for (auto &colony: colonies) {
        for (auto &ant: colony.ants) {
            // move ants by random amount
            int yOffset = dist(rng);
            int xOffset = dist(rng);

            int32_t newX = ant.pos.x + xOffset;
            int32_t newY = ant.pos.y + yOffset;

            // only move the ant if it wouldn't intersect an obstacle, and is in bounds
            if (newX < 0 || newY < 0 || newX >= static_cast<int32_t>(width) ||
                    newY >= static_cast<int32_t>(height) || obstacleGrid[newY][newX]) {
                continue;
            }

            ant.pos.x = newX;
            ant.pos.y = newY;
            pheromoneGrid[ant.pos.y][ant.pos.x]->value += 0.01;
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
    for (uint32_t y = 0; y < height; y++) {
        for (uint32_t x = 0; x < width; x++) {
            if (foodGrid[y][x] != nullptr) {
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
                auto pheromone = pheromoneGrid[y][x]->value;
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
        // TODO draw colony as a circle
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
