# System — Rendering, 3D Layer & Cameras

The promoted `spk::` 3D rendering layer, its depth/lighting behavior,
chunk/creature/overlay rendering, Playground camera controllers, and mouse picking.

Diagrams: [02-class-rendering-3d.puml](../diagrams/02-class-rendering-3d.puml).
Plan: steps [03](../plan/steps/step-03-render3d-hardening.md),
[11](../plan/steps/step-11-battle-overlay.md), [13](../plan/steps/step-13-promotion-3d.md).

---

## 1. Promoted 3D foundation (Step 13)

| Piece | File | Role |
|---|---|---|
| `spk::Transform3D` | `includes/structures/game_engine/spk_transform_3d.hpp` | position/quaternion/scale → cached model matrix, recursively invalidated through entity hierarchy changes |
| `spk::Entity3D` | `includes/structures/game_engine/spk_entity_3d.hpp` | `spk::Entity` owning a `Transform3D` |
| `spk::Camera3D` | `spk_camera_3d.hpp/.cpp` | eye/target/up + perspective params; `viewMatrix()` = `Matrix4x4::lookAt`; static main-camera slot |
| `spk::TextureMeshRenderer3D` | `spk_texture_mesh_renderer_3d.hpp` | mesh + texture + pass/tint component data |
| `spk::TextureMeshRenderLogic` | `spk_texture_mesh_render_logic.hpp` | emits camera (UBO 1), directional light (UBO 3), then opaque and translucent draws |
| `spk::DrawTextureMesh3DRenderCommand` | `spk_draw_texture_mesh_3d_render_command.*` | one textured 3D draw with model UBO 2 and complete GL-state restoration |
| `spk::PrimitiveObject::CreateCube` | `spk_primitive_object.hpp` | centered CCW cube with per-face normals and UVs |
| shaders | `resources/shaders/mesh_3d/` | embedded VP·M textured directional-light shaders |

Contracts locked by D04/D22: camera VP uploaded **in-stream per pass**; model matrix is
write-once per command; `lookAt` for views; CCW right-handed math.

## 2. Hardening (step 03)

- **`spk::TextureVertex3D`** `{ Vector3 position; Vector3 normal; Vector2 uv; }` +
  **`spk::TextureMesh3D`** = `spk::GenericMesh<TextureVertex3D>` with layout attributes
  (0=vec3, 1=vec3, 2=vec2). The renderer and draw command consume this mesh; the cube gains
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

`spk::Camera3D` is the component; Playground **controllers** are logics driving it:

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
pickCell(world, ray)   -> optional<VoxelHit{cell, enterFace, distance, normal}>
    // DDA broad phase, then oriented/flipped render-polygon intersection in each solid cell
pickStandable(world, ray) -> optional<Vector3Int>
    // DDA hit then descend the column to the topmost standable cell (board.md rule)
```

Picking uses simultaneous-axis DDA advancement for exact edge/corner ties, so cells touched
only at zero area are not tested in arbitrary axis order. The narrow phase returns the first
actual shape surface; `enterFace` is `Count` for oblique faces while `normal` always carries
the polygon normal. A ray starting inside closed geometry returns its nearest exit surface;
a ray starting exactly on a crossed surface may return distance zero, while coplanar grazing
does not count as a crossing. Origins, directions, maximum distance, and the traversed cell
range are validated before any float-to-integer conversion.

Used by exploration click-to-move (D27) and battle targeting (D14). Hover feedback = a
highlight quad on the picked cell every frame the mouse moves.

## 6. Promotion result (step 13)

The 3D foundation is owned directly by Sparkle under the exact names listed in section 1;
there are no Playground aliases or duplicate implementations. `spk::DirectionalLight` is
a 48-byte aligned `std140` value block, and
`spk::DirectionalLightUpdateRenderCommand(bindingPoint, light)` uploads it in one operation.
The explicit binding point keeps the command reusable outside the texture-mesh shader,
whose documented contract uses camera/model/light bindings 1/2/3. Playground retains only
game-specific rendering and controllers.

## 7. Testing

Headless: transform composition (parent chains, cache invalidation), camera matrices
(view·projection against hand values), screenToRay round-trips, DDA raycast truth table.
Visual (hand-validated per project convention): depth correctness (two overlapping cubes),
lighting direction, translucent overlay blending, camera blend smoothness.
