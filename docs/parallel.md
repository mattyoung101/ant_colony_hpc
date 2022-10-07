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