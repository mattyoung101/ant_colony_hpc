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

## Secondary grid size