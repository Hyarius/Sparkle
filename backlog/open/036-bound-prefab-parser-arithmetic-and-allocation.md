# Ticket 036 — Bound prefab parser arithmetic and allocation

- **Status:** Ready
- **Priority:** P1 — High
- **Area:** Playground / prefab data loading

## Problem

Prefab coordinates, fill boxes, stair `steps`, and stair `width` have no useful upper bounds. Bounding arithmetic occurs in signed `int`, including negating the lowest coordinate and computing inclusive extents. Malformed but schema-valid JSON can cause signed overflow, enormous loops, or a huge dense-grid allocation before the parser reaches its later validation.

## Evidence

- `playground/srcs/world/prefab_parser.cpp:65-115` expands every stair step/width pair while deriving bounds and checks only that both are at least one.
- `playground/srcs/world/prefab_parser.cpp:124-126` negates `lowest` and evaluates `highest - lowest + 1` in `int`.
- `playground/srcs/world/voxel_content_parser.cpp:354-380` validates fill ordering only after coordinates have been offset and the destination grid already exists.

## Failure scenario

Use `INT_MIN` in prefab content, span a fill from `INT_MIN` to `INT_MAX`, or specify billions of stair steps. Loading can execute undefined arithmetic, spend unbounded time enumerating bounds, or fail with `bad_alloc` instead of a contextual `JsonError`.

## Proposed direction

Validate ordering and configured limits before expanding content. Perform span/offset calculations with checked 64-bit arithmetic, verify conversion to engine coordinates, compute a checked voxel-count budget, and only then allocate or enumerate. Report the exact JSON path that violates the limit.

## Acceptance criteria

- [ ] No signed overflow is possible while parsing prefab coordinates or dimensions.
- [ ] Fill `min <= max` is validated before grid allocation.
- [ ] Stair steps, width, axis extents, and total voxel count have documented limits.
- [ ] Rejected inputs produce `JsonError` with file and field path, not `bad_alloc` or a low-level geometry exception.
- [ ] Valid negative-coordinate prefabs continue to work.

## Tests

- [ ] Cover `INT_MIN`, `INT_MAX`, overflowing spans, inverted fills, and oversized stair runs.
- [ ] Cover a valid prefab containing modest negative coordinates and a below-ground layer.
- [ ] Verify limits are applied before allocation/iteration using a bounded-time test.

## Dependencies / notes

Engine-side coordinate safety is tracked separately in Ticket 014.
