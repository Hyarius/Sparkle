# Replace scalar partial-face occlusion with true boundary overlap

- **Status:** Ready
- **Priority:** P1 — High, rendering correctness
- **Area:** Engine / voxel shapes / transparent meshing

## Problem

Boundary-face occlusion is approximated using extrema, a single Y interval, or total scalar coverage rather than the actual two-dimensional overlap between opposing polygons.

For same-alpha transparent vertical faces, the neighbor is reduced to a Y interval. The current polygon is then clipped across its full horizontal extent, so a narrow or horizontally disjoint neighbor can erase geometry it never overlaps. For horizontal transparent faces, total boundary area is compared; equal-area but disjoint footprints can fully cull one another.

Opaque full-coverage detection accepts any boundary quad whose coordinate extrema reach 0 and 1 on both axes. A diamond touching the four edge midpoints therefore looks like full coverage despite covering only half the square. Multi-polygon coverage sums absolute polygon areas and clamps to one rather than computing their union, so overlaps can overstate coverage. Finally, exact transparency equality is used as a proxy for “same medium,” allowing unrelated materials with the same alpha to remove their interface, while tiny floating differences retain coincident faces.

## Evidence

- Transparent-neighbor compatibility uses exact transparency equality: [`srcs/structures/voxel/spk_voxel_mesher.cpp:170`](../../srcs/structures/voxel/spk_voxel_mesher.cpp#L170).
- Vertical transparent occlusion records only a Y interval: [`srcs/structures/voxel/spk_voxel_mesher.cpp:185`](../../srcs/structures/voxel/spk_voxel_mesher.cpp#L185).
- Other transparent faces compare only total boundary coverage: [`srcs/structures/voxel/spk_voxel_mesher.cpp:198`](../../srcs/structures/voxel/spk_voxel_mesher.cpp#L198).
- Clipping applies that Y interval to the entire polygon: [`srcs/structures/voxel/spk_voxel_mesher.cpp:333`](../../srcs/structures/voxel/spk_voxel_mesher.cpp#L333).
- Opaque full-boundary detection checks one quad and its extrema: [`srcs/structures/voxel/spk_voxel_shape.cpp:127`](../../srcs/structures/voxel/spk_voxel_shape.cpp#L127).
- Coverage sums polygon shoelace areas rather than polygon union: [`srcs/structures/voxel/spk_voxel_shape.cpp:191`](../../srcs/structures/voxel/spk_voxel_shape.cpp#L191).

## Failure scenario

1. Define two adjacent transparent cuboid/custom shapes with equal alpha.
2. Give their opposing vertical faces overlapping Y ranges but disjoint horizontal ranges.
3. The neighbor's Y interval is applied to the full current face, deleting a visible strip.

Equivalent failures occur with disjoint equal-area top/bottom footprints, a diamond boundary quad classified as a full square, or overlapping polygons whose summed area reaches one.

## Proposed direction

Represent occlusion in the two-dimensional coordinate system of the shared boundary plane and compute actual polygon overlap/difference. A conservative fallback that emits extra internal geometry is preferable to deleting visible geometry if arbitrary polygons are too costly or unsupported. Define an explicit compatibility key for transparent media/materials instead of using raw alpha alone; alpha may remain one part of that key.

If the engine intends to support only simple convex boundary polygons, validate and document that constraint. Otherwise, the overlap implementation must handle the admitted polygon set.

## Acceptance criteria

- [ ] A neighbor removes only the portion of a face that actually overlaps on the shared boundary plane.
- [ ] Disjoint faces are never culled because they share a Y range or scalar area.
- [ ] A non-square quad that merely reaches all four extrema is not classified as full coverage.
- [ ] Overlapping polygons do not double-count coverage.
- [ ] Transparent-interface culling uses a documented material/medium compatibility rule.
- [ ] Numerical tolerance behavior is explicit and does not rely on exact floating-point alpha equality.
- [ ] Built-in full cubes and full-width water retain efficient internal-face culling.

## Tests

- [ ] Add vertical same-Y/disjoint-horizontal transparent cuboid cases.
- [ ] Add horizontal equal-area/disjoint-footprint cases.
- [ ] Add partial-overlap tests that verify the remaining polygon area or emitted triangles.
- [ ] Add a diamond-quad regression for opaque full coverage.
- [ ] Add overlapping multi-polygon coverage tests.
- [ ] Add same-alpha/different-medium and near-equal-alpha/same-medium cases.
- [ ] Exercise orientations and flips on both sides of the boundary.

## Dependencies / notes

Changing clipping can alter the count/emission reproducer in ticket 002. The buffer-safety invariant from that ticket must hold regardless of the geometric algorithm selected here.
