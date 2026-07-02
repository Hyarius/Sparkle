# Step 2 — Voxel Core + Mesher (+ modeling-tool seed)

**Phase:** 2 (ERELIA.md §9) · **Namespace:** `pg::` · **Depends on:** Step 1 (3D render path)

## Goal / Definition of Done

Port Erelia's voxel data model and build a **neighbor-occlusion mesher** that turns a
`VoxelGrid` into a `spk::` mesh rendered by Step 1's `MeshRenderLogic`. Load `VoxelDefinition`s
from **JSON**. Render a **hand-authored voxel chunk** (a few shapes, textured, correct
occlusion + depth). This is also the seed of the **3D voxel modeling tool**.

Done means: a multi-voxel structure (cubes + at least one slab/slope) renders correctly with
depth testing and face culling; empty/interior faces are culled; changing the JSON changes the
scene; unit tests cover the data model + mesher face-culling logic.

## What to port (from Unity `Assets/Scripts/Voxel/`, see ERELIA.md §3a)

Reference the Unity C# closely — it is the spec for geometry/occlusion.

| `pg::` file | Mirrors Unity | Notes |
|-------------|---------------|-------|
| `voxel/voxel_enums.hpp` | `VoxelEnums.cs` | `Traversal{Obstacle,Walkable}`, `Orientation{+X,+Z,-X,-Z}`, `FlipOrientation{+Y,-Y}`, `AxisPlane{PosX..NegZ}`. |
| `voxel/voxel_cell.hpp` | `VoxelCell.cs` | `int id=-1`, `Orientation`, `FlipOrientation`; `isEmpty()`. |
| `voxel/voxel_grid.hpp` | `VoxelGrid.cs` | dense 3D array of `VoxelCell` + bounds check. Consider a templated `VoxelGrid<CX,CY,CZ>` for chunks. |
| `voxel/voxel_data.hpp` | `VoxelData.cs` | traversal + tags. |
| `voxel/voxel_shape.hpp` + `voxel/shape/*.hpp` | `VoxelShape.cs` + `Shape/*.cs` | Abstract `VoxelShape` builds render `FaceSet` (InnerFaces + 6 OuterShell axis faces) + collision `FaceSet` + mask set + `CardinalHeightSet` per Y-flip. Concrete: **Cube, Slab, Slope, Stair, CrossPlane**. Faces→polygons→`Vertex{pos,uv}`. Start with **Cube + Slab** (defer Stair/Slope/CrossPlane if time). |
| `voxel/voxel_definition.hpp` | `VoxelDefinition.cs` | `VoxelData` + a `VoxelShape` (polymorphic, `std::unique_ptr`/variant); `Initialize()` builds the faces. |
| `voxel/voxel_registry.hpp` | `VoxelRegistry.cs` | id → `VoxelDefinition*`. |
| `voxel/voxel_mesher.hpp` | `VoxelMesher.{cs,Geometry.cs}` | Build a `pg::Mesh3D` from a `VoxelGrid`+registry with **neighbor face-occlusion culling**: for each cell, map world axis-plane → local plane via cell orientation/flip; skip an outer face if the neighbor occludes it; always emit inner faces when any outer face is visible. Port `MapWorldPlaneToLocal` + occlusion checks. |

New engine-side helper likely needed: **`pg::Mesh3D`** = `spk::GenericMesh<Vertex3D>` where
`Vertex3D { Vector3 position; Vector2 uv; Vector3 normal; }` (normal for lighting). Add matching
layout attributes (loc 0/1/2). Upgrade Step 1's shader to use the normal for simple directional
shading. (If Step 1 reused `TextureMesh2D`, this is the point where we graduate to a real 3D
mesh — a promotion candidate.)

## JSON loading

- Decide the JSON lib first (check `vcpkg.json` / `includes/utils`; if none, add one, e.g.
  nlohmann/json, via vcpkg — this itself may be a small promotion).
- `pg::VoxelDefinitionLoader` parses a `voxels/*.json` (traversal, tags, shape type + shape
  params, texture/UV refs) into `VoxelDefinition`s registered in a `VoxelRegistry`.
- Author 3–5 real voxel JSONs (grass cube, dirt cube, stone slab, a slope) + a texture.

## Depth buffer

Step 2 **requires** real depth testing (non-convex geometry). Resolve the depth-attachment
question flagged in Step 1: ensure the render target has a depth buffer, depth test is
enabled, and depth is cleared each frame.

## Modeling-tool seed

Not the full editor yet — just the reusable pieces this step naturally produces: a
mesh-preview path (render one `VoxelDefinition`'s mesh in isolation) and a shape-parameter
struct that a future tool UI can edit. Keep the editor UI for a later step.

## Tests

Data model (grid bounds, cell defaults, orientation/flip), mesher face-occlusion (two adjacent
cubes hide the shared face; an exposed face survives), cube/slab vertex counts, JSON round-trip.
Hand-validate the rendered chunk.
