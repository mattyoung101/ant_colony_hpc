// Ant colony simulation for COSC3500
// Matt Young, 2022
#include <cstdio>
#include <cstdint>
#include <iostream>
#include <chrono>
#include <thread>
#include "ant_simulation/world.h"
#include "mini/ini.h"
#include "log/log.h"

using namespace ants;

int main() {
    log_set_level(LOG_TRACE);
    log_info("COSC3500 Ant Simulator - Matt Young, 2022");

    // load config file from disk
    log_debug("Loading config file...");
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
    if (config["Simulation"]["recording_enabled"] == "true") {
        recordInterval = std::stoi(config["Simulation"]["disk_write_interval"]);
        world.setupRecording(config["Simulation"]["output_prefix"], recordInterval);
    } else {
        log_debug("PNG TAR recording disabled.");
    }

    // run the simulation for a fixed number of ticks
    uint32_t numTicks = std::stoi(config["Simulation"]["simulate_ticks"]);
    auto begin = std::chrono::steady_clock::now();
    log_info("Now running simulation for %u ticks", numTicks);

    for (uint32_t i = 0; i < numTicks; i++) {
        log_trace("Iteration %u", i);
        if (recordInterval > 0 && i % recordInterval == 0 && i > 0) {
            world.flushRecording();
        }
    }

    world.flushRecording();
    log_info("Simulation done!");

    auto end = std::chrono::steady_clock::now();
    auto diffMs = std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count();
    auto ticksPerSecond = ((double) numTicks) / ((double) diffMs / 1000.0);
    world.writeRecordingStatistics(numTicks, diffMs, ticksPerSecond);

    log_info("Simulated %u ticks in %ld ms (%.2f ticks per second)", numTicks, diffMs, ticksPerSecond);
    return 0;
}
