# Make world ray hits respect actual voxel shape geometry

- **Status:** Ready
- **Priority:** P1 — High, gameplay interaction correctness
- **Area:** Playground / picking / voxel geometry

## Problem

`WorldRaycaster` uses DDA to traverse grid cells and declares a hit as soon as it enters any voxel whose traversal classification is `Solid`. It never intersects the ray against the voxel's registered geometry, orientation, or vertical flip.

As a result, the empty half of a slab, the void above/beside a slope, and gaps around stair geometry behave like solid unit cubes for picking. The returned hit distance and face can describe a cell boundary rather than the rendered surface. Mouse picking can therefore select, highlight, place against, or search below a voxel the ray did not visually touch.

Exact DDA ties are another ambiguity: only one axis is advanced at a time. At an edge/corner crossing the algorithm can test an intermediate cell that the mathematical ray only touches at zero area, returning it instead of the intended diagonal cell. Non-finite or out-of-range ray origins are also converted directly to `int`.

## Evidence

- Candidate solidity is based only on `VoxelTraversal::Solid`: [`playground/srcs/world/world_raycaster.cpp:11`](../../playground/srcs/world/world_raycaster.cpp#L11).
- Ray origin coordinates are floored and directly cast to `int`: [`playground/srcs/world/world_raycaster.cpp:35`](../../playground/srcs/world/world_raycaster.cpp#L35).
- A ray starting in a solid-classified cell returns immediately without a shape test: [`playground/srcs/world/world_raycaster.cpp:40`](../../playground/srcs/world/world_raycaster.cpp#L40).
- DDA tie handling advances only the first matching axis: [`playground/srcs/world/world_raycaster.cpp:53`](../../playground/srcs/world/world_raycaster.cpp#L53).
- Entering a solid-classified cell is treated as the final hit: [`playground/srcs/world/world_raycaster.cpp:71`](../../playground/srcs/world/world_raycaster.cpp#L71).

## Failure scenario

1. Place a half-height slab classified as solid.
2. Cast a horizontal ray through the upper half of its cell without intersecting the rendered slab.
3. DDA enters the cell and returns a hit at the cube boundary.
4. The picker highlights or places relative to invisible geometry.

The same issue occurs when a ray travels through the empty volume of a slope or between stair treads and the enclosing unit-cube boundary.

## Proposed direction

Keep cell DDA as a broad-phase accelerator, but perform a narrow-phase intersection against the oriented/flipped voxel shape before accepting a candidate. The narrow phase could use render polygons, a dedicated picking/collision representation, or well-defined analytic primitives. A dedicated representation may be preferable if visual geometry and interaction geometry are intentionally different.

Define edge/corner tie semantics explicitly—such as supercover traversal or simultaneous-axis advancement—and validate ray inputs before float-to-integer conversion. Traversal classification can still decide whether a voxel participates in picking, but should not substitute for its geometry.

## Acceptance criteria

- [ ] Rays through empty portions of slabs, slopes, stairs, cuboids, and custom supported shapes do not report hits.
- [ ] Reported hit distance and normal/face correspond to the intersected shape surface.
- [ ] Orientation and vertical flip are applied to the intersection geometry.
- [ ] Starting inside a voxel shape has documented, tested behavior.
- [ ] Edge/corner ties have deterministic documented semantics and do not select arbitrary zero-volume intermediate cells.
- [ ] Non-finite and out-of-range origins/directions are rejected or handled without undefined conversion.
- [ ] Full-cube picking behavior remains unchanged.

## Tests

- [ ] Add hit and miss rays through both halves of a slab.
- [ ] Add slope and stair tests from several approach angles.
- [ ] Repeat shape tests across orientations and flips.
- [ ] Add exact grid-edge and grid-corner tie cases.
- [ ] Add rays starting inside and exactly on shape boundaries.
- [ ] Add NaN, infinity, extreme-coordinate, zero-direction, and maximum-distance validation tests.
- [ ] Add an integration test proving mouse hover/selection matches the visible surface.

## Dependencies / notes

If custom render polygons are not guaranteed to be simple/convex, a dedicated validated picking mesh may be safer than directly reusing them. That design decision overlaps the broader custom-shape contract work but need not block correct built-in shapes.
