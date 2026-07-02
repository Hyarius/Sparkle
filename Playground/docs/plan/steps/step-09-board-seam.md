# Step 09 — BoardData seam, live world derivation, deployment zones, BoardFixture

**Phase D · critical path · needs step 07**

## Goal

The headless board the whole battle backend stands on (D03): terrain/navigation/runtime
layers behind `BoardData`, derived live from world voxels or built from synthetic grids,
with deployment zones and the ASCII `BoardFixture` for tests.

## Reading

[board.md](../../03-systems/board.md) (the spec) ·
[05-class-board.puml](../../diagrams/05-class-board.puml) ·
[howto/add-tests.md](../../howto/add-tests.md) §BoardFixture.

## Files

`srcs/board/`: `board_terrain_layer.hpp` (ICellSource + size + isInside),
`board_navigation_layer.hpp/.cpp` (graph rebuild over bounds w/ excluded columns;
isStandable), `board_runtime_registry.hpp/.cpp` (unit occupancy: canRegister/tryRegister/
tryMove/swapUnits/remove/tryGetPosition/hasUnitAt — one unit per cell; interactive objects:
tryAdd/getAt/removeByTags/advanceObjectDurations — forward-declare `BattleObject` minimal
base here: side + tags), `board_data.hpp/.cpp` (façade + worldAnchor + borderCells),
`board_builder.hpp/.cpp` (`fromWorld(world, center, size)` with clamping + degenerate-
terrain nudge/shrink per [board.md](../../03-systems/board.md) §derivation; `fromGrid`),
`deployment.hpp/.cpp` (two opposing strips, depth from game rules, player side by approach
direction: the side whose outward normal best matches player-facing), `line_of_sight.hpp/
.cpp` (3D DDA between standing points + 0.5 eye offset; `losTransparent` tag pass),
`board_raycaster.hpp/.cpp` (mouse ray → board-local standable cell).
`Playground/tests/support/board_fixture.hpp/.cpp` — ASCII layers → `VoxelGrid` →
`BoardData` (legend: `#` cube, `.` empty, `/` slope+Z (`\`,`<`,`>` other orientations),
`s` stair+Z, `_` slab, `b` bush-on-ground, `x` wall 2-high); named-cell access; uses the
step-04 voxel definitions loaded from test literals (not resource files).

## Work items

1. Zero-copy world derivation: `WorldCellSource` translates board-local → world cells;
   assert the world is untouched (const access only).
2. Border ring computation (outermost standable per column of the rectangle edge).
3. Deployment validation + nudge/shrink loop (≤4 tries, log warning on shrink).

## Tests (`[test]`) — the workhorse suite

Fixture self-tests (legend → expected standables). Runtime registry truth table (double
occupancy, move onto occupied, swap, remove, object stacking/durations). Derivation: anchor
math, clamping at map edge, border cells, deployment strips on flat ground, nudge when the
default rectangle hits the plateau wall, shrink on pathological terrain. LoS: wall blocks,
bush doesn't (`losTransparent` on the bush definition — add the tag in data), elevation
shots (plateau→ground), adjacent cells always true. Mouse resolution: descend through
plateau column.

## Definition of Done

`[build]`/`[test]` green (this step is test-only — no visible change; `[run]` still works).
The fixture is documented in [howto/add-tests.md](../../howto/add-tests.md) (update the
legend there if it grew).
