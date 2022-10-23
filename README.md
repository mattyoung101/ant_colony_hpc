# COSC3500 Ant Simulation Project
This is the ant simulation project I'm writing for COSC3500, written in C++17. Please see the report
for more information.

**Important: In order to compile on the getafix HPC cluster, please follow "Compiling on getafix" 
instructions below.**

Author: Matt Young (m.young2@uqconnect.edu.au)

## Features
TODO

## Building and running
### Compiling on getafix
Run `./build_getafix.sh` in the root directory of the project. It should do everything automatically,
and leave you with the release build of `ant_colony` in the root directory. Then, just run `./ant_colony`.

If that doesn't work, or you don't want to run shell scripts, you can execute the following commands:

1. Add a more recent gcc: `module load gnu/10.2.1`
2. Add a more recent version of CMake: `module load cmake/3.9.1`
3. Make a build directory: `mkdir build && cd build`
4. Run CMake: `cmake -DCMAKE_BUILD_TYPE="Release" ..`
5. Compile: `make -j16`
6. Fix paths: `cp ant_colony .. && cd ..`
7. Run: `./ant_colony`

### General instructions
Compile with at least Clang 12 (preferred), GCC 10, or later. The latest available version of CMake
is recommended (at least 3.16.0 required for full features and no hacks).

Make sure that antconfig.ini, maps/, and results/ and the ant_colony executable are in your working
directory. Then run ant_colony and it will do the rest. Feel free to take a look at antconfig.ini
to see what you can change.

Highly recommended to run the project in CLion, as that's what I used to develop.

## Implementation details
See the report for most of this. In future, I'll document it more here.

### Improvements done since serial draft
- Introduced random fuzzing to the pheromone decay rate, which improves results quite a lot
  - Random numbers are loaded from a file using `dump_random.cpp`, which is in turned still backed by PCG
  - This is done because profiling showed generating random numbers was one of the slowest parts of the program
- (TODO planned) Used AVX intrinsics for subtraction in `World::decayPheromones`
- Used locked grid structure during updates, the `SnapGrid`
  - This includes the complex behaviour of killing/spawning more ants, it is all "locked" whil the simulation is running
  - This breaks determinism with the first milestone, but is internally consistent (no race conditions when threading)
- (TODO planned) Parallelised individual ant updates using (either OpenMP or MPI - very unlikely but possibly CUDA)
- (TODO planned) Parallelised the loop in `World::decayPheromones`
- Simulation stops early if all ants die, or all food is eaten
- The delta time of each simulation step, and the number of ants in that step, are measured and logged

## Attribution
The following open source libraries are used:

- [stb_image](https://github.com/nothings/stb/blob/master/stb_image.h): image loading library: Public domain
- [stb_image_write](https://github.com/nothings/stb/blob/master/stb_image_write.h): image writing library: Public domain
- [mINI](https://github.com/pulzed/mINI): INI config parsing library: MIT licence
- [microtar](https://github.com/rxi/microtar): TAR IO library for C: MIT licence
- [pcg-cpp](https://github.com/imneme/pcg-cpp): C++ implementation of the high-quality PCG RNG algorithm: Apache 2.0 licence
- [tinycolormap](https://github.com/yuki-koyama/tinycolormap): Matplotlib colour maps in C++: MIT licence
- [clip](https://github.com/dacap/clip): Clipboard library, used for development purposes only: MIT licence

## Licence
Hopefully eventually MPL 2.0 once the course is done. For now, academic use at UQ only.