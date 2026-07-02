# Step 1 — The 3D Engine Layer

**Phase:** 1 (ERELIA.md §9) · **Namespace:** `pg::` · **Location:** `Playground/srcs/`

## Goal / Definition of Done

Render a **textured cube in true 3D** inside the Playground, viewed through a **perspective
`Camera3D` you can orbit**, with the cube (or camera) rotating each frame. The `DebugOverlay`
shows camera + mesh stats. It builds and runs clean via the `playground` preset, and the pure
math pieces have `tests/` coverage.

This is the flagship first step: it is the biggest capability gap (Sparkle is 2D-only today),
it de-risks everything voxel/battle, and it is the **first promotion candidate** back into
`spk::`. We build it by **mirroring the existing 2D render path in 3D** — no library change
is required.

Concretely, done means:
- A rotating, textured cube is visible and correct (faces in the right places, no inside-out
  look), orbitable with mouse drag or ZQSD/arrows.
- Perspective projection (near objects bigger); resizing the window keeps the aspect correct.
- Overlay shows camera position/target, triangle count, update/render ms.
- `cmake --build build/playground --target SparklePlayground` succeeds; full `tests/` suite
  stays green with the new Step-1 tests added.

## What already exists (reuse — do NOT rebuild)

Verified against current source:
- **`spk::Matrix4x4`** (`math/spk_matrix.hpp`) already has `perspective(fov,aspect,near,far)`,
  `lookAt(from,to,up)`, `translation`, `rotation(Quaternion)` / `rotation(eulerVec3)`,
  `scale`, `inverse`, `operator*`. **All camera/transform math is done.**
- **`spk::TextureMesh2D`** (`geometry/spk_texture_mesh_2d.hpp`) — despite the "2D" name its
  vertex is `TextureVertex2D { spk::Vector3 position; spk::Vector2 uv; }` with GL layout
  attributes (loc 0 = Vector3, loc 1 = Vector2). **We can reuse it as the Step-1 3D mesh**
  (positions are already 3D). `addShape(a,b,c,d)` dedups by (position+uv).
- **`spk::GenericMesh<TVertex>`** base — `addShape`, `layoutBuffer()`, `hashKey()`,
  `indexes()`, `vertices()`.
- **The render-logic pattern** (`game_engine/spk_component_logic.hpp`,
  `spk_sprite_render_logic.hpp`): a `ComponentLogic<TComponent>` overrides
  `_onRenderStarted` / `_parseComponentForRender` / `_executeRender(RenderUnitBuilder&)` and
  emits commands. **`SpriteRenderLogic` is the exact template for `MeshRenderLogic`.**
- **`spk::CameraUpdateRenderCommand(binding, mat4)`** — pushes a matrix into a UBO at a
  binding. `SpriteRenderLogic` uses binding 1 for `projection*view`. **Reuse it verbatim**
  for the 3D camera view-projection.
- **`spk::InstancedSpriteRenderCommand`** (`.hpp` + `srcs/.../*.cpp`) — the template for a
  GPU `RenderCommand`: a static `_program()` built via `spk::Program(vertSrc, fragSrc)`, a
  `SamplerObject`, an SSBO, and `execute(RenderContext&)`. Step 1's `MeshRenderCommand` is a
  **non-instanced** simplification of this.
- **`spk::Camera2D` / `spk::Transform2D` / `spk::Entity2D`** — the shape to mirror for the 3D
  equivalents (static `mainCamera()`/`makeMain()`, cached matrices, `entity()->transform()`).
- **`pg::GameSceneWidget : spk::GameEngineWidget`** — the host. It already: adds logics via
  `gameEngine().add<Logic>()`, adds entities via `engine.addEntity(&e)`, feeds the camera its
  viewport in `_onGeometryChange()`, and calls `invalidateRenderUnit()` each `_onUpdate` so
  the scene keeps redrawing.
- **Shaders**: the library loads embedded shader sources; the Playground loads them from disk
  via `PG_RESOURCE_DIR` (compile-def, absolute path). New shaders go in
  `Playground/resources/shaders/mesh/`.

## New files / classes to add (all `pg::`)

| File | Type | Responsibility |
|------|------|----------------|
| `srcs/components/transform3d.hpp` | `pg::Transform3D : spk::Component` | position `Vector3`, rotation `Quaternion` (default identity), scale `Vector3{1,1,1}`; cached `modelTransform()` = `T * R * S` (mat4). Setters invalidate the cache. Mirror `spk::Transform2D` (incl. optional edition-notify if easy; not required for Step 1). |
| `srcs/components/entity3d.hpp` | `pg::Entity3D : spk::Entity` | Owns a `Transform3D` in its ctor and caches the ptr; exposes `transform()` (ref, +const) and `position()`. Ergonomic mirror of `spk::Entity2D`. Optional but keeps scene code clean. |
| `srcs/components/camera3d.hpp` + `.cpp` | `pg::Camera3D : spk::Component` | static `_mainCamera`, `mainCamera()`, `makeMain()`. Perspective params (`fov=60`, `near=0.1`, `far=1000`); `setAspectFromViewport(Rect2D)` (or store viewport, compute aspect). `projectionMatrix()` (cached `Matrix4x4::perspective`). `viewMatrix()` = `inverse` of the owning entity's world model transform, **or** `Matrix4x4::lookAt(eye, target, up)` when driven as an orbit camera. Mirror `spk::Camera2D`. The one static member lives in the `.cpp`. |
| `srcs/components/mesh_renderer3d.hpp` | `pg::MeshRenderer3D : spk::Component` | Data only: `const spk::TextureMesh2D* mesh`, `const spk::SpriteSheet*`/`spk::Texture*` (optional texture), `spk::Color tint`. Getters/setters. |
| `srcs/logics/mesh_render_logic.hpp` | `pg::MeshRenderLogic : spk::ComponentLogic<MeshRenderer3D>` | Mirror `SpriteRenderLogic`. `_onRenderStarted`: grab `Camera3D::mainCamera()`, compute `VP = projection * view`, clear command list. `_parseComponentForRender`: fetch the entity's `Transform3D` (via `p_component.entity()->component<pg::Transform3D>()`), build/get a `MeshRenderCommand` for (mesh, texture), set its model matrix + tint. `_executeRender`: if any, emit `CameraUpdateRenderCommand(1, VP)` then the mesh command(s). Static `lastMeshCount()` / `lastTriangleCount()` for the overlay. |
| `srcs/rendering/mesh_render_command.hpp` + `.cpp` | `pg::MeshRenderCommand : spk::RenderCommand` | Modeled on `InstancedSpriteRenderCommand` but **non-instanced**. Holds the mesh ref, a model `Matrix4x4`, a `SamplerObject`, optional texture. Static `_program()` loads `PG_RESOURCE_DIR/shaders/mesh/mesh.{vert,frag}` into strings → `spk::Program`. `execute(RenderContext&)`: enable depth test + back-face cull, bind program, bind camera UBO (binding 1, already filled by `CameraUpdateRenderCommand`), set the model-matrix uniform (or a small model UBO at binding 2), upload/point the mesh VAO+IBO from `mesh.layoutBuffer()`, bind texture+sampler, `glDrawElements`. |
| `srcs/geometry/primitive3d.hpp` | free fns | `pg::makeCube(float size = 1.0f) -> spk::TextureMesh2D` — 6 faces × 4 verts, per-face UVs (so corners don't dedup across faces), CCW winding for outward normals. (Analog of `spk::PrimitiveObject::CreateSquare`, which is 2D-only.) |
| `resources/shaders/mesh/mesh.vert` | GLSL | `#version 430`. `layout(location=0) in vec3 aPos; layout(location=1) in vec2 aUv;` camera VP via UBO binding 1 (match `CameraUpdateRenderCommand`), model matrix via uniform/UBO. `gl_Position = uViewProjection * uModel * vec4(aPos,1); vUv = aUv;` |
| `resources/shaders/mesh/mesh.frag` | GLSL | Sample texture at `vUv` × `uTint`; **unlit for Step 1** (lighting needs normals → Step 2). Optionally a cheap `dFdx/dFdy` face normal for a faux-lit look, but keep it simple first. |

Scene wiring (edit `srcs/game_scene_widget.{hpp,cpp}`): swap the 2D demo content for a 3D one —
`gameEngine().add<pg::MeshRenderLogic>()`; a `pg::Entity3D _cameraEntity` with a `Camera3D`
made main; a `pg::Entity3D _cube` with `Transform3D` + `MeshRenderer3D` pointing at a
`makeCube()` mesh + the existing `spriteSheet.png` (or a solid texture). Rotate the cube (or
orbit the camera) in `_onUpdate`. In `_onGeometryChange`, feed `Camera3D::mainCamera()` the new
viewport/aspect (mirror the current `Camera2D::setViewport` call). Keep the `DebugOverlay`.

> The 2D demo (`player_controller`, `player_control_logic`, sprite path) can be kept
> compiling but inactive, or removed — decide during implementation. Keeping the file
> skeleton is fine; the *scene content* becomes 3D.

## Key technical risk: depth buffer

3D needs correct occlusion. Two mitigations, in order:
1. **Back-face culling (`glEnable(GL_CULL_FACE)`) makes a single convex cube render correctly
   even without a depth buffer** — so Step 1 can succeed immediately. Author the cube with
   consistent CCW outward winding and cull back faces.
2. For anything non-convex (multiple objects, and all of Step 2's voxels) we need real
   **depth testing**. Verify whether the `GameEngineWidget`'s framebuffer has a depth
   attachment and whether the render context enables `GL_DEPTH_TEST` + clears depth. If not,
   that's the first thing to add (a depth attachment on the FBO + a clear-depth in the render
   pass). **Investigate this during Step 1; fully solve it by Step 2.** Note it in the status
   log with whatever we find.

Other gotchas to carry from the 2D path (memory `playground-2d-engine-step1`):
`activate()` a program before its `SamplerObject`; SSBO/UBO need GLSL ≥4.3 (fine on this
machine's 4.6 context); `GameEngineWidget` needs `invalidateRenderUnit()` each tick.

## Tests (`tests/`)

Add pure-logic tests (no window needed), following the project's coverage habit:
- `Transform3D.modelTransform` composes `T*R*S` correctly (spot-check a point transform).
- `Camera3D.projectionMatrix` matches `Matrix4x4::perspective(...)` for given params;
  `viewMatrix` matches `lookAt`/`inverse(model)` for a placed camera.
- `makeCube` produces the expected vertex/triangle counts (24 verts, 12 triangles) and
  outward winding.
- (Visual correctness of the rendered cube is hand-validated, not automated.)

## Open decisions for Step 1 (recommendations)

1. **Rotation representation** — `Quaternion` (recommended; `Matrix4x4::rotation(Quaternion)`
   exists and avoids gimbal issues) vs Euler `Vector3`. → Quaternion.
2. **Mesh type** — reuse `spk::TextureMesh2D` (recommended for Step 1; position is already
   `Vector3`) vs define `pg::Mesh3D` with a normal now. → Reuse now; add a normal-carrying
   `pg::Mesh3D` in Step 2 when lighting/voxels need it.
3. **Camera control** — auto-rotate the cube (recommended, simplest visible proof) plus an
   optional mouse-drag / ZQSD orbit. → Auto-rotate first, add orbit input if quick.

## Status / outcome (2026-07-01)

**Implemented and building.** Files added under `Playground/srcs/`: `components/transform3d.hpp`,
`components/entity3d.hpp`, `components/camera3d.{hpp,cpp}`, `components/mesh_renderer3d.hpp`,
`logics/mesh_render_logic.hpp`, `rendering/mesh_render_command.{hpp,cpp}`,
`geometry/primitive3d.hpp` (`makeCube`), shaders `resources/shaders/mesh/mesh.{vert,frag}`;
`game_scene_widget.{hpp,cpp}` + `main.cpp` rewired to the 3D scene. Builds clean via the
`playground` preset; a 5 s smoke run initialized and rendered without crashing or stderr.

Decisions taken during implementation (all within the plan), refined after user review:
- Reused `spk::TextureMesh2D` as the mesh (Vector3 position + UV). ✅
- **`MeshRenderCommand` takes the model matrix in its constructor** (write-once command;
  no `setModelTransform` setter) — per user feedback. ✅
- **Camera view-projection is emitted in the render process** (`MeshRenderLogic` emits a
  `spk::CameraUpdateRenderCommand` at binding 1 each frame, before the mesh draws). This is
  the shape needed for future multi-camera rendering (e.g. a minimap = one camera pass per
  camera + viewport). Model matrix delivered via a UBO at **binding 2**.
- **`Camera3D` is an explicit look-at camera** (eye/target/up): `viewMatrix()` =
  `Matrix4x4::lookAt`. A transform-driven variant (view = `inverse(model)` with the camera
  posed by a `Transform3D` + `Quaternion::lookAt`, plus a separate `CameraLogic` that only
  re-emitted on transform edition) was **tried and reverted** — see the gotcha below.
- Depth: **back-face culling only** (`GL_CULL_FACE`, CCW front), no depth test — correct for
  the single convex cube. Cube wound CCW-outward in `makeCube`.
- No tint; unlit fragment shader forces alpha = 1.

**⚠️ Gotcha (spk math convention):** do NOT derive a camera view matrix from a quaternion,
i.e. `inverse(translation · Matrix4x4::rotation(Quaternion::lookAt(...)))` does **not** equal
`Matrix4x4::lookAt(...)`. `spk::Matrix4x4::rotation(Quaternion)` and `spk::Matrix4x4::lookAt`
use different handedness/transpose conventions, so a transform-driven camera pointed the wrong
way and rendered nothing. Object rotation via `rotation(Quaternion)` is fine (the cube spins
and renders). For cameras, use `Matrix4x4::lookAt` directly. (Worth resolving in the library
before Step 2/3 uses 3D rotations more heavily.)

Deferred (as planned):
- **Automated tests** — the `pg::` layer isn't linked into the `tests/` target (it's app-level,
  not yet promoted to `spk::`), so unit tests wait until promotion. The math it leans on
  (`Matrix4x4::perspective/lookAt`, `Quaternion`) is already library-tested.
- **Depth-buffer investigation** — not needed for the cube; carried into Step 2 (voxels need it).

**Visual check: PASSED (2026-07-01).** Screenshot-verified: the textured cube renders in
perspective with three faces visible, animating; overlay shows 1 mesh / 12 triangles. Step 1
is functionally complete.

## Promotion note

Once solid and validated, `Transform3D`, `Entity3D`, `Camera3D`, `MeshRenderer3D`,
`MeshRenderLogic`, `MeshRenderCommand`, a normal-carrying `Mesh3D`, and `makeCube` are the
first candidates to lift into `spk::` (mirroring the existing 2D set under
`includes/structures/game_engine/` + `graphics/`). Keep them in `pg::` until Step 2 has
exercised them with real voxel geometry.
