# Spatially index chunk realization inputs

- **Status:** Ready
- **Priority:** P1 — High
- Area: Playground / Chunk generation performance

## Problem

Realizing one chunk repeatedly scans data for the entire generated world. Every terrain column scans every paved stair rectangle, and every chunk scans every resolved prefab placement to find intersections. Foundation stamping then recomputes terrain columns for every footprint cell.

This cost is multiplied by synchronous warm-up. The current view range is 5 by 3 by 5 chunks, so spawn and teleport can each request 75 chunks in one call path. Generation time therefore grows with both total world content and the whole warm-up volume rather than mainly with content local to each chunk.

## Evidence

- `playground/srcs/world/generator/plan_chunk_provider.cpp:255-261` scans all `pavedRects` for each requested world column.
- `playground/srcs/world/generator/plan_chunk_provider.cpp:274-309` evaluates every X/Z column in a chunk.
- `playground/srcs/world/generator/plan_chunk_provider.cpp:312-326` scans every resolved placement for every chunk.
- `playground/srcs/world/generator/plan_chunk_provider.cpp:337-355` walks every foundation footprint and calls `_column()` again.
- `playground/srcs/game_scene_widget.cpp:41` defines a `{2,1,2}` chunk view range.
- `playground/srcs/game_scene_widget.cpp:199-208` synchronously warms the spawn neighborhood.
- `playground/srcs/game_scene_widget.cpp:393-403` synchronously warms the same 75-chunk volume at a teleport destination.

## Failure scenario

Generate a large plan containing many buildings, scenery placements, and composed stair rectangles, then teleport across the world. Each of 75 chunks scans all placements, and each of its 256 terrain columns scans all paved rectangles. A world with more content becomes increasingly expensive to stream even when the destination region itself is sparse.

## Proposed direction

Build spatial indexes when constructing `PlanChunkProvider`:

- bin resolved placements by every intersected chunk coordinate;
- bin or rasterize paved rectangles by plan cell/chunk;
- precompute reusable column descriptors where memory permits;
- cache or batch foundation column queries;
- make chunk fill proportional to local intersecting content;
- schedule large warm-up sets over a budget or prepare them asynchronously from immutable plan data.

Preserve deterministic voxel output and placement order when overlapping stamps are possible.

## Acceptance criteria

- [ ] Filling a chunk examines only placements that can intersect that chunk.
- [ ] A column query does not scan every paved rectangle in the world.
- [ ] Foundation generation does not repeatedly perform equivalent global lookups.
- [ ] Adding distant non-intersecting placements does not materially increase a chunk's fill work.
- [ ] Spawn/teleport warm-up has a documented frame-time or work budget.
- [ ] Indexed generation produces byte-for-byte equivalent voxel cells for existing valid plans.

## Tests

- [ ] Compare chunk checksums from indexed and reference generation across multiple seeds and chunk coordinates.
- [ ] Add an instrumented test proving a chunk with no local placements performs zero placement intersection checks after index lookup.
- [ ] Add a scale benchmark with many distant placements and paved rectangles.
- [ ] Cover placements crossing chunk boundaries and foundations extending into lower chunks.
- [ ] Cover deterministic overlap/stamp ordering.

## Dependencies / notes

Ticket 025 addresses a separate synchronous cost that can occur immediately after chunk warm-up. Measure chunk generation and navigation refresh independently so improvements are attributable.
