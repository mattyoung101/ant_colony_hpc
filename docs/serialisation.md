# Serialisation format
Data is serialised into a format known as "PNG TAR", it's simply a TAR file containing a bunch of
PNGs named like 0.png, 1.png, up to _n_.png, which contains a visual representation of the grid
state at each step.

TODO implementation for disk writing (in a separate queue, or whatever)

TODO how do we copy grid data efficiently (this is slow)

TODO PNG serialisation is slow, how do we do it in a thread?


1. Store buffer in queue as raw pixels (fast but memory intensive)
2. Later, thread comes in and compresses pixels to PNG and writes to disk

Further optimisation would be:

1. Store C++ World object in memory (may be faster than iterating over pixels, also may not be though)
2. Send this off in a queue to a thread
3. The thread comes in and compresses pixels to PNG

TODO: should we do this thread stuff now, or do it later?