# Implement modes, encounters, and battle lifecycle

Integrate the headless battle systems from steps 01-11 with both supported presentation
sources. Ordinary Wild and Trainer encounters freeze a region of the live generated world;
Gym and Special encounters load an authored handcrafted board into an isolated arena owned
by the same `GameSceneWidget`. Walking into a Bush or requesting a named debug encounter
must queue a safe transition, run through the common command path, and tear down without
leaving stale entities, contracts, render visibility, terrain mutation, or input state.

This is an implementation task. Modify Playground runtime code, add lifecycle tests and a
small debug-autoplay seam, update implemented-system documentation, and keep the complete
build green. Do not add battle overlays, tactical mouse input, creature views, or the final
HUD in this step; those arrive in steps 13 and 14.

---

# 1. Repository baseline

Implement against the live repository after steps 01-11. Reinspect every path named here
before editing and adapt names to the prerequisite handoff reports without creating a
parallel API.

At the original `6ae3151` baseline:

* `Playground/srcs/main.cpp` owns `Registries`, `GameContext`, and one
  `GameSceneWidget`;
* `GameContext` contains only `EventCenter` and `WorldContext`;
* `WorldContext` owns `VoxelWorld` and `WorldNavigation` and exposes the mutable
  `explorationActive` boolean;
* `GameSceneWidget` privately owns the world seed, immutable `WorldPlan`, player entity,
  player `Actor`, camera, hover renderer, player chunk streamer, fluid simulator, and all
  exploration logics;
* `ActorPathLogic` emits `playerMoved` only after the actor reaches a support cell;
* `GameSceneWidget` independently subscribes to `playerMoved` to queue portal teleports;
* `ExplorationInputLogic`, `ActorPathLogic`, and `CameraControllerLogic` consult
  `world.explorationActive`;
* streaming, fluid simulation, day/night, voxel-light synchronization, and generated-world
  render visibility do not consult that flag;
* no ModeManager or encounter runtime exists.

After steps 01-11, the repository must additionally contain the exact equivalents of:

* persistent `PlayerData` and a six-slot team of `CreatureUnit` values;
* immutable species, AI, encounter, ability, status, and object registries;
* `BoardData` / `BoardBuilder`, `BoardSourceDescriptor`, live-world and handcrafted-board
  construction paths, and deployment-zone contracts;
* `BattleContext`, `BattleSession`, typed events, placement commands, scheduler, command
  validator/resolver, effects/statuses/objects, outcome rules, and ordered-rule AI;
* a read-only battle snapshot/state digest API;
* one value-based submission gateway used by player callers, AI, and tests.

If any prerequisite is absent, stop and finish its earlier step. Do not compensate in this
step with widget-owned combat state or direct mutations.

The old deleted Playground BattleMode is historical evidence only. Do not restore or
cherry-pick it.

---

# 2. Required final behavior

After this step:

1. The executable still boots directly into exploration.
2. F8 requests a configured named debug encounter.
3. Reaching a standable cell whose air cell contains a voxel tagged `Bush` requests the
   current biome's wild encounter.
4. Portal, scripted interaction, and wild-encounter precedence is resolved once in a
   central exploration interaction service.
5. A request never switches mode from inside an engine component iteration or event
   callback; it is queued and committed at the post-update frame boundary.
6. Battle construction is transactional. Failure leaves exploration fully usable.
7. Entering battle stops and clears the player path, disables exploration input, hides the
   stale hover marker and exploration avatar renderer, and pauses fluid simulation.
8. A live-world battle keeps generated-world rendering, player streaming, daylight, and a
   pinned battle region alive. A handcrafted battle hides generated-world chunk rendering,
   activates an isolated immutable arena renderer in the same scene widget, and consumes no
   world-plan, chunk-pin, fluid, content-revision, or world-seed geometry input.
9. The selected board source remains immutable for the session. Live traversal is guarded
   by required-chunk revision stamps; handcrafted traversal owns its local cell source and
   has no stale-live-terrain condition.
10. A production player battle begins in manual Placement and waits for placement commands.
   It is not auto-placed merely because UI is not implemented yet.
11. A development-only autoplay option may submit placement and turn commands through the
    same public gateway so this step can demonstrate a complete round trip before steps
    13-14.
12. A terminal result queues, rather than immediately performs, BattleMode exit.
13. Exit destroys battle-owned contracts, the selected environment lease, and isolated
    arena entities before restoring the exact captured exploration input, generated-world
    visibility, player rendering, fluid activity, streaming, and hover state.
14. The player does not resume the pre-encounter click path after battle.
15. Repeated live-world and handcrafted battles do not accumulate subscriptions, entities,
    streamed regions, arena sources, pending events, or stale session pointers.

The exploration world is never copied or reconstructed to enter battle. A handcrafted arena
is materialized from its own validated board/prefab data; it is not a snapshot of exploration
terrain and does not replace the persistent `VoxelWorld`.

---

# 3. Locked ownership model

Use the following ownership direction:

```text
main
  owns Registries
  owns GameContext                  persistent run/player state
  owns GameSceneWidget
        owns GameEngine and world-facing entities/components
        owns generated-world renderer/visibility control
        owns ExplorationInteractionResolver
        owns ModeManager
              owns optional BattleModeRuntime
                    owns BattleSession
                    owns one battle-environment lease variant
                      LiveWorldBattleLease | HandcraftedArenaLease
                    owns exploration suspension
                    owns lifecycle contracts
```

`GameContext` must not own a `BattleSession` that borrows `VoxelWorld`. The context
outlives `GameSceneWidget`, while the widget destructor explicitly resets the world before
its base engine is destroyed. Keeping a session in `GameContext` would permit it to outlive
the world it references.

The same lifetime rule applies to a handcrafted arena even though it owns rather than
borrows its voxel grid. Its cell source, renderer entities, and `BoardData` are
`BattleModeRuntime` state and must be removed from the game engine before the widget or
engine dies. An authored arena is not a second widget, scene, camera, mode manager, or
headless battle architecture.

Definitions may be referenced from a session because `Registries` outlives the widget.
Persisted state and lifecycle notices still store string IDs/value handles, never definition
pointers.

`ModeManager` and `BattleModeRuntime` are plain Playground classes, not global
singletons and not ECS components. The widget calls their frame-boundary methods. Headless
`BattleSession` remains unaware of widgets, entities, cameras, streamers, and fluids.

---

# 4. Replace the split-brain exploration flag

The original mutable `WorldContext::explorationActive` is too weak to be the mode
authority. Introduce one observable control state equivalent to:

```cpp
enum class GameModeKind
{
    Exploration,
    Battle
};

struct ControlContext
{
    spk::ObservableValue<GameModeKind> mode{GameModeKind::Exploration};

    [[nodiscard]] bool isExploration() const noexcept;
    [[nodiscard]] bool isBattle() const noexcept;
};

struct GameContext
{
    EventCenter events;
    WorldContext world;
    ControlContext control;
    PlayerData player;
    std::uint64_t worldSeed = 0;
    std::shared_ptr<const WorldPlan> worldPlan;
};
```

Equivalent placement is acceptable, but there must be exactly one authoritative mode value.
Refactor the existing logics to call a semantic query such as
`GameContext::isExplorationActive()`. Do not retain an independently writable Boolean that
can disagree with `mode`.

Publish the world seed and immutable world plan only after scene construction succeeds, at
the same transactional commit point used for `world` and `navigation`. On widget
destruction, clear the active battle first, then navigation/world/worldPlan in a documented
order. Preserve the existing construction-failure guarantees.

---

# 5. Lifecycle value types

Add stable, value-owned request and notice types equivalent to:

```cpp
struct BattleStartRequest
{
    ResolvedEncounter encounter;
    spk::Vector3Int worldCell{};
    VoxelOrientation playerApproach = VoxelOrientation::PositiveZ;
    std::string stableEncounterInstanceId;
    std::uint64_t encounterOrdinal = 0; // valid value; encounterSerial stores next
    std::uint64_t encounterResolutionSeed = 0;
    std::uint64_t combatSeed = 0;
    std::optional<std::string> debugAutoplayPlayerProfileId;
};

struct BattleRuntimeGeneration
{
    std::uint64_t value = 0; // 0 invalid; process-local presentation lifetime only
    auto operator<=>(const BattleRuntimeGeneration&) const = default;
};

enum class BattleLifecycleStage
{
    Entered,
    ResultReady,
    WillExit,
    Exited,
    EntryFailed
};

struct BattleLifecycleNotice
{
    BattleLifecycleStage stage;
    BattleRuntimeGeneration generation;
    BattleId battleId;
    std::string encounterDefinitionId;
    std::optional<BattleOutcome> outcome;
};

struct BattleBatchPublication
{
    BattleRuntimeGeneration generation;
    BattleId battleId;
    CommittedBattleBatch batch;
};
```

Use the strong IDs introduced by earlier steps. The shape matters more than spelling:

* requests contain no `Actor*`, widget pointer, registry pointer, or temporary JSON reader;
* notices contain no mutable `BattleContext&`;
* `ModeManager` checked-increments its own generation counter once for every accepted entry
  attempt before entry work; success, `EntryFailed`, every lifecycle notice, and every
  externally published committed batch retain that value;
* presentation added later obtains the active session from `ModeManager` during a
  synchronous notice and validates both generation and `BattleId`;
* the stable encounter instance ID distinguishes repeat wild encounters at the same cell;
* `ResolvedEncounter` is a value copy containing the selected tier/team/board source and
  policy/enemy roster, so entry never rolls the weighted team or changes board kind again;
* resolution and combat seeds are derived before session construction and stored in the
  request/result.

`BattleRuntimeGeneration` is deliberately process-local and presentation-only. Unlike the
32-bit deterministic `BattleId`, it cannot repeat during one process lifetime; checked
counter exhaustion refuses a new entry. It is not copied into `BattleDescriptor`, replay,
result/save data, or any simulation digest. Callback/queued-publication acceptance always
requires the pair `{generation, battleId}`. During exit, mark the generation inactive before
releasing contracts/entities/session; `Exited` still carries the retired value, and every
late item for it is ignored even if a future battle folds to the same `BattleId` and event
sequence.

Construct the step-06 `BattleDescriptor` by copying, without recomputation:

```text
GameContext.worldSeed                       -> worldSeed
request.encounterOrdinal                   -> encounterOrdinal
request.encounterResolutionSeed            -> encounterResolutionSeed
request.combatSeed                         -> combatSeed
request.stableEncounterInstanceId          -> stableEncounterIdentity
request.encounter.encounterDefinitionId    -> encounterDefinitionId
request.encounter.resolvedTier             -> resolvedTier
request.encounter.teamId                   -> resolvedTeamId
request.encounter enemyRoster[i].id        -> participantSeed[i].encounterSpawnMemberId
```

Copy display name, kind, repeatability, taming flag, the complete `BoardSourceDescriptor`,
board policy, and return world cell from the same resolved request/definition. The
descriptor must distinguish `LiveWorld` from `Handcrafted` and, for handcrafted boards,
retain the canonical board-definition ID. Validate the team/member array once; never look
up a new tier/team, substitute another board, or regenerate member IDs during mode entry.
These fields are terminal-result/replay provenance even when the outcome is a loss or
technical abort.

Extend `EventCenter`, or add a widget-owned typed lifecycle signal aggregate, with
value-owned providers for battle lifecycle and committed event-batch notices. Store every
returned RAII contract. Never use a reference-bearing event whose referent is destroyed in
the same callback chain.

---

# 6. ModeManager public contract

Provide one API equivalent to:

```cpp
class ModeManager
{
public:
    struct Dependencies
    {
        GameContext &game;
        const Registries &registries;
        VoxelWorld &world;
        WorldNavigation &worldNavigation;
        Actor &playerActor;
        ActorPathLogic &actorPath;
        spk::TextureMeshRenderer3D &playerRenderer;
        spk::TextureMeshRenderer3D &explorationHoverRenderer;
        spk::VoxelChunkStreamer &playerStreamer;
        spk::VoxelChunkStreamer &battleStreamer;
        spk::VoxelFluidSimulator &fluidSimulator;
        GeneratedWorldPresentationControl &generatedWorldPresentation;
        HandcraftedArenaRendererFactory &arenaRenderers;
    };

    explicit ModeManager(Dependencies);
    ~ModeManager();

    [[nodiscard]] GameModeKind mode() const noexcept;
    [[nodiscard]] bool hasPendingTransition() const noexcept;
    [[nodiscard]] BattleSession *activeBattle() noexcept;
    [[nodiscard]] const BattleSession *activeBattle() const noexcept;
    [[nodiscard]] BattleId activeBattleId() const;

    bool requestBattle(BattleStartRequest);
    bool requestBattleExit(BattleTerminalRecord);

    // Called only by GameSceneWidget after the engine update and deferred teleports.
    void processFrameBoundaryTransitions();

    // Pumps the active session; it never applies a queued mode transition inline.
    void updateBattle();

    // Idempotent and used by GameSceneWidget destruction.
    void shutdown() noexcept;
};
```

Only one pending transition is accepted. Repeated requests while one is pending or while a
battle is active return false without consuming RNG or changing state.

The last two dependency names are responsibility contracts. Reuse the renderer/engine seams
that exist after implementation rather than creating a redundant renderer. They must allow
the runtime to capture/restore generated-world chunk visibility and to register an authored
arena with the existing game engine. Neither interface may expose presentation mutation to
`BattleSession`.

`activeBattle()` is a non-owning query valid only while `mode()==Battle` and until the
next frame-boundary transition. Callers must not cache it across lifecycle notices.

Do not let UI, input, or the encounter resolver call `BattleSession` through a separate
shortcut. Later steps may obtain the session from this manager, but every gameplay mutation
still enters the session's common command gateway.

---

# 7. Frame-boundary transition queue

Events are synchronous and component logics iterate engine registries. Constructing or
destroying battle-facing objects inside `ActorPathLogic::playerMoved` would be a lifecycle
hazard.

Use an explicit queue with at most one value:

```cpp
using PendingModeTransition =
    std::variant<std::monostate, BattleStartRequest, BattleTerminalRecord>;
```

The widget update order must be documented and tested. Use this logical order:

```text
1. update streamer/fluid centers that are valid for the current mode
2. GameEngineWidget::_onUpdate
     - exploration movement may emit playerMoved
     - encounter resolver may queue teleport or BattleStartRequest
3. synchronize voxel lights and other existing post-engine work
4. resolve the one pending exploration interaction
     - teleport has precedence and executes here
     - otherwise forward the battle request to ModeManager
5. ModeManager::processFrameBoundaryTransitions
6. ModeManager::updateBattle
     - AI may submit ordinary commands
     - a terminal result may queue exit for the next boundary
7. invalidate render unit / refresh diagnostics
```

If the established engine ordering after prerequisite steps requires battle pumping before
the render invalidation rather than exactly at item 6, retain the invariant: transition
objects are never destroyed from the callback that requested their destruction.

Do not recursively trigger the same `ContractProvider` to announce appended battle events.
Finish the event batch, queue a value notice, and publish it from the normal pump.

---

# 8. Central exploration interaction resolver

Replace the widget's independent portal subscription with one service:

```cpp
using ExplorationInteraction =
    std::variant<std::monostate, TeleportRequest, BattleStartRequest>;

class ExplorationInteractionResolver
{
public:
    [[nodiscard]] ExplorationInteraction resolvePlayerArrival(
        const spk::Vector3Int &p_supportCell,
        VoxelOrientation p_approach,
        std::uint64_t p_arrivalSequence) const;
};
```

The service borrows immutable portal/world-plan/registry data and the live world for
queries. It owns no session and performs no mutation.

Resolve exactly one interaction in this precedence order:

1. portal/exit pad;
2. explicitly scripted trainer, gym, boss, or story trigger when such markers exist;
3. wild Bush encounter;
4. none.

A portal cell must not also start a wild encounter because subscription order happened to
favor one callback. The resolver result is queued; the widget applies it after update.

Trainer line-of-sight and production gym scripting are later content/progression work.
Keep a typed slot for scripted requests, but do not invent NPC systems in this step.

---

# 9. Actor support-cell and Bush contract

This detail is non-negotiable.

`Actor::cell` is the solid voxel supporting the actor. `isStandable(source, cell)` checks:

```text
cell       = Solid support
cell + Y   = Passable or empty body space
cell + 2Y  = Passable or empty head space
```

The authored Bush voxel is passable and occupies the body-space voxel. Therefore:

```cpp
const spk::Vector3Int bushCell =
    p_supportCell + spk::Vector3Int{0, 1, 0};
const spk::VoxelCell &voxel = world.cell(bushCell);
const VoxelDefinition *definition = world.tryDefinition(voxel);
const bool isBush = definition != nullptr && definition->data.hasTag("Bush");
```

Never test `world.cell(actor.cell)` for the Bush tag; that cell is ground. Use
`VoxelData::hasTag` rather than comparing voxel IDs, so every biome-specific Bush works and
tag case remains normalized.

An arrival can request at most one battle. Starting battle clears the rest of
`Actor::path`, so the actor cannot automatically continue across a patch and immediately
queue another encounter after returning.

No `playerMoved` event is emitted on battle exit, so merely returning to the same Bush cell
does not retrigger. Walking away and intentionally entering a Bush again may trigger a new
wild encounter.

---

# 10. Resolve biome, tier, and encounter deterministically

For an overworld support cell:

1. Compute plan column from world X with `WorldPlan::cellIndexFromWorld(x)`.
2. Compute plan row from world Z.
3. Bounds-check `plan.zone` before indexing.
4. Resolve zone, then `PlanZone::biomeIndex`, then the immutable biome ID.
5. Resolve that biome's wild encounter-table reference established in step 04.
6. Select the tier from current player progression; before step 18 persistence, this is the
   in-memory progression value initialized by step 04.
7. Use the encounter resolver from step 04 to choose the weighted team and board policy,
   and store the returned value-owned `ResolvedEncounter` in `BattleStartRequest`.

Do not collapse the resolved board variant into live-world dimensions. The registry contract
from step 04 is authoritative:

* Wild and Trainer definitions resolve a `LiveWorldBoardPolicyDefinition`;
* Gym and Special definitions resolve a `HandcraftedBoardPolicyDefinition` whose
  `definitionId` must already resolve in `Registries::battleBoards()`;
* Debug definitions may deliberately use either alternative;
* a named development launch preserves the loaded definition's board alternative even when
  its encounter-instance identity uses the `battle/debug/...` grammar.

`BattleStartRequest::worldCell` always remains the exploration support/return cell. For a
handcrafted board it is provenance and the place to which exploration returns, not an arena
origin and not an input to board materialization. The handcrafted definition's
`playerApproach` overrides `BattleStartRequest::playerApproach` when the deployment layout is
built; the request approach is still retained as arrival provenance if the settled request
type stores it.

Before allocating/consuming an ordinal or constructing any RNG, reject a known
Trainer/Gym/Special (all are schema-locked non-repeatable) whose definition ID already
appears in the matching PlayerData cleared set. Return a typed `EncounterAlreadyCleared` interaction result, queue
no transition, and consume no ordinal or draw. Apply the same check to named development
launches of those encounter kinds; schema-locked repeatable `Debug` and `Wild` definitions
are unaffected.

Do not add encounter fields directly to `VoxelDefinition`. Bush material selects the
trigger; biome/encounter data selects the creatures.

The original `WorldPlan` is widget-private. Publish a shared immutable plan through
`WorldContext`/`GameContext` at successful construction so the resolver never reaches
through private widget members.

Derive a stable encounter-instance root seed with the deterministic utility introduced in
step 01, from at least:

```text
world seed
encounter definition/table ID
world support cell
monotonic arrival or encounter ordinal
```

An acceptable semantic path is:

```text
battle/wild/<table>/<x>/<y>/<z>/<ordinal>
```

`PlayerData.encounterSerial` is the **next unconsumed** ordinal. For every resolved Bush
arrival and every accepted named-development-launch request, copy PlayerData, read the current value as
`ordinal`, checked-increment the copy, and derive the identity from the old value. Publish
that working serial exactly once even when the Bush chance fails, so two intentional
arrivals do not silently reuse choices. Until step 18 supplies the save service this is an
in-memory transaction; step 18 must checkpoint the same candidate before allowing further
input or staging a successful battle. Encounter **source**, not `EncounterKind`, selects the
identity grammar: every accepted named development launch uses
`battle/debug/<encounter-id>/<ordinal>` even when the loaded definition remains Trainer,
Gym, Special, Wild, or Debug for taming/clear/outcome policy. A world Bush launch uses only
the wild/table/cell grammar above.

Derive named child streams with the exact step-01 formulas:

```text
encounterResolutionSeed = deriveSeed(worldSeed, instance + "/encounter-resolution")
combatSeed              = deriveSeed(worldSeed, instance + "/combat")
```

The encounter resolver constructs a temporary `BattleRng(encounterResolutionSeed)` for
chance/team selection. `BattleStartRequest` copies the untouched `combatSeed`; step 06
passes it into `BattleDescriptor`, derives `BattleId` from it, and constructs the session
RNG directly. Neither side derives another seed.

Both seeds are derived and recorded for every source kind so encounter/combat provenance is
uniform. Handcrafted board construction consumes **zero** draws and receives neither seed:
its geometry, approach, deployment depth, and opponent placement policy are completely
authored. Changing world seed or encounter ordinal may change encounter/team/combat streams,
but must not change a handcrafted board cell, traversal edge, source digest, or board-build
draw count.

The resolver performs the step-04 exact bounded-sampling calls once before queuing. Raw
`nextU64` draw count may exceed the call count because rejection sampling is unbiased. A
failed Bush chance queues no battle but still consumes that deterministic arrival ordinal
and chance call. A transactional entry retry reuses the already resolved value and combat seed; it
does not reroll the encounter. Debug requests skip only the chance call, as step 04
specifies, and still select their weighted team from the resolution stream.

Never derive a seed from wall time, a pointer, current frame number, unordered iteration,
or the rendered world revision.

---

# 11. Named debug encounter

Reserve F7 for the existing diagnostics. Use F8 to queue a configured encounter ID such as
`debug-battle` from the encounter registry.

The key handler must:

* act only in Exploration with no pending interaction;
* form the same `BattleStartRequest` shape used by Bush encounters;
* resolve the encounter through the same registry and builder;
* never construct a hard-coded species or ability in the key callback;
* print a concise diagnostic when the configured ID is absent;
* consume the key only when it handled the request.

Optionally support a CLI argument:

```text
--debug-battle <encounter-id>
--battle-autoplay <player-ai-profile-id>
```

The autoplay argument is a development smoke-run convenience, not a second battle
architecture. It requires a non-empty profile ID that resolves in `aiBehaviours()`; there is
no flag-only/default profile and no production ID hard-coded in C++. Validate CLI-supplied
profiles during startup (and programmatic profiles before accepting/checkpointing the
request), so an unknown ID consumes no encounter ordinal/RNG and queues no transition.

---

# 12. Transactional BattleMode entry

Build entry in two phases.

## 12.1 Resolve without changing the current mode

1. Validate the already value-resolved encounter request; do not reroll chance/team.
2. Confirm the player's roster can construct its battle projections.
3. Inspect `request.encounter.board` once and branch on its closed variant before creating a
   board request or touching streamer state.
4. For `LiveWorld`, build `WorldBoardRequest` with X/Z size from the resolved policy,
   `minimumWorldY`/`maximumWorldY` copied exactly from the current
   `WorldNavigation::bounds()`, and later pass `gameRules.maxVerticalTraversalGap`; plan the
   required chunk set without reading a partially streamed traversal graph.
5. For `Handcrafted`, resolve the already startup-validated
   `HandcraftedBattleBoardDefinition` and its prefab through immutable registries. Do not
   call `WorldPlan`, inspect `WorldNavigation`, plan/pin chunks, read exploration voxels,
   capture a content revision, use the request world cell as an origin, or pass a seed/RNG
   to the builder.
6. Validate definitions, projections, board source/policy, and placement capacity as far as
   possible without mutating live mode or renderer state. Handcrafted construction uses the
   board definition's size, geometry prefab, approach, and deployment depth and the
   encounter's authored opponent-placement policy.

If any operation fails, destroy the locals, emit one `EntryFailed` notice, and leave
exploration unchanged.

## 12.2 Stage the selected frozen environment with rollback

Use a closed lease variant rather than nullable fields whose combinations can disagree:

```cpp
using BattleEnvironmentLease = std::variant<
    LiveWorldBattleLease,
    HandcraftedArenaLease>;
```

`LiveWorldBattleLease` is the live-only frozen-terrain lease. Terrain mutation must be
paused before the board reads required cells:

1. activate/pin the planned chunks through the battle streamer;
2. verify every required chunk is present;
3. capture and pause the fluid simulator;
4. build `BoardData` and its frozen traversal graph exactly once;
5. capture every required `VoxelChunk::contentRevision()`;
6. retain the required-chunk set, source mapping, and live-world render access for the
   complete session.

`HandcraftedArenaLease` follows a different path:

1. capture and pause the fluid simulator so exploration terrain cannot change invisibly
   while the player is away;
2. materialize the validated prefab at identity into an owning immutable arena cell grid;
3. build `BoardData` from that grid using the handcrafted builder, with no world planning,
   chunk activation/pinning, content-revision capture, or RNG draw;
4. create the isolated arena renderer/entities in the **existing** `GameSceneWidget` and
   engine, but keep them inactive until commit;
5. capture generated-world chunk-render visibility and streamer-facing presentation state;
   do not mutate or replace `VoxelWorld`;
6. retain the owning arena grid for longer than `BoardData` and retain renderer ownership
   until every overlay/unit/object presenter has detached.

Each alternative is the sole RAII owner of the state it changes. The live lease captures
fluid activation plus battle-streamer activation/configuration and leaves player streaming
and generated-world visibility unchanged. The handcrafted lease captures fluid activation,
player/battle streamer activation/configuration, generated-world chunk-render visibility,
and arena renderer registration/activation; it keeps the player streamer processable for a
warm return unless the engine provides a proven no-unload streaming suspension, always
keeps the battle streamer inactive, hides only generated-world render output, and restores
every captured value. Do not duplicate those restorations in `ExplorationSuspension`.

After either source is staged, construct `BattleSession`, enemy roster, player projections,
battle-local combat RNG, event log, Deployment state, and lifecycle subscriptions in local
RAII values. Validate that a production player session is awaiting manual Deployment.

Any failure destroys presentation/session/board locals first, then the staged environment
lease restores fluid and all captured render/stream state exactly. Parsing and cross-registry
validation must already have completed at startup; do not perform avoidable JSON work while
the fluid is paused. `BoardData::terrainIsCurrent()` is unconditionally true for the
handcrafted alternative after successful immutable construction; only explicit runtime
destruction invalidates its presentation binding.

## 12.3 Commit atomically

After preparation succeeds:

1. stop the `Actor` at its current support cell;
2. clear `path`, `segment`, and `segmentProgress` and snap it through
   `ActorPathLogic::placeAtCell`;
3. acquire `ExplorationSuspension`;
4. for a live board, retain generated-world visibility and activate the staged battle
   streamer; for a handcrafted board, hide generated-world chunk rendering, keep the
   exploration world owned and return-ready, and activate the staged isolated arena
   renderer;
5. move the staged `BattleEnvironmentLease`, board, and session into
   `BattleModeRuntime`;
6. set the authoritative mode to Battle;
7. emit `Entered` only after `activeBattle()` and its board source are queryable.

If commit has a potentially throwing operation, arrange RAII rollback so the previous
component activation/visibility states are restored. `Entered` must never be observed with
both generated-world and handcrafted arena renderers visible or with neither selected
source presentable.

---

# 13. Manual placement is the production behavior

The battle session created here must remain in the placement phase until commands arrive.
Enemy placement may use the encounter's authored placement policy, but player placement is
manual:

* no hidden auto-placement in `BattleMode::enter`;
* no direct writes to board occupancy;
* no “temporary” placement that step 13 must undo;
* placement and repositioning use `PlaceUnitCommand` and final confirmation uses
  `ConfirmDeploymentCommand` through the step-06 common gateway;
* confirming before every required player unit is legally placed is rejected without
  mutation.

Step 13 supplies board/keyboard input. Step 14 supplies cards and Ready controls.

For this step's lifecycle smoke run only, an explicitly enabled
`BattleAutoplayDriver` may:

1. choose deployment cells in canonical placement-zone order;
2. submit every placement through the common gateway;
3. submit `ConfirmDeploymentCommand`;
4. ask the step-11 AI planner for both sides;
5. submit every AI command through the same gateway.

The driver must be absent from ordinary Bush and F8 player-controlled battles unless a
validated `--battle-autoplay <profile-id>` was requested. It copies that profile ID and uses
it for every player participant; enemy participants retain their encounter-authored profiles.
It cannot call an effect resolver, occupancy registry,
or outcome setter directly. Its deterministic trace is covered by tests.

---

# 14. ExplorationSuspension

Implement an idempotent RAII object owned by `BattleModeRuntime`. Capture and restore at
least:

* exploration avatar renderer activation;
* exploration hover renderer activation and mesh state;
* any prerequisite input/camera logic activation not already governed by mode;
* diagnostic state needed to prove restoration.

Do not make this object a second owner of fluid, streamers, generated-world visibility, or
arena entities. Those source-specific states belong to the active
`LiveWorldBattleLease | HandcraftedArenaLease`, which captures/restores them exactly. This
split gives every activation flag one RAII owner.

On acquire:

* the authoritative mode query prevents exploration input/path/camera processing;
* clear the hover mesh and deactivate its renderer so the last world hover does not remain
  visible through battle;
* deactivate only the player's `TextureMeshRenderer3D`, not `_playerEntity`;
* assert the already staged environment lease has paused `VoxelFluidSimulator` and selected
  exactly one source presentation;
* leave the player `Actor`, persistent `VoxelWorld`, sun/day logic, voxel-light state, and
  environment-lease-owned streamer/render states untouched.

Why the whole player entity must remain active:

```text
player entity
  Actor                         mode-gated and stationary
  TextureMeshRenderer3D         hidden during battle
  VoxelChunkStreamer            MUST remain active
  VoxelFluidSimulator           explicitly paused
```

Deactivating the entity would also make streaming unprocessable and can unload live terrain.
Even for a handcrafted arena, hide the avatar renderer and generated-world renderer through
their own controls; never deactivate `_playerEntity` as a shortcut.

On release, restore the exact common activation states rather than blindly activating
everything. Clear battle-only hover/selection state before re-enabling exploration. The
environment lease has already removed any isolated arena and restored generated-world,
streamer, and fluid state; assert that ordering in `BattleModeRuntime::shutdown()`.

---

# 15. Source-specific streaming, rendering, and terrain invariants

For a live-world source, use the existing `spk::VoxelChunkStreamerLogic` rather than
manually fighting its ownership rules.

Add a widget-owned, engine-registered battle-streaming entity containing one
`VoxelChunkStreamer`:

* it is inactive in Exploration;
* before Battle entry, set its origin to the board-center chunk;
* set a view range covering the board bounds plus one horizontal chunk of safety and the
  required vertical chunks;
* activate it for the entire `LiveWorldBattleLease`;
* deactivate it only after battle presentation/session teardown.

The ordinary player streamer remains active as well. The streamer logic unions their
windows, which is the correct pinning mechanism. Do not unload/reload the world, copy
chunks, or keep a private voxel snapshot for restoration.

Default 11x11 boards around the stationary player are already inside the player's large
stream window. Assert/diagnose this when preparing the first frame; do not introduce a
manual `setChunkActive` ownership conflict merely to reload chunks that are present.

Pause `VoxelFluidSimulator` before `BoardData` is built and for the whole battle. Board
traversal is built once and treated as frozen. Record the required chunk-coordinate set and
each required `VoxelChunk::contentRevision()` after the build. Before accepting/pumping
commands, diagnose and fail safely if a required chunk disappeared or its content revision
changed. Do not rebuild navigation under units mid-battle.

Do not compare `VoxelWorld::revision()` for this invariant. That global value legitimately
changes when the still-active streamers load or unload an unrelated chunk. Unrelated chunk
presence/revision changes must neither abort the battle nor alter its frozen board state.

Day/night lighting may continue because it does not mutate board cells or battle rules.

A handcrafted source deliberately does none of the live lease work above:

* the ordinary player streamer may remain processable to keep exploration return state
  warm, but its generated-world chunk renderers are hidden behind a captured visibility
  control;
* the battle streamer remains inactive and no handcrafted board coordinate is converted to
  a world chunk coordinate;
* the arena owns an immutable local cell grid and a matching isolated voxel renderer/entity
  set registered in the same engine as exploration;
* board presentation coordinates come from `BoardData`; the presentation binding derives
  its AABB from that mapping plus the arena support shapes, never from
  `BattleStartRequest::worldCell`, `WorldPlan`, or exploration renderer bounds;
* no required-chunk set, `VoxelChunk::contentRevision()`, or global world revision is stored
  for the arena; `terrainIsCurrent()` stays true for its lifetime;
* the persistent camera, HUD, overlay, unit presenters, and input runtime are reused; no
  second scene/widget/camera is created.

Pause the fluid simulator for both source kinds. On handcrafted entry this is an
exploration-world suspension rule, not a board-construction dependency and not evidence that
the authored grid borrows live terrain.

---

# 16. BattleModeRuntime

Use a cohesive owner equivalent to:

```cpp
class BattleModeRuntime
{
private:
    BattleStartRequest _request;
    ExplorationSuspension _explorationSuspension;
    BattleEnvironmentLease _environmentLease;
    BoardData _board;
    std::unique_ptr<BattleSession> _session;
    std::vector<LifecycleContract> _contracts;
    std::optional<BattleAutoplayDriver> _debugAutoplay;

public:
    [[nodiscard]] BattleSession &session();
    void update();
    [[nodiscard]] BattleTerminalRecord takeTerminalRecord();
};
```

Declaration/destruction order must ensure:

1. autoplay/controller callbacks resign;
2. session consumers resign;
3. session is destroyed;
4. board and selected environment lease release; a handcrafted lease removes its isolated
   arena entities before freeing their cell grid, while a live lease releases chunk pins;
5. exploration suspension restores components.

If C++ member destruction order makes that unclear, write an explicit idempotent
`shutdown()` and test it. Do not rely on a comment that contradicts reverse declaration
order.

The runtime owns no final progression/recruit commit. It stages the permanent
`BattleTerminalRecord` plus the still-queryable session/log for step 18 and asks
`ModeManager` to leave. Until step 18, use an explicit `IBattleTerminalSink` seam that
records/logs the terminal record without pretending a full persistent result exists.

Expose a read-only presentation binding containing the active `BoardData`, its
`BoardSourceDescriptor`, a source-agnostic presentation-cell query, and presentation AABB.
Steps 13-14 must not reach around this binding for `VoxelWorld` merely because the current
encounter happens to be live-world based.

---

# 17. Battle update and AI

`ModeManager::updateBattle` drives only headless orchestration:

* if `BattlePhase::Deployment` and debug autoplay is enabled, submit the next ordinary
  placement command;
* otherwise call the step-11 `BattlePump::advanceUntilPlayerInput()` with its bounded
  operation budget; it advances `AwaitingActivation`, plans/submits enemy-owned
  `Activation` commands, and stops at player-owned input, terminal, or a typed stall;
* if a player-owned `Activation` is waiting, return immediately;
* if terminal, construct exactly one immutable `BattleTerminalRecord` and queue exit.

Honor the step-11 command cap/no-state-change guard. Do not spin an entire pathological
battle forever in one render update. Use a bounded logical-operation budget per widget
update while preserving identical event/state results regardless of frame delta.

Render delta never advances the stamina timeline. The scheduler advances through its fixed
logical jumps from step 07.

---

# 18. Result and exit ordering

On terminal outcome:

1. freeze further command acceptance;
2. finalize the value-owned terminal record while keeping the committed event log queryable;
3. emit `ResultReady`;
4. stage the result for later progression/persistence work;
5. queue a `BattleTerminalRecord` transition;
6. return from the current battle update without destroying anything.

At the next safe frame boundary:

1. emit `WillExit` while the session is still queryable, allowing later presenters/HUD to
   unbind;
2. invalidate the active runtime generation and resign runtime/presentation contracts;
3. detach overlays, unit/object presenters, and the arena renderer-facing presentation
   binding while `BoardData` is still alive;
4. destroy debug controller and session;
5. destroy `BoardData`, then release the selected environment lease (live chunk pins or
   handcrafted renderer/grid) and restore fluid plus generated-world visibility/stream
   state exactly;
6. restore the common exploration suspension (input/avatar/hover/camera-facing state);
7. clear every battle pointer/pending command;
8. set control mode to Exploration;
9. emit `Exited` with value IDs/result summary only.

Do not emit `playerMoved`. Do not restore the old actor path. The actor remains snapped to
the encounter support cell and awaits a new click.

Step 18 will insert result-commit and loss-respawn operations between staging and final
exploration restoration. Keep that seam explicit.

---

# 19. Failure, close, and destruction behavior

Cover these cases deliberately:

* registry encounter lookup fails;
* a handcrafted board/prefab reference is missing despite startup validation;
* handcrafted prefab materialization or arena-renderer staging fails;
* biome has no wild table;
* no legal board can be derived;
* team has no deployable creature;
* BattleSession construction throws;
* window closes during Placement;
* widget is destroyed during an AI battle;
* `ModeManager::shutdown` is called twice;
* a result and window close arrive in the same frame.

Entry errors do not partially suspend exploration. Runtime shutdown and user/window close
never invoke gameplay result/progression callbacks and do not fabricate an Aborted battle
outcome. Only a live session reaching one of step 06's closed technical-abort reasons creates
an immutable Aborted result, with no victory/defeat/recruit/progress commit.

`GameSceneWidget::~GameSceneWidget` must call `ModeManager::shutdown` before removing
voxel-light entities and before resetting navigation/world. Add a lifetime test mirroring
the existing construction-failure test.

---

# 20. Diagnostics

Extend the F7 overlay with concise rows:

```text
Mode                 Exploration | Battle
Encounter            <id> | -
Board source          LiveWorld | Handcrafted:<definition-id> | -
Battle phase         Deployment / AwaitingActivation / Activation / Terminal / -
Active side/unit     Player|Enemy / <BattleUnitId> / -
Battle ID            <stable debug value>
Battle seed          <value>
Board present bounds min -> max
Fluid paused         yes/no
Battle stream active yes/no
World render visible yes/no
Arena render active  yes/no
Pending transition   none/start/exit
```

Diagnostics may poll because the F7 overlay is a diagnostic tool. Gameplay HUD added in
step 14 must be event/snapshot-driven.

---

# 21. Expected file changes

Add cohesive files equivalent to:

```text
Playground/srcs/core/game_mode.hpp
Playground/srcs/core/mode_manager.hpp
Playground/srcs/core/mode_manager.cpp
Playground/srcs/core/battle_mode_runtime.hpp
Playground/srcs/core/battle_mode_runtime.cpp
Playground/srcs/core/exploration_suspension.hpp
Playground/srcs/core/exploration_suspension.cpp

Playground/srcs/encounters/exploration_interaction.hpp
Playground/srcs/encounters/exploration_interaction_resolver.hpp
Playground/srcs/encounters/exploration_interaction_resolver.cpp
Playground/srcs/encounters/battle_start_request.hpp

Playground/srcs/battle/battle_environment_lease.hpp
Playground/srcs/battle/live_world_battle_lease.hpp/.cpp
Playground/srcs/battle/handcrafted_arena_lease.hpp/.cpp
Playground/srcs/battle/handcrafted_arena_renderer.hpp/.cpp
Playground/srcs/battle/presentation/battle_presentation_board_binding.hpp
Playground/srcs/battle/presentation/battle_presentation_cell_source.hpp/.cpp
Playground/srcs/battle/debug_battle_autoplay.hpp
Playground/srcs/battle/debug_battle_autoplay.cpp

Playground/tests/core/mode_manager_test.cpp
Playground/tests/core/battle_lifecycle_test.cpp
Playground/tests/encounters/exploration_interaction_resolver_test.cpp
Playground/tests/encounters/bush_encounter_test.cpp
```

Modify at least:

```text
Playground/srcs/core/game_context.hpp
Playground/srcs/core/game_context.cpp
Playground/srcs/core/event_center.hpp
Playground/srcs/game_scene_widget.hpp
Playground/srcs/game_scene_widget.cpp
Playground/srcs/main.cpp
Playground/srcs/logics/exploration_input_logic.cpp
Playground/srcs/logics/actor_path_logic.cpp
Playground/srcs/logics/camera_controller_logic.cpp
Playground/tests/game_scene_widget_lifetime_test.cpp
```

Equivalent organization is acceptable. Do not put encounter resolution or BattleSession
ownership into `main.cpp`.

---

# 22. Required headless tests

## 22.1 Mode queue

Prove:

* requests do not change mode until `processFrameBoundaryTransitions`;
* only one start request is accepted;
* requests during Battle are ignored;
* result queues exit and session remains alive until the next boundary;
* lifecycle notices occur exactly once in the specified order;
* active session queries are valid only between Entered and WillExit.

## 22.2 Transactional entry

Inject failure at every preparation/commit checkpoint and assert:

* mode remains Exploration;
* path/input/renderer/fluid/streamer activation matches pre-entry state;
* no session/contract survives;
* world/navigation pointers remain valid;
* a later valid battle still starts.

Run the injection matrix for both alternatives. Handcrafted checkpoints include definition
lookup, prefab materialization, owning-grid build, arena renderer/entity allocation,
generated-world visibility capture, presentation activation, and commit. On every failure,
the isolated arena has no registered entity, generated-world visibility and streamer state
match their pre-entry values, and no board/grid survives.

## 22.3 Bush support-cell behavior

Build a tiny world/fixture with:

* solid support at `(x,y,z)`;
* Bush at `support + Y`;
* a biome-specific Bush ID;
* a non-Bush passable voxel;
* a Bush-like ground ID without the tag.

Assert only the tagged body-space voxel triggers. This test must fail if the implementation
checks `Actor::cell` directly.

## 22.4 Interaction precedence

Test portal plus Bush on one arrival resolves only teleport. Add scripted request versus
Bush precedence. Test out-of-plan and interior cells safely produce no wild encounter.
Test a cleared trainer/gym/special request is suppressed before ordinal/RNG consumption for
both world/scripted and named-development launch paths, while repeatable
wild/debug requests remain legal.

## 22.5 Determinism

For fixed world seed, cell, encounter ID, ordinal, and progression tier, assert exact
resolved team, board policy, battle seed, initial state digest, and debug autoplay event-log
digest. Changing only ordinal must change the derived stream without using frame time.
Assert Bush resolution makes exactly the step-04 chance/team `uniformBelow` calls and
records the actual one-or-more raw draws for each, debug skips only the chance call, entry
failure/retry does not reroll the copied `ResolvedEncounter`, and combat RNG
is unchanged by the number of resolution draws because it uses the named child seed.
Golden-test a named launch of a Trainer definition: its identity is exactly
`battle/debug/<trainer-id>/<ordinal>`, its kind remains Trainer, winning clears it, and a
second launch is suppressed before ordinal/RNG consumption.

Add an authored Gym/Special fixture and run it under two world seeds, two return cells, and
two encounter ordinals. The encounter/combat provenance may differ as designed, but the
handcrafted board's canonical cells, traversal graph, deployment zones, presentation AABB,
source digest, and zero board-RNG draw count must be identical. Assert its definition
approach overrides the arrival approach and its builder never calls world planning,
navigation, chunk, fluid-cell, or content-revision test doubles.

## 22.6 Suspension, streaming, and arena rendering

Assert:

* player entity stays processable;
* player streamer stays active;
* fluid simulator is inactive only for the battle and restores its previous state;
* player renderer and hover renderer restore;
* day/night update remains allowed.

For a live-world battle also assert the battle streamer is active only while Battle is
committed, every required chunk remains present with its captured `contentRevision()` fixed,
and unrelated streaming may change `VoxelWorld::revision()` without aborting or changing the
battle state digest.

For a handcrafted battle also assert:

* the battle streamer is never activated and no chunk range is planned/pinned;
* generated-world chunk rendering is hidden only after commit and restored exactly on exit;
* the exploration `VoxelWorld`, player entity, and return cell remain alive and unchanged;
* exactly one isolated arena renderer is registered in the existing engine, and it is
  removed before its grid and before the engine;
* only the authored arena is visible while Battle is committed; no second scene or camera
  exists;
* `terrainIsCurrent()` remains true without consulting either world revision;
* exit and every injected failure restore fluid, render visibility, streamer activation,
  hover, avatar, and camera-facing state exactly.

## 22.7 Repeated lifecycle

Run at least 25 start/autoplay/exit cycles in one context, alternating live-world and
handcrafted definitions, and assert stable contract/entity counts, no retained sessions or
arena grids, no duplicate event delivery, no pending paths, exact render visibility
restoration, and deterministic per-source results.

## 22.8 Destruction

Destroy `GameSceneWidget` during Placement and during AI resolution for both sources. Assert
the session/board/environment lease dies before `VoxelWorld`/game engine, handcrafted arena
entities die before their owning grid, and context pointers return to the same safe state as
the existing lifetime tests.

---

# 23. Visual/manual validation

Run:

```powershell
cmake --build build/test --target PlaygroundTests SparklePlayground
ctest --test-dir build/test --output-on-failure -L playground
./build/test/Playground/SparklePlayground.exe --resource-path Playground/resources --debug-battle debug-battle --battle-autoplay training-aggressive
```

Validate honestly:

1. world remains visible throughout the debug round trip;
2. F7 reports Exploration -> Battle -> Exploration;
3. avatar/hover disappear during Battle and return afterward;
4. water stops changing during Battle while lighting continues;
5. loaded chunk count does not collapse;
6. the actor remains at the encounter cell with no resumed route;
7. two consecutive debug battles complete;
8. walking onto a real biome Bush triggers the same lifecycle;
9. a portal near passable scenery does not also trigger battle.
10. launch one handcrafted Gym/Special fixture and verify the generated world is hidden,
    the isolated authored arena is visible in the same window, F7 reports the handcrafted
    source and no battle streamer, and exit restores the exact exploration view.

This step has no battle overlay or unit visuals. Console trace and F7 diagnostics are the
intended visible checkpoint. Do not claim tactical visuals before steps 13-14.

---

# 24. Documentation requirements

Update implemented-system documentation to describe:

* authoritative GameMode state;
* queued post-engine transitions;
* ModeManager/BattleModeRuntime ownership;
* Actor support-cell versus Bush body-cell semantics;
* centralized interaction precedence;
* the `LiveWorldBattleLease | HandcraftedArenaLease` source branch;
* live-world secondary streaming/revision stamps versus immutable authored-arena ownership;
* why fluid pauses for both sources, why live streaming/daylight persist, and how
  generated-world visibility is captured/restored for an authored arena;
* manual placement versus debug autoplay;
* result staging and the step-18 commit seam;
* teardown order.

Correct any current document that still claims a pre-existing BattleMode or that treats
`explorationActive` as a complete mode system.

---

# 25. Non-goals

Do not implement battle mask rendering, tactical camera controls, player target input,
creature cube views, battle HUD, result UI, taming, Feat evaluation, save files, trainer NPC
spawning, production gym scripts, final content, animation, VFX, audio, or tools.

The minimal isolated arena renderer required to make an already-authored handcrafted board
presentable is in scope. A separate scene/widget/camera, copying live terrain into an arena,
procedural Gym geometry, and a board-authoring tool are not.

Do not add a force-win/force-loss mutation path merely for smoke testing. Debug autoplay
must use placement/AI/common commands and ordinary outcome rules.

---

# 26. Acceptance criteria

This step is complete when:

* mode authority cannot disagree with exploration logic gating;
* portal/Bush/scripted arrival resolves centrally and deterministically;
* Bush detection reads `Actor::cell + Y` and works for every tagged biome Bush;
* all transitions commit only at a safe post-update boundary;
* battle construction rolls back cleanly on every failure;
* production battles wait for manual deployment;
* optional debug autoplay uses only the common placement/command gateway;
* player movement/path/hover stop cleanly at entry;
* live-world player/battle streaming and generated-world rendering persist;
* a handcrafted board is built only from its registry definition/prefab, consumes no board
  RNG or world-plan/chunk/revision input, and renders in an isolated arena inside the same
  widget/camera while generated-world rendering is hidden;
* fluid mutation pauses and restores exactly;
* terminal results queue safe teardown;
* repeated battles and widget destruction leave no stale state;
* required tests and full build pass;
* the debug and real-Bush round trips are manually checked or clearly reported as skipped.

---

# 27. Required handoff report

Report:

1. final mode/lifecycle APIs and ownership;
2. exact GameSceneWidget update/transition order;
3. encounter identity/seed derivation;
4. the Bush support/body-cell implementation;
5. what is paused, hidden, streamed, arena-rendered, and restored for each board source;
6. production manual-placement behavior and debug-autoplay behavior;
7. lifecycle notice ordering;
8. result-staging fields left for step 18;
9. tests/build commands actually run;
10. manual scenarios actually checked and any skipped checks.

Explicitly give step 13 the source-agnostic presentation-cell binding,
`BoardData::toPresentationCell` mapping, and presentation AABB it must use; its picker may
not require `VoxelWorld`. Warn step 14 that unit/object presenters use presentation cells
for both sources and that the exploration player entity cannot be deactivated because it
carries streaming.
