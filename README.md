# COSC3500 Ant Simulation Project
This is the ant simulation project I wrote for COSC3500, written in C++17. Please see the report
for more information.

For more information, please see my paper (COSC3500_Report_Milestone_2.pdf).

**Author:** Matt Young (m.young2@uqconnect.edu.au)

## Building and running
### General instructions
Compile with at least Clang 10, GCC 10, or later. The latest available version of CMake
is recommended (at least 3.16.0 required for full features and no hacks). OpenMPI v3 is required to
compile the code, but you don't actually have to run with MPI enabled. Other MPI implementations _may_
work as well, but certainly not below the MPI 3.0 standard.

In `defines.h` you can choose if you want to use OpenMP, MPI, none, or both, to accelerate the simulator.
When you set these defines and recompile, the code will change and everything will Just Work(TM) (seriously!)
If all of them are turned off, the ant sim will default to a serial update. 

**PLEASE NOTE:** The MPI code is **atrocious** and you should NOT use it. It's like 10x slower or something. 
I wrote it during a sleep-deprived bender the night before this project was due. Genuinely actually avoid it.

You can build the project as follows:

```
mkdir cmake-build-release
cd cmake-build-release
cmake -DCMAKE_BUILD_TYPE=Release ..
make -j16  # replace -j16 with however many CPUs you have
```

You will then have a binary called `ant_colony`,
which you can call from the root directory of the project by doing `./cmake-build-release/ant_colony`.

Build targets are Debug (-O0 + ASan + UBSan), Release (-O3 -march=native -mtune=native) and Profile (just -O0),
feel free to choose between them.

## Implementation details
Please see the attached paper for all implementation details. 

### Improvements done since serial draft
- Introduced random fuzzing to the pheromone decay rate, which improves results quite a lot
  - Random numbers are loaded from a file using `dump_random.cpp`, which is in turned still backed by PCG
  - This is done because profiling showed generating random numbers was one of the slowest parts of the program
  - FIXME TODO the above is not implemented correctly yet!!!
- Used locked grid structure during updates, the `SnapGrid`
  - This includes the complex behaviour of killing/spawning more ants, it is all "locked" whil the simulation is running
  - This breaks determinism with the first milestone, but is internally consistent (no race conditions when threading)
- Parallelised individual ant updates using OpenMP
- Parallelised individual ant updates using MPI
  - This was crappy and is like 10x slower
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
- [cereal](https://github.com/USCiLab/cereal): C++ binary serialisation/deserialisation, used for MPI communication: BSD 3-clause licence

## Licence
Mozilla Public License v2.0