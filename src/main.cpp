// Ant colony simulation for COSC3500
// Matt Young, 2022
#include <cstdio>
#include <cstdint>
#include <iostream>
#include <ctime>
#include <iomanip>
#include <chrono>
#include <thread>
#include "ant_simulation/world.h"
#include "microtar/microtar.h"
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

    // setup recording
    uint32_t recordInterval = 0;
    if (config["Simulation"]["recording_enable"] == "true") {
        recordInterval = std::stoi(config["Simulation"]["disk_write_interval"]);
        world.setupRecording(config["Simulation"]["output_prefix"]);
    } else {
        printf("PNG TAR recording disabled.\n");
    }

    // run the simulation for a fixed number of ticks
    uint32_t numTicks = std::stoi(config["Simulation"]["simulate_ticks"]);
    auto begin = std::chrono::steady_clock::now();
    printf("Running simulation for %u ticks\n", numTicks);

    for (uint32_t i = 0; i < numTicks; i++) {
        printf("Iteration %u\n", i);
        if (recordInterval > 0 && i % recordInterval == 0) {
            world.flushRecording();
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    world.flushRecording();
    printf("Simulation done!\n");

    auto end = std::chrono::steady_clock::now();
    auto diffMs = std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count();
    auto ticksPerSecond = ((double) numTicks) / ((double) diffMs / 1000.0);
    world.writeRecordingStatistics(numTicks, diffMs, ticksPerSecond);

    printf("Simulated %u ticks in %ld ms (%.2f ticks per second)\n", numTicks, diffMs, ticksPerSecond);
    return 0;
}
