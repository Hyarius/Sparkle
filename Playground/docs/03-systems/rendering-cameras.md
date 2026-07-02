# System — Rendering, 3D Layer & Cameras

The `pg::` 3D rendering layer (Step-1 base, verified), its hardening (depth + lighting),
chunk/creature/overlay rendering, camera controllers, and mouse picking. First `spk::`
promotion candidate (D18).

Diagrams: [02-class-rendering-3d.puml](../diagrams/02-class-rendering-3d.puml).
Plan: steps [03](../plan/steps/step-03-render3d-hardening.md),
[11](../plan/steps/step-11-battle-overlay.md), [13](../plan/steps/step-13-promotion-3d.md).

---

## 1. What exists (Step 1, screenshot-verified — keep, do not rewrite)

| Piece | File | Role |
|---|---|---|
| `pg::Transform3D` | `srcs/components/transform3d.hpp` | position/quaternion/scale → cached model matrix, composed with nearest ancestor Transform3D |
| `pg::Entity3D` | `srcs/components/entity3d.hpp` | `spk::Entity` owning a Transform3D |
| `pg::Camera3D` | `srcs/components/camera3d.*` | eye/target/up + perspective params; `viewMatrix()` = `Matrix4x4::lookAt`; static main-camera slot |
| `pg::MeshRenderer3D` | `srcs/components/mesh_renderer3d.hpp` | data: mesh* + texture* |
| `pg::MeshRenderLogic` | `srcs/logics/mesh_render_logic.hpp` | emits `CameraUpdateRenderCommand`(UBO 1) then one `MeshRenderCommand` per renderer |
| `pg::MeshRenderCommand` | `srcs/rendering/mesh_render_command.*` | one draw: model matrix (UBO 2, ctor-injected), texture sampler, back-face culling |
| `pg::makeCube` | `srcs/geometry/primitive3d.hpp` | CCW unit cube as `spk::TextureMesh2D` |
| shaders | `resources/shaders/mesh/mesh.vert/.frag` | VP·M transform; unlit texture |

Contracts locked by D04/D22: camera VP uploaded **in-stream per pass**; model matrix is
write-once per command; `lookAt` for views; CCW right-handed math.

## 2. Hardening (step 03)

- **`pg::MeshVertex3D`** `{ Vector3 position; Vector3 normal; Vector2 uv; }` +
  **`pg::Mesh3D`** = `spk::GenericMesh<MeshVertex3D>` with layout attributes (0=vec3,
  1=vec3, 2=vec2). `MeshRenderer3D`/`MeshRenderCommand` switch to `Mesh3D`; `makeCube` gains
  per-face normals.
- **Depth**: `MeshRenderCommand::execute` enables `GL_DEPTH_TEST` (LESS) for its draw and
  restores previous state — same pattern as its existing cull toggle. The frame already
  clears depth (`spk::OpenGLClearCommand` defaults to color+depth — verified).
- **Lighting**: single directional light, lambert + ambient in `mesh.frag`
  (`max(dot(n, -lightDir), 0) * lightColor + ambient`), parameters in a small UBO
  (binding 3, uploaded by `MeshRenderLogic` alongside the camera). Deliberately minimal —
  the voxel look carries the game.
- **Translucent pass**: commands carry an opaque/translucent flag; `MeshRenderLogic` emits
  opaque first, then translucent (blend on, depth **write** off, depth test on) — needed by
  the battle overlay and cross-plane flora.

## 3. Scene rendering

- **Chunks**: the `pg::Chunk` component (a `spk::SynchronizableTrait`, D32) owns its baked
  `renderMesh`/`maskMesh`; `pg::ChunkSynchronizationLogic` re-bakes chunks that
  `needsSynchronization()` in the update phase; `pg::ChunkRenderLogic` emits their draws
  (voxel atlas texture) in the render phase.
- **Creatures/actors**: a creature view entity per placed unit — child entities per model
  part (from `models/*.json`), each with `MeshRenderer3D`; the animation rig drives part
  transforms ([animation.md](animation.md)). Placeholder-cube model in M1 (D26).
- **Battle overlay** (step 11, D31): `pg::BoardOverlayView` component + logic, rendering
  through the **voxel mask system** — each highlighted cell contributes its shape's *mask
  faces* (draped over the walkable surface: flat on cubes, slanted on slopes, stepped on
  stairs), assembled by `VoxelMesher::buildMaskMesh` into one mesh per overlay rebuild and
  textured from the **mask sprite sheet** (`resources/textures/mask.png`; category → atlas
  cell via `game-rules.json` `overlayMasks`). Categories: deployment zones, movement range,
  hovered path, ability range, AoE preview, LoS-blocked, hovered, invalid — stacked layers
  get the standard `0.001·layer` Y offset. Overlay state is a plain struct
  (`pg::BoardOverlayState`: sets of cells per category) written by battle phases/input,
  read by the view — battle logic stays headless. The mesh is translucent-pass rendered.

## 4. Cameras

`pg::Camera3D` stays a component; **controllers** are logics driving it:

- `pg::OrbitFollowCameraLogic` (exploration): target = player position + eye height;
  yaw/pitch from ZQSD or right-drag, wheel zoom, clamped pitch; smooothed follow.
- `pg::TacticalCameraLogic` (battle): frames the board (anchor + size ⇒ distance), fixed
  ~50° pitch, yaw aligned to the player deployment side; **blends** (position+target lerp,
  ~0.6 s ease) from the exploration pose on battle start and back on end — no cut (D03).
- Controllers switch with the mode; both write the same `Camera3D`, so `MeshRenderLogic`
  and picking are controller-agnostic. A future minimap = second `Camera3D` + its own pass
  (D04) — supported by construction, built in the polish phase.

## 5. Mouse picking (`pg::MousePicker`, step 07)

```
screenToRay(camera, viewportSize, mousePx) -> {origin, direction}
    // invert viewProjection, unproject NDC near/far points (Matrix4x4::inverse exists)
pickCell(world, ray)   -> optional<VoxelHit{cell, enterFace, distance}>   // WorldRaycaster DDA
pickStandable(world, ray) -> optional<Vector3Int>
    // DDA hit then descend the column to the topmost standable cell (board.md rule)
```

Used by exploration click-to-move (D27) and battle targeting (D14). Hover feedback = a
highlight quad on the picked cell every frame the mouse moves.

## 6. Promotion shape (step 13, sign-off required)

Lift into `spk::`: `Transform3D`, `Entity3D`, `Camera3D` (minus the pg-specific main-camera
static — promote as an explicit `spk::Camera3D` with the same static slot), `MeshVertex3D/
Mesh3D` (as `spk::TextureMesh3D`), `MeshRenderer3D`, `MeshRenderLogic`,
`MeshRenderCommand` (as `spk::DrawMesh3DRenderCommand`), `makeCube`, the mesh shaders
(embedded like existing spk shader resources), + `tests/TUs` coverage (transform hierarchy,
camera matrices, command state restoration). `pg::` code then aliases/uses the `spk::`
types; Playground keeps only game-specific rendering (chunks, overlay, creature views).

## 7. Testing

Headless: transform composition (parent chains, cache invalidation), camera matrices
(view·projection against hand values), screenToRay round-trips, DDA raycast truth table.
Visual (hand-validated per project convention): depth correctness (two overlapping cubes),
lighting direction, translucent overlay blending, camera blend smoothness.
