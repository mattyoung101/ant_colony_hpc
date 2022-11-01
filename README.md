# COSC3500 Ant Simulation Project
This is the ant simulation project I'm writing for COSC3500, written in C++17. Please see the report
for more information.

### Important! In order to compile on the getafix HPC cluster, please follow "Compiling on getafix" instructions below.

**Author:** Matt Young (m.young2@uqconnect.edu.au)

## Features
TODO

## Building and running
### Compiling on getafix
Run `./build_getafix.sh` in the root directory of the project. It should do everything automatically.
Then, just run `./cmake-build-release-getafix/ant_colony` or `sbatch slurm.sh` or `sbatch gpuslurm.sh`.

Building manually (without the script) is more complicated, but can be done. The main challenge is
that the version of CMake which getafix ships is way too old to be useful. Instead, you will need
to use the version of CMake I included in this release archive.

### General instructions
Compile with at least Clang 10, GCC 10, or later. The latest available version of CMake
is recommended (at least 3.16.0 required for full features and no hacks). CUDA is also (unfortunately) 
recommended, but not required.

You can build the project by doing `mkdir cmake-build-release`, `cd cmake-build-release`,
`cmake -DCMAKE_BUILD_TYPE=Release ..`, `make -j16`. You will then have a binary called `ant_colony`, 
which you can call from the root directory of the project by doing `./cmake-build-release/ant_colony`.

It's highly recommended to run the project in CLion, as that's what I used to develop.

## Implementation details
See the report for most of this. In future, I'll document it more here.

### Improvements done since serial draft
- Introduced random fuzzing to the pheromone decay rate, which improves results quite a lot
  - Random numbers are loaded from a file using `dump_random.cpp`, which is in turned still backed by PCG
  - This is done because profiling showed generating random numbers was one of the slowest parts of the program
  - FIXME TODO the above is not implemented correctly yet!!!
- Used locked grid structure during updates, the `SnapGrid`
  - This includes the complex behaviour of killing/spawning more ants, it is all "locked" whil the simulation is running
  - This breaks determinism with the first milestone, but is internally consistent (no race conditions when threading)
- Parallelised individual ant updates using OpenMP
- (TODO planned) Parallelised individual ant updates using MPI
- Parallelised the loop in `World::decayPheromones`
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
- [cereal](https://github.com/USCiLab/cereal): C++ serialisation framework: BSD 3-clause licence

## Licence
Hopefully eventually MPL 2.0 once the course is done. For now, academic use at UQ only.