// World source file
// Matt Young, 2022
#include <stdexcept>
#include <sstream>
#include <ctime>
#include <iomanip>
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
    colonyGrid = new ColonyTile**[height]{};
    obstacleGrid = new bool*[height]{};

    for (int32_t y = 0; y < imgHeight; y++) {
        auto **foodRow = new Food*[width]{};
        auto **pheromoneRow = new Pheromone*[width]{};
        auto **colonyRow = new ColonyTile*[width]{};
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
            } else if (g == 255) {
                // green, food
                foodRow[x] = new Food();
            } else if (r == 128 && g == 128 && b == 128) {
                // grey, obstacle
                obstacleRow[x] = true;
            } else {
                // colony
                colonyRow[x] = new ColonyTile(r, g, b);
            }
        }

        // add the row to the grid
        foodGrid[y] = foodRow;
        pheromoneGrid[y] = pheromoneRow;
        colonyGrid[y] = colonyRow;
        obstacleGrid[y] = obstacleRow;
    }
    // I think this is incorrect but whatever
    approxGridSize = (width * height * sizeof(Food*)) + (width * height * sizeof(ColonyTile*))
            + (width * height * sizeof(Pheromone*)) + (width * height * sizeof(bool));
    log_debug("Approximate grid RAM usage: %zu MiB", approxGridSize / BYTES2MIB);

    stbi_image_free(image);
}

World::~World() {
    // finalise TAR file recording
    if (tarfileOk) {
        log_debug("Finalising PNG TAR recording");
        mtar_finalize(&tarfile);
        mtar_close(&tarfile);
    } else {
        log_info("PNG TAR recording not initialised, so not being finalised");
    }

    // delete grid
    delete[] colonyGrid;
    delete[] foodGrid;
    delete[] pheromoneGrid;
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
           "\nMilliseconds taken: " << numMs <<
           "\nTicks per second: " << ticksPerSecond;
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
