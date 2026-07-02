# Step 05 — VoxelMesher with neighbour occlusion

**Phase B · critical path · needs steps 03, 04**

## Goal

Grid → `Mesh3D` with occlusion culling; a hand-stamped showcase chunk renders correctly.
This is the visual proof of the whole voxel pipeline.

## Reading

[voxel.md §Shapes/§Meshing](../../03-systems/voxel.md) ·
Unity `VoxelMesher.cs` + `VoxelMesher.Geometry.cs` (the reference implementation —
especially `MapWorldPlaneToLocal`, `TransformFaceCached`, `IsFaceOccludedByNeighbor`).

## Files

- **Create** `srcs/voxel/voxel_mesher.hpp/.cpp` — static:
  `buildRenderMesh(const VoxelGrid&, const VoxelRegistry&) -> Mesh3D`;
  `buildMaskMesh(span<const Vector3Int> cells, maskOf, const ICellLookup&) -> Mesh3D` —
  emits each cell's shape **mask faces** UV'd to the mask atlas cell returned by
  `maskOf(cell)`, stacked-layer Y offset `0.001·layer` (**M1-critical: the battle overlay
  renders through this**, D31);
  internals: `mapWorldPlaneToLocal(plane, orientation, flip)`,
  `transformFace(face, orientation, flip)` with a per-run cache keyed on
  (face ptr, orientation, flip), `isFaceOccludedByNeighbor(...)` per
  [voxel.md](../../03-systems/voxel.md) occlusion rule (cross-planes never occlude/occluded;
  out-of-bounds neighbours never occlude → boundary faces render), flat normals from
  polygon planes, fan triangulation via `Mesh3D::addShape`.
- **Create** `resources/textures/mask.png` — placeholder mask sprite sheet (4×4 grid of
  visually distinct semi-transparent patterns: solid fill, border ring, hatching, arrows…);
  the `overlayMasks` mapping in `game-rules.json` addresses it.
- **Create** `srcs/world/showcase.hpp/.cpp` — `pg::buildShowcaseGrid() -> VoxelGrid`:
  a 12×4×12 grid containing: a buried 3³ cube block (proves interior culling), a ground
  plane, each shape at each of the 4 orientations, a slope ramp chain, a stair run, bushes,
  a slab row, one negY-flipped slope (ceiling wedge) under an overhang.
- **Modify** `game_scene_widget.*` — replace the two-cube scene with one entity rendering
  the showcase grid's mesh (keep the orbit-style rotation or a fixed camera + the debug
  overlay's mesh/tri counts).

## Tests (`[test]`)

- Fully-buried cube contributes 0 faces; a lone cube contributes 6; two adjacent cubes 10.
- Slope beside cube: cube face behind the slope's full back (PosZ) face is culled; cube
  face beside the slope's triangle side is **not**.
- CrossPlane neighbours cull nothing; crossPlane renders both quads.
- Orientation: a slope rotated 4 ways occludes the correct neighbour face each time
  (drive via `mapWorldPlaneToLocal` assertions + a meshed pair's face count).
- Mask mesh: a cube cell yields one flat quad at its top height; a slope cell yields the
  slanted mask face; two stacked layers offset by 0.001; UVs land in the requested atlas
  cell.
- Mesh totals for the showcase grid pinned once (regression guard; regenerate deliberately).

## Definition of Done

- `[build]`/`[test]` green.
- `[run]` visual (user validates): the showcase chunk — no missing faces, no z-fighting, no
  interior faces leaking, slopes/stairs/slabs/bushes correct at every orientation, lighting
  reads correctly. **This screenshot is the flagship checkpoint of Phase B.**
- Meshing the showcase grid takes < a few ms (log it once via the overlay).
