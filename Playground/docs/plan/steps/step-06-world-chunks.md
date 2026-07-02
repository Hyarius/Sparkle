# Step 06 — Chunks, VoxelWorld, map loading, M1 testground

**Phase B · critical path · needs step 05**

## Goal

The persistent world: chunked storage with seamless access, `maps/*.json` loading, per-chunk
render entities, and the M1 hardcoded testground (D12).

## Reading

[world.md](../../03-systems/world.md) §chunks/§providers ·
[04-class-world.puml](../../diagrams/04-class-world.puml) ·
[02-data-model.md §13](../../02-data-model.md) ·
[howto/add-component-logic.md](../../howto/add-component-logic.md).

## Files

`srcs/world/`: `chunk_coordinates.hpp` (int3 + world↔chunk↔local math + hash),
`chunk.hpp/.cpp` — **`pg::Chunk : spk::Component, spk::SynchronizableTrait`** (D32):
`static constexpr Vector3Int Size{16,16,16}` (D29), coords, `VoxelGrid grid`, baked
`Mesh3D renderMesh` (+ empty `maskMesh` placeholder for world decals), `setCell(local,…)` →
`requestSynchronization()`, `_synchronize()` → remesh via `VoxelMesher` (nav slice joins in
step 07);
`voxel_world.hpp/.cpp` (owns one entity+Chunk per loaded chunk; `cell(worldCell)` with
empty-cell sentinel outside; `setCell` routes to the chunk **and
requestSynchronization()s neighbour chunks on border edits** — occlusion crosses borders;
`loadFromMap`), `map_definition.hpp` + `map_parser.cpp` (size/palette/fill/cells/**stamps**/
markers/**portals**/biome per [02-data-model.md §13b](../../02-data-model.md)) +
`prefab_definition.hpp` + `prefab_parser.cpp` (size/palette/fill/cells/anchors, §13),
`chunk_provider.hpp` (`IChunkProvider::fill(Chunk&)` + `MapChunkProvider`),
`world_streamer.hpp/.cpp` (loaded-set = radius around a focus cell; loads via provider,
unloads far chunks + their entities; for map-worlds the radius covers everything).
`srcs/logics/chunk_synchronization_logic.hpp` (the baker: update phase, `synchronize()`
every Chunk with `needsSynchronization()`),
`srcs/logics/chunk_render_logic.hpp` (render phase: emit one command per chunk renderMesh,
voxel atlas texture).
`srcs/core/game_context` — WorldContext grows `VoxelWorld world; const MapDefinition*
activeMap;`. Registries grow `prefabs` + `maps` domains (load order per data model).
`resources/data/maps/m1-testground.json` — 64×16×64 per D12: dirt base, grass top, a
raised 8×8 plateau (2 cells) reachable by a slope ramp on one side and stairs on another, a
stone wall segment (2 high) creating a LoS blocker, a slab path, 3 bush patches (5–8 bushes
each) placed on flat ground, `playerSpawn` + `healPoint` markers, biome `forest` (string —
biome file arrives step 08; parser stores the id unresolved until then: keep it a plain
string field).
`game_scene_widget.*` — scene = load m1-testground into the world, spawn chunk entities via
streamer, fixed overview camera at the map (orbit controls arrive step 07).

## Tests (`[test]`)

Coordinate math round-trips (negative cells included); cross-chunk `cell()` continuity;
`setCell` requests synchronization on exactly the touched chunk, and on the neighbour too
for border cells; the baker clears `needsSynchronization` and rebuilds the mesh; map parser
(fills order, sparse overrides, orientation/flip parsing, stamps, markers, portals, palette
errors); prefab parser (anchors); stamping (prefab cells land at anchor+orientation);
provider slicing matches direct map reads.

## Definition of Done

- `[build]`/`[test]` green.
- `[run]` visual (user validates): the M1 testground — plateau, ramp, stairs, wall, slab
  path, bushes; **no seams or missing faces at chunk borders** (walk the camera along a
  border).
- Editing one cell at runtime (temporary debug key that places a stone block at a fixed
  cell) updates the mesh — remove the key after validating or keep behind `#ifdef`.
