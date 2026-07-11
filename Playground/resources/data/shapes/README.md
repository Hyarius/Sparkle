# Voxel shapes

Every voxel's render geometry is data: a shape file here, referenced by name (the file
stem) from `../voxels/*.json`. Adding a custom shape means adding a JSON file — no C++,
neither in Playground nor in Sparkle.

## Shape schema (`shapes/<name>.json`)

```json
{
  "version": 1,
  "polygons": [
    {
      "slot": "top",
      "positions": [[0, 1, 1], [1, 1, 1], [1, 1, 0], [0, 1, 0]],
      "uvs": [[0, 0], [1, 0], [1, 1], [0, 1]]
    }
  ],
  "heights": { "positiveY": 1.0, "negativeY": 1.0 }
}
```

- `positions` are in normalized cell space (`0..1` on every axis), at least three per
  polygon. Coordinates within `0.0001` of `0`/`1` are snapped exactly onto the boundary.
- `uvs` are local to the atlas cell (`0..1`), one per vertex. The voxel referencing the
  shape decides which atlas cell each `slot` maps to.
- **Winding matters**: vertices must be listed counter-clockwise when viewed from the
  side the face should be visible from. A two-sided plane (see `cross-plane.json`) is
  two polygons with mirrored vertex order.
- **Outer/inner classification is computed, never declared**: a polygon lying on one of
  the six unit-cube boundary planes with its normal pointing out of the cell joins that
  plane's outer shell (it can occlude neighbors and be occluded); everything else is an
  inner face (drawn unless the cell is sealed on all six sides).
- `heights` are the walk heights for the navigation graph, authored explicitly (never
  derived from the geometry). Each flip key takes either one number (uniform) or the
  five directional values (`positiveX`, `negativeX`, `positiveZ`, `negativeZ`,
  `stationary`). Omit the block (or a flip) for non-walkable scenery: it means zero.

## Voxel schema (`voxels/<name>.json`, version 2)

```json
{
  "version": 2,
  "traversal": "solid",
  "tags": ["ground", "grass"],
  "shape": "cube",
  "textures": { "top": [0, 0], "bottom": [2, 0], "side": [1, 0] },
  "transparency": 0.5,
  "fluid": { "maxSpread": 8 }
}
```

- `textures` must provide exactly the slots the shape's polygons use — no more, no less.
- `transparency` (optional, default 0) routes faces to the transparent mesh.
- `fluid` (optional) marks the voxel as a fluid source; the registry synthesizes its
  fill-stage slabs (`<name>#1..maxSpread`) reusing the `top` texture cell.
