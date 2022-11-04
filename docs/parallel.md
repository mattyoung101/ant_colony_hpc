# Parallelising ant updates
## OpenMP
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

**TODO:** compare parallelising ant update loop vs colony update loop

## MPI
Acts as a replacement for OpenMP. We will parallelise the ant update loop only (not the whole colony
loop). Here's what we'll do:

```
// main.cpp
Master/Worker: Run init code (it's deterministic, so should be OK). All workers now have same state.
Master/Worker: Perform code split (change our behaviour based on if we are master or worker).

// World::update
Master (Rank 0): Broadcast the clean buffer for each SnapGrid to all workers.
Worker (Rank N): Receive the clean buffer and copy into the diry buffer (reduces bandwidth required).
Master (Rank 0): Scatter colonies to workers (each worker gets 1 colony)

For each colony:
    Worker (Rank N) including master: 
        #if OMP #pragma omp parallel for #endif
        For each ant we have to process:
            Process the ant.
    Master (Rank 0): Gather all colonies from workers. <-- WE CAN'T DO THIS!
    it would be better to have each snapgrid record tiles we wrote to, then send them back & have master merge them.
    
Serial code (update SnapGrids, colony work, etc).
```

We really want to avoid having to sent around the colony list, if we can avoid that in any way
it would be really good.

One important note from the above code is that MPI and OpenMP _can_ be complementary.

## Evaluation: CUDA vs MPI
**Reasons to use CUDA:**

- It might be faster than MPI
- It might be easier to use than MPI

**Reasons not to use CUDA:**

- It would require rewriting a large amount of the code (move away from STL)
- Limited GPU VRAM greatly restricts how big our maps can be, therefore not easy to compare against
OpenMP
- It might be harder to debug

**Reasons to use MPI:**

- Doesn't require rewriting as much code
- More commonly used in HPC(?)
- Might be easier to debug (we can still use ASan, etc)
- Can test locally (since it doesn't require a GPU)
- CLion will support it better

**Reasons not to use MPI:**

- It also could be harder to debug
- It might be slower than CUDA