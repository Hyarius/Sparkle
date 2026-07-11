# Reject invalid voxel shape parameters at the API boundary

- **Status:** Ready
- **Priority:** P2 — Medium
- Area: Voxel shapes / Validation

## Problem

Shape constructors and mutators accept values that cannot produce valid or bounded render geometry.

A slab height of zero passes validation but creates collinear zero-area side polygons and fails later during initialize(), far from the source of the bad value. Comparison-only floating-point validation allows NaN for slab height, cuboid bounds, and transparency. The public stair shape accepts any positive int even though geometry and transformed variants grow with step count. Transparency can also be changed after initialization/registration without invalidating meshes that were classified and baked using the old value.

## Evidence

- srcs/structures/voxel/spk_slab_voxel_shape.cpp:22 — slab height is accepted by the constructor.
- srcs/structures/voxel/spk_slab_voxel_shape.cpp:26 — validation permits zero and does not reject non-finite values.
- srcs/structures/voxel/spk_slab_voxel_shape.cpp:46 — zero height produces degenerate vertical rectangles.
- srcs/structures/voxel/spk_cuboid_voxel_shape.cpp:19 — cuboid bounds use comparisons that NaN bypasses.
- srcs/structures/voxel/spk_stair_voxel_shape.cpp:22 — the engine stair constructor has no practical upper bound.
- srcs/structures/voxel/spk_stair_voxel_shape.cpp:41 — face storage and construction scale directly with step count.
- srcs/structures/voxel/spk_voxel_shape.cpp:387 — transparency remains publicly mutable.
- srcs/structures/voxel/spk_voxel_shape.cpp:389 — NaN passes the transparency range check.
- playground/srcs/voxel/voxel_parser.cpp:174 — Playground repeats the slab range check and accepts zero.
- playground/srcs/voxel/voxel_parser.cpp:195 — Playground independently caps stair steps at 64, creating an engine/parser contract mismatch.

## Failure scenario

Load a slab definition with height 0. Parsing and construction succeed, but registry initialization later throws a polygon-collinearity error without the useful JSON field context. Through the public engine API, pass quiet NaN as transparency or a cuboid bound and obtain undefined rendering classification. Construct a stair with millions of steps and force enormous geometry allocation.

## Proposed direction

Centralize shape parameter validation in the engine and make parser validation mirror the same constants and intervals. Require finite floating-point values, define slab height as a strictly positive interval if zero-volume slabs are unsupported, and establish a practical engine-level stair limit. Freeze mesh-affecting properties after initialize(), or route changes through a registry-wide invalidation mechanism.

## Acceptance criteria

- [ ] Slab height has a documented valid interval and zero is either supported with valid geometry or rejected immediately.
- [ ] All public shape float inputs reject NaN and infinity.
- [ ] Cuboid bounds remain ordered, finite, and inside the unit cell.
- [ ] Engine and Playground enforce the same documented maximum stair step count.
- [ ] Mesh-affecting shape properties cannot change silently after initialization.
- [ ] Parser errors identify the source file and offending field before registry initialization begins.

## Tests

- [ ] Add constructor tests for zero, negative, greater-than-one, NaN, and both infinities.
- [ ] Add equivalent parser tests with field-specific JsonError assertions.
- [ ] Add stair tests at the maximum and one above it.
- [ ] Add a post-initialize transparency mutation test for the selected immutable/invalidation behavior.
- [ ] Add valid boundary-value tests to prevent accidentally narrowing the supported range.

## Dependencies / notes

Do not treat the later Polygon3D exception as sufficient validation: bad public parameters should fail where they enter the engine. If runtime material changes are required, they need an explicit renderer invalidation design rather than an unrestricted setter.
