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

TODO: compare parallelising ant update loop vs colony update loop

## CUDA
For CUDA we will parallelise the ant update loop, not the colony update loop. This is because 
that way we can use the 3D CUDA grid for (x, y, ant). Also, because vectors are not well supported
on the GPU.

```
Upload the SnapGrids (dirty and clean buffer) to the GPU

For each colony:
    Run CUDA kernel which will simulate each ant in the whole colony
    Run colony tasks (if we should kill the colony, etc)
    
Copy the SnapGrids back from the GPU
Run serial tasks
```

**Vector problem:** Basically the main problem is that in the ant update loop we normally insert
the colony pointer into the `colonyAddAnts` vector. CUDA does not support the STL, so means we 
cannot insert to this vector from the kernel. CUDA also does not support critical sections either.

This is resolved by making an array the size of the number of ants in the colony. Each ant will write
to the index in the array true/false depending on whether it thinks the colony should add more ants.
We then iterate over the array and see if there is at least on true and if so add more ants.

The bigger problem is we need to update the ant's visited positions set. We can't use a set in CUDA.
We would have to replace this with a bool SnapGrid, which would be a lot slower to look through. It's
the only option really though.

## MPI
If CUDA is too hard, we can always do MPI (if that's even any easier lmao fml).

We could either:

- Scatter each colony to the workers, and run the ant update loop using OpenMP
- Scatter each ant to the workers, then gather them (no OpenMP)

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