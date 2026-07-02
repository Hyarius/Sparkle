# Step 29 — Macro world plan: landmass, cities, biomes, transport graph

**Phase J · needs step 06 (parallel-safe with Phases H/I) · headless + debug image**

## Goal

The once-per-seed `MacroWorldPlan` (layer 1 of D — [world.md](../../03-systems/world.md)
§generation): landmass, heights/rivers, 8 biome-anchored cities, satellites, classified
transport graph, road paths — plus a top-down debug image dump (invaluable for iteration).

## Reading

[world.md §Procedural generation](../../03-systems/world.md) (the spec) ·
[02-data-model.md §14](../../02-data-model.md) (worldgen params) ·
`includes/structures/math/spk_perlin.hpp`, `spk_poisson_disk.hpp` (real APIs — verify
signatures before use).

## Files

`srcs/world/generator/`: `worldgen_params.hpp` + parser (`worldgen/default.json`),
`macro_world_plan.hpp` (masks/heights/rivers grids + city/settlement/edge/road/feature
lists + `queryCell(cell) -> PlanInfo{biome, height, road?, river?, feature?}`),
`macro_world_generator.hpp/.cpp` — the 7 stages per spec, each a separately-testable
function: landmass (radial falloff + Perlin distortion + detail), heightRivers (Perlin +
smoothing + downhill river walks), cities (PoissonDisk candidates → flat/land/spacing/
coastal filters → majorCount), biomes (seeded shuffle → weighted Voronoi over land →
noised borders), satellites, transportGraph (MST + extraEdges + village hookup + edge
classification: road/bridge/sea/tunnel by path-cost analysis), roads (A* on the terrain
cost map per edge; record bridge/port/tunnel feature placements).
All randomness from one `std::mt19937_64` seeded `(runSeed, stageId)` — stage isolation
keeps edits from cascading.
`srcs/world/generator/plan_debug_image.hpp/.cpp` — dump PNG(s) (stb_image_write — stb is
already a vcpkg dep): land/height shading, biome colors, cities/settlements as dots,
roads/bridges/ports/tunnels as colored strokes. Written to the scratch/output dir on
demand (debug key or a `--dump-plan` CLI arg on SparklePlayground).
Data: `resources/data/worldgen/default.json` (values from the schema — start conservative:
512×512).

## Tests (`[test]`)

Determinism (same seed ⇒ identical plan hash; different seeds differ); invariants: exactly
`majorCount` cities on land with min spacing; every settlement reachable through the graph;
every edge classified; roads never cross deep water except at bridge/port features; biome
count == city count; rivers reach the sea or a lake basin. Stage isolation: changing
`extraEdges` leaves cities identical.

## Definition of Done

`[build]`/`[test]` green; generate 3 seeds, hand over the 3 debug PNGs — **user validates
the world shapes** (continent silhouette, city spread, road sanity) before any voxel
realization. Iterate params/stages here until validated; this gate protects step 30.
