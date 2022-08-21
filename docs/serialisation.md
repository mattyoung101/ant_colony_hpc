# Serialisation format
Data is serialised into a format known as "PNG TAR", it's simply a TAR file containing a bunch of
PNGs named like 0.png, 1.png, up to _n_.png, which contains a visual representation of the grid
state at each step.

TODO implementation for disk writing (in a separate queue, or whatever)

TODO how do we copy grid data efficiently (this is slow)

TODO PNG serialisation is slow, how do we do it in a thread?