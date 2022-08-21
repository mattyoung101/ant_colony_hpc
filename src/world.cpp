// World source file
// Matt Young, 2022, UQRacing
#include <stdexcept>
#include <sstream>
#include <ctime>
#include <iomanip>
#include "ant_simulation/world.h"
#include "stb/stb_image.h"
#include "stb/stb_image_write.h"
#include "ant_simulation/colony_record.h"
#include "ant_simulation/colony_tile.h"

/// Maximum number of entities on top of each other in a cell
#define MAX_ENTITIES_CELL 4

/// Divide to convert bytes to MiB
#define BYTES2MIB 1048576

using namespace ants;

World::World(const std::string& filename) {
    printf("Creating world from PNG %s\n", filename.c_str());

    int32_t imgWidth, imgHeight, channels;
    uint8_t *image = stbi_load(filename.c_str(), &imgWidth, &imgHeight, &channels, 0);
    if (image == nullptr) {
        std::ostringstream oss;
        oss << "Failed to decode file " << filename << ": " << stbi_failure_reason();
        throw std::runtime_error(oss.str());
    }
    width = imgWidth;
    height = imgHeight;
    grid = new Tile***[height];

    for (int32_t y = 0; y < imgHeight; y++) {
        Tile ***row = new Tile**[width];

        for (int32_t x = 0; x < imgWidth; x++) {
            Tile **cells = new Tile*[MAX_ENTITIES_CELL];

            // https://www.reddit.com/r/opengl/comments/8gyyb6/comment/dygokra/
            uint8_t *p = image + (channels * (y * imgWidth + x));
            uint8_t r = p[0];
            uint8_t g = p[1];
            uint8_t b = p[2];

            if (r == 0 && g == 0 && b == 0) {
                // black, empty square, skip
                cells[0] = new Empty();
            } else if (g == 255) {
                // green, food
                cells[0] = new Empty();
            } else if (r == 128 && g == 128 && b == 128) {
                // grey, obstacle
                //printf("Found grey!\n");
                cells[0] = new Empty();
            } else {
                // colony
                cells[0] = new Empty();
            }

            // fill remaining entities with an Empty entity so we can call methods on it
            for (int i = 1; i < MAX_ENTITIES_CELL; i++) {
                cells[i] = new Empty();
            }

            approxGridSize += MAX_ENTITIES_CELL * sizeof(Tile*); // NOLINT this sizeof is valid

            // insert the list of cells into the row
            row[x] = cells;
        }

        // add the row to the grid
        grid[y] = row;
    }
    printf("Approximate grid size: %zu MiB\n", approxGridSize / BYTES2MIB);

    stbi_image_free(image);
}

World::~World() {
    // finalise TAR file recording
    if (tarfileOk) {
        printf("Finalising PNG TAR recording\n");
        mtar_finalize(&tarfile);
        mtar_close(&tarfile);
    } else {
        printf("Warning: PNG TAR recording failed to initiailse so not being finalised\n");
    }

    // delete grid
    for (uint32_t y = 0; y < height; y++) {
        for (uint32_t x = 0; x < width; x++) {
            for (int entity = 0; entity < MAX_ENTITIES_CELL; entity++) {
                delete grid[y][x][entity];
            }
            delete[] grid[y][x];
        }
        delete[] grid[y];
    }
    delete[] grid;
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
        fprintf(stderr, "%s", oss.str().c_str());
    }

    printf("Opened output PNG TAR file %s for writing\n", filename.c_str());
    printf("Estimated recording buffer max usage: %zu MiB\n", (approxGridSize * recordInterval) / BYTES2MIB);
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

static void write_func(void *context, void *data, int size) {
    // TODO
}

void World::flushRecording() {
    if (!tarfileOk)
        return;

    int count = 0;
    for (const auto &pngData : pngBuffer) {
        // TODO convert to PNG and flush to TAR
        count++;
    }
    pngBuffer.clear();
    printf("Flushed %d in-memory PNGs to TAR file\n", count);
}
