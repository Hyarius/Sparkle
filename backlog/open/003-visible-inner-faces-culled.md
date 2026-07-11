# Preserve visible slope and stair inner faces when shell faces are culled

- **Status:** Ready
- **Priority:** P1 — High, rendering correctness
- **Area:** Engine / voxel mesher / slope and stair shapes

## Problem

The visibility pass emits a shape's `innerFaces` only when at least one registered outer-shell face was emitted, unless the shape has no outer faces at all. This treats “no visible registered shell polygon” as equivalent to “the cell is completely enclosed.” Those conditions are not equivalent for mixed shapes whose important visible surfaces are inner faces and whose open directions have no outer face entry.

Slope inclines and stair treads/risers are inner faces. Their back, bottom, and side geometry is registered as outer-shell faces, while their open top/front directions are not. If cubes occlude all registered shell faces, `hasVisibleShell` stays false and the mesher removes the incline or treads even though they remain visible through the open directions.

## Evidence

- Inner faces are gated by `hasVisibleShell || !shape.hasOuterFaces()`: [`srcs/structures/voxel/spk_voxel_mesher.cpp:428`](../../srcs/structures/voxel/spk_voxel_mesher.cpp#L428), [`srcs/structures/voxel/spk_voxel_mesher.cpp:458`](../../srcs/structures/voxel/spk_voxel_mesher.cpp#L458).
- The slope incline is an inner face, while back, bottom, and sides are outer faces: [`srcs/structures/voxel/spk_slope_voxel_shape.cpp:38`](../../srcs/structures/voxel/spk_slope_voxel_shape.cpp#L38).
- Stair treads and risers are inner faces, with back, bottom, and sides in the outer shell: [`srcs/structures/voxel/spk_stair_voxel_shape.cpp:80`](../../srcs/structures/voxel/spk_stair_voxel_shape.cpp#L80), [`srcs/structures/voxel/spk_stair_voxel_shape.cpp:85`](../../srcs/structures/voxel/spk_stair_voxel_shape.cpp#L85).

## Failure scenario

1. Place a slope in a cell.
2. Place opaque cube neighbors against its registered back, bottom, left, and right outer faces.
3. Leave the incline/top direction open to the camera.
4. Every registered outer face is occluded, so `hasVisibleShell` remains false.
5. The incline inner face is omitted and the visible slope disappears. The equivalent arrangement removes stair treads and risers.

## Proposed direction

Define inner-face visibility independently from the accidental emission of a registered shell face. Options include treating an absent/open boundary plane as potential visibility, recording a shape-level interior visibility policy, or performing a conservative cell-enclosure test before suppressing inner geometry. The design may preserve the optimization that drops inner faces for a genuinely fully enclosed cell, but should not infer full enclosure solely from `hasVisibleShell`.

The resulting contract should cover custom mixed shapes, not just hard-code slope and stair exceptions.

## Acceptance criteria

- [ ] A slope incline remains rendered when its registered outer faces are occluded but an open direction exposes it.
- [ ] Stair treads and risers remain rendered in the equivalent arrangement.
- [ ] Genuinely fully enclosed inner geometry can still be culled if that optimization is retained.
- [ ] Shapes containing only inner faces retain their current behavior.
- [ ] Orientation and vertical flip do not change the correctness of the visibility decision.
- [ ] The visibility rule for custom shapes with both outer and inner faces is documented.

## Tests

- [ ] Add a regression scene/test for a slope with back, bottom, and both sides occluded.
- [ ] Add the corresponding stair test.
- [ ] Repeat for all four horizontal orientations.
- [ ] Cover positive- and negative-Y flips where supported.
- [ ] Add a control case proving a fully enclosed cell does not emit unintended geometry.

## Dependencies / notes

Ticket 004 changes face-occlusion accuracy and may share test fixtures, but this gate is a separate logic error even with perfect outer-face occlusion.
