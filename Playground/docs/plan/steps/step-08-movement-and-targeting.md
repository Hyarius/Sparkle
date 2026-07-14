# Implement movement and ability targeting commands

Build the authoritative command-query layer used by the player UI, enemy AI, scripted
tests, and later presentation. Implement deterministic movement validation and execution,
ability anchor enumeration, range/area/line-of-sight queries, and immutable previews. Every
caller must submit the same small command values through `BattleSession::submit`; no caller
may provide a trusted path, target list, cost, or effect result.

This step makes movement fully executable. It prepares and validates casts without yet
pretending that their effects exist: abilities whose complete effect runtime is unavailable
must be rejected atomically until steps 09-10 install the resolver. Do not add temporary
no-op casts, UI-specific validation, AI shortcuts, or a second combat path.

---

# 1. Repository baseline and prerequisites

Implement after steps 01-07 and read each handoff report before editing. Reinspect the live
repository because exact filenames may have drifted, but preserve these established
contracts:

* `BoardData` owns board-local support-voxel cells, stable occupancy, blocking battle
  objects, a graph built once from the pinned live world, and a canonical cell comparator;
* `Actor::cell` and every battle cell identify the solid support voxel, not the air voxel
  occupied by a model;
* movement is four-directional in x/z and pays the positive terrain cost of the destination;
* `BattleContext` owns all mutable battle state and `BattleSession::submit` is the only
  public mutation gateway;
* commands contain a unit plus a destination or one ability plus exactly one anchor;
* AP/MP refill only at activation start, time is frozen during an activation, and more than
  one legal command may be submitted before `EndTurn`;
* `BattleEventLog` appends one complete action batch before that batch is published;
* definitions use separate `anchorFilter` and `affectedFilter`, explicit
  `EffectApplication::applyTo`, fixed-point/integer values, and strict semantic IDs;
* range ignores elevation, while line of sight remains a three-dimensional voxel query;
* the original Unity draft contains useful intent but is not an authority for architecture,
  floating-point formulas, mutable target lists, or parallel player/AI execution paths.

At the expected baseline there is no final battle query service, no legal-anchor enumerator,
no immutable move/cast plan, and the original pathfinder either has fixed edge cost or lacks
battle occupancy. Extend it or add a battle-specific query without breaking exploration.

Before implementation, locate and record the exact names of:

1. board cell, graph, occupancy, pathfinder, and LoS APIs;
2. `BattleContext`, `BattleSession`, command/result, snapshot, and event-batch APIs;
3. ability/range/area/filter definitions and game-rule limits;
4. effective-stat accessors, especially maximum MP and Range;
5. all tests already added under `Playground/tests/battle` and
   `Playground/tests/board`.

Do not silently introduce aliases if a prerequisite used a different final name. Adapt this
prompt to the handoff and document the adaptation.

---

# 2. Required final behavior

At the end of this step:

1. one pure `BattleQueryService` can enumerate legal moves and legal ability anchors from
   an immutable battle state;
2. previews and command execution share the same validation/planning algorithms;
3. a `MoveCommand` contains only unit and requested destination; the session recomputes
   its path and cost at submission time;
4. units and blocking objects block both traversal and destination; allies cannot be crossed;
5. a successful move resolves ordered cells, charges actual destination costs, updates
   occupancy, and emits both step detail and one aggregate voluntary-movement event;
6. invalid commands change no state, consume no ID/RNG draw, and append/publish no event;
7. every ability cast selects exactly one standable board-cell anchor;
8. anchor range, LoS, anchor filtering, AoE generation, and affected filtering are separate
   deterministic stages;
9. `Range` extends only an ability's maximum anchor range;
10. all captured cells and units have canonical stable order and contain no duplicates;
11. a zero-direction line AoE contains its anchor exactly once;
12. player, AI, tests, and future UI can use immutable previews but cannot mutate through
    them;
13. the executable can run a headless movement/targeting trace on the synthetic board.

No combat payload is silently ignored. Until its full resolver exists, submitting a cast
whose effect catalog is not installed returns a typed unavailable error before spending
resources or assigning an action/batch ID. Step 09 installs core payloads; step 10 completes
statuses and objects and removes the final availability guard.

---

# 3. Final ownership boundary

Add a module under the battle runtime, for example:

```text
Playground/srcs/battle/query/battle_query_service.hpp
Playground/srcs/battle/query/battle_query_service.cpp
Playground/srcs/battle/query/battle_plans.hpp
Playground/srcs/battle/movement/battle_movement.cpp
```

Use repository naming conventions if they differ. Keep these responsibilities distinct:

| Owner | Responsibility |
|---|---|
| `BoardData` | cells, stable occupancy, object blockers, neighbors, terrain cost, LoS source |
| `BattleContext` | units, active unit, resources, phase, definitions, board, log |
| `BattleQueryService` | pure enumeration, legality, path/range/AoE/filter planning |
| `BattleSession` | command phase/issuer checks, action transaction, mutation, commit/publish |
| later `EffectResolver` | execute captured effect applications through session-owned mutation |
| UI/AI/tests | request previews and submit ordinary commands only |

`BattleQueryService` stores no mutable cache that can outlive a state revision. Prefer a
short-lived object borrowing a const context. If caching is proven necessary later, key it
by an explicit battle state revision and preserve canonical results; do not implement it in
this step.

The service must not:

* allocate battle/action/event/object/status IDs;
* draw from `BattleRng`;
* modify occupancy, resources, HP, bars, phases, or definitions;
* call `ContractProvider::trigger`;
* publish events;
* retain references into a temporary preview returned to a caller.

---

# 4. Immutable plan and preview values

Use final values equivalent to:

```cpp
struct PlannedPathStep
{
    BoardCell from;
    BoardCell to;
    std::uint32_t movementPointCost = 0;
};

struct MovePlan
{
    BattleUnitId unit;
    BoardCell origin;
    BoardCell destination;
    std::vector<PlannedPathStep> steps;
    std::uint32_t totalMovementPointCost = 0;
};

struct CastPlan
{
    BattleUnitId source;
    std::string abilityId;
    BoardCell sourceCellAtCapture;
    BoardCell anchor;
    std::optional<BattleUnitId> primaryUnit;
    std::vector<BoardCell> areaCells;
    std::vector<BoardCell> affectedCells;
    std::vector<BattleUnitId> affectedUnits;
};
```

Use strong/content-ID types from prior steps instead of raw strings where already available.
`CastPlan` is the one-time capture required by the locked cast semantics:

* `primaryUnit` is the occupant of the anchor at capture, if any and if accepted by the
  anchor filter;
* `areaCells` is pure geometry before the affected filter;
* `affectedCells` and `affectedUnits` are separately filtered outputs;
* each vector is sorted canonically and deduplicated;
* target membership is not recalculated between authored effects in one cast;
* effect-time state such as current HP, current position, current effective stats, and
  whether a captured unit is still active is read when that effect executes.

The public preview may add display-friendly values such as affordability and rejection
reasons, but must own its data:

```cpp
struct AbilityAnchorPreview
{
    BoardCell anchor;
    bool legal = false;
    std::vector<CommandRejection> reasons;
    std::optional<CastPlan> plan;
};
```

Do not expose mutable unit pointers or registry definition pointers to presentation. A const
definition reference whose registry lifetime is guaranteed is acceptable internally.

---

# 5. One validation pipeline

Implement internal planners with result/error values:

```cpp
Expected<MovePlan, CommandRejection> planMove(
    const BattleContext &p_context,
    BattleUnitId p_unit,
    BoardCell p_destination);

Expected<CastPlan, CommandRejection> planCast(
    const BattleContext &p_context,
    BattleUnitId p_unit,
    std::string_view p_abilityId,
    BoardCell p_anchor);
```

Enumeration must call the same primitives as these planners. Submission must call the
planner again against the current state; it must never execute a preview plan returned on a
previous frame. This makes stale UI/AI choices fail safely if an earlier command changed
resources, occupancy, statuses, or the active unit.

Layer validation in this order so errors and tests are stable:

1. battle is non-terminal and in `Activation`;
2. command unit exists, is an active combatant, is placed, and equals the active unit;
3. issuer may control that unit;
4. requested definition/cell exists;
5. current AP/MP affordability;
6. geometry/path/filter/LoS legality;
7. complete effect-runtime availability for a cast;
8. create the action transaction and execute.

An implementation may collect multiple preview reasons, but authoritative submission
returns a stable first error using this order. Do not allocate `BattleActionId`,
`BattleBatchId`, event sequence, or RNG state until every precondition passes.

---

# 6. Movement planning

## 6.1 Reachability graph

Use the board's final four-directional graph and battle-specific traversal predicate.
A candidate destination is traversable only when:

* it is a standable board support cell;
* it has not been blocked by a battle object;
* it is not occupied by any other placed active combatant.

The moving unit's origin is traversable for that unit only. No ally/enemy pass-through,
diagonal corner cut, jump, flight, height surcharge, zone of control, or opportunity attack
exists in v1.

The edge cost for `from -> to` is exactly the positive movement cost authored on `to`.
Never trust a path length as MP cost. Use a checked wide accumulator while searching and
reject/cap values that cannot fit the runtime resource type.

## 6.2 Deterministic weighted search

Use Dijkstra or an equivalent positive-weight search. A* is acceptable only if its
heuristic cannot change the canonical optimal result. Lock all ties:

1. smaller total MP cost;
2. fewer entered cells;
3. lexicographically smaller complete sequence of board cells using the board comparator.

Neighbor enumeration itself follows the board comparator. Do not let unordered-map order,
pointer address, heap implementation, or insertion accident choose among equal paths.

Step 05's generic `WeightedPath::cells` includes the source; when constructing this step's
`MovePlan`, strip that first source cell. The `MovePlan` path therefore excludes the origin
and includes the destination. `origin == destination`
is not a movement command: return `CommandRejection::NoStateChange` rather than a legal
zero-cost action. This also prevents AI loops.

## 6.3 MP bounds and enumeration

A move is legal only when `totalMovementPointCost <= currentMovementPoints`. Enumeration
returns every reachable non-origin destination within current MP in canonical cell order.
It may include the complete `MovePlan` for each destination.

`maximumMovementPoints` used by AI later is a planning budget layered on top of current
MP; it never authorizes more MP and never changes terrain cost.

Do not deduct a caller-supplied distance or accept a caller-supplied path. Re-run the search
for `MoveCommand::destination` at the gateway.

---

# 7. Movement execution and events

After a `MovePlan` has been validated, begin one action batch and execute its cells in
order. For each planned step:

1. verify the expected origin/destination transition is still valid and current MP is at
   least that destination's exact cost;
2. atomically move occupancy and the unit's support-cell value;
3. subtract the destination's actual MP cost;
4. append `ResourceSpent{resource=MovementPoints, reason=MovementCost}` with requested/
   applied cost and before/after (terrain entry costs are strictly positive);
5. append `UnitMovementStep` with from, to, cost, and cause `Voluntary`;
6. invoke the old-cell `unitLeftCell` seam;
7. if the unit remains placed at the expected destination, invoke the new-cell
   `unitEnteredCell` seam.

Step 10 fills the object-trigger seams. Their ordering is already final:

* occupancy is committed before `unitLeftCell`;
* the leaving object remains anchored to its old cell;
* every leave trigger occurs before the corresponding enter trigger;
* failed transitions emit neither;
* deployment and removal are not movement and invoke neither.

Because this step has no active object resolver, the seam is an explicit no-op, not a
second event-provider trigger. Do not call the same `ContractProvider` recursively from
inside resolution.

If a later trigger can stop, remove, displace, or defeat the mover, movement stops at the
last successfully entered cell. The aggregate values must describe what actually happened,
not the original plan. Reserve that behavior now by accumulating applied step values.
This includes resource reactions: before every next transition, re-read current MP after
the previous cell's leave/enter triggers. If it is now below the next positive terrain
cost, stop successfully before that transition, emit/spend nothing for the unentered cell,
and keep all earlier movement/cost/events. Never underflow/clamp a step cost or continue on
the command-time affordability proof after a trap changed the pool.

After the final applied step—even when an enter trigger removed the mover—finish the accepted
command boundary using its accumulated values. A removed mover's actual final cell is its
retained `lastOccupiedCell`; it is not required to remain in occupancy for the aggregate:

1. append exactly one `UnitMoved` aggregate event with original cell, actual final cell,
   entered-cell count, total applied MP, and cause `Voluntary`;
2. if the mover is still active and placed at that cell, invoke `unitEndedMoveOnCell` and
   then its `afterVoluntaryMove` status/passive hook; a removed mover invokes neither;
3. if removal marked the active-owner close, append exactly one
   `ActivationEnded{ActiveUnitDefeated|ActiveUnitImpressed}` now, after the aggregate and all
   eligible move-end seams, discard (never decrement) the captured owner durations, and
   reset only the removed owner's turn bar before clearing turn ownership;
4. evaluate outcome/no-legal-command seams;
5. append batch final-state metadata, append the batch to the log, then publish it.

The aggregate event is the event consumed by movement conditions. The per-cell event exists
for presentation and object ordering. Never emit one aggregate `UnitMoved` per cell.

If an internal invariant fails before any step applies, roll back/no-op the action and
return an internal error. Once a step is committed, later expected interruptions are
successful partial movement and the batch describes the applied result; they are not
reported as if nothing happened.

---

# 8. Ability ownership and affordability

The active unit may select only an ability ID present in its current derived/unlocked
loadout. Resolve the immutable definition through the registry and reject unknown or locked
IDs.

Affordability uses current pools at submission time:

```text
current AP >= ability.cost.actionPoints
current MP >= ability.cost.movementPoints
```

Costs are not part of target geometry. An unaffordable ability has no legal cast for command
purposes, but the UI may ask for a geometry-only preview if the API names that distinction
explicitly. The default `legalAbilityAnchors` query includes affordability and current
activation ownership.

Do not treat zero-cost abilities as inherently invalid. AI safety is provided by successful
command caps and state-digest/no-progress guards in step 11. An accepted cast must still
produce its authored state/event transaction; a placeholder zero-effect ability is invalid
content or unavailable, never a free successful no-op.

---

# 9. Anchor-cell domain

Every cast has exactly one anchor and that anchor is a standable `BoardData` support cell.
This is true for self, unit-targeted, ground-targeted, single-cell, and AoE abilities.

Do not use:

* a nullable anchor;
* an implicit target list as the command;
* a voxel air position;
* a non-standable arbitrary world voxel;
* a unit ID in place of the cell under that unit;
* one anchor per affected target.

An ability with `RangeShape::Self` has exactly the source cell as its candidate anchor.
If its anchor filter makes that cell illegal, it has no legal anchor.

Candidate enumeration starts from all standable board cells in canonical order, then runs
range, LoS, and anchor-filter checks. Bounds/shape-specific generation may optimize this as
long as output and errors are identical.

---

# 10. Anchor range geometry

Compute x/z deltas from source support cell to anchor support cell:

```text
dx = anchor.x - source.x
dz = anchor.z - source.z
manhattan = abs(dx) + abs(dz)
chebyshev = max(abs(dx), abs(dz))
```

Ignore y for all anchor range-shape tests. Use checked signed arithmetic before absolute
value.

The effective inclusive maximum is:

```text
effectiveMaximum = authoredMaximum + source.effectiveStat(Range)
```

The authored minimum is unchanged. Range does not enlarge AoE radius. Use a checked wide
sum and reject/cap impossible overflow as an invariant error.

Implement exact shapes:

| Shape | Legal geometry |
|---|---|
| `self` | `dx == 0 && dz == 0`; authored min/max were validated consistently |
| `diamond` | Manhattan distance within inclusive min/effective max |
| `orthogonalLine` | `dx == 0 || dz == 0`, distance `max(abs(dx),abs(dz))` in bounds |
| `diagonalLine` | `abs(dx) == abs(dz)`, Chebyshev distance in bounds |

Do not add Euclidean distance, height, facing cones, wraparound, target size, or a Unity
physics query.

---

# 11. Three-dimensional line of sight

If `requiresLineOfSight` is false, skip this stage. If true, query the board's final
three-dimensional voxel LoS from the source standing volume to the anchor standing volume.
Use the step-05 convention consistently (support cell plus the documented eye/target
offset).

Only the source-to-anchor ray is checked. Do not independently raycast to each AoE unit.
Terrain/voxel opacity and blocking battle objects participate exactly as the board seam
defines. `BattleQueryService` constructs the context-backed
`IBoardLineOfSightExtraBlockers` adapter and passes it to `BoardLineOfSight::trace`; source
and anchor endpoint columns do not block their own ray.

LoS is a pure query. It must not load chunks, rebuild the graph, resume fluids, mutate
objects, or depend on renderer/physics frame state.

---

# 12. Anchor filter semantics

Evaluate `AbilityDefinition::anchorFilter` against the anchor's current occupant.
Relationship is relative to the casting source:

* source occupant: requires `allowSelf`;
* another active same-side occupant: requires `allowAllies`;
* active opposing occupant: requires `allowEnemies`;
* defeated/removed occupant: does not normally exist on the board in v1;
* no occupant: requires `allowEmptyCell`.

`allowDefeated` remains a data contract for future systems but cannot make a removed,
non-occupying v1 unit targetable. Do not search a graveyard list and fabricate a cell. Revive
is not a v1 effect.

When an occupied anchor is legal, capture that occupant as `primaryUnit`. When an empty
anchor is legal, `primaryUnit` is empty. An ability may legally capture no affected unit
because it has source-unit or cell-scoped effects; do not require a target count greater
than zero unless the definition schema explicitly does so.

---

# 13. AoE cell generation

Generate `areaCells` once around the legal anchor. Area geometry also ignores elevation.
Board cells remain exact standable support-cell IDs.

For an x/z coordinate selected by an area shape, include every standable board cell whose
x/z matches and whose board-local y is present. This handles stacked walkable layers
without inventing a nearest-height rule. Sort with the board comparator and deduplicate.
The exact anchor is always included when it exists and matches the shape.

Implement:

| Area | Included x/z offsets |
|---|---|
| `single` | anchor only |
| `diamond` | `abs(dx) + abs(dz) <= radius` |
| `square` | `max(abs(dx), abs(dz)) <= radius` |
| `cross` | `(dx == 0 || dz == 0) && max(abs(dx),abs(dz)) <= radius` |
| `line` | from anchor in the source-to-anchor dominant direction, length/radius per schema |

For line direction:

1. compute source-to-anchor x/z delta;
2. choose x when `abs(dx) >= abs(dz)`, otherwise z;
3. sign follows that chosen delta;
4. enumerate anchor plus forward offsets through the authored radius/length;
5. if both deltas are zero, enumerate the anchor exactly once and stop.

The x-on-tie rule is locked. The zero-direction rule must not choose an arbitrary positive x
direction and must not duplicate the anchor. Clip all shapes to cells present in
`BoardData`; absence is not an error.

---

# 14. Affected filters and captured targets

Apply `affectedFilter` after area generation. It is independent from the anchor filter.
For each canonical area cell:

* if occupied by the source, include its unit only if `allowSelf`;
* if occupied by an active ally/enemy, include its unit only if the matching relationship
  flag is true;
* if empty, include the cell only if `allowEmptyCell`;
* a legal occupied cell belongs to `affectedCells` and its unit belongs to
  `affectedUnits`;
* an occupied cell rejected by the unit relationship is not included merely because
  `allowEmptyCell` is true.

As with anchors, `allowDefeated` does not resurrect removed board occupancy.

Capture unit IDs once, sort by the canonical order of their captured support cells, then
`BattleUnitId` as a tie-break. Capture cells in the board comparator. Remove duplicates
explicitly even if the geometry generator should not create them.

Do not infer friendly fire from area membership. An anchor may be an enemy while an ally in
the AoE is excluded, or an empty cell may anchor an enemy-only AoE.

---

# 15. Cast-plan execution seam

Step 08 finishes `planCast`, legal-anchor enumeration, and preview. Extend the command
gateway with an explicit effect-runtime capability query, for example:

```cpp
bool canResolveAllEffects(const AbilityDefinition &p_ability) const;
```

If false, `submit(CastAbilityCommand)` returns
`CommandRejection::EffectRuntimeUnavailable` after all cheap identity/phase checks but before
action allocation, resource spending, or events. This temporary guard is removed
incrementally:

* step 09 returns true for an ability only when every payload in it is a core payload it can
  execute;
* step 10 supports the remaining status/passive/object payloads;
* no step treats an unsupported effect as a successful skip.

Do not add a second temporary cast command. The final `CastAbilityCommand { unit, abilityId,
anchor }` and `CastPlan` remain unchanged.

Step 09 will execute a valid captured plan in this fixed high-level order:

1. deduct actual AP and authored cast MP costs;
2. append effective resource events;
3. append `AbilityCast` with source/anchor/target-distance metadata;
4. execute authored `EffectApplication` entries in order against their explicit scopes;
5. run after-cast hooks;
6. finalize outcome/activation state;
7. append then publish the complete action batch.

The final `AbilityCast` payload also copies canonical `affectedCells` and deduplicated
`affectedUnits` from this one `CastPlan`. Its event header carries the cast's one
`BattleActionId`; `{BattleId, BattleActionId}` is the later ability-condition window ID.
Do not allocate a second "cast window" counter.

Document this seam now and test that step 08 never spends a cost for an unavailable cast.

---

# 16. Legal-command and automatic-end query

Add a pure query equivalent to:

```cpp
bool hasAnyLegalNonEndCommand(
    const BattleContext &p_context,
    BattleUnitId p_unit);
```

It is true if at least one non-origin move is affordable or at least one owned, affordable
ability has a legal anchor and a fully installed effect runtime. `EndTurn` itself is not
counted.

The scheduler/session uses this query after activation-start hooks/objects and after each
completed non-end action. There is no fabricated automatic `EndTurnCommand`:

* if false during activation start, append `ActivationEnded{NoLegalCommands}` and the
  normal bar reset/phase/outcome events to that already-open no-action scheduler/system
  batch; allocate no action ID and no second batch;
* if an accepted move/cast makes it false, append the same automatic activation-end
  transition to that command's existing action batch after its effects/reactions, as locked
  by step 07; allocate no second command/action/batch.

This prevents an activation from waiting for impossible input while preserving the
committed-batch snapshot/range contract.

During this step, an ability rejected only because its resolver is not installed is not a
currently legal command. This may auto-end a core-runtime-less fixture; tests that exercise
target previews can use the query directly without starting an activation. Steps 09-10 make
real content executable.

---

# 17. Command rejections

Extend step 06's one `CommandRejection` enum rather than returning ad hoc strings or
introducing another error type. Retain its earlier values and append these exact values:

```text
UnitNotPlaced
NotActiveUnit
UnknownBoardCell
UnknownAbility
AbilityNotOwned
InsufficientActionPoints
InsufficientMovementPoints
DestinationBlocked
DestinationUnreachable
AnchorOutOfRange
AnchorLineOfSightBlocked
AnchorFilterRejected
EffectRuntimeUnavailable
NoStateChange
```

Use the existing `BattleTerminal`, `WrongPhase`, `WrongController`, `UnknownUnit`, and
`UnitRemoved` values for those cases. `NoStateChange` covers destination equal to origin.
An internal invariant failure after resolution starts is the typed TechnicalAbort path, not
a command rejection.

For preview, preserve enough detail to distinguish geometry, LoS, occupancy, relationship,
and affordability. Do not localize strings in the runtime. Tests assert enum/value and
stable contextual fields, not prose punctuation.

Every rejected command must leave a deep-equal comparison of:

* units and occupancy;
* resources, phase, active unit, and scheduler time;
* RNG state;
* next runtime/action/batch/event IDs;
* event-log length and subscriber observations;
* deterministic state digest.

---

# 18. Snapshot and event consistency

Extend `BattleSnapshot` only with immutable query data actually needed by external
consumers. Prefer exposing query results from the session over copying the whole definition
graph.

Every accepted movement batch must have:

* one new `BattleActionId` and `BattleBatchId`;
* monotonically contiguous event sequence values;
* the same action/batch IDs on all nested resource, step, movement, trigger, and hook events;
* a before digest matching the pre-command state;
* an after digest matching the state after automatic consequences;
* log append before subscriber publication.

Do not call `ContractProvider::trigger` for each event while still mutating the batch.
Build/append the immutable batch first, then publish the batch once through the already
defined provider.

---

# 19. Required movement tests

Add focused tests under `Playground/tests/battle/` and board/path tests where appropriate.

## 19.1 Weighted planning

Test:

* a longer cell-count path wins because its MP cost is lower;
* entering costs 1/2/3 exactly and origin costs nothing;
* exact MP boundary succeeds and one point short fails;
* units and blocking objects block destinations and intermediate cells;
* the moving unit may leave its own origin;
* allies cannot be crossed;
* no diagonal path exists;
* equal-cost/equal-length paths use lexicographically smaller complete cell sequence;
* unreachable, unknown, blocked, origin destination, and overflow paths fail stably.

## 19.2 Authoritative gateway

Construct a preview, mutate occupancy/resources with another accepted command where
possible, then submit the original destination and prove the gateway replans or rejects.
There must be no API accepting a trusted path/cost.

Test every invalid movement failure for deep-equal no mutation/no IDs/no log/no publish.

## 19.3 Movement event golden

For a multi-cell weighted route, assert exact:

1. action start/header metadata;
2. per-cell `ResourceSpent(MovementPoints, MovementCost)` event;
3. `UnitMovementStep`;
4. reserved left then enter seam;
5. next step;
6. one aggregate `UnitMoved`;
7. ended-move and after-voluntary seams;
8. batch commit/publish.

Assert aggregate entered-cell count and MP cost use applied values. Assert there is never one
aggregate event per step.
With a fixture enter trigger that drains MP, assert the next cell is rechecked, an
unaffordable remainder stops before transition, earlier costs stay spent, and the aggregate
reports only the applied prefix.
Add a fatal enter-trigger trace: movement step -> trigger damage/removal -> `UnitMoved`
using `lastOccupiedCell` -> one `ActivationEnded{ActiveUnitDefeated}` -> terminal/continuation
facts. Assert the entire trace retains the same TurnIndex, no ended-move/after-voluntary hook
runs for the removed mover, and step 15 sees the aggregate and removal in the closing turn.

---

# 20. Required targeting tests

## 20.1 Range shapes

For every shape, test min/max inclusive boundaries, points just outside, negative deltas,
height differences ignored, and Range extending max only. Test self has exactly one anchor.

## 20.2 LoS

Test a clear ray, opaque terrain block, transparent-tag exception, blocking battle object,
source/anchor endpoint behavior, and equal x/z with different y under the step-05 LoS
contract. Prove LoS is checked source-to-anchor once and does not reject an AoE unit hidden
from the source after the anchor is visible.

## 20.3 Filter separation

Create cases where:

* enemy anchor is legal while an ally in the AoE is excluded;
* empty anchor is legal and enemies in the AoE are included;
* self anchor is legal but self is excluded from affected units;
* an occupied rejected cell is not treated as empty;
* an ability has a legal anchor and zero affected units but a cell/source scope;
* removed/defeated units are not targetable despite `allowDefeated`.

## 20.4 Area shapes/order

Golden-test every area shape at board edges and across two standable y layers with the same
x/z. Test canonical order, explicit deduplication, line dominant-axis x ties, negative
directions, and source-equals-anchor line yielding exactly one anchor cell.

## 20.5 Capture stability

Capture an AoE plan with several units. In a resolver test seam, move/remove one captured
unit and prove membership/order does not recalculate, while later resolution would observe
its current active state. Prove a unit entering the area after capture is not added.

## 20.6 Cast availability

Assert a valid preview can be produced before the effect runtime exists, but authoritative
cast submission returns `EffectRuntimeUnavailable` with no cost, ID, event, RNG draw, or
publish. Unknown/unowned/unaffordable/out-of-range/blocked/filter-rejected cases return the
stable earlier error.

---

# 21. Integration trace

Extend the headless battle fixture/demo without making normal exploration auto-enter battle:

1. construct the synthetic board and placed debug teams;
2. begin an activation;
3. print reachable destinations with exact weighted costs;
4. submit a multi-cell move through `BattleSession`;
5. print the aggregate movement event and resulting MP/cell;
6. enumerate anchors/previews for `training-strike`;
7. print one selected anchor, area cells, primary unit, and affected units;
8. demonstrate unavailable cast rejection if step 09 is not yet present;
9. end the activation normally.

Output must be deterministic for the fixed fixture. Keep diagnostics behind the existing
debug/headless entry point; do not replace the exploration boot path.

---

# 22. Expected file changes

Adapt paths to repository conventions, but expect changes equivalent to:

```text
Playground/srcs/battle/query/battle_query_service.hpp
Playground/srcs/battle/query/battle_query_service.cpp
Playground/srcs/battle/query/battle_plans.hpp
Playground/srcs/battle/movement/battle_movement.hpp
Playground/srcs/battle/movement/battle_movement.cpp
Playground/srcs/battle/battle_session.cpp
Playground/srcs/battle/battle_command.hpp
Playground/srcs/battle/battle_event.hpp
Playground/tests/battle/battle_movement_tests.cpp
Playground/tests/battle/battle_targeting_tests.cpp
Playground/tests/battle/battle_command_atomicity_tests.cpp
Playground/docs/battle.md
```

Update CMake source/test lists. Modify board/pathfinder files only to expose the final
weighted/blocked query required here; do not fork board ownership. Modify registry/data
definitions only if a prerequisite handoff demonstrates a real schema bug, in which case
stop and reconcile all affected prompts first.

---

# 23. Documentation requirements

Document:

* support-cell coordinates and four-directional movement;
* destination terrain-cost semantics and canonical path ties;
* unit/object blocking and no ally pass-through;
* command values versus recomputed internal plans;
* exact range shapes, elevation exclusion, Range max-only rule, and 3D LoS;
* exactly-one-anchor rule;
* anchor-filter versus affected-filter truth table;
* every AoE shape, stacked-y behavior, line direction/tie/zero case;
* one-time canonical target capture;
* movement step versus aggregate event use;
* append-then-publish transaction behavior;
* temporary complete-effect-runtime rejection and which later step removes it.

Remove any stale documentation that tells a player/AI to pass a path, distance, target list,
or damage result into a command.

---

# 24. Non-goals

Do not implement actual damage/heal/shield/status/object effect execution, status modifiers,
traps, AI rule evaluation, battle UI/input, movement animation, camera, VFX, sound,
opportunity attacks, zones of control, diagonals, jumping, flight, facing, push chains,
multi-cell units, reserves, summons, revive, accuracy, critical hits, random targeting, or
path preview caching.

Do not use Unity navigation/physics as combat authority. Do not create player-only or
AI-only validators. Do not rebuild or edit the live world graph during a battle.

---

# 25. Acceptance criteria

This step is complete only when:

* [ ] one const query service owns final movement and targeting planning;
* [ ] previews and submission share planners and submission always replans;
* [ ] move commands contain no trusted path/cost/distance;
* [ ] weighted four-directional paths honor unit/object blocking and canonical ties;
* [ ] applied movement updates cell/occupancy/MP atomically per step;
* [ ] one aggregate voluntary movement event follows ordered step detail;
* [ ] left/enter/ended/hook seams have final ordering and are no-ops until step 10;
* [ ] every cast uses exactly one standable support-cell anchor;
* [ ] range shapes, Range bonus, and 3D LoS match the locked rules;
* [ ] anchor and affected filters are independent;
* [ ] all AoE cells/units are canonical, unique, and captured once;
* [ ] line AoE has deterministic dominant direction and exact zero-direction behavior;
* [ ] unsupported casts fail before all mutation/ID/event/RNG activity;
* [ ] no-legal-command query cannot count `EndTurn` as a productive action;
* [ ] invalid commands pass deep atomicity tests;
* [ ] movement/targeting unit and integration tests pass;
* [ ] full Playground build/tests and exploration smoke run remain green.

---

# 26. Required handoff report

Report:

1. exact query/plan/preview types and public methods;
2. exact path cost/tie algorithm and blocking predicates;
3. exact command revalidation and invalid-command atomicity mechanism;
4. exact movement event/trigger ordering and partial-interruption policy;
5. exact range, LoS, anchor-filter, AoE, affected-filter, and capture algorithms;
6. exact stacked-y and line-zero decisions;
7. complete effect-runtime availability behavior left for steps 09-10;
8. command errors added;
9. files created/materially changed/deliberately untouched;
10. repository drift and adaptations;
11. commands actually run and results;
12. deterministic headless trace actually observed;
13. work explicitly deferred to later steps.
