# Implement the tactical board seam

Implement the headless logical board used by every battle: a board-local view over either the
live voxel world or an owned handcrafted arena, immutable traversal data for the lifetime of
one battle, stable unit/object occupancy, deterministic weighted movement, 3D voxel line of
sight, deployment zones, and reusable synthetic fixtures.

This is an implementation task. Modify Playground code and tests, but do not add battle
units, turns, abilities, AI, widgets, overlays, or resource-authoring tools in this step.

---

# 1. Repository baseline

Implement against the repository after steps 01-04 and inspect the checked-out files before
editing. In particular, read:

```text
Playground/srcs/board/cell_source.*
Playground/srcs/board/traversal_graph.*
Playground/srcs/board/traversal_graph_builder.*
Playground/srcs/board/pathfinder.*
Playground/srcs/world/voxel_world.*
Playground/srcs/world/world_navigation.*
Playground/srcs/voxel/voxel_data.hpp
Playground/srcs/voxel/voxel_parser.cpp
Playground/srcs/components/actor.hpp
Playground/tests/world_raycaster_test.cpp
```

The verified starting behavior is:

* `ICellSource`, `GridCellSource`, and `WorldCellSource` already abstract cell and voxel
  definition lookup;
* `WorldCellSource` applies an origin offset, which is the correct seam for board-local
  coordinates over the persistent world;
* `TraversalGraphBuilder` already finds standable support voxels and four-directional edges,
  including slope/stair height compatibility;
* `Pathfinder` currently assumes cost `1`, has no occupancy callback, and its priority queue
  does not define equal-priority ordering;
* there is no `BoardData`, board occupancy, deployment layout, board builder, battle LoS, or
  battle fixture;
* `Actor::cell` is the **support voxel** under the actor, not the empty cell containing its
  feet. Battle cells use the same convention;
* `VoxelWorld::revision()` changes for chunk loads/unloads and committed voxel edits, even
  when the changed chunk is unrelated to the board. It is therefore too broad to be the
  battle staleness key;
* normal world navigation may rebuild when the world revision changes, but a battle board
  must not rebuild while a battle is in progress;
* step 04 supplies strict `HandcraftedBattleBoardDefinition` JSON backed by an existing
  `PrefabDefinition`, plus a registry getter and identity-transform bounds validation.

The Unity `BoardData`/`BoardRuntimeRegistry` are useful examples of the terrain/navigation/
runtime split. Do not port their object-pointer dictionaries, unweighted movement, mutable
navigation rebuild, or Unity component ownership.

---

# 2. Required final behavior

At the end of this step, headless tests must be able to:

1. Build a rectangular board from a synthetic voxel grid.
2. Plan and then build a board over a pinned region of the live `VoxelWorld` without copying
   any voxel cells.
3. Build a production handcrafted board by materializing its referenced prefab into an
   immutable owned grid with no world/chunk/RNG dependency.
4. Convert every logical cell to presentation space and, for live boards only, between
   board-local and world support-cell coordinates.
5. Query standability, terrain movement cost, borders, deployment zones, occupancy, and LoS.
6. Place/move/swap/remove stable unit IDs while enforcing one unit per standable cell.
7. Stack stable battle-object IDs on a cell without confusing them with unit occupancy.
8. Compute deterministic minimum-cost paths and reachable cells with destination terrain
   cost and occupied cells as blockers.
9. Detect stale live terrain rather than using a graph built against a changed world, while
   an immutable handcrafted source remains current by construction.
10. Reuse an ASCII/synthetic `BoardFixture` in all later battle tests.

`BoardData` is the only terrain/navigation/occupancy surface later battle rules receive.
They must not read `VoxelWorld`, chunks, render entities, or `WorldNavigation` directly.

---

# 3. Locked coordinate and lifetime model

Use board-local **support voxel** coordinates everywhere in headless battle code. Record the
source explicitly; never infer it from a zero anchor or from whether a world pointer happens
to be available:

```cpp
enum class BoardSourceKind { LiveWorld, Handcrafted };

struct BoardSourceDescriptor
{
    BoardSourceKind kind = BoardSourceKind::LiveWorld;
    std::optional<std::string> definitionId;
    auto operator<=>(const BoardSourceDescriptor &) const = default;
};
```

For a production `LiveWorld` board, `definitionId` is absent. For a production
`Handcrafted` board it is the non-empty stable ID resolved from `battleBoards()`. Test-only
synthetic fixtures also use the owned-grid/`Handcrafted` source path and must supply an
explicit stable fixture definition ID such as `test-flat-board`; that test ID is never
serialized as production content or resolved through the registry. Any other nullability
combination is rejected at construction.

Live coordinates are:

```text
local = worldSupportCell - worldAnchor
world = localSupportCell + worldAnchor
```

Presentation coordinates are defined for both sources:

```text
presentation = localSupportCell + presentationOrigin
live world:   presentationOrigin = worldAnchor
handcrafted:  presentationOrigin = (0,0,0)
```

Define the canonical battle-board cell type at this boundary and reuse it in every later
command, plan, occupancy entry, event, condition replay value, snapshot, picker, and HUD
contract:

```cpp
using BoardCell = spk::Vector3Int; // board-local solid support-voxel coordinate
using BoardCellLess = CellPositionLess;
```

`BoardCell` is a coordinate value, not a numeric identity; do not introduce or use the name
`BoardCellId`. Raw `spk::Vector3Int` remains appropriate for explicitly world-space or
presentation-space voxel coordinates. Even though the alias has the same representation,
live-world conversions must go through explicitly live-only
`BoardData::fromLiveWorldCell`/`toLiveWorldCell`; both reject a non-live source. Generic
render/picking code uses `BoardData::toPresentationCell` and
`fromPresentationCell` instead of pretending every board belongs to the exploration world.
APIs must name the space in their parameter/result types or documentation.

All origin addition/subtraction uses checked integer arithmetic and no conversion clamps.
`fromPresentationCell` returns `nullopt` for overflow or a coordinate outside the declared
extent/traversal bounds or absent from the immutable navigation topology. The live-only pair
reports checked-arithmetic failure and callers use `isInside`/`isStandable` when accepting an
externally supplied world cell.

For a live board, `worldAnchor.y` is `0`. Local `y` therefore remains the world voxel Y;
only X/Z are rebased. This avoids inventing a vertical offset and keeps slope/stair heights,
voxel DDA, and presentation placement consistent.

One unit standing on local cell `(x,y,z)` is visually positioned at:

```text
(presentationX + 0.5,
 walkHeightAtCenter(source, localCell),
 presentationZ + 0.5)
```

Do not add `+1` a second time: `walkHeightAtCenter` already returns the top walking height.

`BoardData` owns the `ICellSource` adapter, graph, layout, and runtime occupancy. A live
`WorldCellSource` still borrows `VoxelWorld`; registries and world must outlive the board.
Step 12 must destroy `BattleSession`/`BoardData` before unpinning or replacing that world. A
handcrafted/synthetic `GridCellSource` owns or shares ownership of its immutable
`VoxelGrid`; it may borrow the immutable voxel registry but never a stack-local grid. Its
lifetime is independent of exploration chunks and streaming.

---

# 4. Live-world contract: plan, pin, pause, then build once

Decision D03 is non-negotiable. A normal battle is an overlay on the same `VoxelWorld` that
exploration renders. Do not create a battle scene, copy a voxel grid, stamp a temporary
arena, or restore terrain after the fight.

This paragraph governs the `LiveWorld` source used by Wild/Trainer encounters. It does not
forbid the isolated, definition-backed `Handcrafted` source required by Gym/Special
encounters; that source is constructed independently in section 4.2 and never stamps into or
copies the exploration world.

Split live construction into a read-free planning stage and a terrain-reading build stage:

```cpp
struct WorldBoardRequest
{
    spk::Vector3Int encounterSupportCell;
    spk::Vector2Int size;                 // x,z extents
    int minimumWorldY = 0;                // inclusive support-cell scan bound
    int maximumWorldY = 0;                // exclusive
    VoxelOrientation approachDirection;   // horizontal only
    int deploymentDepth = 0;
    std::size_t playerSeatCount = 0;
    std::size_t enemySeatCount = 0;
};

struct WorldBoardPlan
{
    WorldBoardRequest request;
    std::vector<spk::Vector3Int> candidateWorldAnchors;
    std::vector<spk::Vector3Int> requiredChunkCoordinates;
    spk::Vector3Int pinWindowOriginChunk;
    spk::Vector3Int pinWindowRange;
};

struct BoardBuildResult; // complete value type follows BoardData in section 5

class BoardBuilder
{
public:
    [[nodiscard]] static WorldBoardPlan planWorld(const WorldBoardRequest &p_request);
    [[nodiscard]] static BoardBuildResult buildWorld(
        const VoxelWorld &p_world,
        const WorldBoardPlan &p_plan,
        double p_maxVerticalGap);
};
```

The lifecycle caller does not invent vertical values from encounter JSON. It copies
`minimumWorldY = WorldNavigation::bounds().minimum.y` and
`maximumWorldY = WorldNavigation::bounds().maximum.y` from the navigation object that made
the triggering support cell reachable, requiring `minimum < maximum` and the encounter
support Y inside that half-open interval. It passes the exact validated
`GameRules::maxVerticalTraversalGap` to `buildWorld`. X/Z still come solely from the
encounter board size. This keeps battle standability aligned with exploration and gives the
pin planner exact checked Y bounds.

Equivalent cohesive value/result types are acceptable. Preserve these responsibilities:

* `planWorld` performs checked coordinate/chunk arithmetic and does not load or inspect
  cells;
* step 12 uses the returned chunk window/list to keep the whole candidate envelope active;
* step 12 pauses the processable `VoxelFluidSimulator` before `buildWorld` reads terrain;
* `buildWorld` verifies every required chunk is loaded and active, selects one candidate,
  creates a `WorldCellSource(world, worldAnchor)`, and builds the final traversal graph
  exactly once;
* the graph is never refreshed during the battle;
* the board records the exact required chunk coordinates and each
  `VoxelChunk::contentRevision()` used to build it;
* every authoritative board query exposes `terrainIsCurrent()` or calls
  `requireCurrentTerrain()`. A required chunk that disappears, becomes inactive, or changes
  content revision is a controlled battle-abort error, not a silent graph rebuild;
* loading, unloading, or editing a chunk outside the stamped required set does not stale the
  board.

Chunk streaming outside the board region must not be used as a gameplay timer. Step 12 owns
the actual pin component and fluid pause because those are mode/lifecycle concerns; this
step supplies and tests the required region calculation.

## 4.1 Candidate anchors

The primary anchor is:

```text
anchor.x = encounter.x - floor(size.x / 2)
anchor.y = 0
anchor.z = encounter.z - floor(size.z / 2)
```

Use this exact deterministic candidate offset order, in world X/Z cells:

```text
( 0, 0)
(+2, 0), (-2, 0), (0,+2), (0,-2)
(+2,+2), (-2,+2), (+2,-2), (-2,-2)
```

Reject checked-overflow candidates. Inspect candidates in that order and choose the first
whose player and enemy deployment strips contain at least their requested number of
standable nodes. Candidate inspection may scan standability directly; do not construct and
discard several `TraversalGraph` instances. Build the final graph only after selection.

If no candidate can seat both teams, return a structured `BoardBuildError` with counts per
candidate. Do not shrink the authored board, drop creatures, overlap units, or fabricate
ground. Encounter/lifecycle code may log and safely decline the encounter.

The required chunks cover every candidate rectangle, the entire requested Y scan range, and
the two head-clearance cells above `maximumWorldY - 1`. Sort and deduplicate coordinates by
`x,y,z`. Use `spk::VoxelChunk::coordinatesFromWorldCell` and checked corner arithmetic;
do not duplicate negative-coordinate division rules.

## 4.2 Handcrafted source construction

Add a production entry point equivalent to:

```cpp
[[nodiscard]] static BoardBuildResult buildHandcrafted(
    const HandcraftedBattleBoardDefinition &p_definition,
    const PrefabDefinition &p_geometry,
    const VoxelRegistry &p_voxels,
    std::size_t p_playerSeatCount,
    std::size_t p_enemySeatCount,
    double p_maxVerticalGap);
```

The caller resolves both definitions from the already validated registries. Construction is
still defensive and transactional:

1. Recheck the definition's exact size, horizontal approach, deployment depth, checked
   volume, and `geometryPrefabId == p_geometry.id` contract before allocating.
2. Allocate an empty `spk::VoxelGrid` of exactly `size` and keep it in shared/owning storage.
3. Enumerate every listed prefab voxel with `forEachAppliedVoxel` at
   `destination = p_geometry.prefab.pivot()` and orientation `PositiveZ`. That is the exact
   identity transform: authored prefab coordinates become board-local coordinates without
   rotation, flip, or translation.
4. Validate **all** transformed positions against the grid before writing any cell. Include
   explicitly empty/carve entries. If one lies outside, return a structured error naming the
   board, prefab, and offending coordinate; never call clipping `applyTo` and never publish a
   partial grid.
5. Apply cells in prefab insertion order into the empty grid, create an owning
   `GridCellSource`, and build traversal/deployment/border data once.
6. Require both deployment zones to seat their requested teams. A pretty but unplayable
   arena is invalid; do not fabricate cells, drop participants, or fall back to live terrain.

The baseline `GridCellSource` stores a raw grid reference, so extend it with an owning
constructor that accepts `std::shared_ptr<const spk::VoxelGrid>` and retains that pointer for
its complete lifetime. Keep the existing borrowed-grid constructor for short-lived legacy
navigation callers if required for source compatibility, but `buildHandcrafted` and
`BoardFixture` must use the owning constructor. The voxel registry remains an immutable
borrow whose lifetime is already guaranteed by `Registries`; name that lifetime explicitly
in the class contract.

Handcrafted local coordinates occupy the declared half-open box
`[0,size.x) x [0,size.y) x [0,size.z)`. Its `BoardExtent.size` is `{size.x,size.z}` and its
traversal bounds are exactly `minimum={0,0,0}`, `maximum=size`; it does not reuse exploration
navigation Y bounds. The board source descriptor is
`{Handcrafted, definition.id}`, its presentation origin is zero, and it has no
`LiveBoardTerrainStamp`. It consumes no battle RNG, asks for no world plan/chunk pin, reads no
`VoxelWorld`, and does not pause or tick fluids. `terrainIsCurrent()` is therefore always
true for the immutable owned source after successful construction.

Do not duplicate the graph/layout logic between source kinds. After the supporting value
types in section 5 are declared, factor an internal operation equivalent to:

```cpp
[[nodiscard]] BoardBuildResult buildFrozenSource(
    BoardSourceDescriptor p_source,
    std::shared_ptr<const ICellSource> p_cells,
    BoardExtent p_extent,
    spk::Vector3Int p_presentationOrigin,
    std::optional<spk::Vector3Int> p_liveWorldAnchor,
    std::optional<LiveBoardTerrainStamp> p_liveTerrainStamp,
    spk::VoxelOrientation p_playerApproach,
    int p_deploymentDepth,
    std::size_t p_playerSeatCount,
    std::size_t p_enemySeatCount,
    double p_maxVerticalGap);
```

The live builder performs world planning/stamp checks and then calls this once with a
`WorldCellSource`. The handcrafted/fixture builder performs grid validation/ownership and
then calls it once with a `GridCellSource`. The shared operation alone builds the traversal
graph, movement/deployment queries, border list, and capacity checks, so both production
source kinds obey identical tactical rules.

---

# 5. Board data model

Add types equivalent to:

```cpp
struct BoardExtent
{
    spk::Vector2Int size;           // x,z, both strictly positive
    TraversalBounds traversalBounds;
};

class BoardData
{
private:
    BoardSourceDescriptor _source;
    std::shared_ptr<const ICellSource> _cells;
    spk::Vector3Int _presentationOrigin{};
    std::optional<spk::Vector3Int> _liveWorldAnchor;
    BoardExtent _extent;
    TraversalGraph _navigation;
    BoardOccupancy _occupancy;
    DeploymentLayout _deployment;
    std::vector<BoardCell> _borderCells;
    std::optional<LiveBoardTerrainStamp> _liveTerrainStamp;

public:
    [[nodiscard]] const BoardSourceDescriptor &sourceDescriptor() const noexcept;
    [[nodiscard]] const ICellSource &cells() const noexcept;
    [[nodiscard]] const TraversalGraph &navigation() const noexcept;
    [[nodiscard]] const BoardOccupancy &occupancy() const noexcept;
    [[nodiscard]] const DeploymentLayout &deployment() const noexcept;
    [[nodiscard]] const BoardExtent &extent() const noexcept;
    [[nodiscard]] const spk::Vector3Int &presentationOrigin() const noexcept;
    [[nodiscard]] const std::optional<spk::Vector3Int> &liveWorldAnchor() const noexcept;

    [[nodiscard]] bool isInsideColumn(int p_x, int p_z) const noexcept;
    [[nodiscard]] bool isInside(const BoardCell &p_local) const noexcept;
    [[nodiscard]] bool isStandable(const BoardCell &p_local) const noexcept;
    [[nodiscard]] bool isBorderCell(const BoardCell &p_local) const noexcept;
    [[nodiscard]] int movementCost(const BoardCell &p_local) const;
    [[nodiscard]] spk::Vector3Int toPresentationCell(const BoardCell &p_local) const;
    [[nodiscard]] std::optional<BoardCell> fromPresentationCell(
        const spk::Vector3Int &p_presentation) const;
    [[nodiscard]] spk::Vector3Int toLiveWorldCell(const BoardCell &p_local) const;
    [[nodiscard]] BoardCell fromLiveWorldCell(const spk::Vector3Int &p_world) const;
    [[nodiscard]] bool terrainIsCurrent() const noexcept;
    void requireCurrentTerrain() const;
};

enum class BoardBuildErrorCode
{
    InvalidRequest,
    NumericOverflow,
    RequiredChunkUnavailable,
    NoFeasibleWorldAnchor,
    InvalidHandcraftedDefinition,
    PrefabVoxelOutOfBounds,
    TraversalBuildFailed,
    InsufficientDeploymentCapacity
};

struct BoardBuildError
{
    BoardBuildErrorCode code;
    std::string boardOrEncounterId;
    std::optional<std::string> prefabId;
    std::optional<spk::Vector3Int> cell;
    std::size_t requiredPlayerSeats = 0;
    std::size_t availablePlayerSeats = 0;
    std::size_t requiredEnemySeats = 0;
    std::size_t availableEnemySeats = 0;
    std::string diagnosticDetail;
};

struct BoardBuildResult
{
    std::optional<BoardData> board;
    std::optional<BoardBuildError> error;
};
```

Equivalent names are acceptable only if later prompts are updated consistently.
`BoardBuildResult` has exactly one populated optional. Expected content/terrain failures use
the closed error value; exceptions remain reserved for allocation failure or a truly broken
process invariant. Diagnostics may add copied context but may not replace the stable code.

Constructor invariants are closed: `LiveWorld` requires a live anchor and live terrain stamp,
requires an absent definition ID, and sets presentation origin to that anchor;
`Handcrafted` requires a definition ID, forbids both live values, and requires a zero
presentation origin. `toLiveWorldCell`/`fromLiveWorldCell` require `LiveWorld` and fail fast
on misuse; they are not aliases for presentation conversion.
`fromPresentationCell` performs checked subtraction and returns a value only when the exact
coordinate maps to a board cell present in the immutable topology; it never clamps or scans
downward. It is the canonical inverse used by picking for both sources.

`sourceDescriptor`, presentation origin, optional live anchor, extent/traversal bounds,
canonical navigation cells/edges, deployment cells, border cells, and immutable terrain
identity belong to the board topology snapshot used by later battle snapshots/replays.
Step 06 copies the source descriptor into `BattleDescriptor`/`BattleSnapshot` and hashes it
in the material digest. A handcrafted board may never masquerade as live merely because its
presentation origin is zero.

`isInside` means X/Z is in `[0,size)` and Y is within the traversal bounds. `isStandable`
means the graph contains exactly that support cell. Never infer standability only from the
column. Multiple standable support cells with the same X/Z are legal (bridges/overhangs).

Expose mutable occupancy only to `BattleContext`/the command resolver, for example through a
private friend or an internal `BoardMutation` facade. UI, AI query code, and snapshots receive
const access. The existence of a low-level occupancy mutator is not permission to bypass the
single command path introduced in step 06.

Border cells are standable graph nodes whose X or Z is on the outer rectangle. Sort them
with `BoardCellLess`; do not include arbitrary solid/nonstandable boundary voxels.

For a live board, use a value shape equivalent to:

```cpp
struct RequiredChunkStamp
{
    spk::Vector3Int coordinates;
    std::uint64_t contentRevision = 0;
};

struct LiveBoardTerrainStamp
{
    const VoxelWorld *world = nullptr; // borrowed; guarded by the map lifetime token
    std::weak_ptr<const void> mapLifetime;
    std::vector<RequiredChunkStamp> requiredChunks; // coordinate sorted
};
```

For a live source, `terrainIsCurrent()` first checks the weak map lifetime, then checks every
stamped coordinate still resolves to the same **active** `VoxelChunk` and has the same
`VoxelChunk::contentRevision()`. It never compares `VoxelMap::revision()` or
`VoxelWorld::revision()`. This is safe on the single owning update thread and ignores
unrelated streamer churn. Handcrafted/synthetic boards have no live stamp and are always
current after construction because their backing grid is immutable; `requireCurrentTerrain`
is a no-op for that source kind.

---

# 6. Terrain movement cost

Add an optional type-level voxel field:

```json
"movementCost": 2
```

Extend `VoxelData` and every supported voxel parser version with:

```cpp
int movementCost = 1;
```

The field is optional with default `1`, so current voxel JSON remains valid. When present it
must be an integer in `[1, 1000000]`. It belongs to the semantic voxel type and is shared by
all states/orientations of that type. Passable/head-space voxels never contribute movement
cost; entering a standable node pays the cost of its solid support voxel.

Do not derive costs from free-form tags, transparency, slope height, or render shape. A mud
voxel can explicitly cost `2`; a slope still costs its authored value.

`BoardData::movementCost(cell)` requires a current, standable cell with a resolved
definition and returns a positive integer. Broken registry/source invariants throw with the
local and presentation coordinates plus the board source descriptor; live errors additionally
include the world coordinate. They do not silently return `1`.

---

# 7. Stable occupancy

Use the step-01 strong IDs, never `BattleUnit*`, component pointers, vector addresses, or
display names. Define the mutation result once:

```cpp
enum class BoardMutationError
{
    InvalidId,
    UnknownUnit,
    UnknownObject,
    DestinationNotStandable,
    DestinationOccupied,
    UnitAlreadyPlacedElsewhere,
    ObjectAlreadyPlacedElsewhere,
    CannotSwapSameUnit
};

struct BoardMutationResult
{
    bool accepted = false;
    bool changed = false;
    std::optional<BoardMutationError> error;
};

class BoardOccupancy
{
public:
    [[nodiscard]] std::optional<BattleUnitId> unitAt(BoardCell p_cell) const noexcept;
    [[nodiscard]] std::optional<BoardCell> cellOf(BattleUnitId p_unit) const noexcept;
    [[nodiscard]] std::span<const BattleObjectId> objectsAt(BoardCell p_cell) const noexcept;
    [[nodiscard]] std::optional<BoardCell> cellOf(BattleObjectId p_object) const noexcept;

    // Internal mutation surface used by BattleContext/resolvers only.
    [[nodiscard]] BoardMutationResult placeUnit(BattleUnitId, BoardCell);
    [[nodiscard]] BoardMutationResult moveUnit(BattleUnitId, BoardCell);
    [[nodiscard]] BoardMutationResult swapUnits(BattleUnitId, BattleUnitId);
    [[nodiscard]] bool removeUnit(BattleUnitId) noexcept;
    [[nodiscard]] BoardMutationResult placeObject(BattleObjectId, BoardCell);
    [[nodiscard]] bool removeObject(BattleObjectId) noexcept;
};
```

Rules:

* `accepted` is true exactly when `error` is null; `changed:false` is then a successful
  idempotent same-placement/same-destination no-op. A failure is `accepted:false`,
  `changed:false`, and one error; no failure partially changes either occupancy map;
* ID `0` is invalid and is rejected;
* a unit destination must be standable and contain no other unit;
* placing an already placed unit at the same cell is an idempotent no-op, not a duplicate;
* moving an unknown unit fails;
* `swapUnits` is all-or-none and preserves both forward/reverse maps;
* removing an unknown ID is an idempotent `false` result;
* any number of battle objects may stack on one standable cell;
* object IDs at a cell are stored/returned in ascending ID order;
* objects do not block movement merely by existing. Step 10 consults their immutable
  `blocksMovement` definitions when building a movement blocker;
* all mutation methods preserve bidirectional-map invariants on failure.

Use ordered containers or explicitly sorted outputs. If hash maps are used internally for
lookup, never expose their iteration order and add invariant tests after every operation.

---

# 8. Deterministic weighted pathfinding

Keep the existing exploration overloads source-compatible, but route them through a new
weighted query or add a cohesive battle-specific query beside `Pathfinder`:

```cpp
struct WeightedPath
{
    std::vector<BoardCell> cells;       // includes source and destination
    int totalCost = 0;                  // excludes source; sums entered cells
};

struct TraversalCostQuery
{
    // nullopt means blocked; otherwise a strictly positive destination-enter cost.
    std::function<std::optional<int>(const spk::Vector3Int &)> enterCost;
};

[[nodiscard]] std::optional<WeightedPath> findWeightedPath(
    const TraversalGraph &p_graph,
    BoardCell p_source,
    BoardCell p_destination,
    int p_budget,
    const TraversalCostQuery &p_query);

[[nodiscard]] std::map<BoardCell, int, BoardCellLess> floodWeighted(
    const TraversalGraph &p_graph,
    BoardCell p_source,
    int p_budget,
    const TraversalCostQuery &p_query);
```

The board movement query supplies:

```text
enterCost(cell) = nullopt
    if cell has a unit other than the moving source
    or a step-10 movement-blocking object
enterCost(cell) = BoardData::movementCost(cell)
    otherwise
```

V1 has no ally pass-through. An occupied node blocks both traversal and destination. The
source cell is always admitted at cost `0` even though occupied by the moving unit.

Movement remains four-directional through graph edges. Do not use geometric adjacency in
place of slope/stair edges. The path budget and costs are non-negative integers; checked
addition rejects overflow.

Use a deterministic Dijkstra label whose complete comparison key is:

```text
(total entered-cell MP cost,
 entered-cell count,
 lexicographic complete cell sequence under BoardCellLess)
```

The source participates in the sequence but not the entered-cell count/cost. Sort candidate
neighbors by `BoardCellLess`. A relaxation replaces the current label whenever the complete
three-part candidate key is smaller—not only when MP cost is smaller. The priority queue
uses the same label order, with destination cell and graph node index only as final
implementation tie breakers. Stale queued labels must be detected by the complete label,
not cost alone. It is acceptable to store complete paths on these deliberately small
boards; a predecessor-chain optimization is acceptable only if it produces exactly the
same full-sequence comparison.

An A* implementation is allowed only if it preserves that exact result, does not return on
the first equal-cost goal before hop/sequence alternatives are settled, and has golden
equality tests against the reference Dijkstra implementation. Do not use heap insertion
order or “keep first predecessor on equal cost” as a tie rule.

This is the canonical path. `from == to` yields `{from}` with cost `0`. Missing endpoints,
negative budgets, blocked destinations, or costs over budget return no path. Flood output
includes the source at `0`; callers may omit it from selectable movement highlights.

Upgrade the legacy fixed-cost `findPath`/`floodReachable` to deterministic ties too. Their
observable paths must no longer depend on `std::priority_queue` accidents.

---

# 9. Deployment layout

Add:

```cpp
struct DeploymentLayout
{
    int stripDepth = 0;
    VoxelOrientation playerApproachDirection;
    std::vector<BoardCell> playerCells;
    std::vector<BoardCell> enemyCells;

    [[nodiscard]] bool contains(BattleSide p_side, BoardCell p_cell) const noexcept;
    [[nodiscard]] std::span<const BoardCell> cells(BattleSide p_side) const noexcept;
};
```

Only horizontal `PositiveX`, `NegativeX`, `PositiveZ`, and `NegativeZ` are valid. Interpret
approach as the player's last move into the encounter. The player deploys on the entry edge,
opposite the movement vector:

```text
approach +Z -> player low-Z strip, enemy high-Z strip
approach -Z -> player high-Z strip, enemy low-Z strip
approach +X -> player low-X strip, enemy high-X strip
approach -X -> player high-X strip, enemy low-X strip
```

`stripDepth` must be positive and `2 * depth <= extent on the deployment axis`; otherwise
board construction fails. Include every standable node whose column lies in the strip,
including distinct vertical nodes in one column.

Store zone cells in a base deterministic order: row nearest the board centre first, then rows
toward the outer edge; within a row, increasing perpendicular coordinate, then Y, then full
`BoardCellLess`. Manual player placement and seeded-random candidate sorting use this base
order. Authored `byLine` placement does **not** reuse it blindly: step 06 applies
`rowsFromEnemyEdge` plus the exact left/right/center-out policy from step 04. Expose helpers
for edge-relative row index, perpendicular coordinate, and per-column support ordering so
that algorithm does not reverse-engineer orientation from unordered cells.

Deployment cells are data; they are not occupied until placement commands succeed.

---

# 10. Three-dimensional line of sight

Add a headless DDA over `ICellSource`, not over `VoxelMap`, so synthetic, handcrafted, and
live boards execute identical rules:

```cpp
struct LineOfSightResult
{
    bool clear = false;
    std::optional<spk::Vector3Int> firstBlockingVoxel;
};

class IBoardLineOfSightExtraBlockers
{
public:
    virtual ~IBoardLineOfSightExtraBlockers() = default;
    [[nodiscard]] virtual bool blocksVoxel(
        const spk::Vector3Int &p_boardLocalVoxel) const noexcept = 0;
};

class BoardLineOfSight
{
public:
    [[nodiscard]] static LineOfSightResult trace(
        const BoardData &p_board,
        BoardCell p_fromSupport,
        BoardCell p_toSupport,
        const IBoardLineOfSightExtraBlockers *p_extraBlockers = nullptr,
        float p_eyeHeight = 0.65f);
};
```

Endpoints are:

```text
P = (cell.x + 0.5,
     walkHeightAtCenter(source, cell) + eyeHeight,
     cell.z + 0.5)
```

The default eye height is a board-query constant, not JSON balance data in this slice. Both
support cells must be standable and current.

A traversed voxel blocks when its definition is `Solid` and does **not** carry the exact tag
`"losTransparent"`, or when the optional pure extra-blocker query returns true. Empty and
`Passable` voxels do not block by themselves. Ignore voxels in either endpoint X/Z column
for both terrain and extras so the caster's/target's own support and adjacent shape do not
self-block.

Implement a supercover Amanatides-Woo traversal: when the segment crosses an edge/corner at
equal `t`, inspect all newly touched cells in canonical X/Y/Z order before proceeding. This
prevents diagonal corner leaks and makes ties reproducible. Stop before/at the target point,
bound traversal by checked board/world coordinates, and reject non-finite calculations.

Step 10 supplies a context-backed implementation that maps each `blocksLineOfSight:true`
battle object to exactly one opaque standing voxel immediately above its support cell:
`{support.x, support.y + 1, support.z}`. Multiple blockers in that voxel are still one
boolean obstruction. The query resolves stable object IDs through the immutable definition
registry; `BoardData` itself never owns definition pointers. `BattleQueryService` passes this
adapter to `trace` for authoritative and preview queries. Do not make LoS depend on rendering
transparency, physics, or camera raycasts.

Ability distance is **not** computed here. Step 08 uses X/Z range geometry that ignores
elevation, then calls this 3D query only when the ability requires LoS.

---

# 11. Synthetic board fixture

Create a reusable fixture under tests, not an authoring/runtime tool:

```text
Playground/tests/fixtures/board_fixture.hpp
Playground/tests/fixtures/board_fixture.cpp
```

It should own its voxel grid/source for at least as long as `BoardData` and support compact
layered ASCII input. At minimum provide symbols for:

```text
.  empty/passable air
#  ordinary cost-1 solid support
2  cost-2 solid support
3  cost-3 solid support
b  passable losTransparent bush
W  solid LoS-blocking wall
/ \\ < >  oriented slopes or named fixture helpers
s  stair fixture helper
```

Allow named cells such as `player`, `enemy`, and `anchor` to be bound explicitly. Do not
encode battle units in voxel symbols; occupancy is a separate seam.

Fixture construction must use the same parsers/registry definitions or a deliberately tiny
validated test registry. It must not rely on the graphical executable's working directory.
It calls the same owned-grid `buildFrozenSource` path as a handcrafted board and supplies a
stable explicit test definition ID; it does not add a third production source kind. When it
uses a tiny registry, the fixture owns that registry until after its `BoardData` is destroyed.

Later tests should be able to request a flat board in a few lines and receive a `BoardData`,
named cells, and helpers to place stable IDs.

---

# 12. Error handling and determinism

Use structured result/error values where failure is expected (`BoardBuildResult`, path not
found, occupancy rejection). Reserve exceptions for violated invariants/invalid constructor
arguments in the repository's existing style.

Failures must include enough data to diagnose:

* invalid dimensions/deployment depth/Y bounds;
* invalid board-source descriptor/nullability or source-specific conversion;
* handcrafted definition/prefab mismatch, checked volume overflow, or applied voxel outside
  the declared grid;
* coordinate/chunk overflow;
* missing/inactive required chunks;
* no candidate with deployment capacity;
* unresolved standable voxel definition;
* required-chunk disappearance/deactivation/content-revision drift;
* invalid/duplicate runtime IDs;
* inconsistent forward/reverse occupancy maps.

No board algorithm may depend on unordered-container iteration, pointer ordering, wall-clock
time, render delta, or RNG. This step consumes no battle RNG. In particular, handcrafted
materialization is independent of world seed, encounter ordinal, loaded chunks, streaming,
and fluid state.

---

# 13. Required tests

Add focused tests under `Playground/tests/board/` and the shared fixture under
`tests/fixtures/`. Cover at least the following.

## 13.1 Coordinate/source ownership

Test:

* board source descriptor/nullability invariants and copied-value lifetime;
* live board-local/world round trips, including negative world X/Z;
* local Y remains world Y;
* live presentation mapping equals world mapping;
* handcrafted presentation origin is exactly zero and presentation mapping equals local;
* presentation round trips succeed for every topology cell and reject outside/hole cells
  without clamping or downward search;
* live-world conversion APIs reject a handcrafted board;
* support-cell walking height on cubes, slabs, slopes, and stairs;
* returned synthetic board keeps its grid alive;
* live source reads the same voxel the world contains;
* `Actor::cell` and board support-cell placement agree.

## 13.2 World planning/build

Test:

* exact odd and even primary anchors;
* exact nine-candidate order;
* exact sorted/deduplicated required chunk set across negative chunk coordinates;
* head-clearance chunks are included;
* coordinate overflow is rejected;
* missing/inactive chunks fail before graph use;
* first feasible candidate is selected;
* no feasible candidate returns counts and does not shrink;
* graph is built once for the chosen candidate;
* edit, unload, or deactivate of a **required** chunk makes `terrainIsCurrent()` false;
* load/unload/edit of an unrelated chunk leaves `terrainIsCurrent()` true;
* no automatic rebuild occurs.

## 13.3 Handcrafted construction

Load strict board/prefab definitions through the real registries, then test:

* unsupported version, missing required field, unknown field, malformed size, unknown prefab,
  and non-horizontal approach fail during the step-04 parser/reference transaction;
* exact identity materialization, including non-zero prefab pivot and explicitly empty carve
  cells, produces the expected complete owned grid without translation or clipping;
* every board size/parity/depth/approach boundary and checked grid volume;
* a prefab voxel below zero or at/exceeding any declared upper bound fails before any grid or
  board is published;
* definition-to-prefab ID mismatch and missing prefab fail with both IDs/path context;
* the resulting traversal graph, movement costs, border order, LoS, and both deployment
  zones match an equivalent synthetic fixture;
* insufficient player or enemy capacity is a typed failure, not a live-world fallback;
* the returned board retains its grid after all construction temporaries die;
* `terrainIsCurrent()` remains true and `requireCurrentTerrain()` succeeds with no
  `VoxelWorld`, loaded chunk, pin, streamer, fluid simulator, world seed, or RNG object;
* two builds from the same definitions are topology/source-descriptor equal and consume zero
  battle RNG draws;
* live-board planning/build regressions remain byte/value unchanged by the common-builder
  refactor.

## 13.4 Terrain and occupancy

Test:

* default and authored movement costs;
* zero/negative/non-integer/excessive JSON costs fail with field context;
* placement requires standability;
* one unit per cell;
* move/swap/remove keep both maps consistent;
* failed operations make no mutation;
* ID zero rejection;
* object stacking and ascending object order;
* unit and object IDs cannot alias through the type system.

## 13.5 Weighted movement

Use a board where the geometrically shortest route crosses cost-3 terrain and a longer
cost-1 route is cheaper. Assert:

* chosen path and exact summed destination costs;
* source cost excluded;
* budget boundary accepted and one-less rejected;
* occupied enemy and ally block traversal/destination;
* the moving source does not block itself;
* no ally pass-through;
* unreachable destination;
* `from == to` contract;
* canonical equal-cost path across graph insertion orders;
* equal MP cost chooses fewer entered cells even when that path is discovered later;
* equal MP cost and equal entered-cell count choose the lexicographically smaller complete
  `BoardCell` sequence even when predecessor/neighbor insertion order is reversed;
* canonical flood output;
* checked overflow behavior;
* legacy exploration path results remain valid and deterministic.

## 13.6 Deployment

Test all four approach directions, strip boundaries, centre-first ordering, vertical nodes,
insufficient capacity, invalid depth, and player/enemy non-overlap.

## 13.7 LoS

Test:

* clear flat line;
* solid wall blocks and reports its voxel;
* passable bush does not block;
* solid `losTransparent` voxel does not block;
* caster/target support columns do not self-block;
* elevated wall, bridge, slope, and overhang cases;
* diagonal edge and corner supercover cannot leak;
* reversing endpoints gives the same clear/blocked answer;
* multiple blockers report the first in traversal order;
* invalid/nonstandable endpoints are rejected.

Run all existing navigation, world, fluid, voxel, and raycaster tests as regression coverage.

---

# 14. Expected file changes

Add cohesive files equivalent to:

```text
Playground/srcs/board/board_data.hpp
Playground/srcs/board/board_data.cpp
Playground/srcs/board/board_occupancy.hpp
Playground/srcs/board/board_occupancy.cpp
Playground/srcs/board/board_builder.hpp
Playground/srcs/board/board_builder.cpp
Playground/srcs/board/deployment_layout.hpp
Playground/srcs/board/deployment_layout.cpp
Playground/srcs/board/board_line_of_sight.hpp
Playground/srcs/board/board_line_of_sight.cpp
Playground/srcs/board/weighted_path.hpp

Playground/tests/fixtures/board_fixture.hpp
Playground/tests/fixtures/board_fixture.cpp
Playground/tests/board/board_data_test.cpp
Playground/tests/board/board_builder_test.cpp
Playground/tests/board/handcrafted_board_builder_test.cpp
Playground/tests/board/board_occupancy_test.cpp
Playground/tests/board/weighted_path_test.cpp
Playground/tests/board/deployment_layout_test.cpp
Playground/tests/board/board_line_of_sight_test.cpp
```

Modify at least `cell_source.*`, `pathfinder.*`, `voxel_data.hpp`, `voxel_parser.cpp`, test
CMake sources, and any schema/board documentation touched by the new optional voxel field
and handcrafted materialization path.
Equivalent organization is acceptable; do not create one-line wrapper files.

Do not modify Sparkle `includes/` or `srcs/`. If a generic engine change appears necessary,
stop and report it; this step is expected to fit entirely in `pg::`.

---

# 15. Documentation requirements

Update the implemented board/data documentation to record:

* support-cell and board-local/world conventions;
* explicit board source descriptor and generic presentation-coordinate convention;
* live source borrowing and destruction order;
* plan/pin/pause/build-once lifecycle;
* handcrafted prefab identity materialization, owned-grid lifetime, and always-current
  terrain contract;
* revision drift behavior;
* terrain movement-cost schema and destination-cost rule;
* unit/object occupancy rules;
* canonical path tie-break;
* deployment direction mapping;
* 3D LoS blocker/tag/supercover semantics.

Replace stale claims that battle movement always costs one or that BoardData is already
implemented. Do not describe step-12 pinning/pause integration as implemented here.

---

# 16. Non-goals

Do not implement battle units, creature projection, phases, stamina, commands, effects,
statuses, traps, AI, encounter triggering, a mode manager, overlays, mouse picking, camera
behavior, HUD, save data, additional handcrafted-board authoring tools, terrain mutation
during battle, hex or diagonal movement, ally pass-through, jumping/flying, multi-cell
units, dynamic graph
rebuilds, or any authoring/editor tool.

`BattleObjectId` spatial storage exists now so step 10 does not replace board ownership;
object definitions and trigger behavior remain step 10.

---

# 17. Acceptance criteria

This step is complete when:

* `BoardData` is the coherent headless terrain/navigation/occupancy seam;
* every board carries a validated `LiveWorld|Handcrafted` source descriptor and all later
  snapshots/digests can copy that value;
* live boards read `WorldCellSource` and never copy/restore world voxels;
* handcrafted boards materialize the referenced strict prefab at identity into an owned
  immutable grid, never clip, and build the same tactical layers;
* live board planning reports the exact pin region, captures per-required-chunk content
  stamps, and final construction builds one graph;
* fluid pause/pin ownership is explicitly left to step 12;
* handcrafted construction needs no world plan/chunks/fluids/RNG and is always current;
* local coordinates consistently identify support voxels;
* generic presentation conversion works for both sources and world conversion is live-only;
* stale required terrain is detected without reacting to unrelated global map revisions and
  is never silently rebuilt;
* terrain MP costs are strict JSON data and charge the entered destination;
* occupancy uses stable IDs and preserves one-unit-per-cell invariants;
* weighted paths/floods block every occupied cell and use canonical ties;
* LoS is a deterministic 3D `ICellSource` supercover query;
* deployment zones follow the locked approach mapping and ordering;
* the reusable fixture supports later battle tests;
* all existing and new Playground tests pass;
* `SparklePlayground` still builds.

---

# 18. Required handoff report

Report the final BoardData/source ownership model, source-descriptor and presentation/world
coordinate contracts, world candidate and pin calculation, handcrafted prefab/grid
materialization algorithm, revision policy, movement-cost schema, canonical path ordering,
LoS rules, deployment mapping, fixture API, files changed, and exact commands/tests actually
run. State explicitly that battle construction, actual mode-owned pinning/fluid pause, and
rendered live/handcrafted arena presentation remain unimplemented.
