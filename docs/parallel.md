# Parallelising ant updates

Observation: Each ant update is independent of every other ant. That means we should be able to
update each ant in the world independently, i.e. in a separate thread block. Here's how it will work:

1. "Lock" the world: When each ant is updated, it sees the world as it was when the tick began. The
world does not change during ant update, only at the end. We can also implement this in the serial
version.
2. Update ants: Each ant is updated in a thread block, storing its requested changes to the world in some type of
data structure.
3. "Unlock" the world: In a serial loop, all the requested changes made by each ant are merged
together, and the world is updated.

## CoW block/snapshot grid idea
Inspired by the back buffer used in programming. What this allows us to do, I think, is save one
large memcpy per tick. It allows us to "lock" and "unlock" the grid as described above. Here's how
it works.

There are two arrays: dirty and clean.

1. Dirty and clean start off holding the exact same contents: the 2D grid data structure
2. When `CowGrid::read(x,y)` is called, data from the _clean_ array is returned
3. When `CowGrid::write(x,y,value)` is called, the data is written into the _dirty_ array
4. When `CowGrid::commit()` is called (at the end of the tick), the dirty grid is memcpyd to the
clean grid, and we can start over again.

Looking at this I'm not actually sure it's as smart as it sounded in the shower :( I don't think we
actually save any memcpys. Maybe we could avoid that last memcpy, it would be nice, but I'm not sure
how.

Implementation details:
- Probably should template this class