# COSC3500 Ant Simulation Project
This is the ant simulation project I'm writing for COSC3500. It's written in C++20 and will be optimised
as much as possible over the duration of the course.

Author: Matt Young (m.young2@uqconnect.edu.au)

## Ant behaviour and game rules
- The user provides a PNG bitmap that specifies the locations of food (green), empty tiles (black) and ant
colonies (any other colour)
- Each ant colony starts with a fixed number of ants
/* - Each ant colony has a hunger timer that depletes by a fixed amount per game tick. In order to produce
more ants, the colony hunger meter must be full, in which case it can produce one ant every fixed number
of ticks. */
- Each ant individual has a unique hunger meter. Ants can go and get food, and come back to the colony.
Once they do this, they can replenish their hunger meter. If they are no longer hungry, they can produce
one more ant. If every ant from a colony dies, the colony is disqualified.
- When an ant walks, it leaves a pheromone trail. If an ant has food, the pheromone trail is stronger.
- Pheromone trails decay by a fixed amount per tick over time if they are not used.
- Ants follow the strongest pheromone trail to get places. If they are in a certain range of a food tile, they walk
to the food directly. If none of these conditions are available, the ants wander randomly.

## Implementation details
- Grid-based: ants can only be in one integer grid cell at a time
  - This is to ideally to improve performance and make it easier to vectorise
  - If you want a denser grid, use a higher resolution grid

## Building and running
- Compile with Clang 12 or later, CMake and Ninja or Unix Makefiles
- GCC may work as well, but you may have to change some stuff related to profiling

## Attribution
The following open source libraries are used:

- [lodepng](https://github.com/lvandeve/lodepng): zlib licence

## Licence
Currently proprietary until the course is done.