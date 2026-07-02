# Step 03 — 3D hardening: Mesh3D, depth test, lambert lighting, translucent pass

**Phase B · critical path · needs step 01 (build split only)**

## Goal

Upgrade the verified Step-1 3D layer from "unlit convex cube" to "voxel-terrain-ready":
per-vertex normals, real depth testing, one directional light, and an opaque→translucent
pass split. This closes the flagship render risk before any voxel work.

## Reading

[rendering-cameras.md §1/§2](../../03-systems/rendering-cameras.md) ·
[02-class-rendering-3d.puml](../../diagrams/02-class-rendering-3d.puml) ·
[howto/add-render-command.md](../../howto/add-render-command.md) ·
current `srcs/rendering/mesh_render_command.*`, `srcs/logics/mesh_render_logic.hpp`,
`srcs/geometry/primitive3d.hpp`, `resources/shaders/mesh/*` ·
`includes/structures/graphics/opengl/spk_opengl_clear_command.hpp` (depth already cleared).

## Files

- **Create** `srcs/geometry/mesh3d.hpp` — `pg::MeshVertex3D{position, normal, uv}`
  (+ `operator==`, `std::hash` — copy the `spk::TextureVertex2D` pattern) and
  `pg::Mesh3D : spk::GenericMesh<MeshVertex3D>` with layout attributes 0=vec3, 1=vec3,
  2=vec2.
- **Modify** `srcs/geometry/primitive3d.hpp` — `makeCube` returns `Mesh3D` with per-face
  normals.
- **Modify** `srcs/components/mesh_renderer3d.hpp` — mesh type → `Mesh3D`; add
  `bool translucent = false;` and `spk::Color tint = white;` (tint consumed by the flash
  step later; shader multiplies it).
- **Modify** `srcs/rendering/mesh_render_command.*` — takes `Mesh3D`; enables
  `GL_DEPTH_TEST` (GL_LESS) + keeps back-face cull, **restoring prior state** after the
  draw; translucent variant flag: blend on (`SRC_ALPHA, ONE_MINUS_SRC_ALPHA`), depth write
  off, depth test on; uploads tint in the model UBO (extend `ModelData` block: mat4 + vec4).
- **Modify** `resources/shaders/mesh/mesh.vert/.frag` — pass normal; fragment =
  `texture.rgb * tint.rgb * (ambient + max(dot(n, -lightDir), 0) * lightColor)`, alpha =
  `texture.a * tint.a` (opaque path forces 1.0 — keep the current behavior for opaque).
- **Modify** `srcs/logics/mesh_render_logic.hpp` — emit `CameraUpdateRenderCommand`
  (binding 1) + a new tiny `LightUpdateRenderCommand` (binding 3: direction, color, ambient;
  create it in `srcs/rendering/`) once per pass; then all opaque commands, then translucent.
  Light parameters: constants in the logic for now (`normalize(1,-2,0.5)`, warm white, 0.35
  ambient).
- **Modify** scene (`game_scene_widget.cpp`) — **two** cubes, one behind the other partially
  overlapping from the camera, one translucent (tint alpha 0.5), to prove depth + blending.

## Tests (`[test]`)

Mesh3D vertex hashing/dedup (same vertex reused across faces stays split when normals
differ); makeCube: 24 unique vertices, 36 indices, normals unit-length and axis-aligned.
(GL state itself is visual.)

## Definition of Done

- `[build]`/`[test]` green.
- `[run]` visual (user validates): near cube correctly occludes far cube from every orbit
  angle (rotate the scene as in step 1); faces are shaded by orientation; the translucent
  cube blends and never "punches holes" in geometry behind it.
- Frame stats in the debug overlay still live (mesh/triangle counts).
- No `spk::` changes (clear command already handles depth).
