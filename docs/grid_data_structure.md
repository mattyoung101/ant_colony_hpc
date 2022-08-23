# Grid format
## Current design
- Massive pointer mess
- Array of width, height, and up to MAX_ENTITIES_CELL (currently 4), where each cell contains a reference
to a Tile subclass
- We would loop over the grid, check each Tile class and figure out what subclass
it actually is (polymorphism), and update accordingly.
- TODO how we handle nests and nest lookups

**Problem:** C++ has no easy and fast "instanceof" operator - you would have to go through
dynamic_cast which is very slow, https://stackoverflow.com/a/500495/5007892

**Fixes**

(0) This may not actually be a problem, if we just call `Tile->update()`, it will delegate to the subclass
anyway. (although virtual functions are still not the fastest)

(1) Simply cop the dynamic cast, it may not be that expensive especially on -O3

(2) Skip virtual methods and dynamic casts by having something like this: 
```c++
struct Tile {
    TileType_t type; // = TILE_ANT
    Ant ant; // = data in here
    Food food; // = nothing
    Pheromone pheromone; // = nothing
}; 
```
Then we would look up which type it is and do a `static_cast`

(3) Maybe this? https://en.wikipedia.org/wiki/Curiously_recurring_template_pattern Or other template stuff

(4) Rewrite the grid data structure (see below)

## Second design
- Grid data structure that _only_ contains empty tiles, pheromones, and food
  - Basically a single Tile class with an enum for if its Empty, Pheromone or Food
- List of ColonyRecords to keep track of colony statistics and which ants belong to which colony
- Ants are still integer, but are stored in an array in each ColonyRecord
- **This method seems the best**

## Second design, v2
- Same as above, except that we divide the world into "layers": pheromone, food and colony tiles
- **Since colonies are being a pain, we could always remove them from the game and just keep them around
to write to the PNG TAR as visualisation**
- Each layer is a 2D array of pointers to the thing in question

## Continuous design
- Colonies are circles
- Ants can move around continuously
- Ants leave pheromone circles when they move (would have to be inserted into some kind of list)
- Think this is a bad idea