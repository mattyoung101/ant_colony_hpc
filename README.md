# COSC3500 Ant Simulation Project
This is the ant simulation project I'm writing for COSC3500, written in C++17. Please see the report
for more information.

**Important: In order to compile on the getafix HPC cluster, please follow "Compiling on getafix" instructions below.**

Author: Matt Young (m.young2@uqconnect.edu.au)

## Building and running
### Compiling on getafix
**You must follow this guide to compile on getafix. Compilation will not work by default** due to the fact
that getafix ships ancient versions of most tools by default.

1. Add a more recent gcc: `module load gnu/10.2.1`
2. Add a more recent version of CMake: `module load cmake/3.9.1`
3. Make a build directory: `mkdir build && cd build`
4. Run CMake: `cmake -DCMAKE_BUILD_TYPE="Release" ..`
5. Compile: `make -j16`
6. Fix paths: `cp ant_colony .. && cd ..`
7. Run: `./ant_colony`

### General instructions
Compile with at least Clang 12 (preferred), GCC 10, or later. The latest available version of CMake
is recommended. If I end up adding PGO, you may need to change some compiler options to get PGO working
in GCC.

Make sure that antconfig.ini, maps/, and results/ and the ant_colony executable are in your working
directory. Then run ant_colony and it will do the rest. Feel free to take a look at antconfig.ini
to see what you can change.

Highly recommended to run the project in CLion, as that's what I used to develop.


## Ant behaviour and game rules
- The user provides a PNG bitmap that specifies the locations of food (green), empty tiles (black), obstacles (grey), 
and ant colonies (any other colour)
- Each ant colony starts with a fixed number of ants
- Each ant colony has a hunger timer that depletes by a fixed amount per game tick. In order to produce
more ants, the colony hunger meter must be full, in which case it can produce one ant every fixed number
of ticks. If the colony hunger meter becomes empty, then the colony is disqualified.
- When an ant walks, it leaves a pheromone trail. If an ant has food, the pheromone trail is stronger.
- Pheromone trails decay by a fixed amount per tick over time if they are not used.
- Ants follow the strongest pheromone trail to get places. If they are in a certain range of a food tile, they walk
to the food directly. If none of these conditions are available, the ants wander randomly.

## Implementation details
See the docs folder for most of this.

TODO document some in here

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
Currently proprietary until the course is done. Hopefully eventually MPL 2.0.