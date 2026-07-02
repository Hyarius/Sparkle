# Step 07 — Traversal graph + A*, mouse picking, click-to-move player, orbit camera

**Phase C · critical path · needs steps 02, 06**

## Goal

A player creature walking the M1 testground by mouse click (D27): traversal graph over the
world, A*, screen→cell picking, path-driven movement with `playerMoved` events, and the
orbit/follow camera. The graph + A* built here are the same machinery the battle board
reuses (step 09).

## Reading

[board.md](../../03-systems/board.md) §graph/§ICellSource ·
[world.md](../../03-systems/world.md) §Actors ·
[rendering-cameras.md](../../03-systems/rendering-cameras.md) §4/§5 ·
[21-seq-exploration-movement.puml](../../diagrams/21-seq-exploration-movement.puml).

## Files

`srcs/board/` (shared machinery lives board-side from day one):
`cell_source.hpp` (`ICellSource` + `WorldCellSource` + `GridCellSource`),
`traversal_graph.hpp/.cpp` (nodes + 4-dir links + dense index),
`traversal_graph_builder.hpp/.cpp` (standable scan + best-candidate-per-column edges, per
[board.md](../../03-systems/board.md)), `pathfinder.hpp/.cpp` (A* + Dijkstra
`floodReachable`).
`srcs/world/world_navigation.hpp/.cpp` — assembles/refreshes the world graph from chunk
regions; `Chunk::_synchronize()` now also invalidates its nav slice (extend step 06's
chunk).
`srcs/world/world_raycaster.hpp/.cpp` — DDA over `VoxelWorld` (`raycast(world, ray, maxDist)
-> optional<VoxelHit{cell, enterFace, distance}>`).
`srcs/rendering/mouse_picker.hpp/.cpp` — `screenToRay(camera, viewportSize, mousePx)`
(unproject via `Matrix4x4::inverse` of viewProjection), `pickStandable(world, ray)`
(DDA hit → descend column to topmost standable).
`srcs/components/actor.hpp` — cell, facing, speed, active path (cells + segment t).
`srcs/logics/actor_path_logic.hpp/.cpp` — consumes `actorMoveRequested`; A* on the world
graph; advances along segments with Y from walk heights (smooth over slopes/stairs);
`playerMoved(cell)` per cell reached (player actor only); retarget on new request;
unreachable → `invalidTarget` feedback event.
`srcs/logics/camera_controller_logic.hpp/.cpp` — orbit/follow: target = player + eye
height; yaw/pitch via ZQSD + right-drag; wheel zoom; clamped pitch; smoothed.
`srcs/core/exploration_mode.*` — real enter/exit: binds a mouse-input logic (left click →
pick → `actorMoveRequested`), activates camera controller.
`game_scene_widget.*` — spawn the player entity (placeholder cube via `makeCube`, an
`Actor`, a `MeshRenderer3D`) at the map's `playerSpawn` marker.

## Work items

1. Graph/A*: build over the loaded map region; rebuild affected region when a chunk
   resynchronizes.
2. Picking precision: DDA must return the entry face so partial shapes resolve to the right
   column; descend to standable (slope cells resolve to themselves).
3. Path smoothing: linear interpolation between cell walk-height points
   (edge heights at boundaries, stationary at centers) — no bobbing on flat ground, smooth
   ramps on slopes/stairs.
4. Hover feedback: highlight the picked cell each mouse-move — **use the mask system**: a
   1-cell `buildMaskMesh` with the `hovered` mask (D31), rendered translucent.

## Tests (`[test]`)

Graph on synthetic grids (this is where `BoardFixture`'s precursor asserts standability/
edges — full fixture lands step 09): flat, slope chain up 3 levels, stairs, wall gaps,
head-clearance blocks. A* shortest paths + unreachable. Flood reachability with maxCost.
DDA truth table (axis rays, diagonal rays, from inside a solid, misses). screenToRay
round-trip (project a known world point, ray through its pixel passes within ε).
Path-driver: cell-reached event sequence for a known path; retarget mid-path.

## Definition of Done

- `[build]`/`[test]` green.
- `[run]` visual (user validates): click-to-move across the testground — player walks
  around the wall (not through), up the slope and stairs onto the plateau, hover highlight
  tracks the mouse (mask-textured), camera orbits/zooms smoothly, clicks on walls/void do
  nothing but flash invalid feedback.

## Implementation notes (2026-07-02)

- Shared board-side traversal builds standable nodes and best height-compatible four-way
  links. A* and bounded Dijkstra flood operate on the same graph used by exploration.
- `WorldNavigation` lazily rebuilds after the voxel-world revision changes. Slope and stair
  links use oriented cardinal edge heights and the configured vertical-gap limit.
- DDA raycasting reports entry planes and supports axis, diagonal, inside-solid, and miss
  cases. Mouse unprojection uses the inverse camera view-projection matrix.
- The player actor spawns from `playerSpawn`, moves along A* paths with center/edge height
  interpolation, emits `playerMoved`, and supports mid-path retargeting.
- Exploration input builds hovered/invalid one-cell mask meshes. The follow camera supports
  ZQSD orbit, right-drag orbit, wheel zoom, pitch clamps, and smoothed targeting.
- The actor/overlay render pass reuses the chunk pass camera/light state; this avoids
  duplicate frame-state commands while preserving opaque-then-translucent ordering.
- The Playground suite passes all 85 tests, including 12 new graph, path, movement,
  raycasting, and picking tests. The visible exploration window remains stable with no
  stderr diagnostics. No engine (`spk`) source files were changed.
