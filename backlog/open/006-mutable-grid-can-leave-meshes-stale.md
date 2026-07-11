# Close mutation paths that bypass complete chunk mesh invalidation

- **Status:** Ready
- **Priority:** P1 — Medium/high, API correctness and concurrency
- **Area:** Engine / voxel chunk mutation / mesh invalidation

## Problem

`VoxelChunk::grid()` marks the chunk renderer dirty once and returns a persistent mutable `VoxelGrid&`. A caller can retain that reference, allow a subsequent bake to consume the dirty state, and then mutate the grid again without calling the accessor. The later write has no way to request another bake, leaving the rendered mesh stale. A retained `VoxelCell&` has the same problem and can also race a background mesh read if used concurrently.

`VoxelChunk::setCell()` requests synchronization for its own chunk, but does not invalidate a neighboring chunk when the edited cell lies on a chunk boundary. `VoxelMap::setCell()` contains that cross-chunk invalidation, so callers that consistently mutate through the map are safe from this particular neighbor defect. The public chunk/grid API does not enforce that path.

Current Playground providers mostly use mutable chunk grids while initially generating a chunk, before its first bake. This reduces exposure in the existing happy path, but does not make the public API invariant safe for runtime edits or engine users.

## Evidence

- Mutable `grid()` dirties once, then returns an unrestricted reference: [`srcs/structures/voxel/spk_voxel_chunk.cpp:50`](../../srcs/structures/voxel/spk_voxel_chunk.cpp#L50), [`includes/structures/voxel/spk_voxel_chunk.hpp:35`](../../includes/structures/voxel/spk_voxel_chunk.hpp#L35).
- Direct chunk mutation only requests synchronization for its own renderer: [`srcs/structures/voxel/spk_voxel_chunk.cpp:66`](../../srcs/structures/voxel/spk_voxel_chunk.cpp#L66).
- Map mutation detects boundary cells and invalidates loaded neighbors: [`srcs/structures/voxel/spk_voxel_map.cpp:183`](../../srcs/structures/voxel/spk_voxel_map.cpp#L183), [`srcs/structures/voxel/spk_voxel_map.cpp:197`](../../srcs/structures/voxel/spk_voxel_map.cpp#L197).

## Failure scenario

1. Store `VoxelGrid& grid = chunk.grid()`, which marks the renderer dirty.
2. Let the normal render logic bake and publish the chunk, clearing/consuming the pending synchronization state.
3. Modify `grid.cell(position)` through the retained reference.
4. No method is called that can dirty the renderer, so the visual mesh keeps the old cell indefinitely.

Alternatively, call `chunk.setCell()` on a boundary position: the current chunk rebakes, while the adjacent chunk can keep a face culled against the old cell.

## Proposed direction

Establish one authoritative mutation mechanism that owns all required invalidation. Options include removing long-lived mutable grid access, exposing a scoped edit transaction that dirties on completion, providing mutation callbacks/proxies, or routing chunk edits through the owning map. Initial generation may use a distinct unpublished construction API if bulk writes need to avoid repeated invalidation.

Document thread ownership for voxel writes and mesh reads. Do not attempt to solve this only by dirtying on `grid()` access; mutation can occur arbitrarily later.

## Acceptance criteria

- [ ] Every supported runtime cell mutation eventually requests a fresh mesh for the edited chunk.
- [ ] Boundary mutations also invalidate every affected loaded neighbor.
- [ ] Retaining an object returned by a public API cannot later mutate cells invisibly, or such mutation is explicitly impossible by type/lifetime.
- [ ] Bulk chunk generation remains practical and does not require one expensive synchronization operation per cell.
- [ ] The allowed thread/context for voxel mutation is documented and enforced where feasible.
- [ ] Existing map-based runtime edits retain their current behavior.

## Tests

- [ ] Reproduce mutation through a retained grid/cell alias after a bake and verify a new mesh is published.
- [ ] Edit each of the six chunk boundaries through every supported mutation API and verify both chunks rebake.
- [ ] Add a non-boundary control proving unrelated neighbors are not invalidated.
- [ ] Cover no-op writes so they do not trigger unnecessary work unless intentionally specified.
- [ ] Add a bulk-generation test for the chosen construction/edit path.

## Dependencies / notes

This ticket should distinguish runtime editing from unpublished chunk generation. It does not claim the existing Playground generator path currently exhibits stale meshes; the defect is the unrestricted public mutation contract and direct chunk-edit path.
