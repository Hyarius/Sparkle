# Make two-pass voxel mesh emission safe against lookup instability

- **Status:** Ready
- **Priority:** P0 — High, memory safety; conditional trigger
- **Area:** Engine / voxel mesher / extension contracts

## Problem

The mesher first counts the exact number of vertices and indices, allocates fixed-size builder spans, and then performs a second traversal to emit geometry. Neighbor occlusion is queried during both passes. The code assumes that `IVoxelCellLookup` returns an identical effective world snapshot in both traversals, but that requirement is neither enforced nor documented by the interface.

If a custom or stateful lookup changes between the count and emission passes, the second pass can emit more geometry than was counted. Partial transparent occlusion is an important trigger because one original polygon can be split into two polygons. Emission writes through unchecked `std::span::operator[]`; the count mismatch is detected only after the out-of-bounds writes have already occurred.

Normal `VoxelMap` baking currently keeps the lookup stable, so this is a conditional public-API defect rather than evidence that every ordinary bake corrupts memory. It is still a memory-safety hole for a valid `IVoxelCellLookup` implementation and for future concurrent/stateful uses.

## Evidence

- The count pass computes visibility and exact buffer sizes: [`srcs/structures/voxel/spk_voxel_mesher.cpp:595`](../../srcs/structures/voxel/spk_voxel_mesher.cpp#L595), [`srcs/structures/voxel/spk_voxel_mesher.cpp:603`](../../srcs/structures/voxel/spk_voxel_mesher.cpp#L603).
- Builder storage is resized to those exact counts: [`srcs/structures/voxel/spk_voxel_mesher.cpp:628`](../../srcs/structures/voxel/spk_voxel_mesher.cpp#L628).
- The emission pass queries visibility again and writes without cursor bounds checks: [`srcs/structures/voxel/spk_voxel_mesher.cpp:641`](../../srcs/structures/voxel/spk_voxel_mesher.cpp#L641), [`srcs/structures/voxel/spk_voxel_mesher.cpp:647`](../../srcs/structures/voxel/spk_voxel_mesher.cpp#L647).
- Cursor/count disagreement is checked only after emission: [`srcs/structures/voxel/spk_voxel_mesher.cpp:662`](../../srcs/structures/voxel/spk_voxel_mesher.cpp#L662).
- The public overload accepts an arbitrary `IVoxelCellLookup`: [`srcs/structures/voxel/spk_voxel_mesher.cpp:580`](../../srcs/structures/voxel/spk_voxel_mesher.cpp#L580).

## Failure scenario

1. Supply an `IVoxelCellLookup` that reports an empty neighbor during the count pass.
2. During emission, have the same lookup report a same-alpha transparent neighbor covering the middle Y interval of a boundary polygon.
3. The count pass reserves one polygon, while emission clips it into two polygons.
4. Vertex/index cursors exceed their spans and overwrite adjacent memory.
5. The final `logic_error` check runs too late to prevent corruption.

## Proposed direction

Remove the unchecked dependency on two identical external observations. Possible solutions include snapshotting all cells/occlusion results used by the first pass, retaining the fully clipped polygon plan for emission, emitting into growth-capable temporary buffers, or adding pre-write checks with a safe recount/retry strategy. Merely documenting stability is insufficient while an accidental violation can corrupt memory.

Whichever design is selected should preserve useful overflow checks and avoid silently accepting a lookup that changes geometry mid-build.

## Acceptance criteria

- [ ] No lookup behavior can cause a write past a vertex or index buffer.
- [ ] A changed lookup either produces a valid mesh from one coherent snapshot or fails before any out-of-bounds write.
- [ ] The stability/snapshot semantics of `IVoxelCellLookup` during a build are documented.
- [ ] Count overflow and 32-bit index-limit errors remain well-defined.
- [ ] Stable built-in `VoxelMap` output remains byte-for-byte or geometrically equivalent unless an intentional meshing change is documented.

## Tests

- [ ] Add a custom lookup that changes from empty to partially occluding between observations.
- [ ] Cover a case where one counted polygon becomes two emitted polygons.
- [ ] Cover the inverse case, where fewer polygons are emitted than counted.
- [ ] Exercise both opaque and transparent output buffers.
- [ ] Run the regression under AddressSanitizer or an equivalent bounds-checking configuration.

## Dependencies / notes

This issue is related to, but distinct from, the geometric correctness work in ticket 004. Fixing transparent clipping may alter the exact reproducer, but the mesher must remain memory-safe for any count/emission disagreement.
