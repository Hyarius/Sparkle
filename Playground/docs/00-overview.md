# Playground overview

`SparklePlayground` generates a deterministic voxel world from a seed and streams chunks
around the player. The exploration controller provides cell picking, pathfinding,
movement, camera orbit, and zoom. Press F7 to toggle streaming/render diagnostics.

`EreliaTools` contains the voxel modeler used to create, duplicate, edit, preview, and save
voxel definitions under `resources/data/voxels`.

No gameplay loop beyond exploration is implemented in this playground.
