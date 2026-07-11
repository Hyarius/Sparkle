# Make VoxelMap iteration and generation reentrancy-safe

- **Status:** Ready
- **Priority:** P1 — High
- Area: Voxel map lifecycle / API safety

## Problem

VoxelMap invokes unrestricted user code while its internal unordered map is in a structurally fragile state.

forEachLoadedChunk iterates the container directly and passes a live chunk reference to the callback. A callback can load, unload, or clear chunks, invalidating the active iterator or destroying the referenced chunk.

chunk inserts and exposes a new chunk before invoking the user-supplied generator. Recursive generation can observe that chunk only partially initialized. A generator that unloads or clears the map can destroy the local result and invalidate the saved iterator before chunk returns or performs rollback. Engine entity registration is also outside the try block, so an addEntity exception can leave the newly inserted chunk behind without generator completion.

## Evidence

- includes/structures/voxel/spk_voxel_map.hpp:71 — mutable loaded-chunk iteration calls arbitrary callbacks directly inside the container loop.
- includes/structures/voxel/spk_voxel_map.hpp:81 — the const overload has the same structural reentrancy issue.
- srcs/structures/voxel/spk_voxel_map.cpp:58 — comments account for rehashing, but not erasure or clear during generation.
- srcs/structures/voxel/spk_voxel_map.cpp:60 — the chunk is inserted before registration and generation complete.
- srcs/structures/voxel/spk_voxel_map.cpp:62 — engine registration occurs outside the rollback try block.
- srcs/structures/voxel/spk_voxel_map.cpp:66 — arbitrary generator code runs while result and emplaced are expected to remain valid.
- srcs/structures/voxel/spk_voxel_map.cpp:70 — rollback dereferences and erases state that reentrant code may already have removed.

## Failure scenario

Install a generator that calls clear(), unloadChunk() for its own coordinates, or recursively requests the same chunk. The outer chunk() call can use a destroyed reference, erase an invalid iterator, return a partially generated chunk, or double-remove its engine entity. Similarly, call unloadChunk() from a forEachLoadedChunk callback and invalidate the loop.

## Proposed direction

Define and enforce a reentrancy model instead of relying on unordered-map node stability.

For iteration, either provide a snapshot API that does not expose destructible live references or explicitly prohibit structural mutation with a runtime guard and documented exception. For generation, track an explicit Generating state, reject or safely handle same-coordinate recursion, and use a scope-bound transaction covering insertion, entity registration, generation, synchronization, and rollback. Destructive map operations must not invalidate an in-progress transaction.

## Acceptance criteria

- [ ] Structural map mutation during loaded-chunk enumeration has defined, documented behavior and cannot trigger undefined behavior.
- [ ] Requesting a chunk already being generated has deterministic behavior and cannot expose partially generated data as complete.
- [ ] A generator cannot erase or clear the chunk out from under the active chunk() call.
- [ ] Exceptions from both engine registration and the generator restore consistent map and engine state.
- [ ] Neighbor synchronization runs exactly once for the documented success or rollback transition.
- [ ] Normal recursive generation of different coordinates remains supported, if that is the chosen contract.

## Tests

- [ ] Add callback tests for unloading the current chunk, another chunk, loading a chunk, and clearing the map.
- [ ] Add generator tests for same-coordinate recursion and different-coordinate recursion.
- [ ] Add a generator test that unloads or clears during generation.
- [ ] Add exception-injection tests for addEntity and generator failures.
- [ ] Assert after every failure that loaded coordinates and registered engine entities agree.

## Dependencies / notes

Choosing a strict mutation guard is acceptable if it is explicit and tested; silently depending on caller discipline is not. This work should precede new cache eviction callbacks or other systems that mutate maps while enumerating them.
