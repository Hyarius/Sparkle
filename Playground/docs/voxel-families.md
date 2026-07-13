# Voxel families

Voxel families are a Playground content feature. Sparkle still receives ordinary voxel
types and runtime states; it does not know how those states were generated.

`resources/data/voxel_families/terrain.json` defines a base shape and named variants.
Each variant selects a shape and maps that shape's texture slots back to the base
material slots. A material opts in with:

```json
{
  "version": 2,
  "family": "terrain",
  "textures": {
    "top": [0, 0],
    "bottom": [2, 0],
    "side": [1, 0]
  }
}
```

The loader registers one semantic voxel type with contiguous states:

| State | Name | Shape |
|---:|---|---|
| 0 | `default` | family base (`cube`) |
| 1 | `slab` | half-height slab |
| 2 | `stair` | stair |
| 3 | `slope` | slope |

Biome palettes continue to reference only material IDs such as `grass-block` or a
weighted road pool. When terrain realization needs a transition slab, it keeps the
already-selected material type and switches to that type's named `slab` state.

Voxel-scale terrain relief is controlled by
`WorldGenConfig::terrainVariationFeatureBlocks`, `terrainVariationOctaves`,
`terrainVariationPersistence`, and `terrainVariationThreshold`. It is normalized,
multi-octave Perlin noise seeded per height level: its broad 256-block base octave spans
connected same-level macro cells, while lower-weight finer octaves prevent it from looking uniform. Relief is
still limited to one voxel up or down. The lower side of each one-voxel edge receives
a 1–`terrainVariationTransitionBlocks`-wide half-slab apron; a separate broad Perlin
field chooses its width along the edge. It is suppressed at height-plate edges, water,
towns, stairways, and prefab footprints so authored connections remain exact.
