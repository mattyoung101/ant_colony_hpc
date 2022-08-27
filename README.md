# COSC3500 Ant Simulation Project
This is the ant simulation project I'm writing for COSC3500. It's written in C++20 and will be optimised
as much as possible over the duration of the course.

Author: Matt Young (m.young2@uqconnect.edu.au)

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

## Building and running
Compile with at least Clang 12 (preferred), GCC 10, or later. The latest available version of CMake
is recommended. If I end up adding PGO, you may need to change some compiler options to get PGO working
in GCC.

Debug and Release targets are supported in CMake. The Release target is designed to run on UQ SMP's
_getafix_ "high"-performance computing training server, which runs a decade old Intel CPU, so it uses
a particular march flag for this. It is also statically linked for similar reasons (getafix ships a decade
old CPU and decade old gcc version, so no compiling on there). The Debug target enables AddressSanitizer
and UndefinedBehaviourSanitizer and is slow for that reason.

Make sure that antconfig.ini, maps/, and results/ and the ant_colony executable are in your working
directory. Then run ant_colony and it will do the rest. Feel free to take a look at antconfig.ini
to see what you can change.

## Attribution
The following open source libraries are used:

- [stb_image](https://github.com/nothings/stb/blob/master/stb_image.h): image loading library: Public domain
- [stb_image_write](https://github.com/nothings/stb/blob/master/stb_image_write.h): image writing library: Public domain
- [mINI](https://github.com/pulzed/mINI): INI config parsing library: MIT licence
- [microtar](https://github.com/rxi/microtar): TAR IO library for C: MIT licence
- [XoroshiroCpp](https://github.com/Reputeless/Xoshiro-cpp): C++ implementation of the Xoshiro PRNG algorithm: MIT licence
- [tinycolormap](https://github.com/yuki-koyama/tinycolormap): Matplotlib colour maps in C++: MIT licence

## Licence
Currently proprietary until the course is done.