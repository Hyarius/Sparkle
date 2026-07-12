# Level 3 — Deep dive

> Function-level detail. Same numbering as [level-1-overview.md](level-1-overview.md)
> and [level-2-systems.md](level-2-systems.md). Line references are as of `23a107c`.

## 1. Startup & data loading

### 1.1 JSON plumbing (`core/json.hpp`)

Since 2026-07-11 (decision D42) the implementation lives in the library as
`spk::JSON::Reader`/`Error`/`Loader` (`structures/container/spk_json_reader.hpp`);
`core/json.hpp` only aliases them into `pg::`.
`JsonLoader::parseFile` wraps `spk::JSON::Value` parsing. `JsonReader` carries the
file + JSON path for error messages, and offers `require<T>` / `optional<T>` /
`requireEnum` / `child()` / `contains()` / `forbidUnknown({...})` — every parser uses
`forbidUnknown` so a typo in a data file fails loudly with its exact JSON path
(`JsonError{file, "$.shape.textures.top", message}`).

### 1.2 Generic registry (`core/registry.hpp`)

`Registry<TDefinition>::load(directory, parseFunction)` enumerates `*.json`, sorts by
filename (deterministic ids), parses each through the supplied function, rejects
duplicate ids. `ids()` returns sorted string ids; `get`/`tryGet`/`getMutable` do the
rest. All app registries (`biomes`, `prefabs`, `interiors`, and the discovery step of
voxels) reuse it.

### 1.3 Voxel loading (`voxel/voxel_parser.cpp`, `voxel/voxel_registry.cpp`)

`parseVoxelDefinition` reads `version`, `traversal` (passable|solid), `tags`,
`transparency`, and a `shape` block whose `type` selects the `spk::*VoxelShape`
subclass and the walk-height formulas (`CardinalHeightCollection`, ported verbatim
from the historical pg shapes so navigation behaves identically):

| type | shape | notes |
|---|---|---|
| `cube` | `CubeVoxelShape` | `side` or per-direction textures |
| `fluid` | `CubeVoxelShape` + `FluidData` | `maxSpread` 1–16, top texture reused for stages |
| `cuboid` | `CuboidVoxelShape` | unit-box min/max validated |
| `slab` | `SlabVoxelShape` | height 0–1 |
| `slope` | `SlopeVoxelShape` | cardinal top heights `{.5,.5,1,0,.5}` |
| `stair` | `StairVoxelShape` | stepCount 1–64 |
| `crossPlane` / `cross` | diagonal / axis quads | no walk heights |

`VoxelRegistry::load` then registers every parsed voxel as ONE type through the spk
registry's batch `registerType()` (type/state/runtime ids all dense, in registration
order) and mirrors it in the gameplay table. For each fluid: the authored source is
state `0` and stages `1..maxSpread` become slab states (`fill = k/maxSpread`, the
last one full height); all states share the type's traversal/tags/transparency and
`setTransparentOcclusionGroup(sourceId)` so the whole family culls internal faces
together. No synthetic string ids exist anymore (debug formatting shows
`water@source` / `water@3`). `FluidRef` entries map every runtime id (source
included) back to (family, stage, isSource).

### 1.4 Biomes (`world/biome_definition.cpp`)

`parseBiomeDefinition` reads `displayName`, a `palette` of weighted voxel pools
(`surface`, `subsurface`, `deep`, `road`, `stair`, `slope`, `flora` — each entry
either `"id"` or `{"id": ..., "weight": ...}`, voxel ids validated against the
registry) and an optional `worldgen` block: `heightShift`, `peak`, `mapColor` (hex
string → `spk::Color`), `scenery` array (prefabId, density, spacing), and per-entity
prefab pools. Biomes without `worldgen` (cave) never enter the generator.

### 1.5 Prefab & interior parsing (`world/prefab_parser.cpp`, `world/interior_parser.cpp`)

`parsePrefabDefinition` reads a `voxels` array of entries (`at` or `from`/`to` box,
`voxel` id or weighted pool, `orientation`, `flip`, or `"empty": true` to carve),
optional `pivot`, optional `clearance` box, optional `anchors`
(`{name, at}`), optional `interior` id. Cell picks from pools are deterministic
per-position (hash of prefab id + position). The result is a `spk::Prefab` (bounds
deduced, never declared) plus the app metadata.

`parseInteriorDefinition` reads `entryPrefabId`, `minRooms`/`maxRooms`, and a
weighted `rooms` list (`InteriorRoomOption{prefabId, weight}`).

### 1.6 Load order (`core/registries.cpp`)

game-rules → voxels → biomes → prefabs → `synthesizeClimbPrefabs(prefabs, biomes,
voxels)` (mutates biome worldgen traits with the generated pool ids) → interiors →
placement rules (which validates prefab ids, so it loads after synthesis).

## 2. World plan generation

The `Generator` struct (declared in `world/generator/world_plan_generator.hpp`,
namespace `pg::worldgen`) holds the growing `WorldPlan` plus scratch state:
`continents`, `zoneContinent`, `borders`, `distOcean`, `hardClaims`,
`stairCells`, `stairFootprints`, `interiorSlotsUsed`, and three RNG streams
(`roadRng`, `prefabPickRng`, plus per-stage `rngFor(path)`). Its stage
implementations are split over topic files (2026-07-11):

| File | Contents |
|---|---|
| `world_plan_math.{hpp,cpp}` | `Cell`, `Rng`, `deriveSeed`, noise, BFS helpers |
| `world_plan_terrain.cpp` | graph, landmass + zones, heights, water (2.1–2.5) |
| `world_plan_infrastructure.cpp` | gateways, entities, ports, roads, boats, bridges, buildings (2.6–2.10, 2.12) |
| `world_plan_stairways.cpp` | claims, stair candidates, stair placement (2.11) |
| `world_plan_interiors.cpp` | room composition and portals (2.12) |
| `world_plan_scenery.cpp` | biome dressing (2.13) |
| `world_plan_generator.cpp` | orchestration (`run`), stats, `planBiomesFrom`, `report`, `generateWorldPlan` |

### 2.0 Shared helpers (`world_plan_math.{hpp,cpp}`)

- `deriveSeed(master, path)` — FNV-1a over `"<master>::<path>"`.
- `Rng` — mt19937_64 with `uniform()`, `below(n)`, `poisson(mean)`, `shuffle`.
- `valueNoise(rng, size, scale)` — bilinear-smoothstep value noise in [0,1].
- `distanceTo(mask)` — multi-source BFS distance.
- `farthestPointSeeds(candidates, k, rng)` — greedy farthest-point sampling.
- `labelComponents(mask, count)` — 4-connected component labelling.
- `countDiagonalOnly(mask)` — cells connected only diagonally (invariant checks).
- quarter-turn ↔ `spk::VoxelOrientation` arithmetic for claim rotation — since
  2026-07-11 (decision D41) provided by the library
  (`structures/voxel/spk_voxel_orientation.hpp`), no longer defined here.

### 2.1 buildWorldGraph

Shuffles biome indices and progression ranks; zone *i* gets
`biomeIndices[i % biomeCount]` and a shuffled progression.

### 2.2 buildLandmass + dropTinyIslands

`field = (1 - radialDistance) + coastNoise*(noise-0.5) + 0.25*(fine-0.5)`; if
`fragmentation > 0`, subtracts 1–3 gaussian "channel" troughs at random angles with
wobbled distance. `land = field > landThreshold`. Islands under
`max(20, 1% of area)` cells are dropped (keeping at least the biggest). Continents
labelled; `distOcean = distanceTo(!land)`.

### 2.3 assignZones + enforceContiguousZones + mapZoneContinents + findBorders

Farthest-point seeds over land; per-cell warped (±12% value noise) nearest-seed
assignment. Two passes reattach non-main zone fragments to a bordering zone. Each
zone maps to its majority continent. `borders[(zoneA,zoneB)]` collects boundary
cells.

### 2.4 assignHeights

Peak cells: `~2*(size/124)²` picks in peak-flagged zones, restricted to the 40% most
inland cells of the zone. Then per land cell:
`level = floor(coastTrend + undulation + peakLift*weight + biomeShift)` clamped to
`[0, maxHeightLevel]`, where `coastTrend = coastLevels*(1-exp(-dCoast/reach))`,
undulation is 3 octaves of value noise scaled by `undulationLevels`, `peakLift`
decays over `cellsPerLevel*maxHeight*0.7` cells with pow 1.5, weight 1.0 in peak
biomes / 0.25 elsewhere. Cells ≤1 from the coast are forced to level 0.

### 2.5 generateWater

- `effective = height + jitter` (ocean = −1e6).
- **Priority flood**: ocean-seeded min-heap fills depressions; `receiver[cell]`
  records each cell's downstream neighbor on the filled surface.
- **Lakes**: cells with `filled-effective ≥ lakeMinDepth`, component-labelled;
  components ≥ `lakeMinSize` kept; oversized ones trimmed to a `lakeMaxSize`-cell
  core grown depth-first from the deepest cell. Marked `water` + `lake`.
- **Rivers**: per zone, `riversPerZone` sources drawn by farthest-point sampling
  from the highest inland candidates, then each walks `receiver` links until sea /
  existing water; the walked channel becomes `water`.

### 2.6 resolveGateways

Per zone-pair border: the cell nearest the border centroid becomes a primary
gateway; borders > 8 cells have a 50% chance of a secondary gateway at the farthest
cell.

### 2.7 placeEntities + assignPorts

Per zone: candidate cells = in-zone, dry land. `sample(kind, count, block,
distRatio, coastRule)` shuffles candidates and accepts cells respecting (a) blocking
radius vs every placed entity, (b) settlement-to-settlement spacing
`distRatio * zoneRadius`, (c) the coast rule. Gyms: inland only, spacing relaxed
by halving until placed. Cities: inland first, then anywhere. POIs (rare/uncommon/
normal): anywhere. `assignPorts` promotes each continent's most coastal cities
(within `2*coastDistCells`) to port cities.

### 2.8 buildRoads + findPath + stepCost + removeRoadSquares

Hub = the zone's gym (fallback: first settlement). Roads: hub → each settlement
(nearest first), hub-adjacent roads → `poiRoadConnections` random POIs (attached at
`nearestRoadCell`), then hubA → gateway → hubB for primary gateways.

`findPath` is A* with octile heuristic; diagonal moves are only legal through a
passable orthogonal *elbow* cell and are materialized as two orthogonal steps at
reconstruction (`elbow[]`), so emitted paths are strictly 4-connected.
`stepCost`: ocean blocked; `wouldFormRoadSquare` blocked (a cell is refused when 3
other corners of any containing 2×2 are already road); water +6 (bridge); height
step `dh` +7·dh, `dh > maxComposedStairLevels` blocked; existing road ×0.25.
`removeRoadSquares` erases the redundant corner of any surviving 2×2 (only when its
two outside neighbors are non-road, so connectivity is preserved through the L).

### 2.9 addBoatLinks

Labels road components. Each component needs ≥1 port city — port-less components
promote their most coastal city. Component #1 is the base; every other component
links its nearest port pair to it (`boatLinks`), making the road graph logically
one component (checked in `report()`).

### 2.10 markBridges

`road && (water || !land)` ⇒ `bridge`.

### 2.11 Stairways (`world_plan_stairways.cpp`) — the intricate part

Concepts:

- **Claims** (`Claim`, `hardClaims`, `claimBoxFor`, `zoneIsFree`) — world-space
  AABBs. `claimBoxFor` rotates the prefab's clearance (or content bounds) around
  its pivot by the placement's quarter-turns and translates it to the resolved
  destination. Priority: stairways claim unconditionally; buildings and scenery
  test-then-claim.
- **Footprints** (`StairFootprint`, `stairFootprintOf`, `stairFootprintFits`) —
  XZ rectangles used only between stairways and against terrain:
  every covered plan cell must be land at the low level, dry, un-bridged, without
  entity; plus a road rule — `Require` (straight road ramps), `Allow` (composed
  road climbs may leave the road but stay in-zone), `Forbid` (wild climbs).
- `resolveBox` mirrors `PlanChunkProvider`'s placement maths so plan-side
  reasoning matches the voxels that will be stamped (centered footprint vs
  pivot-anchored).

`emitStairChain(row, col, dr, dc, onRoad)` handles one cliff edge between two
neighboring cells (levels differ):

1. `steps = highLevel - lowLevel`; reject if > cap (`maxComposedStairLevels` /
   `maxWildStairLevels`).
2. **steps ≥ 2 — composed staircase.** Local frame (`StairSite`): *across* = axis
   perpendicular to the cliff, *along* = axis beside it. The flight hugs the cliff
   wall on the low side: top platform at `highSurface+1` padding the road strip of
   the crossing, one 3×3×3 flight per level descending along the wall, bottom
   platform at `surface+1`. Each layout builder (`makeOnePassCandidate`,
   `makeSwitchbackCandidate`, tried for 2 tangents × 3 cross offsets each) shapes
   a `StairCandidate`:
   - the placements (`anchorToPivot` platforms/flights, all `foundation`),
   - an unstamped checked **approach band** (3 columns beside the structure, the
     walkway from the road dead-end to the bottom platform),
   - the 3 reserved **exit cells** on the high plateau (must be level, dry land;
     reserved so two flights never meet face-to-face),
   - one claim box over the whole structure + band (low ground to above the top
     platform) and one over the exit.
   `tryCommitStairCandidate` — the single commit gate for every layout —
   re-validates every footprint, then commits: appends the placements as one
   contiguous run, the claims, `stairCells` (spacing), and pushes the
   `PlanStairway` record carrying the footprints (painted road-colored on the
   map) and, for road climbs, the paved approach band. First fitting layout wins.
3. **Fallback — straight ramp** (`makePerpendicularCandidate`). One centered
   flight per level, stacked against the boundary, orientation facing uphill; road
   rule `Require`/`Forbid`. Used for 1-level steps and as fallback for 2-level
   road steps when composition fails (`maxRoadStairLevels`). Goes through the same
   commit gate, so one-level ramps get a `PlanStairway` record too.

`placeStairways` scans every road cell's east/south road neighbor with a height
step. `placeWildStairways` collects per-zone off-road cliff candidates, shuffles,
enforces Chebyshev spacing `wildStairSpacingCells` against every stairway, places
up to `wildStairsPerZone`.

### 2.12 Buildings (`world_plan_infrastructure.cpp`) & interiors (`world_plan_interiors.cpp`)

`placeBuildings`: for each entity with a pool (biome override first, then
placements.json), anchor = cell center at `surfaceY(level)+1`, try 9 nudges (center,
±1/±2 on each axis) until the claim box is free. Vital kinds (gym/city/port) are
placed even on conflict (warning); plain POIs are skipped (`skippedPoiPlacements`).

`composeInterior(buildingPlacement)`: needs the building's `interiorId`, `door`
anchor, and the interior's entry room with `entry`/`exit` anchors. Reserves void
slot #N east of the world (`interiorRegionMinX() + slot`, ground at
`surfaceY(0)+1`). Places the entry room at the slot center, then up to 64 attempts
to grow `minRooms..maxRooms` extra rooms: pick a random open connector
(`connector:±x/±z` anchors), pick a weighted room having a mate connector facing
back, place it wall-against-wall (`destination = mateWall - (mateAnchor - pivot)`),
reject if outside the slot or colliding with a placed room, else stamp an
`interior-doorway-{x,z}` carve prefab over the two facing wall cells and retire
both connectors. Finally two one-way portals: door cell → entry pad, exit pad →
one cell outside the door (outward = dominant door-offset axis).

### 2.13 placeScenery (`world_plan_scenery.cpp`)

Per zone: candidate cells = in-zone dry land, no road/entity/stairs. Per cell, each
scenery entry rolls `poisson(density)` requests; each request gets 8 attempts: a
random rotation + position inside the cell, footprint must stay in-zone, same
height, off-road, dry; center-distance spacing against previously placed scenery
(`placedScenery` map + `maximumSpacing` search radius); claim must be free
(scenery yields to structures). Placement stored with random orientation.

### 2.14 computeStats + report (`world_plan_generator.cpp`)

Counts land/road/river cells, gyms too close to coast, diagonal-only road/river
steps, road 2×2 squares (must be 0), gateways, boat links, road components. The
report prints all quotas and invariants, flagging violations inline.

### 2.15 Validation (`world_plan_validation.cpp`)

`validateWorldGenConfig` checks every field's range (and inter-field constraints:
`lakeMinSize ≤ lakeMaxSize`, `blocksPerLevel ≤ blocksPerCell`, level arithmetic
fitting int8, world width fitting int...) and throws `std::invalid_argument`
naming the field — called from `generateWorldPlan` and pre-flight in `main`.

## 3. Prefabs & climb synthesis

### 3.1 `spk::Prefab` (library)

- `addVoxel` / `addVoxelRange` grow deduced bounds; the grid constructor lists
  *every* cell including empties (whole-box overwrite semantics).
- `forEachAppliedVoxel(destination, orientation, visitor)` yields
  `destination + rotate(position - pivot)` per voxel in insertion order; non-empty
  cells get their own orientation composed with the stamp rotation.
- `applyTo(grid, destination, orientation)` stamps with out-of-bounds skipping.
- `rotatedBounds(orientation)` rotates the two corners around the pivot and
  re-sorts min/max.

### 3.2 Climb synthesis (`climb_prefabs.cpp`)

`makeFlight(base, steps, rng)`: 3×3×3 `VoxelGrid` — base fill at (x,0,1), (x,0,2),
(x,1,2); step blocks at (x,0,0), (x,1,1), (x,2,2) — every cell drawn from its
weighted pool; converted to a whole-box `spk::Prefab` (empties carve the air over
the ramp). `makePlatform`: 3×3 at y=−1 from the base pool, clearance
`{0,-1,0}..{2,3,2}`. `buildCategory` registers `kFlightVariants = 8` variants
(1 if both pools are single-entry), each seeded by FNV-1a of its own id so content
is stable per data set. Road category uses (road pool, stair pool); wild category
(surface pool, slope pool).

## 4. World realization

### 4.1 Column derivation (`plan_chunk_provider.cpp::_column`, 150)

Order of decisions per (x,z):

1. interior band (`x ≥ interiorRegionMinX`) → empty column;
2. ocean cell → `groundTop=1` sand, stone deep, water at y=2;
3. else: `groundTop = surfaceY(height)`, surface/subsurface/deep ids picked by
   `pickVoxel` (per-column avalanche hash of (x,z,seed,salt) into the weighted
   pool — deterministic, pattern-free);
4. lake → carve 2 down, water at `surface-1`; river → same but only inside the
   3-wide **cross** (center strip + arms toward wet neighbors);
5. road → inside the road cross: surface = road pool pick, `waterY = -1`
   (bridge decks fill their pier);
6. paved stair approach bands (linear scan over `stairways` records with a
   `pavedApproach`) override the surface id.

### 4.2 fill + stamping (266)

Chunks fully below y=0 stay empty. `editCells` writes each column: depth 0 =
surface id, depth ≤ `subsurfaceDepth` (3) = subsurface, deeper = deep id; `waterY`
adds one water cell. Then every `ResolvedPlacement` whose box intersects the chunk
(with a 32-cell downward margin for foundations) is stamped inside one more
`editCells`: foundation pillars fill `column.groundTop+1 .. worldMin.y-1` with the
column's deep id, then `Editor::applyPrefab` stamps the rotated prefab (empties
carve).

### 4.3 spawn (376)

Home = gym of the zone with the lowest progression. Spiral over Chebyshev rings
radius ≥ 2 for a road, non-bridge, entity-free cell; spawn at its center column,
y = `surfaceHeight`.

## 5. Scene & streaming runtime

### 5.1 Construction order (`game_scene_widget.cpp::_buildScene`, 159)

The exact sequence matters: logics are registered before entities so priorities
sort once; the world is staged in `_stagedWorld` and only published to
`GameContext` after every throwing step has passed (commit-at-the-end pattern,
see ticket 001); teardown order in the destructor mirrors it (navigation →
world → provider) because the map unregisters chunk entities from the engine.

Engine logic order: `VoxelChunkStreamerLogic` (priority 100) →
`VoxelChunkRenderLogic` → `TextureMeshRenderLogic` (actors) →
`VoxelChunkTransparentRenderLogic` (lowest, draws last). `FluidSimulationLogic`
registers at priority 50 (after streaming, before baking).

### 5.2 Frame update (385)

`_onUpdate`: streamer origin ← player chunk; if the player crossed a chunk,
`navigation->resetBounds` (±48 cells, y 0..64); engine update; pending portal
teleport (`_executeTeleport`: warm 17×3×17 chunks at destination, reset actor path,
recenter streamer, shift camera rig); duration probes; overlay refresh.

### 5.3 Streaming (spk)

`VoxelChunkStreamer` (component): origin + inclusive view range, validated against
overflow and a per-update chunk budget. `VoxelChunkStreamerLogic`: on first drive
of a map, binds the map's mutation thread; each update it unions all streamers'
windows, diffs against its per-map owned set (lifetime-token guarded), loads or
re-activates entering chunks (budgeted), deactivates or unloads leaving ones
(depending on `retainInactiveChunks`), and repairs owned chunks that were manually
deactivated.

`VoxelMap::chunk()` creates + generates on first access (generator callback =
`PlanChunkProvider::fill`); `setCell` routes through the owning chunk's editor;
boundary edits mark the touching neighbor chunks dirty
(`_requestNeighborSynchronization`). `applyPrefab` pre-loads every overlapped
chunk then stamps per-chunk.

## 6. Rendering

### 6.1 Shape initialization (`spk_voxel_shape.cpp`)

`initialize()` builds, per face: the 8 transform variants (4 orientations × 2
flips) with rotated positions and remapped normals, plus boundary metadata per
axis plane: does the outer face lie exactly on the cell boundary
(`BoundaryEpsilon`), does it fully cover it, and its fractional coverage
(polygon-area sum on the unit square). Texture slots resolve through `atlasUV`
(8×8 atlas by default).

### 6.2 Mesher (`spk_voxel_mesher.cpp`)

Two passes over the grid:

1. **Plan** (`planVisiblePolygons`): for every non-empty cell and every face,
   decide against the neighbor (in-grid or via `IVoxelCellLookup` in world
   coordinates): an opaque neighbor face that covers the shared plane culls the
   face (`faceOcclusionByNeighbor`); partial cover triggers 2-D polygon
   subtraction on the boundary plane (`subtractConvex` → `clipHalfPlane`, convex
   clipping with epsilon-cleaned output); transparent faces cull only within the
   same `transparentOcclusionGroup` (or same shape instance) so unrelated
   materials keep their interface. Inner faces are suppressed when the cell is
   sealed (`cellIsFullyEnclosed`) — only probed for shapes that actually own
   inner faces, so full cubes never pay it. The plan stores descriptors
   (polygon pointer + offset + alpha), never polygon copies.
2. **Emit** (`MeshEmitter`): counts first (overflow-checked `checkedAdd`), then
   fills vertex/index buffers; opaque and transparent polygons go to separate
   meshes; transparent alpha = 1 − shape transparency.

Throughput structures (all immutable or per-bake, safe under parallel
`WorkerPool` bakes; output pinned byte-for-byte by
`voxel_mesher_invariance_test.cpp`):

- `spk::VoxelWorldToLocalPlaneTable` (`spk_voxel_orientation.hpp`): compile-time
  48-entry table of `mapWorldPlaneToLocal` (which stays the source of truth,
  truth-table tested) over `(orientation, flip, world plane)`; the hot path
  indexes it instead of re-running the quarter-turn arithmetic.
- `spk::VoxelFaceRemnantCache` (`spk_voxel_mesher_occlusion.hpp`): per-bake
  memoization of partial-occlusion remnants keyed on the
  `(source face, occluder face)` pointer pair — a recurring adjacency
  (slope-against-cube…) is clipped once per bake instead of once per cell. The
  cached remnants also back the plan descriptors for clipped faces.
- Neighbor snapshot: in-bounds reads are flat-index reads; external cells sit in
  six per-face boundary slabs (flat buffers sized to the face area) sampled once
  from the world lookup before planning — no hashed world-position map.

### 6.3 Chunk bake pipeline

`VoxelChunk::editCells` → `_commit` → renderer `requestSynchronization` (+
neighbor invalidation by boundary bits). `VoxelChunkRenderLogic` each update:
prune unloaded chunks from cache; collect dirty renderers
(`beginMeshSynchronization` claims); dispatch mesh builds to the `WorkerPool`
(probe "voxel chunk render/chunk"), join (probe ".../batch"); each result
published via `publishMesh` — a brand-new `shared_ptr<VoxelRenderMeshes>` pair so
in-flight render commands keep drawing the old mesh. Draw commands are cached per
chunk and rebuilt only when `meshRevision()` moved (probe ".../assembly").
Transparent pass: same cache, sorted back-to-front by camera distance, submitted
after everything opaque.

## 7. Navigation & movement

### 7.1 Graph build (`board/traversal_graph_builder.cpp`, `voxel/voxel_traversal.cpp`)

For every column in bounds, standable cells = solid voxel below (per its cardinal
walk height) with enough passable clearance above. Nodes connect to 4-neighbors
when the height difference between the two walk surfaces ≤ `maxVerticalGap`
(game-rules; stairs/slopes expose intermediate cardinal heights so 1-voxel climbs
happen on their shapes). `WorldNavigation::refresh` rebuilds when
`world.revision()` moved; `topStandableInColumn` is the spawn/teleport helper.

### 7.2 Input → path → motion

`ExplorationInputLogic` (on the player Actor): asks `spk::Camera3D` for the ray
through the mouse pixel, then `spk::VoxelRayCast` DDA-steps unit voxel boundaries
until the pg solid-traversal predicate accepts a cell (no shape-polygon intersection); hover
mesh tints the target cell (`hovered` vs `invalid` atlas masks from game rules);
click → `Pathfinder` A* over graph nodes (cost = euclidean, no diagonals through
blocked corners) → `Actor::path`. `ActorPathLogic` advances
`segment`/`segmentProgress` by `speed * dt`, interpolates the entity transform,
emits `events.playerMoved(cell)` on cell changes — the portal hook in chapter 5
listens to exactly this. `CameraControllerLogic` smooth-follows the actor,
right-drag orbit (yaw/pitch clamped), wheel zoom (distance clamped), AEZS keys.

## 8. Fluid simulation

### 8.1 Registry side

See 1.3: stage voxels exist before the simulation ever runs, so a saved cell id
is all the state a fluid cell needs (`stage` = its fill level).

### 8.2 `FluidSimulationLogic::_step` details

Interleaving: alternates frontier pops and source visits (`_serveFrontierNext`)
so a huge frontier cannot starve sources and vice versa; `_sourceCursor` rotates
across steps so an over-budget source population is eventually all served.
Per cell:

1. classify via `tryFluidRef` (cells overwritten by non-fluid are dropped);
2. **fall**: `falls && below empty` → write stage `maxSpread` below, requeue
   below (and self, if not a source — the column must stay supported);
3. **support check**: only cells resting on *solid non-fluid* ground spread; a
   mid-fall cell just sustains the column (prevents waterfalls bleeding outward
   at every level); unloaded support requeues self;
4. **spread**: outflow = `maxSpread` for sources and waterfall bases (fed from
   above, same family), else `stage - 1`; if ≥ 1, examine the 4 sides: empty +
   in-range neighbors are candidates; if any candidate's floor is empty
   ("drop"), only drops are poured (narrow cascades), else all candidates get
   stage `outflow`; produced cells enqueue; a producing non-source requeues.

Budget: `_maxCellsPerTick` (default in header) per 150 ms tick; `_range` limits
simulation to a box around the player (`setSimulationCenter` from the player
Actor each update).

## 9. Headless tools & diagnostics

### 9.1 `--check-stairs` (`main.cpp`, 22–194)

`VoxelSampler` lazily realizes chunks through `PlanChunkProvider::fill` into a
private map (no engine, no window) and answers `standHeight(x, z)` queries.
`checkComposedStairways` scans `plan.placements` for platform → lengths →
platform runs, verifies they form a strict lattice (same across-line, pieces
every 3 cells, one 3-voxel level per piece — anything else is coincidence and is
skipped), then per group asserts: (1) exactly one across-side of the top platform
is flush high plateau; (2) walking the middle column descends ≤1 per cell and
lands on the bottom platform; (3) the 3-column approach band is flat low ground,
and paved with a road block for road groups. Exit code 0 only if all groups pass
and at least one exists.

### 9.2 `--map-only` / `writeWorldMapPng`

Renders per-cell pixels (zone color shaded by height, water/lake blues, road,
bridge, stair rects in road color), then entity label boxes using a built-in 5×7
bitmap font, and writes the PNG (8× upscale) to `Playground/world_map.png`.

### 9.3 Profiler & overlay

`spk::Profiler` (app-owned, threaded through `UpdateContext`) aggregates named
probes (min/max/avg). `GameSceneWidget::_refreshOverlay` snapshots it every
frame and syncs one overlay row per probe (rows rebuilt only when the probe set
changes). Fixed rows: camera/player/hovered cell, loaded chunk count, update and
render durations measured around the base-class calls, frame delta.
