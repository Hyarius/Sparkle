# Define and enforce custom voxel shape geometry contracts

- **Status:** Ready
- **Priority:** P2 — Medium
- Area: Voxel shape extensibility / Geometry validation

## Problem

VoxelShape is publicly extensible, but the geometric contract required by the mesher is only partially documented and weakly enforced.

Polygon3D checks that vertices are coplanar, but not that polygons are simple, convex, consistently wound, finite, or suitable for fan triangulation. The mesher always emits a triangle fan. Concave or self-intersecting polygons can therefore render incorrectly. VoxelShapeFace requires exact floating-point equality between polygon normals, making a valid tessellated slanted face fragile to tiny rounding differences.

Texture atlas coordinates are not constrained to the configured atlas. Invalid voxel IDs or enum values can also reach a grid and fail repeatedly during asynchronous rebakes rather than at a clear validation boundary.

## Evidence

- includes/structures/graphics/geometry/spk_polygon_3d.hpp:30 — Polygon3D computes a normal from the first three vertices.
- includes/structures/graphics/geometry/spk_polygon_3d.hpp:46 — later validation checks only distance from that plane.
- includes/structures/voxel/spk_voxel_shape.hpp:48 — face composition accepts independently constructed polygons.
- includes/structures/voxel/spk_voxel_shape.hpp:50 — normals must compare exactly equal.
- includes/structures/voxel/spk_voxel_shape.hpp:73 — outer and inner shell semantics are described, but geometric validity requirements are not.
- srcs/structures/voxel/spk_voxel_shape.cpp:260 — createPolygon validates only list length before delegating to Polygon3D.
- srcs/structures/voxel/spk_voxel_shape.cpp:274 — atlas UV generation consumes AtlasCell without verifying it lies inside the atlas.
- srcs/structures/voxel/spk_voxel_shape.cpp:341 — initialize transforms and caches custom faces without a comprehensive validation pass.
- srcs/structures/voxel/spk_voxel_mesher.cpp:654 — every polygon is triangulated as a fan from vertex zero.

## Failure scenario

Implement a custom voxel shape with a concave five-vertex face. It passes construction and initialization, but fan triangulation fills an area outside the polygon or reverses part of it. Splitting a slanted face into two mathematically coplanar polygons can instead throw because separately normalized float normals differ by a few bits. An out-of-range atlas cell generates UVs into unrelated texture space.

## Proposed direction

Publish a precise custom-shape contract and validate it once during shape initialization or registry registration. Either require simple convex polygons with consistent winding and finite vertices/UVs, or replace fan triangulation with a robust triangulator that supports the documented polygon class. Compare normals using an angular tolerance and verify compatible direction, not exact component equality.

Validate atlas cells against atlas dimensions. Define where voxel IDs, orientations, and flips become trusted values, and reject malformed values before scheduling a mesh build.

## Acceptance criteria

- [ ] Public documentation states the allowed polygon topology, winding, coordinate range, normal, and shell rules.
- [ ] Non-finite, degenerate, self-intersecting, and unsupported concave polygons are rejected with actionable errors, or are triangulated correctly if supported.
- [ ] Coplanar face polygons with numerically equivalent normals are accepted within a documented tolerance.
- [ ] Oppositely wound polygons in one face are rejected.
- [ ] Atlas cells must be non-negative and inside the configured atlas.
- [ ] Invalid voxel IDs, orientations, and flips fail at a defined validation boundary rather than causing endless rebake failures.
- [ ] All built-in shapes continue to initialize under the same public rules.

## Tests

- [ ] Add custom-shape tests for convex, concave, self-intersecting, repeated-vertex, collinear, non-planar, and non-finite polygons.
- [ ] Add near-equal-normal and opposite-normal multi-polygon face tests.
- [ ] Add atlas-cell tests for negative, boundary, and out-of-range coordinates.
- [ ] Add invalid voxel ID/orientation/flip tests at the selected validation boundary.
- [ ] Build and mesh every built-in shape as a compatibility regression.

## Dependencies / notes

This ticket defines authoring validity. It does not replace the separate tickets for transparent face-overlap occlusion or full-boundary coverage computation. If concave polygons are deliberately supported, robust triangulation and coverage calculations must share the same topology assumptions.
