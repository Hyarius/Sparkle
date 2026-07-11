# Ticket 031 — Separate water collision and standability semantics

- **Status:** Ready
- **Priority:** P2 — Medium
- **Area:** Playground / traversal / fluid data

## Problem

The two-value `VoxelTraversal` model conflates “occupies space”, “blocks movement”, and “can be stood on”. Authored/source water is marked `Solid`, so it becomes a standable navigation node and a solid raycast target. Generated stages of the same fluid are marked `Passable`. Rivers behave as obstacles mainly because their source cells are one block lower than the bank, rather than because water has an explicit non-walkable semantic.

This makes visually similar water cells behave differently and makes traversal change when channel depth or the allowed vertical gap changes.

## Evidence

- `playground/resources/data/voxels/water.json:1` declares water as `"traversal":"solid"`.
- `playground/srcs/board/cell_source.cpp:46-68` treats every solid cell with clear headroom as standable.
- `playground/srcs/world/world_raycaster.cpp:11-15` uses the same flag as its solid-hit test.
- `playground/srcs/voxel/voxel_registry.cpp:66-101` deliberately marks generated fluid stages as passable.

## Failure scenario

Place source water flush with a bank, teleport onto it, or raise the maximum traversal gap to one block. The water can become a valid walk node even though rivers are intended to be obstacles. A generated full-height stage next to it remains passable despite looking like the same material.

## Proposed direction

Represent collision/occupancy and standability independently. For example, introduce a blocked-but-not-standable traversal class or explicit `blocksMovement`, `supportsActor`, and `raycastable` properties. Define source and generated-stage behavior as one fluid-family policy rather than as unrelated hard-coded values.

## Acceptance criteria

- [ ] Water obstacle behavior no longer depends on a one-block bank height difference.
- [ ] Authored water and generated fluid stages have explicitly documented traversal and picking semantics.
- [ ] A water source with clear headroom is not accidentally considered a normal walkable floor.
- [ ] Bridges remain traversable while the surrounding water remains non-walkable.
- [ ] Existing solid terrain, passable vegetation, slopes, and stairs retain their current behavior.

## Tests

- [ ] Add navigation tests for source water, generated stages, flush banks, and one-block-lower channels.
- [ ] Add a bridge connectivity test with water on both sides.
- [ ] Add picking tests that state whether each fluid stage is selectable.

## Dependencies / notes

Coordinate this ticket with Ticket 007 (shape-aware picking) and Ticket 030 (fluid convergence).
