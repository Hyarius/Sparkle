# Use a monotonic world revision for navigation invalidation

- **Status:** Ready
- **Priority:** P2 — Medium
- Area: Playground / World navigation invalidation

## Problem

`VoxelWorld::revision()` is not a revision counter. It adds an edit counter to the current number of loaded chunks. Different world states can produce the same sum, so `WorldNavigation` can incorrectly reuse a graph built for different terrain.

Chunk loading, unloading, and clearing do not increment an explicit generation. Mutable access through `VoxelWorld::map()` can also change cells or chunk state without touching `_editRevision`. Conversely, `VoxelWorld::setCell()` increments the counter whenever the map reports a loaded destination, even when the cell value did not change, causing unnecessary rebuilds.

## Evidence

- `playground/srcs/world/voxel_world.hpp:28-31` stores `_editRevision` separately from the underlying map.
- `playground/srcs/world/voxel_world.hpp:42-43` exposes mutable map access.
- `playground/srcs/world/voxel_world.cpp:51-58` increments only through the wrapper's `setCell()` path.
- `playground/srcs/world/voxel_world.cpp:61-74` loads, unloads, and clears without incrementing `_editRevision`.
- `playground/srcs/world/voxel_world.cpp:81-85` returns `_editRevision + loadedChunkCount()`.
- `playground/srcs/world/world_navigation.cpp:24-29` skips rebuilding solely when that scalar compares equal.

## Failure scenario

1. Load chunk A and build the navigation graph.
2. Unload A and load a different chunk B, leaving the loaded count unchanged.
3. Do not make a wrapper-level edit.
4. `revision()` has the old value, so `WorldNavigation::refresh()` retains nodes for A and has none for B.

An edit followed by an unload can produce a similar arithmetic collision because one side of the sum rises while the other falls.

## Proposed direction

Replace the sum with one monotonic world-generation counter. Increment it exactly once for every successful change that can alter navigation input:

- actual cell value changes;
- first-time chunk load/generation;
- successful unload;
- non-empty clear;
- provider or traversal-definition changes that affect existing cells.

Route mutations through revision-aware APIs or give `VoxelMap` a change notification that `VoxelWorld` subscribes to. Remove or constrain the mutable map escape hatch. No-op writes should not increment the generation.

## Acceptance criteria

- [ ] Two distinct observable world mutations cannot intentionally produce the same consecutive revision value.
- [ ] Replacing one loaded chunk with another invalidates navigation even when chunk count is unchanged.
- [ ] Successful load, unload, clear, and actual cell changes invalidate the graph.
- [ ] Writing the same cell value is a no-op and does not force a rebuild.
- [ ] Direct mutable map operations cannot bypass invalidation.
- [ ] Counter type and wraparound policy are documented.

## Tests

- [ ] Build a graph, unload one chunk, load another, and assert a refresh occurs.
- [ ] Cover the edit-plus-unload arithmetic-collision sequence.
- [ ] Cover clear followed by reload to the previous chunk count.
- [ ] Verify that a no-op `setCell()` leaves the revision unchanged.
- [ ] Verify every supported mutation entry point updates revision consistently.

## Dependencies / notes

Current Playground streaming retains inactive chunks by default, so routine play generally does not exercise unload/reload replacement. This lowers current urgency but does not make the public API correct. Ticket 030 may add more world-edit paths; they must participate in the same invalidation mechanism when traversal semantics change.
