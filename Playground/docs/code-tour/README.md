# Code tour — the voxel world since commit `6eaf476`

A layered walkthrough of everything built between commit `6eaf476` ("first draft of the
voxel engine library" era) and `23a107c` (2026-07-11): the spk voxel module, the macro
world generator, prefab/interior stamping, fluids, and the exploration runtime.

Every file below uses the **same chapter numbering**, so you can read the big picture in
level 1, then jump to the same chapter number in level 2 or 3 when you want more depth.

| File | Depth |
|---|---|
| [level-1-overview.md](level-1-overview.md) | One paragraph per chapter: "the program does this, then this". |
| [level-2-systems.md](level-2-systems.md) | Each chapter decomposed into its pipeline steps, with the classes involved. |
| [level-3-details.md](level-3-details.md) | Function-level detail: what each stage computes and in what order. |
| [cleanup-proposals.md](cleanup-proposals.md) | What should move, what is overcomplicated, what could be promoted into spk. |

## Chapters

1. **Startup & data loading** — from `main()` to a fully-populated `pg::Registries`.
2. **World plan generation** — the once-per-seed macro skeleton (`WorldPlan`).
3. **Prefabs & climb-prefab synthesis** — authored JSON prefabs + generated staircases.
4. **World realization** — turning the plan into voxels, chunk by chunk (`PlanChunkProvider`).
5. **Scene & streaming runtime** — `GameSceneWidget`, the chunk streamer, teleports.
6. **Rendering** — the spk voxel module: shapes, occlusion mesher, bake & draw passes.
7. **Navigation & movement** — traversal graph, pathfinding, click-to-move, camera.
8. **Fluid simulation** — sources, spreading stages, waterfalls.
9. **Headless tools & diagnostics** — `--map-only`, `--check-stairs`, the F7 overlay.
