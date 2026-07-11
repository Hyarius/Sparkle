# Scale prefab stamping with populated chunks

- **Status:** Ready
- **Priority:** P1 — High
- Area: Voxel prefab application / Performance / Exception safety

## Problem

Prefab is represented sparsely, but VoxelMap::applyPrefab treats its bounding box as dense. It loads every chunk between the rotated minimum and maximum bounds, even when no prefab voxel lands in most of those chunks. It then scans the complete prefab once for every chunk and asks Prefab::applyTo to discard out-of-bounds voxels.

This turns a sparse operation into work proportional to bounding-box volume multiplied by voxel count. It also contradicts the useful sparse semantics described by Prefab. If a later chunk generator throws, earlier chunks may already have been modified even though the API comment says the prefab always lands whole.

## Evidence

- includes/structures/voxel/spk_prefab.hpp:10 — Prefab documents that only listed voxels are applied.
- includes/structures/voxel/spk_voxel_map.hpp:59 — map stamping promises all overlapped chunks are generated so the prefab lands whole.
- srcs/structures/voxel/spk_voxel_map.cpp:229 — chunk coverage is derived from the full rotated bounding box.
- srcs/structures/voxel/spk_voxel_map.cpp:232 — inclusive nested loops visit every chunk in that box.
- srcs/structures/voxel/spk_voxel_map.cpp:238 — every visited chunk is generated on demand.
- srcs/structures/voxel/spk_voxel_map.cpp:240 — the entire prefab is rescanned once per visited chunk.
- srcs/structures/voxel/spk_prefab.cpp:170 — applyTo loops over all stored voxels and skips those outside the target grid.

## Failure scenario

Create a prefab containing two voxels one million cells apart. Stamping it along one axis attempts to generate roughly 62,501 chunks between those voxels and scans both voxel entries for every chunk, even though only two chunks contain authored cells. If generation fails near the end, the first part of the structure remains stamped.

## Proposed direction

Transform each listed voxel once, determine its target chunk, and bucket operations by touched chunk while preserving insertion order within each bucket. Generate or preflight only those chunks containing listed voxels. Separate preparation from cell mutation so a generation failure cannot leave a partially applied cell set; roll back newly created chunks if the documented transaction requires that as well.

Batch renderer and neighbor invalidation once per affected chunk rather than once per voxel.

## Acceptance criteria

- [ ] Runtime work and generated chunk count scale with listed voxels and actually touched chunks, not bounding-box volume.
- [ ] Prefab voxel insertion order is preserved when multiple entries target the same cell.
- [ ] Explicit empty cells still carve targets and unlisted positions remain untouched.
- [ ] Rotation and pivot semantics remain unchanged for positive and negative coordinates.
- [ ] A preparation or generation exception leaves no partially stamped cell changes.
- [ ] Each affected chunk and boundary neighbor is invalidated no more often than necessary.

## Tests

- [ ] Add a two-voxel, very-wide sparse prefab test and assert only the two touched chunks load.
- [ ] Add multi-chunk tests for all four orientations and a pivot outside the prefab bounds.
- [ ] Add duplicate-position tests proving insertion-order behavior is unchanged.
- [ ] Inject a generator failure after some target chunks are prepared and assert cell-level atomicity.
- [ ] Add a regression benchmark comparing sparse stamping against the old bounding-box complexity.

## Dependencies / notes

Coordinate arithmetic hardening should be applied to transformed positions and chunk bucketing. The exact guarantee for newly generated but otherwise untouched chunks must be documented; the minimum requirement is that prefab cell edits are not partially committed.
