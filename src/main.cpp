// Ant colony simulation for COSC3500
// Matt Young, 2022
#include <cstdio>
#include <cstdint>
#include <iostream>
#include <ctime>
#include <iomanip>
#include <chrono>
#include "ant_simulation/world.h"
#include "ant_simulation/ant.h"
#include "ant_simulation/entity.h"
#include "mini/ini.h"

using namespace ants;

int main() {
    printf("COSC3500 Ant Simulator - Matt Young, 2022\n");

    // load config file from disk
    printf("Loading config file...\n");
    mINI::INIFile configFile("antconfig.ini");
    mINI::INIStructure config{};
    if (!configFile.read(config)) {
        throw std::runtime_error("Failed to load antconfig.ini, check your working dir");
    }

    // load the world into memory
    std::string &worldPng = config["Simulation"]["grid_file"];
    auto world = World(worldPng);

    // run the simulation for a fixed number of ticks
    uint32_t numTicks = std::stoi(config["Simulation"]["simulate_ticks"]);
    printf("Running simulation for %u ticks\n", numTicks);
    auto begin = std::chrono::steady_clock::now();

    for (uint32_t i = 0; i < numTicks; i++) {
        ;
    }

    auto end = std::chrono::steady_clock::now();
    auto diffMs = std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count();
    auto ticksPerSecond = (double) numTicks / ((double) diffMs * 1000.0);
    printf("Simulation done!\n");
    fprintf(stderr, "Simulated %u ticks in %ld ms (%.2f ticks per second)\n", numTicks, diffMs, ticksPerSecond);

    // write and compress the output
    // we could save a lot of size by only storing the diff between frames (but would take more time to compute)
    // https://stackoverflow.com/a/16358111/5007892
    auto t = std::time(nullptr);
    auto tm = *std::localtime(&t);
    std::ostringstream oss;
    oss << std::put_time(&tm, "ants_%d-%m-%Y_%H-%M-%S.zip");
    auto fileName = oss.str();
    fprintf(stderr, "Saving data to %s...\n", fileName.c_str());

    printf("All done!\n");

    return 0;
}
