# How-to — Promote a pg:: feature into spk::

Promotions are the project's second goal (D06/D18) — and the only sanctioned way to touch
`includes/`/`srcs/`. **Each promotion needs explicit user sign-off before any `spk::` edit.**

## Checklist

1. **Eligibility** — the feature is proven in the game (used by shipped steps, stable API,
   no `pg::`-specific assumptions: no game types, no `pg::resourceRoot()` reliance, no
   EventCenter coupling).
2. **Propose** — short message to the user: what moves, target headers, API changes made
   while lifting, what stays behind in `pg::`. Wait for the go-ahead.
3. **Move** — new home mirrors library layout: header under
   `includes/structures/<area>/spk_<name>.hpp`, source under
   `srcs/structures/<area>/spk_<name>.cpp`, namespace `spk::`, Sparkle naming (`p_` params,
   `_` privates). Shader/resource payloads follow the library's embedded-resource mechanism
   (single shared resource header — check how existing spk shaders/fonts embed before
   inventing anything; ODR note in the CMake module split).
4. **Re-point Playground** — `pg::` code includes the `spk::` header; keep a thin
   `pg::X = spk::X` alias only if churn would be large, otherwise rename call sites.
   Delete the `pg::` original (history preserves it).
5. **Tests move up** — add `tests/TUs/srcs/structures/<area>/<name>_test.cpp` covering the
   promoted API (the pg-level tests for it either move or shrink to game-specific behavior).
6. **Verify** — `[spk-test]` green, `[test]` green, `[build]`+`[run]` unchanged behavior,
   clang-tidy preset clean on the new files (`cmake --preset clang-tidy` build — check-only;
   never bulk `--fix`).
7. **Record** — decision-log entry (what/why/date), status note in the step file.

## Planned promotions

| When | What |
|---|---|
| step 13 | Transform3D, Entity3D, Camera3D, Mesh3D/MeshVertex3D, MeshRenderer3D, MeshRenderLogic, MeshRenderCommand, makeCube, mesh shaders |
| step 37 | tools::Viewport3D, tools::GraphCanvas, tools::PropertyPanel (+IdPicker) |
| opportunistic | ObservableResource/ObservableList (tiny, generic), voxel mesher primitives, easing header — propose when they stabilize |
