// Copyright (c) 2022 Matt Young. All rights reserved.
//
// This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
// If a copy of the MPL was not distributed with this file, You can obtain one at
// http://mozilla.org/MPL/2.0/.
#include <cstdio>
#include <cstdint>
#include <iostream>
#include <chrono>
#include <thread>
#include <utility>
#include <omp.h>
#include "ants/world.h"
#include "mini/ini.h"
#include "log/log.h"
#include "ants/utils.h"
#include "stb/stb_image_write.h"
#include "ants/defines.h"
#include <mpi.h>

/// Counts the time in milliseconds between end and begin as a double. The time is counted as nanoseconds
/// for accuracy, then converted as a double to milliseconds.
#define COUNT_MS(end, begin) static_cast<double>( \
    std::chrono::duration_cast<std::chrono::nanoseconds>(end - begin).count()) / 1e6

using namespace ants;

/// Used for stbi image write, as the context parameter
struct ImageContext {
    std::string filename;
    World &world;
    ImageContext(std::string filename, World &world) : filename(std::move(filename)), world(world) {}
};

// for stbi_write
// the context pointer is the world instance
static void writeFunc(void *context, void *data, int size) {
    auto imageContext = static_cast<ImageContext*>(context);
    imageContext->world.writeToTar(imageContext->filename, static_cast<uint8_t *>(data), size);
}

// TODO intercept CTRL+C and write out the recording

int main(int argc, char *argv[]) {
    log_set_level(LOG_DEBUG);
    log_info("COSC3500 Ant Simulator - Matt Young, 2022");

    // ugly preprocessor macros begin here - sorry

    // initialise OpenMP
#if USE_OMP
#pragma omp parallel default(none)
    {
#pragma omp master
        log_info("Using OpenMP ant update with %d thread(s)", omp_get_num_threads());
    }
#endif
#if USE_MPI
    log_info("Using MPI ant update. Number of workers will be determined shortly.");
    MPI_Init(&argc, &argv);
#endif
#if !USE_OMP && !USE_MPI
    log_info("Using serial ant update");
#endif
    // load config file from disk
    std::string configPath = "antconfig.ini";
    if (argc == 2) {
        configPath = std::string(argv[1]);
    } else if (argc > 2) {
        throw std::invalid_argument("Usage: ./ant_colony [config_path]");
    }
    log_debug("Loading config file from %s", configPath.c_str());
    mINI::INIFile configFile(configPath);
    mINI::INIStructure config{};
    if (!configFile.read(config)) {
        throw std::runtime_error("Failed to load antconfig.ini, check your working dir");
    }

    // load the world into memory
    auto world = World(config["Simulation"]["grid_file"], config);

    // setup recording
    bool recordingEnabled;
#if USE_MPI
    // in MPI, only the master should record
    if (world.mpiRank == 0) {
#endif
    recordingEnabled = config["Simulation"]["recording_enabled"] == "true";
    if (recordingEnabled) {
        world.setupRecording(config["Simulation"]["output_prefix"]);
    } else {
        log_debug("PNG TAR recording disabled");
    }
#if USE_MPI
    } else {
        log_debug("Not MPI master, so not going to record");
        recordingEnabled = false;
    }
#endif

    // run the simulation for a fixed number of ticks
    uint32_t numTicks = std::stoi(config["Simulation"]["simulate_ticks"]);
    auto wallClockBegin = std::chrono::steady_clock::now();
    double simTimeMs = 0;
    log_info("Now running simulation for %u ticks", numTicks);

    // buffer of uncompressed pixels that need to be turned into PNGs and written to disk
    std::vector<std::vector<uint8_t>> images{};
    images.reserve(numTicks);
    std::ostringstream antTimeData;
    antTimeData << "NumAnts,TimeMs\n";

    // begin simulation update loop!
    for (uint32_t i = 0; i < numTicks; i++) {
        log_debug("Iteration %u", i);

        auto simTimeBegin = std::chrono::steady_clock::now();
#if USE_MPI
        bool shouldContinue = world.updateMpi();
#else
        bool shouldContinue = world.update();
#endif
        auto simTimeEnd = std::chrono::steady_clock::now();
        simTimeMs += COUNT_MS(simTimeEnd, simTimeBegin);
        antTimeData << world.maxAntsLastTick << "," << COUNT_MS(simTimeEnd, simTimeBegin) << "\n";

#if USE_MPI
        // sync up everyone after each loop
        MPI_Barrier(MPI_COMM_WORLD);
#endif

        // render world and add to uncompressed queue
        if (recordingEnabled) {
            images.emplace_back(world.renderWorldUncompressed());
        }

        // check early exit
        if (!shouldContinue) {
            log_info("Performing early exit now on iteration %d", i);
            break;
        }
    }
    // end simulation update loop

    // after the simulation:
    // go over each uncompressed image and compress it to the TAR file
    if (recordingEnabled) {
        size_t totalBytes = 0;
        log_info("Finalising PNG output (%zu images)", images.size());
        for (size_t i = 0; i < images.size(); i++) {
            auto image = images[i];
            totalBytes += image.size();

            // generate the filename and context for stb image write
            std::ostringstream ostream;
            ostream << i << ".png";
            ImageContext context(ostream.str(), world);

            // encode the png
            int w = static_cast<int>(world.width);
            int h = static_cast<int>(world.height);
            // source: https://solarianprogrammer.com/2019/06/10/c-programming-reading-writing-images-stb_image-libraries/
            stbi_write_png_to_func(writeFunc, &context, w, h, 3, image.data(), w * 3);
        }
        log_debug("Uncompressed image RAM usage was %zu MiB", totalBytes / BYTES2MIB);
    } else {
        log_debug("Not finalising PNG output because recording was not enabled");
    }
    log_info("Simulation done!");

    // record times
    auto wallClockEnd = std::chrono::steady_clock::now();
    auto wallTimeMs = COUNT_MS(wallClockEnd, wallClockBegin);
    auto wallFps = static_cast<double>(numTicks) / (static_cast<double>(wallTimeMs) / 1000.0);
    auto simFps = static_cast<double>(numTicks) / (static_cast<double>(simTimeMs) / 1000.0);

#if USE_MPI
    // if in MPI mode, the below code should only be run by the master (it's recording related)
    if (world.mpiRank == 0) {
#endif
    // finalise recording
    world.writeRecordingStatistics(numTicks, TimeInfo(wallTimeMs, wallFps),
                                   TimeInfo(simTimeMs, simFps));
    std::string antTime = antTimeData.str();
    world.writeToTar("ants_vs_time.csv", (uint8_t *) antTime.c_str(), antTime.size());
    world.finaliseRecording();

    log_info("=============== Timing Report ===============");
    log_info("Wall time: %.3f ms (%.3f ticks per second)", wallTimeMs, wallFps);
    log_info("Sim time: %.3f ms (%.3f ticks per second)", simTimeMs, simFps);
    auto nonSim = wallTimeMs - simTimeMs;
    log_info("Time spent in non-simulator tasks: %.3f ms (%.3f%%)", nonSim, (nonSim / wallTimeMs) * 100.0);

#if USE_MPI
    // quick way to
    }

    MPI_Finalize();
#endif

    return 0;
}