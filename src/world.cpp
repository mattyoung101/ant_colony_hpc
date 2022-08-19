// World source file
// Matt Young, 2022, UQRacing
#include <stdexcept>
#include <sstream>
#include <ctime>
#include <iomanip>
#include "ant_simulation/world.h"
#include "stb/stb_image.h"
#include "stb/stb_image_write.h"

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

    for (int32_t y = 0; y < imgHeight; y++) {
        std::vector<std::vector<Tile>> row{};
        for (int32_t x = 0; x < imgWidth; x++) {
            Tile cell;
            // https://www.reddit.com/r/opengl/comments/8gyyb6/comment/dygokra/
            uint8_t *p = image + (channels * (y * imgWidth + x));
            uint8_t r = p[0];
            uint8_t g = p[1];
            uint8_t b = p[2];

            if (r == 0 && g == 0 && b == 0) {
                // black, empty square, skip
                cell = Empty();
            } else if (g == 255) {
                // green, food
                //printf("Found green!\n");
                cell = Empty();
            } else if (r == 128 && g == 128 && b == 128) {
                // grey, obstacle
                //printf("Found grey!\n");
                cell = Empty();
            } else {
                // colony
                cell = Empty();
            }

            // we only have one item in this cell to start with, but we require
            std::vector<Tile> entry{};
            entry.emplace_back(cell);
            // add to row
            row.emplace_back(entry);
        }
        grid.emplace_back(row);
    }

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

    int err = mtar_open(&tarfile, filename.c_str(), "w");
    if (err != 0) {
        std::ostringstream oss;
        oss << "Warning: Failed to open PNG TAR file " << filename << " for writing: err " << err << "\n";
        fprintf(stderr, "%s", oss.str().c_str());
    }

    printf("Opened output PNG TAR %s for writing\n", filename.c_str());
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
