// World source file
// Matt Young, 2022, UQRacing
#include <stdexcept>
#include <sstream>
#include "ant_simulation/world.h"
#include "stb/stb_image.h"

using namespace ants;

/// Maximum number of entities that can occupy a single grid cell
#define MAX_ENTITIES_CELL 4

World::World(const std::string& filename) {
    printf("Creating world from PNG %s\n", filename.c_str());

    int32_t imgWidth, imgHeight, channels;
    uint8_t *image = stbi_load(filename.c_str(), &imgWidth, &imgHeight, &channels, 0);
    if (image == nullptr) {
        std::ostringstream oss;
        oss << "Failed to decode file " << filename << ": " << stbi_failure_reason();
        throw std::runtime_error(oss.str());
    }

    printf("Creating %d byte array\n", imgWidth * imgHeight * MAX_ENTITIES_CELL);
    // FIXME use a vector, should be easier
    grid = static_cast<Tile *>(calloc(imgWidth * imgHeight * MAX_ENTITIES_CELL, sizeof(Tile)));
    width = imgWidth;
    height = imgHeight;

    for (int32_t y = 0; y < imgHeight; y++) {
        for (int32_t x = 0; x < imgWidth; x++) {
            // https://www.reddit.com/r/opengl/comments/8gyyb6/comment/dygokra/
            uint8_t *p = image + (channels * (y * imgWidth + x));
            uint8_t r = p[0];
            uint8_t g = p[1];
            uint8_t b = p[2];

            if (r == 0 && g == 0 && b == 0) {
                // black, empty square, skip
                grid[x + imgWidth * y] = Empty();
            } else if (g == 255) {
                // green, food
                //printf("Found green!\n");
            } else if (r == 128 && g == 128 && b == 128) {
                // grey, obstacle
                //printf("Found grey!\n");
            } else {
                // colony
            }
        }
    }

    stbi_image_free(image);
}