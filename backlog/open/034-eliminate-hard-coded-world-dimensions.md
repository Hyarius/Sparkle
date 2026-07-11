# Ticket 034 — Eliminate hard-coded dimensions that contradict world configuration

- **Status:** Ready
- **Priority:** P2 — Medium
- **Area:** Playground / world generation / navigation

## Problem

Several algorithms advertise configurable dimensions but contain assumptions tied to the current defaults. Non-default values can create gaps or overlaps in stair assemblies, truncate foundations, exclude tall content from navigation, or underestimate the realized world height.

## Evidence

- `playground/srcs/world/generator/climb_prefabs.cpp:74-95` synthesizes fixed 3×3×3 flights and 3×3 platforms.
- `playground/srcs/world/generator/world_plan_generator.cpp:2138-2139` spaces them using configurable `blocksPerLevel`.
- `playground/srcs/game_scene_widget.cpp:212-219` initially derives navigation height, while `:368-373` later hardcodes maximum Y to 64.
- `playground/srcs/world/generator/plan_chunk_provider.cpp:316-321` assumes foundations extend at most 32 blocks.
- `playground/srcs/world/generator/plan_chunk_provider.cpp:368-370` computes maximum height from terrain plus 10, ignoring actual placements/interiors.

## Failure scenario

Set `blocksPerLevel` to a value other than three, add a tall prefab, or use a foundation more than 32 blocks above terrain. Generated geometry and traversal no longer agree, and content can disappear from lower chunks or become unreachable after the first streaming-focus update.

## Proposed direction

Derive climb geometry, placement bounds, foundation intersections, and navigation bounds from actual configuration and resolved prefab extents. If a setting is not genuinely supported, reject it explicitly during configuration validation rather than accepting it.

## Acceptance criteria

- [ ] Stair/climb geometry and spacing use one shared rise/run definition.
- [ ] Navigation bounds are derived consistently at startup and after focus changes.
- [ ] Foundation intersection covers the full terrain-to-placement span without a magic depth.
- [ ] Maximum realized height includes prefabs and interiors.
- [ ] Unsupported configurations fail early with a clear error.

## Tests

- [ ] Generate and validate worlds with at least two supported `blocksPerLevel` values.
- [ ] Cover a foundation deeper than 32 blocks.
- [ ] Cover a prefab/interior above Y=63 across a streaming-focus change.

## Dependencies / notes

Ticket 008 should establish the authoritative configuration invariants first.
