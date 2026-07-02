# Step 30 — Chunk realization: terrain, roads, flora, prefab stamping

**Phase J · needs steps 29, 26 (prefabs authored in the editor)**

## Goal

Layer 2: `GeneratedChunkProvider` turns the macro plan into streamed voxel chunks — the
game boots into a **procedural world** for the first time (the testground remains available
behind a config/CLI switch for testing).

## Reading

[world.md §Layer 2](../../03-systems/world.md) ·
[02-data-model.md §12 palettes](../../02-data-model.md).

## Files

`srcs/world/generator/generated_chunk_provider.hpp/.cpp` — per column: plan query → biome
palette layers (surface/subsurface/deep by depth), height steps smoothed with slopes/stairs
along road paths (roads guarantee walkable grades), river water voxels (a `water` voxel:
solid, `losTransparent`, tagged — swimming out of scope, rivers are obstacles bridged by
roads), road surface voxels, flora scatter from a per-column seeded hash (density per biome;
bushes on biome flora list), prefab stamps for settlements/gyms/tunnel-entrances from plan
features (prefab anchors bind portals — step 31 finishes tunnel interiors; this step stamps
entrances sealed with a placeholder door voxel).
`world_streamer` goes real: radius-based load/unload around the player over the provider;
spawn: a plan-chosen start city road cell.
Data: biome palettes completed for all 8 biomes (voxels authored in the modeler — reuse
textures freely, distinct tints); `prefabs/`: town-house, gym-shell, tunnel-entrance (made
in the world editor).
Performance note: chunk fill + mesh must stay under a few ms each; the streamer loads ≤N
chunks per frame (budgeted queue) — measure via the debug overlay.

## Tests (`[test]`)

Provider determinism (chunk hash per seed+coords); cross-chunk continuity (heights at
borders differ ≤ walkable gap along roads); road walkability (A* along a sampled road path
succeeds end-to-end on realized voxels); flora density within tolerance per biome; stamps
land where the plan says.

## Definition of Done

`[build]`/`[test]` green; `[run]` (user validates): boot into the generated world — walk a
road out of the start city through a biome transition, over a bridge, past bushes that
trigger biome-correct encounters; streaming shows no seams/hitches; revisiting a chunk
reproduces it exactly. Debug-PNG cross-check: what the map shows is what you walk.
