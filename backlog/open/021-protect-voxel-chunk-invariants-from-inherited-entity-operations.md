# Protect voxel chunk invariants from inherited entity operations

- **Status:** Ready
- **Priority:** P2 — Medium
- Area: Engine / Voxel chunk lifecycle

## Problem

`spk::VoxelChunk` inherits the general-purpose `Entity3D` interface, but it relies on invariants that the inherited interface does not preserve. Its constructor installs a mandatory `VoxelChunkRenderer` component and stores a raw pointer to it. Callers can remove that component through the inherited component API; subsequent calls to `grid()`, `setCell()`, or `renderer()` dereference the dangling pointer.

The inherited mutable transform creates a second split invariant. Chunk coordinates and the renderer's world lookup origin remain fixed, while callers can move or rotate the visual entity. Rendering, neighbor-face culling, and world-cell lookup can then disagree about where the chunk exists.

## Evidence

- `includes/structures/voxel/spk_voxel_chunk.hpp:12` derives `VoxelChunk` publicly from `Entity3D`.
- `includes/structures/voxel/spk_voxel_chunk.hpp:18-20` stores immutable-looking coordinates alongside a raw `_renderer` pointer.
- `srcs/structures/voxel/spk_voxel_chunk.cpp:23-24` initializes the entity transform and renderer origin from `worldOrigin()` only during construction.
- `srcs/structures/voxel/spk_voxel_chunk.cpp:50-53`, `66-74`, and `77-84` dereference `_renderer` without verifying that the component still exists.
- `includes/structures/game_engine/spk_entity.hpp:78-88` exposes component removal to all derived entities.
- `includes/structures/game_engine/spk_transform_3d.hpp:39-41` exposes position and rotation mutation.

## Failure scenario

1. Create a `VoxelChunk` and obtain its renderer component.
2. Remove that component through `Entity::removeComponent`.
3. Call `chunk.setCell(...)` or `chunk.renderer()`.
4. The chunk dereferences a pointer to the removed component, producing undefined behavior.

Alternatively, move the chunk transform after construction. Its mesh is drawn at the new transform while cross-chunk lookup and culling still use the original coordinate-derived origin, producing missing or extra boundary faces.

## Proposed direction

Make the renderer and spatial relationship explicit invariants instead of conventions:

- prevent removal of the mandatory renderer, or own it through a mechanism whose lifetime cannot be invalidated by the generic component API;
- avoid caching an unvalidated raw component pointer;
- decide whether chunks are immovable world objects or movable entities;
- if immovable, reject or hide transform mutation for chunks; if movable, update coordinates, renderer origin, map indexing, and neighbor synchronization atomically;
- retain support for optional user-added components.

## Acceptance criteria

- [ ] Removing a chunk's mandatory renderer through the public entity API is impossible or fails safely without invalidating the chunk.
- [ ] `grid()`, `setCell()`, and `renderer()` cannot dereference a removed component.
- [ ] The transform, `coordinates()`, `worldOrigin()`, renderer origin, and map key cannot disagree after any supported operation.
- [ ] The chosen movable/immovable contract is documented in the public header.
- [ ] Optional components can still be added and removed normally.

## Tests

- [ ] Add a test that attempts to remove the mandatory renderer and verifies the supported failure behavior.
- [ ] Add a test covering position and rotation mutation according to the chosen contract.
- [ ] Add a cross-chunk culling test proving that visual placement and world lookup remain aligned.
- [ ] Run the full voxel and entity test suites.

## Dependencies / notes

This overlaps with the broader borrowed-lifetime work in ticket 022, but it can be fixed independently. A narrowly scoped first fix may make the mandatory component non-removable and make chunk transforms immutable.
