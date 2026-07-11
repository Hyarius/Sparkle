# System — World (chunks, streaming, maps, procedural generation)

The persistent 3D voxel world: chunk storage and streaming, authored maps, actors moving in
it, and (late phase) the two-layer procedural generator.

Diagrams: [04-class-world.puml](../diagrams/04-class-world.puml),
[21-seq-exploration-movement.puml](../diagrams/21-seq-exploration-movement.puml).
Plan: steps [06](../plan/steps/step-06-world-chunks.md),
[07](../plan/steps/step-07-player-exploration.md), [29–31](../plan/implementation-plan.md).
JSON: maps §13, biomes §12, worldgen §14 of [02-data-model.md](../02-data-model.md).

---

## Responsibilities

- Store voxels in **16×16×16 chunks** (D11) addressed by `ChunkCoordinates` (int3); provide
  seamless cell access across chunk borders (`VoxelWorld::cell(worldPos)`).
- Load/unload chunks around the player; own one render-mesh entity per loaded chunk.
- Load authored **maps** (M1 testground, interiors) and stamp **prefabs** into them.
- Maintain the **world traversal graph** region around the player for click-to-move (D27),
  and re-derive affected regions when voxels change.
- (Phase 8) Generate the world from the seeded **macro plan** → per-chunk realization.

## Key types (`Playground/srcs/world/`)

```
pg::ChunkCoordinates { Vector3Int };  fromWorldCell(cell); worldOrigin()

pg::Chunk : spk::Component, spk::SynchronizableTrait          (D32)
    static constexpr spk::Vector3Int Size{16, 16, 16};        // compile-time (D29)
    ChunkCoordinates coords;
    VoxelGrid grid;                       // Size³
    // baked products (rebuilt by _synchronize()):
    Mesh3D renderMesh;
    Mesh3D maskMesh;                      // persistent world decals (empty until used)
    TraversalGraph::Slice navSlice;       // this chunk's contribution to world navigation
    setCell(local, VoxelCell)             // writes grid + requestSynchronization()
    _synchronize() override               // remesh + rebuild navSlice (needs neighbours →
                                          //  resolved through the owning VoxelWorld)
pg::ChunkSynchronizationLogic (spk::ComponentLogic<Chunk>)    // the "baker":
    update phase: for each chunk with needsSynchronization(): synchronize()
    // border edits also requestSynchronization() on the touched neighbour chunk
    // (occlusion + navigation cross borders — VoxelWorld::setCell handles the fan-out)
pg::ChunkRenderLogic (spk::ComponentLogic<Chunk>)             // render phase:
    emit one draw per chunk renderMesh (voxel atlas) + maskMesh when non-empty

pg::VoxelWorld       // chunk container + accessors (owns chunk entities)
    cell(Vector3Int worldCell) -> const VoxelCell&        // empty-cell sentinel outside
    setCell(worldCell, VoxelCell)                         // routes to chunk(s), fans out
                                                          //  requestSynchronization()
    forEachLoadedChunk(fn)
    loadFromMap(const MapDefinition&)                     // fills + cells + prefab stamps
pg::WorldStreamer    // decides the loaded chunk set (radius around player), fires load/unload
pg::WorldNavigation  // world traversal graph assembled from chunk navSlices (shared graph
                     // machinery with the board, see board.md)
pg::WorldRaycaster   // cell DDA + oriented shape-polygon intersection: surface hit/normal/distance
pg::Actor / pg::ActorPathLogic                             // see §Actors
pg::WorldContext     { seed; active VoxelWorld; navigation; active map info; portals }
```

The promoted runtime chunk type is `spk::VoxelChunk`. Its `grid()` view is always const.
Runtime and generated writes use `setCell(...)` or a short-lived `editCells(Editor&)`
callback; the editor exposes setters and const reads, never mutable grid/cell aliases. A
bulk callback coalesces all writes into one dirty request, and a map-owned chunk reports the
exact changed boundaries so only affected loaded neighbours rebake. Voxel writes belong to
the chunk's construction/update thread and finish before the render logic starts and joins
background mesh builds.

`FluidSimulationLogic` keeps persistent sources and the moving frontier as separate fair
work streams. A rotating source cursor alternates with the insertion-ordered frontier while
both have work, and candidate processing never exceeds `maxCellsPerTick`. Source chunks are
sorted explicitly and out-of-range chunks are rejected before their source vectors are
visited, making a fixed world/configuration/tick sequence reproducible without allowing a
fresh source prefix to starve deferred flow.

One `spk::Entity` per loaded chunk carrying its `Chunk` component — the chunk **is** the
data package; there is no separate view component (D32). For M1 the whole
`maps/m1-testground.json` map is loaded up front (64×16×64 = 4×1×4 chunks — "streaming"
trivially keeps everything loaded); the streamer becomes real in the world-gen phase. Build
the streamer interface in step 06 anyway (loaded-set decided by radius), so generation later
plugs into `provideChunk(coords)` without touching callers.

### Chunk providers

```
pg::IChunkProvider { virtual void fill(Chunk&) = 0; }
  ├─ MapChunkProvider        // slices a loaded MapDefinition (M1, interiors)
  └─ GeneratedChunkProvider  // Phase J: queries the MacroWorldPlan
```

## Actors & click-to-move (D27)

- `pg::Actor` (data component on an `Entity3D`): current world cell, facing, move speed,
  optional current path (list of cells + interpolation t).
- Click → `ExplorationMode` asks `pg::MousePicker` (see
  [rendering-cameras.md](rendering-cameras.md)) for the hovered standable cell →
  `EventCenter.actorMoveRequested{actor, targetCell}`.
- `pg::ActorPathLogic` (ComponentLogic<Actor>): on request, runs A* on the world traversal
  graph (`pg::Pathfinder`, shared with the board — see [board.md](board.md)); stores the
  path; each update advances interpolation along cell-to-cell segments using the cells'
  **stationary/edge walk heights** (smooth Y over slopes); on each cell reached fires
  `EventCenter.playerMoved(cell)` (for the player actor) — which encounter emission listens
  to. A new click retargets from the current cell. Unreachable target = brief "invalid"
  feedback (cell flash), no movement.
- The **follow camera** orbits the player (yaw/pitch via ZQSD or right-mouse drag, zoom via
  wheel) — a `CameraControllerLogic` behavior, not part of the actor.

## Interiors & portals (D37)

Spaces connect through named **portals**: each portal is `{name, cell, target:{map,
portal}}`. Stepping onto a portal cell loads the target map (its own `VoxelWorld`) and
places the player at the **target portal's** cell — still ExplorationMode, the one
legitimate world swap (D03). Because links are portal→portal, multi-exit interiors are
natural: a tunnel map has `west` and `east` portals bound to two different overworld doors;
no "return to entry position" state exists anywhere.

Interior sources:
- **hand-authored maps** (houses, gyms) — `maps/*.json`, portals authored in the file;
- **generated interiors** (caves/tunnels, step 31) — built in memory by the generator
  (CellularAutomata carving + prefab dressing), portals bound at generation time to the
  overworld features that spawned them (a tunnel's two entrances). The macro plan records
  the binding so re-entry is deterministic per seed.

`pg::PortalService` owns the transitions: it watches `playerMoved`, matches portal cells of
the active map, performs the world swap (load target map if needed via `WorldContext`,
reposition player, rebuild navigation focus). Overworld-side doors are themselves portals
recorded by the map/generator. Deferred to step 24 (authored) / 31 (generated).

## Procedural generation (Phase 8 — steps 29–31)

Two-layer model, from the Unity `WorldGenerationIdeas.md` archive, mapped onto Sparkle math
primitives (`spk::Perlin`, `spk::PoissonDisk`, `spk::CellularAutomata`):

**Layer 1 — `pg::MacroWorldPlan` (generated once per seed, saved with the run):**

1. *Landmass:* radial falloff + `Perlin.sample2D` distortion + weaker detail pass →
   land/water mask over `worldgen.worldSize`.
2. *Height & rivers:* Perlin height on land, smoothed; rivers descend from high cells to the
   sea (greedy downhill walk with jitter).
3. *Major cities:* `spk::PoissonDisk` candidates filtered by flatness/land/min-spacing (+
   coastal bias) until `majorCount` remain.
4. *Biomes:* shuffle `worldgen.biomes` with the seed; assign one per city; weighted Voronoi
   growth over land cells (distance / city weight), borders noised.
5. *Satellites:* per city, `satellitesPerCity` picks on good terrain within
   `satelliteRadius`, same biome.
6. *Transport graph:* MST over cities + `extraEdges` shortest non-tree edges; villages
   connect to nearest hub. Classify each edge by what its path crosses: land road / bridge
   (narrow river) / port+sea (wide water) / tunnel (mountain cost > `tunnelTriggerCost`).
7. *Roads:* A* per edge on a terrain cost map (flat cheap, slope dear, water forbidden);
   bridge/tunnel/port segments recorded as plan features, not carved voxels.

Plan data: masks + height field + city/settlement/road/feature lists, all queryable by cell.

**Layer 2 — realization (`GeneratedChunkProvider`):** per chunk, for each column: biome →
palette (surface/subsurface/deep voxels), height from the plan (slopes/stairs smooth 1-cell
steps along roads), road overlay voxels, river water, flora scatter (seeded per-column hash;
bushes obey biome `encounterRules` density), settlement/gym footprints stamp **prefabs**
(`prefabs/*.json`), tunnel entrances stamp their prefab and bind its `door` anchor to the
generated interior's portal (D37). Fully deterministic: same seed + coords ⇒ same chunk, no
cross-chunk order dependence.

## Dependencies

Uses: voxel core, `spk::Perlin/PoissonDisk/CellularAutomata`, registries, EventCenter,
rendering layer. Used by: exploration mode, board derivation, encounter emission, save
(seed + respawn), tools (world editor drives `VoxelWorld::setCell`).

## Testing

Headless: chunk addressing (world↔chunk↔local coords round-trip), cross-border cell access,
map loading (fills/cells/markers), path driver cell events, provider determinism (same seed
⇒ identical chunks), macro-plan invariants (8 cities on land, min spacing, all settlements
connected, every edge classified). Visual: M1 testground composition; later, macro-plan
debug view (a top-down image dump of masks/biomes/roads — cheap and invaluable; build it with
step 29).
