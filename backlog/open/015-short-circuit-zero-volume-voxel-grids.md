# Short-circuit zero-volume voxel grids

- **Status:** Ready
- **Priority:** P2 — Medium
- Area: Voxel grid / Meshing / Performance

## Problem

VoxelGrid permits any non-negative dimensions. If one axis is zero, cellCount is zero regardless of the other two dimensions, so no storage is allocated. Algorithms nevertheless loop over the stored dimensions rather than recognizing zero volume.

A grid such as {0, INT_MAX, INT_MAX} is logically empty and allocates no cells, but meshing and prefab conversion can execute an astronomical number of outer-loop iterations while the innermost X loop does no work. This creates a low-memory denial-of-service and surprising performance behavior for a validly constructed grid.

## Evidence

- includes/structures/voxel/spk_voxel_grid.hpp:31 — cell-count validation accepts zero on any axis.
- includes/structures/voxel/spk_voxel_grid.hpp:50 — multiplying by a zero axis produces zero cells.
- srcs/structures/voxel/spk_voxel_mesher.cpp:403 — visibility storage is empty for a zero-volume grid.
- srcs/structures/voxel/spk_voxel_mesher.cpp:406 — the first meshing pass still iterates Y and Z dimensions.
- srcs/structures/voxel/spk_voxel_mesher.cpp:488 — the masked emission pass repeats those dimension loops.
- srcs/structures/voxel/spk_prefab.cpp:60 — conversion from VoxelGrid also iterates stored dimensions rather than empty cell storage.

## Failure scenario

Construct VoxelGrid({0, INT_MAX, INT_MAX}) and call VoxelMesher::buildRenderMesh or construct a Prefab from it. Both operations can spend effectively unbounded time iterating empty planes despite cells().empty() being true.

## Proposed direction

Treat zero-volume grids consistently as empty values. Add fast exits to algorithms whose work is based on cells, and centralize an empty()/volume() query so future algorithms do not reproduce dimension-only loops. If partial-zero dimensions are not a supported semantic, reject them consistently at construction instead; do not leave them accepted but pathological.

## Acceptance criteria

- [ ] The engine explicitly documents whether partial-zero dimensions are valid empty grids or invalid sizes.
- [ ] Every accepted zero-volume dimension permutation completes mesh building and prefab conversion in constant time.
- [ ] Empty mesh results contain no vertices, indexes, or shapes.
- [ ] Empty prefab conversion contains no voxels and has the documented zero bounds.
- [ ] Non-empty grid iteration order and output remain unchanged.

## Tests

- [ ] Add tests for {0, 0, 0}, {0, N, N}, {N, 0, N}, and {N, N, 0}.
- [ ] Use very large nonzero companion axes to prove the tests do not scale with those dimensions.
- [ ] Verify both mesher passes and Prefab construction return empty results.
- [ ] Add a normal non-empty regression test for traversal order.

## Dependencies / notes

This is independent of allocation-overflow validation: the current cell-count multiplication checks are useful and should remain. Prefer a shared semantic decision over one-off early returns.
