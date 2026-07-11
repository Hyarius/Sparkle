# Ticket 033 — Validate spawn against the realized voxel world

- **Status:** Ready
- **Priority:** P2 — Medium
- **Area:** Playground / world generation / navigation

## Problem

Spawn selection considers the macro road/entity/bridge grids, not the final stamped voxel world. It ignores stair and scenery placements, full prefab footprints, headroom, and actual graph connectivity. Scene setup then selects the topmost standable cell in that column, which can silently move the spawn onto a roof or another structure instead of the intended road.

## Evidence

- `playground/srcs/world/generator/plan_chunk_provider.cpp:373-436` selects a macro road cell and only excludes entity anchors and bridges.
- `playground/srcs/game_scene_widget.cpp:212-226` resolves that column to its topmost navigation node.
- `playground/srcs/world/generator/plan_chunk_provider.cpp:119-147` resolves prefabs with footprints that can extend away from their anchors.

## Failure scenario

A wide building, stair assembly, or other stamped prefab overlaps the chosen road column while being anchored elsewhere. The topmost standable result becomes its roof, or no reachable node exists even though a nearby valid road cell does.

## Proposed direction

Make spawn validation a realized-world operation after the warm-up chunks are loaded. Search deterministic nearby candidates and require the intended terrain/road surface, two-cell headroom, graph membership, and reachability. Alternatively, reserve and validate a complete spawn footprint during planning and assert it after realization.

## Acceptance criteria

- [ ] Spawn is placed on the intended road/ground surface, not merely the topmost object in its column.
- [ ] Candidate validation includes headroom and navigation-graph membership.
- [ ] Prefab footprints, stairs, water, portals, and bridges are accounted for.
- [ ] Fallback search is deterministic for a given seed and data set.
- [ ] Failure reports a useful diagnostic rather than selecting an unrelated surface.

## Tests

- [ ] Cover a wide prefab whose footprint crosses a candidate road column.
- [ ] Cover a roof above a valid road and a blocked-headroom column.
- [ ] Verify deterministic fallback to a nearby reachable road cell.

## Dependencies / notes

Ticket 025 may change the navigation-query/indexing mechanism used for realized spawn validation.
