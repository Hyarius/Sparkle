# Step 04 — Voxel core: definitions, 5 shapes, heights, cells/grids, JSON

**Phase B · critical path · needs steps 01, 03**

## Goal

The full voxel data model — all five shapes with cardinal heights (slopes/stairs walkable
from M1, D11) — loaded from JSON into a registry. No meshing yet (step 05).

## Reading

[voxel.md](../../03-systems/voxel.md) (whole doc — it is the spec) ·
[03-class-voxel.puml](../../diagrams/03-class-voxel.puml) ·
[02-data-model.md §3](../../02-data-model.md) ·
Unity `Assets/Scripts/Voxel/` — especially `VoxelShape.cs`, `Shape/*.cs` (copy the exact
face vertices/UV winding and the height tables), `CardinalHeightSet.cs`, `VoxelEnums.cs`.

## Files

`srcs/voxel/`: `voxel_enums.hpp` (Traversal{Solid,Passable}, Orientation, Flip, AxisPlane),
`voxel_data.hpp` (traversal + tags + hasTag), `voxel_shape.hpp/.cpp` (abstract base +
FaceSet/Face/Polygon/Vertex + MaskSet + CardinalHeightSet + CardinalHeightCollection +
`createRectangle/createTriangle` helpers + atlas-UV helper), `shapes/cube_shape.*`,
`shapes/slab_shape.*`, `shapes/slope_shape.*`, `shapes/stair_shape.*`,
`shapes/cross_plane_shape.*`, `voxel_cell.hpp`, `voxel_grid.hpp`,
`voxel_definition.hpp`, `voxel_parser.cpp` (JSON + shape factory),
`voxel_registry.hpp/.cpp` (string↔numeric id, sorted assignment),
`voxel_traversal.hpp/.cpp` (`isSolid/isPassableSpace/isStandable/resolveWorldHeights/
resolveLocalDirection` — the orientation→direction remap is subtle; derive it carefully and
lock it with the truth-table tests below. Unity's `VoxelTraversalUtility` shows a working
remap if you want to cross-check results, D28).
**No collision products anywhere** (D30) — shapes build render faces, mask faces, heights.

`resources/data/voxels/`: `grass-block.json`, `dirt-block.json`, `stone-block.json`,
`slope-grass.json`, `stair-stone.json`, `slab-stone.json`, `bush.json` (crossPlane,
passable, tags `["bush","flora"]`), `wall-stone.json`.
`resources/textures/voxels.png`: a placeholder atlas (8×8 grid of distinct colored/labelled
cells — generate with any image tool or a tiny script; ugly is fine, distinct matters).

Registries: add `voxels` to `Registries::loadAll` (after config).

## Work items

1. Shapes construct faces/UVs/heights at `initialize()` exactly per
   [voxel.md](../../03-systems/voxel.md) §Shapes (slope heights: posZ 1, negZ 0, others 0.5;
   negY-flip all 1; stair heights from stepCount midpoints; crossPlane all 0, no shell).
2. Atlas UV baking: texture slot `[col,row]` → UV rect (uniform grid from the sheet).
3. Parser: shape factory (`cube|slab|slope|stair|crossPlane`), per-shape slot validation
   (missing/unknown slot = error), traversal/tags.
4. `VoxelGrid`: dense storage, bounds-checked accessors, `isWithinBounds`.

## Tests (`[test]`) — this step is test-heavy on purpose

Per shape: face counts, a spot-checked vertex table, height tables (both flips).
`resolveWorldHeights` for all 4 orientations × asymmetric heights (slope) — truth table.
Standable contract on a hand-built 3-cell column (solid+clear², head-blocked, passable-only).
Parser: each sample file parses; error cases per
[howto/add-json-definition.md](../../howto/add-json-definition.md) §4. Registry numeric-id
stability (sorted by id).

## Definition of Done

`[build]`/`[test]` green; `[run]` unchanged (nothing rendered yet); all 8 voxel JSON files
load at boot (log the count). No `spk::` changes.
