# Erelia Port — Implementation Steps

Ordered, concrete build steps for the Erelia port. Each `stepN.md` describes exactly what to
add (classes, files, shaders), how it integrates with current Sparkle, a definition of done,
and tests. This folder is the *execution* layer under the master plan in
[../ERELIA.md](../ERELIA.md) (read that first for the why; §9 is the phase roadmap).

## Conventions (all steps)

- All game code lives under `Playground/srcs/` in namespace **`pg::`** — never touch the
  `spk::` library (`includes/`, `srcs/`) except a deliberate, signed-off **promotion**.
- Mirror Sparkle idioms: data `spk::Component` + `spk::ComponentLogic<T>`; render logics emit
  `RenderCommand`s into the `RenderUnitBuilder`; resources resolved via `PG_RESOURCE_DIR`.
- Build: `cmake --build build/playground --target SparklePlayground`, then run the exe.
- Add `tests/` coverage for pure logic; hand-validate visuals (see memory
  `sparkle-visual-test-workflow`).
- Keep [../ERELIA.md](../ERELIA.md) §11 status log + this table updated as steps land.

## Step index (maps to ERELIA.md §9 phases)

| Step | Phase | Title | Status | Detail |
|------|-------|-------|--------|--------|
| 1 | Phase 1 | **3D engine layer** — Transform3D / Camera3D / MeshRenderer3D + logic + command + shader → a textured cube | ✅ **done — screenshot-verified** (textured cube renders in perspective) | [step1.md](step1.md) |
| 2 | Phase 2 | **Voxel core + mesher** — VoxelShape/Definition/Cell/Grid + neighbor-occlusion mesher + JSON defs → a hand-authored voxel chunk; seed of the modeling tool | planned (moderate) | [step2.md](step2.md) |
| 3 | Phase 3 | **Exploration slice** — VoxelChunk/World streaming + player entity + follow camera + encounter triggers | planned (outline) | [step3.md](step3.md) |
| 4+ | Phase 0/4… | Foundations (EventCenter/GameContext/ModeManager/JSON) + battle backend seam | deferred | authored as we approach them |

> Steps beyond 3 are intentionally left as the roadmap (ERELIA.md §9) until we learn from
> steps 1–3 — writing them in detail now would be speculative. Foundations (Phase 0) are
> deliberately **not** step 1: they only matter once there are modes/data to manage, and the
> flagship risk is the 3D render path, so we prove that first.
