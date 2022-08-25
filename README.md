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
- Grid-based: ants can only be in one integer grid cell at a time
  - This is to ideally to improve performance and make it easier to vectorise
  - If you want a denser grid, use a higher resolution grid
- OR:
- Do it based on entities and circles. Continuous. So for example the food source is a circle.

## Building and running
- Compile with Clang 12 or later, CMake and Ninja or Unix Makefiles
- GCC may work as well, but you may have to change some stuff related to profiling

## Attribution
The following open source libraries are used:

- [stb_image](https://github.com/nothings/stb/blob/master/stb_image.h), an image loading library: Public domain
- [stb_image_write](https://github.com/nothings/stb/blob/master/stb_image_write.h), an image writing library: Public domain
- [mINI](https://github.com/pulzed/mINI), an INI config parsing library: MIT licence
- [XoroshiroCpp](https://github.com/Reputeless/Xoshiro-cpp): C++ implementation of the Xoroshiro algorithm: MIT licence

## Licence
Currently proprietary until the course is done.