# Adaptive stairways

One world-plan height level is `blocksPerLevel` voxels (currently 3). Stairways are
composed at generation time from exactly two prefabs per biome, so a single layout
adapts to any height difference:

- `<biome>-stair-length`: a three-wide flight of stairs climbing one level along its
  local +Z (stairs plus support fill, carving box clears the headroom above);
- `<biome>-stair-platform`: a 3x3 pad whose slab sits at local y = -1, so it paves the
  ground at the bottom and floats flush with the plateau surface at the top. Its
  authored `clearance` claims four voxels of headroom.

Cells outside any zone fall back to the shared `stair-length` / `stair-platform` pair.
There are no per-height or mirrored prefab variants; the generator rotates and repeats
the two pieces instead.

## Layouts

| Height difference | Layout |
|---|---|
| 1 level | one straight stair-length centered on the crossing, perpendicular to the cliff |
| 2+ levels | composed staircase parallel to the cliff (see below); a 2-level road climb falls back to two chained straight flights when the composed footprint is obstructed |
| above `maxComposedStairLevels` (roads) / `maxWildStairLevels` (wild) | edge rejected |

## The composed staircase

A composed climb hugs the cliff on the low side, in the three columns directly against
the wall (no gap column):

- a 3x3 **top platform** centered on the crossing's road strip, its slab flush with
  the high plateau surface, so stepping across the boundary is seamless;
- one **stair-length per level**, descending parallel to the cliff away from the
  platform (both tangent directions and three sideways nudges are tried);
- a 3x3 **bottom platform** paving the low ground at the end of the flight.

Every piece is placed with `foundation`, so the flight and the top platform stand on
solid pillars down to the terrain instead of floating.

Two rectangles are validated without being stamped:

- the **walkway lane**, the fourth column out from the wall along the full structure:
  guaranteed flat, clear low ground so the road dead-end under the top platform stays
  connected to the bottom platform;
- the **exit cells**, the three high-plateau cells the top platform opens onto: must be
  dry, level land, and are reserved as a stair footprint and hard claim so no other
  flight, building, or scenery ever blocks the exit face to face.

One hard claim covers the whole strip (lane included) from the low ground to above the
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

Committed stair rectangles are stored in `WorldPlan::stairRects` and painted in road
color on the preview map at sub-cell resolution, so the drawn road network shows where
a climb actually detours the path.

`SparklePlayground --check-stairs [--seed N]` realizes chunks around every composed
staircase and verifies, on the actual stamped voxels, that the top platform is flush
against exactly one cliff side, the strip descends at most one voxel per cell from the
high stand height to the low one, and the walkway lane is flat clear ground end to end.
