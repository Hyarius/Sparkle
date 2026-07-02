# System — Board & Pathfinding

The headless tactical-board seam (`BoardData`), the shared traversal graph + A*, voxel
line-of-sight, and the live derivation of a board from world voxels (Plan Gamma, D03).

Diagrams: [05-class-board.puml](../diagrams/05-class-board.puml).
Plan: steps [07](../plan/steps/step-07-player-exploration.md) (graph+A*),
[09](../plan/steps/step-09-board-seam.md) (seam + derivation).

---

## Responsibilities

- `pg::BoardData` = the **only** world battle logic sees: terrain cells + navigation graph +
  runtime registry of units/objects. Populated from world voxels at runtime, from synthetic
  grids in tests.
- The **traversal graph** (standable nodes + 4-way edges honoring cardinal heights) and
  **A\*** — shared by exploration click-to-move (D27) and battle movement.
- **Voxel LoS raycast** between cell centers; **mouse→cell resolution** on the board.
- **Live board derivation**: pick an 11×11 (default) region around the player and expose it
  as a `BoardData` *without copying or mutating world voxels*.

## Key types (`Playground/srcs/board/`)

```
pg::TraversalGraph
    Node { Vector3Int position; Node* posX,negX,posZ,negZ; }
    dense node index by cell; allNodes(); tryGetNode(cell)
pg::TraversalGraphBuilder
    build(const ICellSource&, bounds, excludedColumns={}) -> TraversalGraph
    //  nodes = standable cells (Solid + 2 passable above; see voxel.md contract)
    //  edges: per world direction, best same-column candidate whose
    //         |exitHeight-entryHeight| <= maxVerticalTraversalGap (0.5),
    //         preferring smallest gap then smallest |Δy|
    //  (Unity's VoxelTraversalGraphBuilder is a worked example of this exact rule)
pg::ICellSource      // adapter: cell(Vector3Int) + registry lookup
    ├─ WorldCellSource(VoxelWorld&, originOffset)      // live world region
    └─ GridCellSource(VoxelGrid&)                      // synthetic tests / handcrafted boards
pg::Pathfinder       // static A*: (graph, from, to, maxCost?) -> optional<vector<Vector3Int>>
                     // cost = 1/cell v1 (terrain MP-cost modifiers hook here later);
                     // heuristic = Manhattan on x,z; blocked nodes = occupied cells param
pg::BoardTerrainLayer   { ICellSource& cells; Vector3Int size; isInside(local) }
pg::BoardNavigationLayer{ TraversalGraph graph; rebuild(terrain, excluded); isStandable(local) }
pg::BoardRuntimeRegistry
    unit placement: canRegister/tryRegister/tryMove/swapUnits/remove/tryGetPosition/hasUnitAt
    interactive objects: tryAdd/getAt/removeByTags/advanceObjectDurations
    (one unit per cell; objects stack per cell; positions are board-local Vector3Int)
pg::BoardData
    Terrain, Navigation, Runtime (as Unity: thin façade methods —
    isInside, isStandable, isBorderCell, tryPlace/tryMove/remove, tryGetUnitAt…)
    Vector3Int worldAnchor;              // world cell of board-local (0,0,0)
    std::vector<Vector3Int> borderCells; // ring used to flee/visual edge
pg::BoardBuilder
    fromWorld(VoxelWorld&, Vector3Int center, Vector2Int size) -> BoardData   // Gamma live
    fromGrid(VoxelGrid&) -> BoardData                                          // tests/handcrafted
pg::LineOfSight
    hasLine(ICellSource&, Vector3Int fromCell, Vector3Int toCell) -> bool
    // 3D DDA between standing points (cell center at walk height + eye offset 0.5);
    // blocked by Solid cells whose definition lacks tag "losTransparent";
    // the from/to columns themselves never block
pg::BoardRaycaster
    resolveMouseCell(BoardData&, worldRay) -> optional<Vector3Int>
    // world DDA hit -> board-local -> descend to topmost standable in that column
```

## Board-local vs world coordinates

Board logic runs entirely in **board-local** cells (`local = world − worldAnchor`). The
overlay/view layer converts back with `worldAnchor` when drawing highlights or placing unit
views. Handcrafted boards have `worldAnchor = 0` and their own `GridCellSource`.

## Live derivation (Gamma) — `BoardBuilder::fromWorld`

1. Center = player's current cell; size from encounter data (default 11×11, D13).
2. Anchor = center − size/2 (clamped so the region is fully inside loaded chunks); Y range =
   full loaded height (nodes exist only where standable anyway).
3. Terrain layer wraps a `WorldCellSource` — **no voxel copy**; the world stays live and
   read-only for the battle's duration (battle never edits terrain in v1).
4. Navigation rebuilds over the region, **excluding columns** outside the rectangle.
5. Border cells = outermost standable ring (flee/visual edge).
6. Validation: if standable cells < (unit count × 4) or a deployment zone can't seat a full
   team, nudge the anchor ±2 cells up to 4 tries, then shrink to the best found — encounters
   in pathological terrain still start (log a warning). Deployment zones: two opposite
   `deploymentStripDepth`-deep strips, player side facing the approach direction (D13).

During battle, exploration triggers are frozen (mode-level), other world actors pause, and
world streaming keeps the battle region loaded (trivial in M1).

## Movement & reachability (battle)

- Reachable cells for a unit = Dijkstra flood from its cell over the graph, cost ≤ current
  MP, cells occupied by units excluded (pass-through allies: **not** in v1 — blocked).
- `MoveAction` distance = path length; MP cost = distance (terrain multipliers later).
- Range shapes (circle/line/diagonal) and AoE shapes (square/cross/circle/line) are computed
  on **x,z Manhattan/Chebyshev geometry** ignoring elevation (GDD: elevation does not affect
  range); LoS is the only 3D check. Exact shape definitions in
  [battle.md](battle.md) §targeting.

## Dependencies

Uses: voxel traversal contract, VoxelWorld (via ICellSource), game rules config.
Used by: battle (all rules), exploration click-to-move (graph+A* parts), overlay rendering,
AI, tools (encounter editor board preview).

## Testing (this system is the test workhorse)

All headless: graph building on synthetic grids (flat, slope chains, stairs, walls,
overhangs, head-clearance failures), A* paths and unreachability, LoS truth table (wall
blocks, bush with `losTransparent` doesn't, elevation cases), runtime registry rules
(occupancy, swaps, object durations), derivation (anchor/border/deployment placement,
degenerate-terrain nudge), mouse-cell resolution (column descent). Build a reusable
`BoardFixture` (board from ASCII-art grid strings — worth the investment; every battle test
reuses it).

```
BoardFixture example: "#" cube, "." empty column, "/" slope+Z, "s" stair, "b" bush on ground
layers parsed bottom-up; fixture returns BoardData + named cells.
```
