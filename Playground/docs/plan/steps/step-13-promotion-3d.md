# Step 13 — Promotion: the 3D layer → `spk::`

**Phase E · needs Milestone 1 (step 12) · ⚠ requires explicit user sign-off before any
`spk::` edit (D06/D18)**

## Goal

Lift the proven 3D layer into the Sparkle library, with library-grade tests. First
promotion; also the template for all later ones.

## Reading

[howto/promote-to-spk.md](../../howto/promote-to-spk.md) (the checklist — follow it
verbatim) · [rendering-cameras.md §6](../../03-systems/rendering-cameras.md) ·
how existing spk shaders/resources embed (check `srcs/structures/graphics` resource usage +
the shared resource header noted in the CMake module split before moving shader files).

## What moves (pg:: → spk::)

| pg:: | spk:: target |
|---|---|
| `Transform3D` | `includes/structures/game_engine/spk_transform_3d.hpp` |
| `Entity3D` | `spk_entity_3d.hpp` |
| `Camera3D` | `spk_camera_3d.hpp/.cpp` (keep the main-camera static slot — mirrors `Camera2D`) |
| `MeshVertex3D` / `Mesh3D` | `graphics/geometry/spk_texture_mesh_3d.hpp` |
| `MeshRenderer3D` | `game_engine/spk_mesh_renderer_3d.hpp` |
| `MeshRenderLogic` (+ light command) | `game_engine/spk_mesh_render_logic.hpp` + `graphics/rendering/command/spk_light_update_render_command.*` |
| `MeshRenderCommand` | `graphics/rendering/command/spk_draw_mesh_3d_render_command.*` |
| `makeCube` | `graphics/geometry/spk_primitive_object.hpp` (extend the existing primitives home) |
| `mesh.vert/.frag` | embedded like existing spk shader resources |

Stays in `pg::`: chunk rendering, overlay, mouse picker (uses game world), camera
controllers (game behavior), voxel meshes' consumers.

## Work items

1. Present the proposal to the user (API diffs made while lifting, e.g. naming to Sparkle
   conventions); **wait for go-ahead**.
2. Move + rename per table; namespace/style sweep (`p_` params, `_` privates).
3. Re-point every Playground include/usage; delete the `pg::` originals.
4. New library tests `tests/TUs/srcs/structures/game_engine/transform_3d_test.cpp`,
   `camera_3d_test.cpp`, `graphics/texture_mesh_3d_test.cpp`: transform hierarchy +
   cache invalidation, view/projection matrices vs hand values, mesh vertex dedup/layout.
5. clang-tidy check preset clean on the new files (check-only — never bulk `--fix`).

## Definition of Done

`[spk-test]` green · `[test]` green · `[build]`+`[run]`: game renders identically
(screenshot compare with step 12 by eye). Decision-log entry added. User confirms the
`spk::` API shape.
