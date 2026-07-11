# Level 1 — Big picture

> One paragraph per chapter. Same numbering as [level-2-systems.md](level-2-systems.md)
> and [level-3-details.md](level-3-details.md).

## 1. Startup & data loading

`main()` parses a few CLI flags, then loads **every JSON data file** under
`resources/data/` into `pg::Registries`: voxel definitions, biomes, prefabs, interior
recipes, placement rules, and game rules. Loading also *generates* content that is not
authored on disk: fluid fill-stage voxels and per-biome staircase prefabs. After this
point all game data is immutable and addressed by string or numeric id.

## 2. World plan generation

`generateWorldPlan()` produces the **world skeleton** for a seed: a ~124×124 grid of
macro cells where each cell knows if it is land, which zone/biome it belongs to, its
height level, and whether it carries water, a road, or a bridge. On top of the grids it
places entities (gyms, cities, POIs), roads connecting them, staircases wherever a road
or wild cliff crosses a height step, buildings with composed interiors, and scenery.
The output — a `WorldPlan` — is a pure data snapshot; nothing here touches voxels yet.

## 3. Prefabs & climb-prefab synthesis

A `spk::Prefab` is a reusable, rotatable batch of voxels (empty entries carve air).
Prefabs come from two sources: **authored JSON** (`resources/data/prefabs/*.json` —
trees, houses, POIs, interior rooms) and **synthesis at load time** — each biome gets
staircase "flight" and "platform" prefabs generated from its palette voxels, so climbs
match the biome's materials.

## 4. World realization

`PlanChunkProvider` turns the plan into actual voxels, one 16³ chunk at a time, on
demand: for every column it derives ground height and material layers from the plan
cell (biome palette, road strip, river carving, ocean), then stamps every prefab
placement whose box intersects the chunk. The same input always produces the same
chunk — chunks are never saved, only regenerated.

## 5. Scene & streaming runtime

`GameSceneWidget` builds the whole scene: generates the plan, creates the
`VoxelWorld`/`spk::VoxelMap`, warms the chunks around the spawn, spawns the player and
camera, and registers the update/render logics. Each frame, the player's chunk window
drives the `spk::VoxelChunkStreamer` (chunks load/unload around them), and stepping on
a door cell teleports the player into a composed interior placed in a void band east
of the world.

## 6. Rendering

The spk voxel module renders the map: each chunk owns a `VoxelChunkRenderer` that
bakes its grid into an opaque + transparent mesh pair via the **occlusion mesher**
(faces hidden by neighbors — including across chunk boundaries — are culled). A worker
pool re-bakes dirty chunks each frame, then two render logics submit the geometry:
opaque first, transparent last (back-to-front) so water blends over the scene.

## 7. Navigation & movement

`WorldNavigation` maintains a **traversal graph** over a bounded box around the player
(rebuilt whenever the world revision changes) using per-voxel walk heights from the
gameplay registry. Left-click raycasts into the world, pathfinds over the graph, and
`ActorPathLogic` walks the actor along the path; the camera orbits/zooms around it.

## 8. Fluid simulation

Worldgen only places fluid **source** blocks. `FluidSimulationLogic` (a cellular
automaton budgeted per tick, near the player only) makes them flow: fluid falls into
empty space, and when resting on solid ground spreads sideways getting one "stage"
thinner per cell until its `maxSpread` runs out. The stages are voxels generated at
load time (chapter 1); flow is transient and regenerates after chunk reload.

## 9. Headless tools & diagnostics

`SparklePlayground --map-only` generates a plan and writes `world_map.png` without a
window; `--check-stairs` re-derives every composed staircase from the plan and
walk-tests it voxel by voxel. In game, F7 toggles a debug overlay with positions,
chunk counts, frame timings and the profiler table (bake timings etc.).
