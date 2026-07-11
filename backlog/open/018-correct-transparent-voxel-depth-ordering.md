# Correct transparent voxel depth ordering

- **Status:** Ready
- **Priority:** P2 — Medium
- Area: Voxel transparent rendering / Visual correctness

## Problem

Transparent rendering sorts entire chunks by the distance from the camera to each chunk center. Geometry inside a chunk remains in fixed mesher emission order, and interpenetrating or overlapping chunks are represented by a single center-distance key.

Standard alpha blending with depth writes disabled requires back-to-front ordering at a finer granularity. Multiple water layers or translucent custom shapes in one chunk can therefore blend differently from different viewpoints, hide farther surfaces, or produce popping as chunk-center order changes.

## Evidence

- srcs/structures/voxel/spk_voxel_chunk_transparent_render_logic.cpp:93 — camera position is sampled once for chunk sorting.
- srcs/structures/voxel/spk_voxel_chunk_transparent_render_logic.cpp:94 — the sort operates on the renderer/chunk list only.
- srcs/structures/voxel/spk_voxel_chunk_transparent_render_logic.cpp:96 — each chunk is represented by one transformed center.
- srcs/structures/voxel/spk_voxel_chunk_transparent_render_logic.cpp:99 — back-to-front order is based solely on squared center distance.
- srcs/structures/voxel/spk_voxel_chunk_transparent_render_logic.cpp:102 — each complete transparent mesh is then submitted as one draw.
- srcs/structures/voxel/spk_voxel_mesher.cpp:641 — transparent polygons are emitted in grid traversal and shape-face order, not camera depth order.

## Failure scenario

Place two translucent surfaces at different depths inside the same chunk and move the camera from one side to the other. Submission order does not change because both surfaces are in one mesh. Their blended result is correct from at most one direction. A similar problem occurs when long transparent geometry in two chunks overlaps despite their centers sorting in the opposite order from the visible surfaces.

## Proposed direction

Choose and document a transparency strategy that is correct for supported content. Options include sorting transparent draw ranges or faces by camera depth, splitting meshes into sortable spatial sections, using a material-specific solution for connected fluids, or adopting an order-independent transparency technique. Keep opaque batching unchanged and add deterministic tie-breaking to avoid flicker.

## Acceptance criteria

- [ ] Supported transparent geometry is ordered at a granularity finer than one whole chunk, or an order-independent method is used.
- [ ] Reversing the camera across two transparent layers produces the correct reversed compositing order.
- [ ] Transparent geometry spanning or overlapping chunk boundaries does not rely only on chunk-center distance.
- [ ] Equal-depth ordering is deterministic and does not flicker frame to frame.
- [ ] Opaque mesh batching and depth-writing behavior are unaffected.
- [ ] The performance cost and any remaining unsupported transparency cases are documented.

## Tests

- [ ] Add a render-order test with two transparent layers in one chunk and cameras on opposite sides.
- [ ] Add a cross-chunk overlap test where center order differs from surface order.
- [ ] Add a deterministic tie test.
- [ ] Add a visual regression scene with stacked water and translucent custom shapes.
- [ ] Add a performance benchmark representative of the chosen sorting granularity.

## Dependencies / notes

This ticket addresses draw ordering, not the separate mesher occlusion errors for partially overlapping transparent faces. Those geometry-generation fixes should be tested independently.
