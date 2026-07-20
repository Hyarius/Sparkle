# Cleanup proposals

Assessment of what should move, what is more complex than it needs to be, and what
could be promoted into the sparkle library. Correctness/performance issues already
tracked in `backlog/open/` are referenced by ticket number instead of repeated.

Ordered by expected value.

---

## A. Move the stairway harness out of `main.cpp` — *quick win*

`srcs/main.cpp` is 60% verification harness: `VoxelSampler` + `checkComposedStairways`
(lines 22–194) implement the `--check-stairs` walk-test inline. `main` should only
parse arguments and dispatch.

**Proposal**
- New file pair `srcs/world/generator/stairway_check.{hpp,cpp}` exposing
  `int pg::checkComposedStairways(const Registries&, const WorldPlan&)`.
- **Delete `VoxelSampler` entirely**: it is a hand-rolled on-demand chunk cache —
  which is exactly what `spk::VoxelMap` already is (the engine pointer is optional).
  Replace with:
  ```cpp
  spk::VoxelMap map(registries.voxels().renderRegistry(),
                    [&](spk::VoxelChunk &c) { provider.fill(c); });
  // map.cell(worldPos) replaces sampler.at(worldPos)
  ```
  `standHeight` becomes a 6-line local helper over `map.cell`.

`main.cpp` drops to ~120 lines, and the harness reuses tested library code.

## B. Record composed stairways in the plan instead of re-inferring them

> **Status: DONE** (2026-07-11, second pass). `PlanStairway` now carries a
> `StairLayout` enum, the contiguous placement range, its footprints, an
> `optional<PlanStairRect>` paved approach, its low plan cell, and the walk
> path — `wildStairs`/`stairRects`/`pavedRects` were removed from `WorldPlan`.
> `emitStairChain` is split into per-layout `StairCandidate` builders funneled
> through a single `tryCommitStairCandidate`, and one-level ramps are recorded
> too. The sketch below is the original proposal, kept for context.

This is the root cause of the "why is this so complicated?" feeling around stairs.
A composed staircase currently exists as *implicit structure* in four places that
must agree:

1. `emitStairChain` composes it (placements at 3-cell lattice spacing),
2. `commitStairGroup`/`stairFootprintFits` validate it,
3. `checkComposedStairways` **reverse-engineers the group from the flat placement
   list** (the ~20-line strict-lattice detection exists only because the plan never
   says "these N placements are one staircase"),
4. `PlanChunkProvider` re-derives its paved band from `pavedRects`.

**Proposal**: when `commitStairGroup` succeeds, append a first-class record to the
plan:

```cpp
struct PlanStairway
{
    spk::Vector3Int topAnchor;     // top platform pivot
    spk::Vector3Int bottomAnchor;  // bottom platform pivot
    int steps = 0;                 // height levels climbed
    bool alongX = false;
    int tangent = 1;
    bool road = false;             // road climb (paved band) vs wild
};
std::vector<PlanStairway> stairways; // on WorldPlan
```

The checker then iterates `plan.stairways` directly (no lattice inference, no
false-positive filtering), the preview map and `pavedRects` can both derive from
it, and future gameplay (minimap icons, fast travel) gets the data for free.

## C. Split `world_plan_generator.cpp` (2 953 lines)

> **Status: DONE** (2026-07-11, after B). `Generator` survived as the
> state-owning orchestrator, declared in `world_plan_generator.hpp`
> (namespace `pg::worldgen`); its stage implementations moved into
> `world_plan_math.{hpp,cpp}`, `world_plan_terrain.cpp`,
> `world_plan_infrastructure.cpp` (entities, ports, roads, buildings),
> `world_plan_stairways.cpp` (claims included), `world_plan_interiors.cpp`,
> and `world_plan_scenery.cpp`; `world_plan_generator.cpp` keeps `run()`,
> stats, and the public entry point (~260 lines). The sketch below is the
> original proposal, kept for context (final file names differ).

The internal structure is actually clean — one `Generator` struct with well-named
stages — but it is the only file in the project an editor chokes on, and unrelated
concerns (grid math, road A*, stair composing, interior growing) share one blob.
Natural seams, in increasing coupling order:

| New file | Contents | Coupling |
|---|---|---|
| `plan_grid_math.{hpp,cpp}` | `Cell`, `Rng`, `deriveSeed`, `valueNoise`, `distanceTo`, `farthestPointSeeds`, `labelComponents`, `countDiagonalOnly`, `forNeighbors4` | none — pure helpers (~300 lines) |
| `plan_claims.{hpp,cpp}` | `Claim`, `ResolvedBox`, `resolveBox`, `claimBoxFor`, `claimsOverlap`, `zoneIsFree` as a small `ClaimRegistry` class | prefab registry only |
| `plan_stairways.cpp` | `StairFootprint` machinery, `emitStairChain`, `commitStairGroup`, `placeStairways`, `placeWildStairways` | plan + claims (~670 lines) |
| `plan_interiors.cpp` | `composeInterior`, `connectorDirection`, `pickWeightedRoom` | plan + claims (~200 lines) |
| `plan_terrain.cpp` | landmass, zones, heights, water | plan + grid math |
| `plan_infrastructure.cpp` | entities, ports, roads (A*), boat links, bridges | plan + grid math |

The `Generator` struct can survive as the orchestrator; the stage groups become
free functions or small structs taking `(WorldPlan&, const WorldGenConfig&, ...)`.
Do this *after* B (the stair extraction gets simpler once groups are explicit).

## D. Deduplicate placement resolution (plan ⇄ realization)

The formula "rotated bounds → worldMin → destination" exists **three times**,
byte-for-byte in spirit:

- `world_plan_generator.cpp::resolveBox` (line 1865, commented "Mirrors
  PlanChunkProvider's placement resolution"),
- `world_plan_generator.cpp::stairFootprintOf` (line 1932 — a copy of `resolveBox`
  projected to XZ; it should just call it),
- `plan_chunk_provider.cpp` constructor (line 128).

If one changes, staircases silently stop matching the terrain. **Proposal**: one
free function in a small shared header (e.g. `world/prefab_placement_math.hpp`):
`ResolvedBox pg::resolvePlacement(const PrefabDefinition&, const PrefabPlacement&)`,
used by all three call sites.

## E. Give `emitStairChain` a coordinate frame

220 lines in which the `p_dc == 1` ternary appears ~15 times to swap x/z. The
across/along frame is already the mental model (the comments explain it); it is
just never reified. A tiny local struct kills most of the noise:

```cpp
struct CliffFrame
{
    bool alongX;                     // cliff runs along x or z
    spk::Vector3Int at(int across, int y, int along) const;
    StairFootprint rect(int acrossMin, int acrossMax, int alongMin, int alongMax) const;
    spk::VoxelOrientation ascend(int tangent) const;
};
```

Same behavior, roughly half the volume, and the composed-layout candidate loop
becomes readable. Combine with B/C.

## F. One home for hashing & weighted picking

> **Status: DONE** (2026-07-11, D43). Stable hashing and semantic seed derivation
> now live in `spk_deterministic_random.hpp`; validated half-open weighted
> selection lives in `spk_weighted_pool.hpp`. Playground keeps thin aliases.

Four weighted-pool picks and three FNV/avalanche implementations exist:

| Site | Duplicated bits |
|---|---|
| `world_plan_generator.cpp` | `deriveSeed` (FNV-1a), `pickWeightedRoom` |
| `climb_prefabs.cpp` | `hashSeed` (FNV-1a, identical), `pickCell` |
| `plan_chunk_provider.cpp` | `splitMix64Finalize`, `toUnitInterval`, `pickVoxel` |
| `voxel_content_parser.cpp` | `splitMix64Finalize`, `toUnitInterval`, `pickCellFromPool` |

**Implemented**: `core/deterministic_random.hpp` with `fnv1a64::hash(string_view)`,
`splitMix64Finalize(u64)`, `toUnitInterval(u64)`, and a single
`pickWeighted(span<pair<T,double>>, double roll)`. Every call site keeps its own
seeding policy (that part is intentional), only the mechanics unify.

## G. Fluid → spk promotion: yes, but in two steps

The fluid system is a good promotion candidate *by design*: the simulation talks
only to `spk::VoxelMap`/`VoxelChunk` public API and never touches the render path
(the height-fade lesson doesn't apply). What blocks a direct move:

1. `FluidData/FluidFamily/FluidRef` + stage generation live inside
   `pg::VoxelRegistry::load`, entangled with gameplay `VoxelDefinition`
   (traversal/tags),
2. open fluid tickets: 005 (frontier starvation), 030 (convergence across
   streaming), 031 (water traversal semantics), 032 (debug waterfall) — promote a
   stable thing, not a moving one,
3. per `docs/howto/promote-to-spk.md`, promotion needs explicit sign-off.

**Proposed shape when ready**:
- `spk::FluidCatalog` (or a helper on `spk::VoxelRegistry`): registers a fluid
  family from (source shape, texture, maxSpread) and generates the stage shapes —
  the pg registry keeps only its gameplay mirror of the generated ids;
- `spk::VoxelFluidSimulationLogic` — today's logic minus the two pg couplings,
  which become injection points: the fluid classification (from the catalog) and
  the simulation center (a plain `setSimulationCenter` setter the app calls; drop
  the `Actor` template parameter).

Until then it is fine where it is.

## H. Other spk promotion candidates (opportunistic)

- ~~**Quarter-turn helpers**~~ — **done 2026-07-11 (D41)**: now
  `structures/voxel/spk_voxel_orientation.hpp` (+ `composeOrientations` /
  `composeFlips`); the `spk_prefab.cpp` and `world_plan_generator.cpp` duplicates
  are deleted, and `spk::Prefab::rotated(orientation, flip)` returns a
  pre-transformed copy.
- ~~**`JsonReader`**~~ — **done 2026-07-11 (D42)**: now `spk::JSON::Reader` /
  `Error` / `Loader` in `structures/container/spk_json_reader.hpp`;
  `pg::core/json.hpp` is thin aliases.
- ~~**`Registry<T>` directory loading**~~ — **done 2026-07-11 (D43)** as
  `spk::DefinitionRegistry<T>` + transactional `spk::loadJsonDirectory`; parsers
  receive filename-derived ids explicitly.
- ~~**`PlanGrid<T>` storage**~~ — **done 2026-07-11 (D43)** by retaining the
  world-specific square/size guard while delegating storage to `spk::Grid2D<T>`;
  no duplicate generic grid was promoted.

## I. Naming: two `VoxelRegistry` classes

`pg::VoxelRegistry` *contains* an `spk::VoxelRegistry` (`renderRegistry()`), and
call sites juggle both names (`registries.voxels().renderRegistry()`). Renaming
the pg one to `VoxelCatalog` (it is a catalog: render shape + gameplay definition
+ fluid family per id) removes a recurring double-take for one afternoon of
mechanical renaming.

## J. Smaller items

- **`stepCost` leftover** — `double step = isLand(...) ? 1.0 : 1.0;`
  (`world_plan_generator.cpp:1309`): both branches equal; either differentiate or
  simplify to `1.0`.
- **Plan-generation call triplication** — `main.cpp` (twice) and
  `game_scene_widget.cpp` each build `WorldGenConfig{seed,size}` +
  `generateWorldPlan(cfg, planBiomesFrom(...), rules, prefabs, interiors)`.
  A `pg::makeWorldPlan(const Registries&, uint64 seed, int size)` helper removes
  the repetition (ticket 038 also touches this).
- **Stale docs** — `docs/README.md` still says "The editor executable is
  `EreliaTools`", which was deleted; `docs/03-systems/*` predates the plan
  pipeline in places. Worth a sweep now that this code tour exists.
- **Dead code** — `perlin_chunk_provider.{hpp,cpp}` unused: ticket 039 already
  covers it (remove or revive).
- **Overlay bookkeeping in `GameSceneWidget`** — **deferred by user decision**
  until `DebugOverlay` supports dynamic widget composition instead of only text
  labels. The current ~150 lines of profiler-row/overlay management
  (`_configureOverlay`, `_syncProfilerRows`,
  `_applyOverlayGeometry`, `_refreshOverlay`) could move to a small
  `pg::SceneDebugOverlay` helper class; the widget keeps only construction and
  the frame hook. Cosmetic, do it whenever the file is touched next.

## Already ticketed — not repeated here

Realization/streaming performance and spatial indexing (026), warm-up
synchronicity (001/026), fluid correctness (005/030/031/032), navigation rebuild
cost (024/025), mesher partial-occlusion
correctness (004), worldgen config/runtime unification (038), hard-coded world
dimensions (034), stale voxel functionality (039).

## Suggested order

1. **A** (harness out of main, `VoxelSampler` → `spk::VoxelMap`) — one sitting.
2. **J.stepCost + J.makeWorldPlan + D** (shared placement math) — one sitting.
3. **B** (`PlanStairway` record) then **E** (CliffFrame) — makes stairs legible.
4. **C** (file split) — mechanical after B/E.
5. ~~**F**~~, **I**; **J.overlay** waits for the dynamic-widget debug refactor.
6. **G/H** (spk promotions) — propose per `promote-to-spk.md` once the fluid
   tickets are closed.
