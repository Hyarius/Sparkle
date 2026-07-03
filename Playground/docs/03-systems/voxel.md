# System — Voxel Core & Mesher

The voxel data model and mesh generation. This is the foundation the world, the board, and
the modeling tool all stand on. The Unity `Assets/Scripts/Voxel/` implementation is a
worked example worth consulting for behaviors (D28) — the design below is the authority,
with renames per [D20](../decision-log.md) and no collision geometry (D30).

Diagrams: [03-class-voxel.puml](../diagrams/03-class-voxel.puml).
Plan: steps [04](../plan/steps/step-04-voxel-core.md), [05](../plan/steps/step-05-voxel-mesher.md).
JSON: [02-data-model.md §3](../02-data-model.md).

---

## Responsibilities

- Define what a voxel **is** (`VoxelDefinition` = data + shape) and what a placed voxel
  **instance** is (`VoxelCell` = id + orientation + flip).
- Build shape geometry once per definition (`VoxelShape::initialize()`), and per-grid
  meshes on demand (`VoxelMesher`) with neighbour-occlusion culling.
- Provide **mask faces** per shape — how an overlay decal drapes over the voxel's top
  surface — consumed by the battle overlay (D31, M1 scope) and later by world decals.
- Provide the traversal inputs (solidity + `CardinalHeightSet`) that navigation
  ([board.md](board.md), [world.md](world.md)) consumes. All spatial queries are analytic
  (DDA raycasts + walk heights) — **there is no collision geometry** (D30).

Out of scope here: chunk streaming (world.md), walkability graph (board.md), rendering
commands (rendering-cameras.md).

## Key types (`Playground/srcs/voxel/`)

```
pg::VoxelTraversal   enum class { Solid, Passable }                       (D20)
pg::VoxelOrientation enum class { PositiveX, PositiveZ, NegativeX, NegativeZ }  // rotation about Y
pg::VoxelFlip        enum class { PositiveY, NegativeY }
pg::VoxelAxisPlane   enum class { PosX, NegX, PosY, NegY, PosZ, NegZ }

pg::VoxelData        { VoxelTraversal traversal; std::vector<std::string> tags;
                       bool hasTag(view) const;  // trimmed, case-insensitive }

pg::VoxelShape (abstract)
  ├─ built products (filled by initialize()):
  │    FaceSet   render;      // innerFaces + outerShell (6 optional axis faces)
  │    MaskSet   mask;        // overlay decal faces per flip (posY/negY lists) — D31
  │    CardinalHeights heights; // per flip: {posX, negX, posZ, negZ, stationary} floats
  ├─ virtuals: _constructRenderFaces(), _constructMask(),
  │            _constructPositiveYHeights(), _constructNegativeYHeights() (=positive)
  └─ concrete: CubeShape, SlabShape, SlopeShape, StairShape, CrossPlaneShape
  // NO collision products (D30): raycasts are DDA over cells, movement is walk-height
  // analytic. If a future feature needs sub-voxel surface geometry (physics debris,
  // shadow-casting geometry), add a dedicated product then, with its consumer documented.

pg::VoxelShape::Face     { std::vector<Polygon> }        // Polygon = ≥3 Vertex{pos, uv}
pg::VoxelCell            { int32_t id = -1; VoxelOrientation; VoxelFlip; bool isEmpty() }
pg::VoxelGrid            { Vector3Int size; dense std::vector<VoxelCell>;
                           cell(x,y,z) accessors; bounds checks }
pg::VoxelRegistry        // Registry<VoxelDefinition> + stable numeric id per string id
pg::VoxelMesher          // static: buildRenderMesh / buildCollisionMesh / buildMaskMesh
```

Geometry vertices are authored in **local cell space** `[0,1]³`; a cell's world offset is its
integer coordinate. UVs address the voxel atlas (each face's texture slot gives an atlas cell
→ UV rect; the shape bakes final UVs at `initialize()`).

## Shapes — geometry, heights, occlusion

All shapes: local +Z is the shape's "up-slope / back" direction before orientation is
applied. Heights are surface heights in `[0,1]` at each edge midpoint + center.

| Shape | Render faces | Heights (posY flip) | Notes |
|---|---|---|---|
| **Cube** | outer shell only: 6 unit quads | all 1.0 | the baseline |
| **Slab** | shell: 4 side quads at `height`, bottom; top as shell face at y=`height` | all = `height` (default 0.5) | side faces occlude only the covered portion — v1 keeps it simple: side shell faces are *not* occludable (inner), bottom is shell |
| **Slope** | inner: slope quad (0 at −Z → 1 at +Z); shell: back (PosZ), bottom (NegY), 2 side triangles | posZ 1.0, negZ 0.0, sides+stationary 0.5; **negY flip: all 1.0** (ceiling wedge) | mirror of Unity `VoxelSlopeShape` |
| **Stair** | inner: `stepCount` treads + risers; shell: back, bottom, side profiles | posZ 1.0, negZ = 1/(2·steps)… stationary 0.5 (compute midpoints per stepCount) | Unity `VoxelStairShape` is the reference for exact faces |
| **CrossPlane** | inner: two crossed quads (diagonals), double-sided; **no shell** | all 0.0 — never standable; typically `passable` | bushes/grass; never occludes neighbours, never occluded |

**Occlusion rule (the heart of the mesher):** a shell face on world plane P of cell A is
skipped when the neighbour across P has, on its facing plane, a shell face that fully covers
it. v1 implements the Unity approximation: neighbour occludes if it is non-empty and its
facing shell face exists and is renderable (full-quad assumption for cube-like faces; slope
side-triangles never occlude). Inner faces are emitted whenever *any* shell face of the cell
is visible (or the cell has no shell at all, e.g. cross-planes).

**Orientation:** at mesh time the mesher maps each **world** plane to the cell's **local**
plane (`mapWorldPlaneToLocal(plane, orientation, flip)`) to pick which authored face to test,
then transforms the chosen face's vertices by the orientation rotation (about the cell
center, Y axis) and flip (mirror Y, reverse winding). Cache transformed faces per
(face, orientation, flip) — the Unity mesher's `FaceTransformCache` — since definitions are
immutable.

**CardinalHeights under orientation:** heights are authored local; navigation queries world
directions. `resolveWorldHeights(local, orientation)` remaps directions (see Unity
`VoxelTraversalUtility.ResolveLocalDirection` — port its switch exactly; it is subtle and
already debugged). Flip selects the posY/negY authored set.

## Meshing outputs

- `buildRenderMesh(grid, registry) -> spk::TextureMesh3D` — vertex = position + **normal** + uv.
  Normals are per-face (flat), computed from the polygon plane at emission.
- `buildMaskMesh(cells, maskOf, registry) -> spk::TextureMesh3D` — **M1-critical: this is how the
  battle overlay renders (D31).** For each requested cell, emits the cell's shape **mask
  faces** (flat quad on cubes/slabs, slanted on slopes, stepped on stairs — draped over the
  walkable surface) with a small Y offset per stacked layer (`0.001 * layerIndex`), UV'd
  into the **mask sprite sheet** (`resources/textures/mask.png`) at the atlas cell chosen
  per cell by the caller (`maskOf(cell) -> atlas cell` — overlay categories map via
  `game-rules.json` `overlayMasks`). The same output later serves world decals
  (paths/roads) with a persistent mask layer per chunk.
- **No collision mesh** (D30) — see the note in Key types.

Meshes triangulate polygons as fans (`spk::GenericMesh::addShape` already does exactly
this). One render mesh per chunk; rebuild the whole chunk mesh on any cell edit (16³ is
cheap; see [01-architecture.md §11](../01-architecture.md)). Mask meshes for the battle
overlay are rebuilt whenever the overlay state changes (a handful of cells — trivial).

## Traversal contract (consumed by board/world)

- `isSolid(cell)` = non-empty ∧ definition.traversal == Solid.
- `isPassableSpace(cell)` = empty ∨ definition.traversal == Passable.
- **Standable(x,y,z)** = isSolid(cell) ∧ isPassableSpace(above) ∧ isPassableSpace(above²).
- Walk height at a cell edge = `y + resolveWorldHeights(shape.heights(flip), orientation).get(direction)`.
- Adjacent cells connect when |exitHeight − entryHeight| ≤ `maxVerticalTraversalGap` (0.5).

This contract is what makes slopes and stairs walkable in M1 (D11): a slope's 0.0 edge
meets flat ground (gap 0), its 1.0 edge meets the upper cube's surface — note the upper
*cell* is one Y above, so the traversal graph must connect nodes across Y layers via a
per-column best-candidate scan (smallest height gap wins;
[board.md](board.md) specifies it; Unity's `VoxelTraversalGraphBuilder` is a worked
example of the same idea).

## Dependencies

Uses: `spk::GenericMesh`, `spk::Vector3(Int)`, the voxel atlas texture, `pg::Registry`.
Used by: world (chunks), board (navigation), rendering (chunk meshes), tools (modeler).

## Testing

Headless-testable end to end (PlaygroundTests): shape face/height construction per shape ×
orientation × flip; occlusion cases (buried cube emits nothing; exposed face emits; slope
next to cube; cross-plane never occludes); mesher vertex/index counts on tiny grids; the
traversal contract truth table. Visual hand-validation: one showcase chunk containing every
shape at every orientation (step 05's Definition of Done).
