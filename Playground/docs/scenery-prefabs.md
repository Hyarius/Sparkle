# Regional scenery prefabs

Outdoor scenery is data-driven. A biome's `worldgen.prefabs.scenery` array selects
prefabs and controls their expected density per suitable 8x8 plan cell and their minimum
center-to-center spacing in voxel columns. The generator rejects roads, water, entities,
stairs, biome boundaries, height changes and overlapping spacing radii.

## Shape vocabulary

- `cube` remains the best choice for joined block canopies.
- `cross` and `crossPlane` are passable foliage planes for bushes and palm fronds.
- `cuboid` is a box bounded by `min` and `max` coordinates inside one voxel cell. It is
  used for narrow trunks, cactus columns, logs, boulders and basalt pillars. The box is
  transformed by the cell's normal orientation and flip.

Example pillar:

```json
{
  "type": "cuboid",
  "min": [0.28, 0.0, 0.28],
  "max": [0.72, 1.0, 0.72],
  "textures": {"top": [3, 6], "side": [4, 6], "bottom": [3, 6]}
}
```

## Surface-region catalog

| Region | Primary silhouettes | Secondary dressing |
|---|---|---|
| Coast | palm trees | shore rocks, driftwood, bushes |
| Desert | tall cacti, cactus groves | sandstone clusters, sparse bushes |
| Forest | oak, birch and pine trees | fallen logs, rocks, dense bushes |
| Highland | sparse pine trees, stone spires | rock clusters, scrub |
| Meadow | isolated oak and birch trees | sparse rocks, bushes |
| Swamp | willow and dead trees | reed clumps, fallen logs, bushes |
| Tundra | frosted pine trees | snow-capped rocks, rare dead trees and scrub |
| Volcano | basalt clusters and spires | charred trees, extremely sparse scrub |

The cave biome has no outdoor `worldgen` block, so it does not participate in this
scatter pass. Cave dressing should be attached to the interior/cave generator when that
system begins placing room prefabs.

## Authoring guidance

- Keep one strong silhouette per prefab; variety comes from pools and random quarter-turns.
- Use `solid` for trunks, cacti and rocks so navigation routes around their voxel column.
- Use `passable` plus `losTransparent` for leaves and reeds.
- Prefer several low-density entries over a single high-density mixed prefab. This keeps
  region tuning independent and allows the spacing solver to interleave species.
- A scenery prefab's authored bounding box is also its cleared stamp volume. Avoid large
  empty margins because they erase other content when stamped.

## Wild-encounter bushes

Every bush definition uses `crossPlane`, which constructs a
`DiagonalCrossVoxelShape`, and is passable so actors can enter it. Every variant carries
the exact `"Bush"` tag; `VoxelData::hasTag` compares tags case-insensitively, so encounter
code may query either `hasTag("Bush")` or `hasTag("bush")`.

Bushes occupy the passable cell directly above the standable ground cell. Encounter code
tracking a navigation/ground cell should therefore inspect the actor's occupied air cell
(normally `groundCell + [0,1,0]`) rather than only the solid ground voxel below it.
