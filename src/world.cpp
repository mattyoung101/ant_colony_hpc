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
#include "ants/colony_tile.h"
#include "ants/food.h"
#include "ants/ant.h"
#include "log/log.h"

/// Divide to convert bytes to MiB
#define BYTES2MIB 1048576

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

World::World(const std::string& filename) {
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

    // TODO big brain idea for writing out grid - store original image as a background layer and just
    //  insert ants on top?? would be super easy

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
                // colony
                // store as a unique colour
                uniqueColours[RGBColour(r, g, b)] = Vector2i(x, y);
            }
        }

        // add the row to the grid
        foodGrid[y] = foodRow;
        pheromoneGrid[y] = pheromoneRow; // always empty but add it anyway
        obstacleGrid[y] = obstacleRow;
    }
    // I think this is incorrect but whatever
    approxGridSize = (width * height * sizeof(Food*))
            + (width * height * sizeof(Pheromone*)) + (width * height * sizeof(bool));
    log_debug("Approximate grid RAM usage: %zu MiB", approxGridSize / BYTES2MIB);

    log_debug("Have %zu unique colours (unique colonies)", uniqueColours.size());

    // setup colonies
    for (const auto &pair : uniqueColours) {
        auto [colour, pos] = pair;
        log_debug("Colony colour (%d,%d,%d) at %d,%d", colour.r, colour.g, colour.b, pos.x, pos.y);

        Colony colony{};
        colony.colour = colour;
        colony.pos = pos;
        // add starting ants
        // TODO get real number of ants from config
        for (int i = 0; i < 100; i++) {
            Ant ant{};
            // ant starts at the centre of colony
            ant.pos = pos;
            colony.ants.emplace_back(ant);
        }
        colonies.emplace_back(colony);
    }

    stbi_image_free(image);

    // TODO put in seed here
    rng = XoshiroCpp::Xoroshiro128StarStar(256);
}

World::~World() {
    // finalise TAR file recording
    // FIXME move this to not a finaliser, have it as as pecial function instead
    if (tarfileOk) {
        log_debug("Finalising PNG TAR recording");
        mtar_finalize(&tarfile);
        mtar_close(&tarfile);
    } else {
        log_info("PNG TAR recording not initialised, so not being finalised");
    }

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

void World::setupRecording(const std::string &prefix, uint32_t recordInterval) {
    auto filename = generateFileName(prefix);

    int err = mtar_open(&tarfile, filename.c_str(), "w");
    if (err != 0) {
        std::ostringstream oss;
        oss << "Warning: Failed to create PNG TAR recording in " << filename << ": " << mtar_strerror(err) << "\n";
        log_warn("%s", oss.str().c_str());
    }

    log_info("Opened output PNG TAR file %s for writing", filename.c_str());
    log_debug("Estimated recording buffer max usage: %zu MiB", (approxGridSize * recordInterval) / BYTES2MIB);
    tarfileOk = true;
}

void World::writeRecordingStatistics(uint32_t numTicks, long numMs, double ticksPerSecond) {
    if (!tarfileOk)
        return;

    std::ostringstream oss;
    oss << "========== Statistics =========="
           "\nNumber of ticks: " << numTicks <<
           "\nWall time (ms): " << numMs <<
           "\nWall TPS: " << ticksPerSecond;
    auto str = oss.str();
    mtar_write_file_header(&tarfile, "stats.txt", str.length());
    mtar_write_data(&tarfile, str.c_str(), str.length());
}

// for stbi_write
static void write_func(void *context, void *data, int size) {
    // TODO
}

void World::flushRecording() {
    if (!tarfileOk)
        return;

    int count = 0;
    /*for (const auto &pngData : pngBuffer) {
        // TODO convert to PNG and flush to TAR
        count++;
    }
    pngBuffer.clear();*/
    log_trace("Flushed %d in-memory PNGs to TAR file", count);
}

void World::update() {
    std::uniform_int_distribution<int> dist(-1, 1);

    for (const auto &colony : colonies) {
        for (auto ant : colony.ants) {
            // move ants by random amount
            int yOffset = dist(rng);
            int xOffset = dist(rng);

            ant.pos.x += xOffset;
            ant.pos.y += yOffset;
        }
    }
}
