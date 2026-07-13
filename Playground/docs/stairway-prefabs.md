# Adaptive stairways

One world-plan height level is `blocksPerLevel` voxels (currently 3). Stairways are
composed at generation time from two prefab families, so a single layout adapts to any
height difference. Each family has exactly two prefabs per biome:

- **Road climbs** (built look): `<biome>-stair-length`, a three-wide flight of stairs
  climbing one level along its local +Z, and `<biome>-stair-platform`, a 3x3 pad of the
  biome's road block.
- **Wild climbs** (natural look): `<biome>-slope-length`, the same one-level run made of
  `slope-<biome>` ramps with the biome's surface block as fill, and
  `<biome>-slope-platform`, a 3x3 pad of the biome's surface block.

Platform slabs sit at local y = -1, so they pave the ground at the bottom of a climb
and float flush with the plateau surface at the top; their authored `clearance` claims
four voxels of headroom. Cells outside any zone fall back to the shared
`stair-length` / `stair-platform` pair. There are no per-height or mirrored prefab
variants; the generator rotates and repeats the pieces instead.

Wild climb frequency can be tuned per biome with an optional `worldgen.wildStairs`
block:

```json
"wildStairs": {
  "allowCrossZone": true,
  "maxLevels": 6,
  "spacingCells": 3,
  "candidateRatio": 1.0
}
```

When `worldgen.wildStairs` is present, omitting `maxPerZone` leaves the biome uncapped:
the generator keeps placing off-road slope stairways until the suitable cliff
candidates are exhausted by `maxLevels`, `spacingCells`, and `candidateRatio`.
`allowCrossZone` lets the low-side biome place a wild climb into an adjacent zone,
`maxLevels` is the tallest height difference a wild climb may bridge, `spacingCells`
is the minimum plan-cell distance from any other stairway, and `candidateRatio` is the
percentage of suitable cliff edges kept before placement. `maxPerZone` is still
accepted as an optional hard cap; setting it to `null` also means uncapped. Missing
`allowCrossZone` defaults to `false`, missing `maxLevels` or `spacingCells` falls back
to the global `WorldGenConfig` value, and missing `candidateRatio` defaults to `1.0`.

## Layouts

| Height difference | Layout |
|---|---|
| 1 level | one straight length centered on the crossing, perpendicular to the cliff |
| 2+ levels | try one run parallel to the cliff, then a two-run switchback, then a perpendicular chain as the final fallback |
| above `maxComposedStairLevels` (roads) / `maxWildStairLevels` (wild) | edge rejected |

## The composed staircase

A composed climb hugs the cliff on the low side, in the three columns directly against
the wall (no gap column):

- a 3x3 **top platform** centered on the crossing's road strip, its slab flush with
  the high plateau surface, so stepping across the boundary is seamless;
- one **length per level**, descending parallel to the cliff away from the platform
  (both tangent directions and three sideways nudges are tried);
- a 3x3 **bottom platform** paving the low ground at the end of the flight.

Every piece is placed with `foundation`, so the flight and the top platform stand on
solid pillars down to the terrain instead of floating.

If neither direction has enough length for that one-pass layout, the generator folds
the climb into a switchback. The first run descends along the cliff, a 3x6 landing
crosses to a second lane, and the second run reverses direction. This needs roughly
half as much length along the cliff. Only when both parallel and switchback candidates
are obstructed does a road climb try the less attractive perpendicular chain;
`maxRoadStairLevels` caps that final fallback (the default matches the composed-climb
limit).

Two regions are validated without stamping prefabs:

- the **approach band**, the three columns beyond the structure along its full length:
  guaranteed flat, clear low ground. For road climbs the realization paves it with the
  zone's road block (`PlanStairway::pavedApproach`), so the road visibly turns at the
  crossing dead-end and runs beside the flight to the bottom platform instead of
  stopping at the wall. Wild climbs do not reserve this road-style band;
- the **exit cells**, the three high-plateau cells the top platform opens onto: must be
  dry, level land, and are reserved as a stair footprint and hard claim so no other
  flight, building, or scenery ever blocks the exit face to face.

One hard claim covers the whole strip (band included) from the low ground to above the
top platform, keeping later placements out from under and over the staircase.

## Placement feasibility

A stair group commits atomically only if every footprint rectangle passes:

- every column maps to land at the lower endpoint's height level;
- no column contains water, a bridge, or an entity plan cell;
- road interaction by mode: straight road flights must stay on road cells, composed
  road climbs may also use clear terrain in the lower zone, wild stairways must not
  touch roads and stay inside their lower zone;
- no overlap with any already accepted stair footprint (reserved exits included).

Rejected groups add no placements, so generation cannot leave half a staircase.
Road A* treats height changes above `maxComposedStairLevels` as impassable and applies
a per-level penalty so roads still prefer contouring. `rejectedStairways` in the world
report exposes seeds that need topology tuning.

## Map and verification

Every committed staircase (one-level ramps included) is one `PlanStairway` record on
`WorldPlan::stairways`: its layout, the contiguous placement range it stamped, its
validated footprint rectangles, the optional paved approach, its low plan cell, and
the top-to-bottom walking route. The footprint rectangles are painted in road color on
the preview map at sub-cell resolution, so the drawn road network shows where a climb
actually detours the path.

`SparklePlayground --check-stairs [--seed N]` realizes chunks around every composed
staircase and verifies, on the actual stamped voxels, that the top platform is flush
against exactly one cliff side, the strip descends at most one voxel per cell from the
high stand height to the low one, and the approach band is flat clear ground end to
end — paved with a road block for road climbs. When `--seed` is omitted, the command
prints its generated seed so the same check can be repeated.
