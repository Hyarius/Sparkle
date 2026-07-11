# Document and enforce borrowed voxel lifetimes

- **Status:** Ready
- **Priority:** P2 — Medium
- Area: Engine / Voxel ownership and lifecycle

## Problem

Several voxel types retain raw pointers or references to externally owned objects without expressing or enforcing the required destruction order. The most dangerous example is `VoxelMap`: its destructor calls `clear()`, and `clear()` dereferences the stored `GameEngine*`. Destroying the engine before the map therefore turns otherwise ordinary teardown into undefined behavior.

The same implicit contract applies to registries, grids, lookup providers, maps, textures, cameras, worker pools, and profilers. The APIs do not consistently state which object must outlive which, whether a dependency may be detached, or whether moving the referenced object is supported.

## Evidence

- `includes/structures/voxel/spk_voxel_map.hpp:25-29` stores borrowed `VoxelRegistry*` and `GameEngine*` values.
- `srcs/structures/voxel/spk_voxel_map.cpp:24-27` captures those addresses without an ownership wrapper.
- `srcs/structures/voxel/spk_voxel_map.cpp:35-37` calls `clear()` from the destructor.
- `srcs/structures/voxel/spk_voxel_map.cpp:133-143` dereferences `_engine` while clearing loaded chunks.
- `includes/structures/voxel/spk_voxel_chunk_renderer.hpp:22-24` retains pointers to a grid, registry, and optional world lookup.
- `includes/structures/voxel/spk_voxel_chunk_streamer.hpp:18-24` retains a raw map pointer.
- `includes/structures/voxel/spk_voxel_chunk_render_logic.hpp:38-55` and `includes/structures/voxel/spk_voxel_chunk_transparent_render_logic.hpp:34-36` retain additional borrowed rendering dependencies.

## Failure scenario

Create a `VoxelMap` with an engine, generate at least one chunk, then destroy the engine before the map. When the map is later destroyed, `VoxelMap::clear()` calls `removeEntity()` through the dead engine pointer.

Equivalent dangling-reference failures are possible when a registry or map is destroyed or moved while its chunks, renderers, or streamers still exist.

## Proposed direction

Define an ownership model for every retained dependency and encode it where practical:

- document required outliving relationships in constructors and class comments;
- add explicit detach/shutdown operations where teardown order cannot be structurally guaranteed;
- prefer ownership-aware handles or engine-managed registration tokens over unvalidated raw pointers for callbacks during destruction;
- prohibit moves that invalidate retained addresses, or update dependants safely;
- add debug assertions or lifetime tokens for contracts that must remain borrowed.

`VoxelMap` should be safe to destroy even after its engine has begun shutdown; normal destructor correctness should not depend solely on caller convention.

## Acceptance criteria

- [ ] Every voxel constructor that stores a dependency documents ownership, nullability, thread expectations, and required lifetime.
- [ ] Destroying or explicitly detaching a map after engine shutdown does not dereference the old engine.
- [ ] Registry, grid, lookup, map, texture, and camera movement/destruction behavior is either prevented or defined.
- [ ] Streamer and render-logic teardown has a safe, documented order.
- [ ] Debug builds diagnose contract violations before a dangling dereference where compile-time ownership cannot enforce them.

## Tests

- [ ] Add a teardown test in which a populated map outlives the engine registration context.
- [ ] Add tests for the supported map-detach or shutdown sequence.
- [ ] Add compile-time or runtime tests for prohibited registry/map moves and expired dependencies.
- [ ] Run tests under AddressSanitizer or the project's closest available lifetime sanitizer configuration.

## Dependencies / notes

Ticket 001 contains a concrete exceptional-construction path that violates the current map/engine ordering. Ticket 021 covers the mandatory renderer pointer specifically. Coordinate the API shape, but keep each regression test focused.
