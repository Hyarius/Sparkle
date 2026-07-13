# Level 2 — Systems

> Each chapter decomposed into its pipeline steps, naming the classes and files
> involved. Same numbering as [level-1-overview.md](level-1-overview.md); drill further
> in [level-3-details.md](level-3-details.md).

## 1. Startup & data loading

Files: `srcs/main.cpp`, `srcs/core/registries.cpp`, `srcs/core/json.hpp` (aliases to
`spk::JSON::Reader` since 2026-07-11, see decision D42),
`srcs/core/registry.hpp`, `srcs/voxel/voxel_registry.cpp`, `srcs/voxel/voxel_parser.cpp`,
the `world/*_parser.cpp` files, `world/generator/climb_prefabs.cpp`.

1. `main()` parses `--seed`, `--size`, `--map-only`, `--check-stairs`; validates the
   requested worldgen config early.
2. `Registries::loadAll(data/)` loads, in dependency order:
   - **game rules** (`config/game-rules.json`),
   - **voxels** (`voxels/*.json`) → `pg::VoxelRegistry` builds *two lockstep views*:
     an `spk::VoxelRegistry` of render shapes and a parallel `VoxelDefinition` table
     (traversal/tags on the type, walk heights per state). Each voxel file is ONE
     semantic type (`spk::VoxelTypeId`) whose states (`spk::VoxelStateId`) each own a
     dense `spk::VoxelRuntimeId` (the value cells store). Voxels with a `fluid` block
     additionally generate one slab *state* per fill stage (water states `1..8`,
     state `0` = source) and a `FluidFamily` record.
   - **biomes** (`biomes/*.json`) → palettes as *weighted voxel pools* (surface,
     subsurface, deep, road, stair, slope) + a `worldgen` block (height shift, peak
     flag, scenery, entity prefab pools).
   - **prefabs** (`prefabs/*.json`) → `PrefabDefinition` wrapping an `spk::Prefab`
     (sparse voxel list, pivot, optional clearance box, named anchors, interior link).
   - **climb prefab synthesis** — `synthesizeClimbPrefabs()` registers per-biome
     `<biome>-stair-length#0..7` / `-stair-platform` (from road+stair pools) and
     `-slope-length#k` / `-slope-platform` (from surface+slope pools).
   - **interiors** (`interiors/*.json`) → room sets with weights and min/max counts.
   - **placement rules** (`worldgen/placements.json`) → global entity→prefab pools.
3. Depending on flags, `main` runs a headless tool (chapter 9) or builds the game
   scene (chapter 5).

## 2. World plan generation

Files: `world/generator/world_plan.hpp` (data model),
`world_plan_generator.hpp` (the `Generator`, stages split over
`world_plan_{math,terrain,infrastructure,stairways,interiors,scenery,generator}.cpp`),
`world_plan_validation.cpp`.

`generateWorldPlan()` validates the config then runs `Generator::run()`, a fixed
stage sequence. Every stage draws randomness from `rngFor("semantic/path")` — a
mt19937_64 seeded by FNV-1a over `masterSeed::path` — so adding a stage never
reshuffles earlier stages. The stages:

1. **buildWorldGraph** — assigns a biome and a progression rank to each zone.
2. **buildLandmass** — radial falloff + value noise → land mask; optional
   fragmentation troughs; drops tiny islands; labels continents; computes
   distance-to-ocean.
3. **assignZones** — farthest-point seeds + warped Voronoi over land cells;
   reattaches disconnected fragments; maps zones to continents; records inter-zone
   border cells.
4. **assignHeights** — strata level per cell = coast trend + 3-octave undulation +
   peak lift (peak-flagged biomes) + biome height shift, clamped; shores forced to 0.
5. **generateWater** — priority-flood depression filling → drainage tree; deep large
   basins become lakes; per-zone river sources walk the drainage tree to the sea.
6. **resolveGateways** — for each zone border, the cell nearest the border centroid
   becomes the primary gateway (sometimes a secondary far one).
7. **placeEntities** — derives each zone's settlement target from its biome's usable
   land area and `worldgen.towns.densityDistanceCells`, enforces its independent
   `minimumDistanceCells`, always reserves a gym, and reserves a waterfront port when
   `worldgen.towns.requiresPort` is set; POIs use their normal blocking and spacing rules.
8. **buildRoads** — per zone, A* (`findPath`) from the hub (the gym) to every other
   settlement and a few POIs, then across primary gateways. Step costs penalize
   water (bridges) and height steps; a "no 2×2 road square" invariant is enforced
   during search and by a final `removeRoadSquares` pass.
9. **addBoatLinks** — labels road components and joins components that already contain
   configured ports through their nearest port pair.
10. **markBridges** — road cells on water become bridge cells.
11. **placeStairways / placeWildStairways** — wherever the road (or, for wild ones,
    a random in-zone cliff) crosses a height step, `emitStairChain` composes a
    staircase out of climb prefabs and commits it against footprint + claim rules
    (details in level 3 — this is the most intricate part of the generator).
12. **placeBuildings** — one prefab per entity from the biome/global pool, nudged
    around the cell center until its claim box is free; vital buildings (gym, city)
    are kept even on conflict. Each building with an `interiorId` triggers
    **composeInterior**: rooms are grown connector-to-connector inside a reserved
    void slot east of the world, doorways carved, and two one-way portals pair the
    building door with the interior entry/exit pads.
13. **placeScenery** — per-zone Poisson-sampled decorative prefabs (trees, rocks…)
    with spacing rules, yielding to all structural claims.
14. **computeStats** — invariant counters (road squares, diagonal steps, gym on
    coast…) surfaced by `WorldPlan::report()`.

Cross-cutting rule: **claimed zones**. Stairways claim first (they must exist),
buildings second, scenery last; each placement's claim box (authored `clearance` or
content bounds, rotated) must not overlap earlier claims.

## 3. Prefabs & climb-prefab synthesis

Files: `includes/structures/voxel/spk_prefab.hpp`, `srcs/structures/voxel/spk_prefab.cpp`
(library); `world/prefab_definition.hpp`, `world/prefab_parser.cpp`,
`world/generator/climb_prefabs.cpp` (app).

- `spk::Prefab` stores a sparse list of (position, cell); positions may be negative
  (content below the anchor sinks into terrain). Empty cells **carve**. The pivot is
  the rotation fixed-point and the cell that lands on the stamp destination;
  `rotatedBounds()` gives the AABB for any of the four orientations.
- `PrefabDefinition` adds app-side metadata: named anchors (`door`, `entry`, `exit`,
  `connector:+x`…), an optional clearance box, and an interior link.
- Climb synthesis builds, per biome: 8 pre-mixed 3×3×3 **flight** variants (steps
  ascending +Z over a base fill, both drawn per-cell from weighted pools) and one
  **platform** (3×3 slab at y=-1 with authored clearance). Pool ids land on the
  biome's `PlanBiome` so the generator picks per-segment variants.

## 4. World realization

Files: `world/generator/plan_chunk_provider.{hpp,cpp}`, `world/voxel_world.{hpp,cpp}`,
`world/chunk_provider.hpp`, (`world/generator/perlin_chunk_provider.cpp` — legacy,
unused).

1. The constructor resolves everything once: biome palettes to numeric-id pools, and
   every `PrefabPlacement` to a `ResolvedPlacement` (rotated world-space box +
   destination).
2. `fill(chunk)` is the on-demand generator invoked by `spk::VoxelMap`:
   - For each of the 16×16 columns, `_column(x,z)` computes a `Column` descriptor
     from the plan: interior band → void; ocean → sand shelf + water; otherwise
     surface/subsurface/deep ids hash-picked from the biome pools, then modified by
     lake/river carving (3-wide cross), road paving (3-wide cross, bridges filled),
     and stairway approach-band paving.
   - The column is materialized into cells (surface at `groundTop`, water at
     `waterY`).
   - Every resolved placement intersecting the chunk is stamped: optional
     **foundation** pillar down to the terrain, then `Editor::applyPrefab`.
3. `pg::VoxelWorld` is a thin adapter that owns the `spk::VoxelMap`, forwards
   cell/chunk access, and maintains a `revision()` counter the navigation graph
   watches.
4. `spawnCell()` picks the gym of the first-progression zone and finds open road
   nearby.

## 5. Scene & streaming runtime

Files: `srcs/game_scene_widget.{hpp,cpp}`, `srcs/core/game_context.hpp`,
spk side: `spk_voxel_map.cpp`, `spk_voxel_chunk_streamer.cpp`,
`spk_voxel_chunk_streamer_logic.cpp`.

1. `_buildScene()` (constructor): loads textures → registers engine logics in order
   (streamer, opaque chunk render, actor render, transparent chunk render) → creates
   camera → **generates the world plan** (and prints its report) → wires portals →
   creates `PlanChunkProvider` + `VoxelWorld` → warms a 17×3×17 chunk window around
   the spawn → builds `WorldNavigation` bounds around spawn → spawns the player
   entity (mesh + `Actor` + `VoxelChunkStreamer`) → hover marker entity → registers
   `ActorPathLogic`, `CameraControllerLogic`, `ExplorationInputLogic`,
   `FluidSimulationLogic`. Only after full success is the world published into
   `GameContext`.
2. Each `_onUpdate`: re-centers the streamer window on the player's chunk, re-bounds
   navigation when the player crosses a chunk, runs the engine update, then executes
   any pending portal teleport (warm destination chunks, move actor, shift camera).
3. `spk::VoxelChunkStreamerLogic` reconciles the union of every streamer window with
   the map: loads/activates entering chunks, deactivates/unloads leaving ones, with
   per-update budgets and ownership rules (it never claims manually-activated
   chunks).

## 6. Rendering

Files: `includes/ + srcs/structures/voxel/` — shapes (`spk_voxel_shape.cpp`, cube /
cuboid / slab / slope / stair / cross variants), `spk_voxel_mesher.cpp`,
`spk_voxel_chunk.cpp`, `spk_voxel_chunk_renderer.cpp`, `spk_voxel_chunk_render_logic.cpp`,
`spk_voxel_chunk_transparent_render_logic.cpp`.

1. **Shapes** — each voxel id maps to a `VoxelShape` whose geometry is split into an
   *outer shell* (≤1 face per boundary plane, participates in occlusion) and *inner
   faces* (always drawn). At `initialize()` the shape precomputes all 8
   orientation/flip variants of every face plus boundary-coverage metadata.
2. **Mesher** — `VoxelMesher::buildRenderMesh(grid, registry, worldLookup, origin)`
   walks the grid, and for each face decides visibility against the neighbor cell
   (possibly in the next chunk via `IVoxelCellLookup`): full occlusion, partial
   occlusion (2-D polygon subtraction on the shared plane), and transparent-group
   rules (same `transparentOcclusionGroup` ⇒ internal faces culled, so a water body
   reads as one volume). Output: separate opaque and transparent meshes.
3. **Chunk & editor** — `spk::VoxelChunk` (an `Entity3D`, 16³ grid) only mutates
   through `editCells(editor)` so every write batch ends in exactly one mesh
   synchronization request; boundary-touching edits also invalidate the neighbor
   chunks. A mutation-thread guard catches off-thread writes once streaming begins.
4. **Bake & draw** — `VoxelChunkRenderLogic` collects dirty renderers each update,
   re-bakes them on a `WorkerPool` (profiled: batch / chunk / assembly probes),
   publishes each new mesh pair atomically (shared_ptr swap), caches draw commands
   per chunk keyed by mesh revision, and submits opaque geometry.
   `VoxelChunkTransparentRenderLogic` runs at lower priority and submits transparent
   meshes back-to-front.

## 7. Navigation & movement

Files: `srcs/board/*` (`traversal_graph.cpp`, `traversal_graph_builder.cpp`,
`pathfinder.cpp`, `cell_source.cpp`), `srcs/world/world_navigation.cpp`,
`srcs/world/world_raycaster.cpp`, `srcs/voxel/voxel_traversal.cpp`,
`srcs/logics/{exploration_input,actor_path,camera_controller}_logic.cpp`,
`srcs/components/actor.hpp`.

1. `TraversalGraphBuilder` scans a bounded box of world cells (via `WorldCellSource`)
   and produces nodes where an actor can stand, connected by edges whose step
   heights respect the per-voxel cardinal walk heights (`voxel_traversal.cpp`) and
   the max vertical gap from game rules.
2. `WorldNavigation` caches the graph and rebuilds lazily whenever
   `VoxelWorld::revision()` moves or bounds are reset (streaming recenters them).
3. `ExplorationInputLogic` builds a camera ray and uses `spk::VoxelRayCast` (unit-cell
   boundary DDA, filtered by pg traversal semantics) to hover-highlight a cell
   (mask overlay mesh) and, on click, asks `Pathfinder` (A*
   over the graph) for a path to the target.
4. `ActorPathLogic` advances the actor along its path segments at `Actor::speed`,
   emits `playerMoved` events per cell (which chapter 5 uses for portals), and
   snaps the entity to walk heights.
5. `CameraControllerLogic` orbits/zooms the camera around the player (right-drag /
   AEZS / wheel) and follows them smoothly; `teleportBy` shifts the whole rig
   without a swoop.

## 8. Fluid simulation

Files: fluid part of `srcs/voxel/voxel_parser.cpp` / `srcs/voxel/voxel_registry.cpp`,
`srcs/logics/fluid_simulation_logic.cpp`; the engine core lives in Sparkle
(`structures/voxel/spk_voxel_fluid.hpp`, `spk_voxel_fluid_simulation.hpp`).

1. **Data** (load time): a version-3 voxel JSON with a `"fluid"` block declares
   `maxSpread` (viscosity) plus top/bottom/side textures; `shape` and authored
   `states` are forbidden. The registry calls `spk::VoxelRegistry::registerFluid`,
   which generates one semantic type — state 0 the source, states 1..maxSpread
   slabs of height k/maxSpread sharing one family-unique transparent occlusion
   group — and classifies every runtime id via `spk::VoxelRegistry::tryFluidRef`
   (O(1)). Fluid states expose flat cardinal heights equal to their fill height.
2. **Runtime** (priority 50, between streamer and render): `FluidSimulationLogic`
   is only a scheduler around `spk::VoxelFluidSimulation` — it picks the tick
   interval, the player-centred bounds and the cell budget, and supplies the two
   traversal callbacks (passable ⇒ replaceable by fluid, solid ⇒ support). Source
   discovery, frontier scheduling, falling, weakening sideways spread, drop
   priority and support-climbing all live in the Sparkle simulation.
3. All edits go through `VoxelMap::setCell`, so chunks re-bake like any other edit
   and the map's own `revision()` (which `VoxelWorld::revision()` now forwards)
   moves for the navigation cache.

## 9. Headless tools & diagnostics

Files: `srcs/main.cpp` (harness), `world/generator/world_map_image.cpp`,
`srcs/game_scene_widget.cpp` (overlay part), `spk_debug_overlay`, `spk_profiler`.

- `--map-only`: generate plan → print `report()` → `writeWorldMapPng()` renders the
  plan (zones shaded by height, water, roads, stair rects, entity label boxes with a
  built-in 5×7 pixel font) to `Playground/world_map.png`.
- `--check-stairs`: `checkComposedStairways()` re-detects composed staircase groups
  from the flat placement list (strict lattice test), then walk-tests each one by
  sampling realized voxels through a private chunk cache (`VoxelSampler`): top
  platform flush against exactly one cliff side, middle column descends ≤1 per cell,
  approach band flat and (for road climbs) paved.
- In game: F7 toggles the `DebugOverlay` (camera/player/hovered cell, loaded chunks,
  update/render/delta timings) plus one row per `spk::Profiler` probe (max/avg/min),
  fed through `UpdateContext::profiler`.
