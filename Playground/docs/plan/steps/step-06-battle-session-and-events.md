# Implement the battle session, unit projection, placement, and typed event log

Implement the final headless ownership boundary for one battle: project persistent/encounter
creatures into stable `BattleUnit` values, own them with the tactical board and battle-local
RNG in `BattleContext`, expose mutation only through `BattleSession`, perform deterministic
deployment through the common command gateway, and commit value-owned typed event batches.

This is an implementation task. Add headless code and tests. Do not implement stamina time
advancement, movement/cast resolution, effects, status runtime, AI execution, modes, views,
HUD, persistence, or progression evaluation in this step.

---

# 1. Repository baseline and prerequisites

Implement after steps 01-05. Before editing, read their handoff reports and inspect their
actual APIs rather than copying the declarations below blindly.

The required prior contracts are:

* step 01 supplies `BattleTime`, `BattleId`, `BattleUnitId`, `BattleObjectId`,
  `CreatureInstanceId`, `BattleSide`, `EncounterKind`, `BattleOutcome`, and `BattleRng`;
* step 02 supplies immutable ability/status/object definitions, explicit
  `EffectApplication::applyTo`, separate target filters, durations, and local effect IDs;
* step 03 supplies immutable Feat/taming condition definitions that will later consume the
  event vocabulary established here;
* step 04 supplies `CreatureUnit`, `CreatureAttributes`, `DerivedCreatureState`, six-slot
  `PlayerRoster`, species/AI/encounter definitions, and encounter spawn records;
* step 05 supplies board-local support cells, `BoardData`, `BoardSourceDescriptor`,
  `BoardOccupancy`, `DeploymentLayout`, deterministic weighted queries, live-terrain
  staleness detection, and immutable owned handcrafted boards.

There is no battle runtime in the current Playground baseline. Do not restore deleted code
or port Unity's pointer-owned `BattleContext`, static `EventCenter` reporting, derived
`WildBattleUnit`, observable resources, service locator, or MonoBehaviour presenters.

The Unity draft demonstrates why this step needs stronger contracts: it identifies units by
object identity, emits global events while state is mid-mutation, lets placement mutate the
board directly, and has separate `HasLeftBattle`/defeat checks. Playground uses stable IDs,
one active predicate, and append-then-publish batches.

---

# 2. Required final behavior

At the end of this step, a headless test can:

1. Create a complete battle session transactionally from a live-world or handcrafted
   `BoardData`, a player roster, and an encounter team.
2. Observe player units assigned IDs first in team-slot order, then enemy units in encounter
   spawn order.
3. Inspect battle-local copies/projections without mutating `CreatureUnit` or definitions.
4. Auto-place enemies using an authored fixed/by-line/seeded-random policy.
5. Manually place/rearrange every player unit with value commands and confirm deployment.
6. Reject invalid commands without changing state, counters, RNG, or event log.
7. Receive one committed typed event batch for each accepted command.
8. Copy a deterministic `BattleSnapshot` and event range, including explicit board-source
   provenance, without any component/window.
9. Prove all events contain stable values and remain valid after source temporaries die.

The visible checkpoint is a small headless trace listing stable unit IDs, placements, and
event sequence numbers. It is not a rendered battle yet.

---

# 3. Final ownership boundary

Use this responsibility split:

```text
immutable Registries
    definitions addressed by semantic string ID

persistent PlayerData / CreatureUnit
    instance ID + species ID + Feat progress only

BattleUnit
    one encounter-local mutable projection; no rendering

BattleContext
    owns board, units, RNG, phase state, counters, log, later statuses/objects

BattleSession
    sole public mutation facade, validation/resolution guard, snapshots/batches

presentation / AI / tests
    submit value commands and read copied snapshots/events
```

`BattleContext` is not a global and is not stored in `GameContext`. The eventual
`BattleMode` owns one `BattleSession`. The session borrows immutable registries. A live board
also borrows the world through `WorldCellSource`, so registries/world outlive it; a
handcrafted board instead owns its immutable grid and has no world lifetime dependency.

Do not expose a public mutable `BattleContext &context()`. Provide a const view/snapshot for
queries and keep mutating helpers private to the session/gateway/resolvers. Tests must use
commands too; a narrowly named fixture builder may seed preconditions before publication.

---

# 4. Runtime sequence IDs

Add distinct checked value types, not raw array positions:

```cpp
struct BattleActionId
{
    std::uint64_t value = 0; // 0 invalid; accepted commands start at 1
    auto operator<=>(const BattleActionId &) const = default;
};

struct BattleBatchId
{
    std::uint64_t value = 0; // committed batches start at 1
    auto operator<=>(const BattleBatchId &) const = default;
};

struct BattleEventSequence
{
    std::uint64_t value = 0; // committed events start at 1
    auto operator<=>(const BattleEventSequence &) const = default;
};

struct TurnIndex
{
    std::uint64_t value = 0; // activations start at 1 in step 07
    auto operator<=>(const TurnIndex &) const = default;
};
```

Equivalent reusable strong wrappers are acceptable. They are scoped to one `BattleId` and
are never persistent save identifiers.

Allocate an action ID only **after** complete command validation succeeds. Allocate batch
and event sequences only while committing the successful command/timeline batch. Check
overflow before mutation. A rejected/no-op command leaves every next counter unchanged.

Reserve representable terminal-abort capacity from construction onward:

```text
normal batch commits always leave at least 1 unused BattleBatchId
normal event commits always leave at least 3 contiguous BattleEventSequence values
normal action/object/status/shield/turn IDs never allocate the numeric maximum of their
respective underlying type
```

The three event values cover the maximum abort tail:
`ActivationEnded{TechnicalAbort}` when an activation is open, then `BattleAborted`, then
`BattleEnded{Aborted}`. An abort without an active turn uses only the latter two. The abort
uses the current next contiguous IDs—it does not jump to numeric maximum—and then marks all
ordinary allocation exhausted.

Every construction, command, timeline, and derived-system transaction resolves in scratch
first, where its exact staged event count and instance-ID demand are known. Before publishing
any scratch state, preflight those counts with checked wide arithmetic and require the abort
reserve to remain even when the ordinary transaction would itself end the battle. A capacity
failure discards the complete scratch transaction and opens the reserved no-action
`TechnicalAbort{InternalInvariantViolation}` batch. This rule also covers overflow of later
object/status/shield IDs and `TurnIndex`; none may partially allocate its maximum value.
Because there is no mid-battle load/resume, the only way the reserve can already be absent is
memory corruption/programmer violation; assert/fail fast at that impossible outer boundary
rather than emitting a malformed or non-contiguous log.

`TurnIndex` is declared now because every event header has an optional turn, but step 07
assigns its first non-zero value.

---

# 5. Battle-unit state

Use one homogeneous value type; do not derive player, enemy, wild, or AI unit subclasses:

```cpp
enum class RemovalReason
{
    None,
    Defeated,
    Impressed
};

struct NextActivationPenalty
{
    int actionPoints = 0;
    int movementPoints = 0;
};

class BattleUnit
{
private:
    BattleUnitId _id;
    BattleSide _side;
    std::uint32_t _rosterOrder = 0;
    std::optional<CreatureInstanceId> _persistentCreatureId;
    std::optional<std::string> _encounterSpawnMemberId;
    std::string _speciesId;
    std::string _formId;
    std::optional<std::string> _aiBehaviourId;

    CreatureAttributes _baselineAttributes;
    CreatureAttributes _effectiveAttributes;
    std::vector<std::string> _abilityIds;
    std::vector<std::string> _passiveStatusIds;
    std::vector<std::string> _inheritedCompletedFeatNodeIds; // enemy recruit provenance

    int _health = 0;
    int _actionPoints = 0;
    int _movementPoints = 0;
    NextActivationPenalty _nextActivationPenalty;
    BattleTime _turnBarFill{};

    bool _placed = false;
    std::optional<BoardCell> _lastOccupiedCell;
    RemovalReason _removalReason = RemovalReason::None;

    // Step 10 adds status/shield instance containers here; step 16 may add a tracker.

public:
    [[nodiscard]] bool isActiveCombatant() const noexcept;
};
```

Adapt the fields to the exact step-04 `DerivedCreatureState` names. Preserve these rules:

* a player unit stores its stable persistent creature ID; an encounter-spawned enemy does
  not fabricate one;
* an enemy stores the exact non-empty `EncounterSpawnDefinition.id` from which it was
  projected; a player has no encounter-spawn member ID;
* both sides store species/form/ability/passive semantic IDs, not definition pointers;
* enemy inherited completed Feat-node IDs are copied in authored order from the exact spawn
  member; player values are empty because their persistent progress is addressed by stable
  creature ID and snapshotted in step 17;
* `baselineAttributes` is the derived state at battle entry;
* `effectiveAttributes` initially equals baseline and is recalculated from active
  status/passive modifiers in step 10;
* current health begins at effective maximum health;
* AP/MP begin at zero and are authoritatively refilled at activation start in step 07;
* turn-bar fill begins at zero;
* next-activation penalties begin at zero;
* no level or random stat scaling occurs;
* no battle-local HP/AP/MP/status/position is copied back to `CreatureUnit`;
* no `BattleUnit` owns or mutates Feat progress.

The one active predicate is exact and must be used by scheduler, targeting, AI, taming, and
outcome code:

```text
placed
and current health > 0
and removalReason == None
```

An undeployed unit, defeated unit, or impressed unit is not active. `Impressed` is not
defeat and must never grant kill credit. When a unit leaves occupancy, preserve its last
support cell for events/result presentation.

Revive is a deliberate non-goal of this series and step 02 has no revive effect. Do not keep
a half-working revive path that lacks deterministic legal re-placement.

---

# 6. Projection inputs and validation

Do not make `BattleSession` replay Feat rewards independently from step 04/17. Accept or
build one value seed per actual participant using the shared derivation service:

```cpp
struct BattleParticipantSeed
{
    BattleSide side;
    std::uint32_t rosterOrder = 0;
    std::optional<CreatureInstanceId> persistentCreatureId;
    std::optional<std::string> encounterSpawnMemberId;
    std::string speciesId;
    std::string formId;
    CreatureAttributes attributes;
    std::vector<std::string> abilityIds;
    std::vector<std::string> passiveStatusIds;
    std::optional<std::string> aiBehaviourId;
    std::vector<std::string> inheritedCompletedFeatNodeIds; // enemy/result provenance
};
```

Equivalent use of `DerivedCreatureState` by value is preferred if step 04 already provides
it. Session creation validates all seeds before publishing anything:

* 1-6 actual units per side;
* player roster order is the original team slot and unique;
* enemy roster order is encounter spawn-array order and unique;
* player persistent IDs are present and unique and their spawn-member IDs are absent;
  enemies have no persistent ID and have a non-empty spawn-member ID equal to the selected
  encounter team's member at `rosterOrder`;
* enemy spawn-member IDs are unique within the resolved team and remain attached to the
  projected `BattleUnit` and every later participant/result snapshot;
* species, form, abilities, passives, and enemy AI IDs resolve;
* every ability/passive is in the supplied derived state and capacity limits hold;
* attributes obey step-04 bounds and stamina is at least the game-rule minimum;
* no passive carries the reserved `stun` tag;
* the board carries a valid source descriptor and it exactly equals the descriptor copied
  into the battle construction request;
* that descriptor also matches the resolved encounter board alternative: `liveWorld` maps
  to `{LiveWorld,nullopt}`, while `handcrafted` maps to
  `{Handcrafted,resolved.board.definitionId}`;
* Wild/Trainer sessions use `LiveWorld`, Gym/Special sessions use `Handcrafted`, and Debug
  may use either, matching step 04's already validated encounter alternative;
* the board deployment layout can seat both sides;
* encounter kind/taming metadata is internally consistent (step 16 consumes it later).

Build all seeds, IDs, and enemy placement plans in locals. The remaining construction path is
source-agnostic: it consumes the supplied immutable `BoardData` interfaces and never branches
into world generation, prefab loading, or grid stamping. On any error, return a structured
`BattleConstructionError` and publish no session, event, RNG state, or board occupancy.

---

# 7. Unit-ID allocation and ordered storage

Allocate valid `BattleUnitId` values starting at 1 in this exact order:

1. non-empty player team slots in increasing slot/roster order;
2. enemy spawns in increasing encounter roster order.

Skipping an empty team slot does not allocate an ID, but the unit retains the original slot
as its roster order. Do not derive IDs from species, pointers, or `CreatureInstanceId`.

Store authoritative unit iteration in two ordered vectors of IDs plus one lookup map:

```cpp
std::vector<BattleUnitId> _playerOrder;
std::vector<BattleUnitId> _enemyOrder;
std::map<BattleUnitId, BattleUnit> _units;
```

An equivalent index-stable vector plus ordered lookup is acceptable. All externally visible
unit traversals use side first (Player, Enemy), roster order, then unit ID. Never expose
unordered-map order.

`BattleObjectId` allocation remains owned by `BattleContext` and starts at 1 when step 10
places the first object. Do not share the unit allocator even though both wrappers contain a
`uint32_t`.

---

# 8. Battle context and construction descriptor

Use values equivalent to:

The descriptor/enums may be declared here, but the complete `BattleContext` declaration in
the real headers must come after section 13's complete `BattleEventLog`: `_events` owns that
type by value. The narrative presents ownership before the event catalog for readability;
it is not permission to paste the class in an uncompilable order or replace the value with a
pointer merely to tolerate an incomplete type.

```cpp
enum class BattlePhase
{
    Deployment,
    AwaitingActivation,
    Activation,
    Terminal
};

enum class BattleAbortReason
{
    RequiredTerrainChanged,
    SchedulerNoFutureProgress,
    TimelineBoundaryMadeNoProgress,
    NumericInvariant,
    EffectChainDepthExceeded,
    InternalInvariantViolation
};

struct BattleDescriptor
{
    BattleId battleId;
    std::string stableEncounterIdentity;
    std::string encounterDefinitionId;
    std::string encounterDisplayName;
    EncounterKind encounterKind;
    bool allowsTaming = false;
    bool repeatable = true;
    BoardSourceDescriptor boardSource;
    std::uint64_t worldSeed = 0;
    std::uint64_t encounterOrdinal = 0;
    std::uint64_t encounterResolutionSeed = 0;
    std::uint64_t combatSeed = 0;
    std::uint32_t resolvedTier = 0;
    std::string resolvedTeamId;
    spk::Vector3Int returnWorldCell{};
};

class BattleContext
{
private:
    BattleDescriptor _descriptor;
    const Registries *_registries = nullptr;
    BoardData _board;
    std::map<BattleUnitId, BattleUnit> _units;
    std::vector<BattleUnitId> _playerOrder;
    std::vector<BattleUnitId> _enemyOrder;
    BattleRng _rng;
    BattlePhase _phase = BattlePhase::Deployment;
    BattleOutcome _outcome = BattleOutcome::Undecided;
    std::optional<BattleAbortReason> _abortReason;
    std::optional<BattleUnitId> _activeUnit;
    BattleTime _elapsed{};
    BattleEventLog _events;
    // checked next-ID/counter fields
};
```

`stableEncounterIdentity`, `encounterDefinitionId`, and `resolvedTeamId` are non-empty.
`boardSource` is a copied value and must exactly equal `BoardData::sourceDescriptor()`; a
handcrafted definition ID therefore survives after the encounter registry lookup and arena
materialization are gone. Step 12 copies world seed, consumed ordinal, both named child
seeds, selected tier/team, board source, and the exact resolved enemy member IDs into this
descriptor before session construction. The session derives `battleId` from the supplied
`combatSeed` with step 01's fixed fold and
constructs `BattleRng(combatSeed)` directly. Validate that a caller-supplied descriptor ID,
if the construction DTO retains one, equals the derived value. `worldSeed` and
`encounterResolutionSeed` are result/replay provenance only and are never alternate combat
RNG paths. Do not derive `combatSeed` again or re-resolve the tier/team.

Treat `BattleDescriptor` as immutable result-facing provenance. Every later terminal result,
transcript/replay construction record, diagnostic trace header, and persistence/result notice
copies `boardSource` from this descriptor; no consumer guesses the source from encounter kind,
world seed, origin, renderer state, or definition lookup. The final result still uses
`returnWorldCell` for exploration return even when the battle board itself was handcrafted.

`BattleContext` checks `board.requireCurrentTerrain()` before every accepted state mutation.
For `LiveWorld`, a required chunk disappearing or changing content revision after entry
transitions once to `BattleOutcome::Aborted` with
`BattleAbortReason::RequiredTerrainChanged`; it never rebuilds navigation. For
`Handcrafted`, the same call is an always-current no-op over the immutable owned grid, so
there is no chunk/staleness abort path and no need for a dummy world. Entry-time terrain or
arena-build failures remain typed `EntryFailed` errors and never create an aborted battle
result.

Do not put `BattleContext` or `BattleUnit` in `spk::Component`. They must construct and run
inside `PlaygroundTests` with no engine/window/GL context.

---

# 9. Final command vocabulary and issuer

Declare the complete value-command variant now so later steps extend behavior without
replacing the gateway:

```cpp
struct PlaceUnitCommand
{
    BattleUnitId unit;
    BoardCell destination;
};

struct ConfirmDeploymentCommand
{
    BattleSide side;
};

struct MoveCommand
{
    BattleUnitId unit;
    BoardCell destination;
};

struct CastAbilityCommand
{
    BattleUnitId unit;
    std::string abilityId;
    BoardCell anchor; // exactly one anchor
};

enum class EndTurnRequestCause
{
    Explicit,
    AiRepeatedStateGuard,
    AiCommandCap,
    AiInvalidPlannedCommand,
    AiNoRuleProducedCommand
};

enum class ActivationEndReason
{
    Explicit,
    NoLegalCommands,
    CommandCap,
    ActiveUnitDefeated,
    ActiveUnitImpressed,
    BattleTerminal,
    TechnicalAbort,
    AiRepeatedStateGuard,
    AiCommandCap,
    AiInvalidPlannedCommand,
    AiNoRuleProducedCommand
};

struct EndTurnCommand
{
    BattleUnitId unit;
    EndTurnRequestCause cause = EndTurnRequestCause::Explicit;
};

using BattleCommand = std::variant<
    PlaceUnitCommand,
    ConfirmDeploymentCommand,
    MoveCommand,
    CastAbilityCommand,
    EndTurnCommand>;

enum class CommandController
{
    System,
    Player,
    EnemyAi,
    DebugAutoplay
};

struct CommandIssuer
{
    CommandController controller;
};
```

`MoveCommand` intentionally has no caller-supplied path, distance, or MP cost. Step 08
recomputes all three authoritatively. `CastAbilityCommand` has one and only one anchor; AoE
cells/targets are resolved internally once.

Controller rules:

* `Player` may issue deployment/activation commands only for Player-side units;
* `EnemyAi` may issue activation commands only for Enemy-side units;
* `DebugAutoplay` may control Player-side units only in an explicitly configured
  development/autoplay session; record it distinctly in replay input;
* `System` is restricted to transactional setup/enemy deployment and internal lifecycle
  transitions; it is not a public UI bypass;
* tests use Player/EnemyAi issuers and the same gateway. A fixture-only setup builder is not
  an alternate action resolver.

Only `Player` may come from normal battle UI, and it may submit only
`EndTurnRequestCause::Explicit`. `EnemyAi` and `DebugAutoplay` may submit `Explicit` or one
of the four `Ai*` safety causes. `System` does not submit a public `EndTurnCommand`; automatic
ends are internal continuations of the already-open batch. Reject an issuer/cause mismatch
before allocating IDs or mutating state. Map an accepted request cause one-to-one to the
same-named `ActivationEndReason`; automatic reasons (`NoLegalCommands`, `CommandCap`,
`ActiveUnitDefeated`, `ActiveUnitImpressed`, `BattleTerminal`, `TechnicalAbort`) are not
representable in a submitted command and therefore cannot be spoofed by UI, AI, or replay.

In this step, move/cast/end-turn commands are rejected with `CommandUnavailable` because no
activation runtime exists yet. Their shapes remain final.

---

# 10. Common command gateway and result

Use one entry point and one closed construction result:

```cpp
enum class BattleConstructionErrorCode
{
    InvalidDescriptor,
    InvalidParticipantCount,
    DuplicateParticipantIdentity,
    InvalidRosterOrder,
    UnknownDefinitionReference,
    InvalidDerivedLoadout,
    BoardSourceMismatch,
    InsufficientDeploymentCapacity,
    EnemyPlacementInvalid,
    NumericOverflow
};

struct BattleConstructionError
{
    BattleConstructionErrorCode code;
    std::string encounterDefinitionId;
    std::optional<BattleSide> side;
    std::optional<std::uint32_t> rosterOrder;
    std::optional<BattleUnitId> unitId;
    std::string referencedId;
    std::string diagnosticDetail;
};

class BattleSession;

struct BattleConstructionResult
{
    std::unique_ptr<BattleSession> session;
    std::optional<BattleConstructionError> error;

    explicit BattleConstructionResult(std::unique_ptr<BattleSession> p_session);
    explicit BattleConstructionResult(BattleConstructionError p_error);
    BattleConstructionResult(const BattleConstructionResult&) = delete;
    BattleConstructionResult& operator=(const BattleConstructionResult&) = delete;
    BattleConstructionResult(BattleConstructionResult&&) noexcept;
    BattleConstructionResult& operator=(BattleConstructionResult&&) noexcept;
    ~BattleConstructionResult(); // define after BattleSession is complete
};
```

Exactly one of `session`/`error` is populated. The result is move-only; define its destructor
out of line after `BattleSession`. Expected data/board failures use the stable code plus
copied context and publish no partial session. Allocation/process-invariant exceptions are
not flattened into a misleading content code.

Do not complete the `BattleSession` class at this point in the header. Declare `EventRange`
and `CommandResult` immediately below; the event catalog, snapshot, and committed-batch
values follow in sections 12-13. Keep only the session forward declaration here, then place
the complete public interface after all of those value types as shown in section 15. This is
a required declaration order, not permission to duplicate the class.

Command results are value-owned and typed:

```cpp
struct EventRange
{
    BattleEventSequence first;          // inclusive
    BattleEventSequence onePastLast;    // exclusive
};

enum class CommandRejection
{
    SessionBusy,
    BattleTerminal,
    WrongPhase,
    WrongController,
    UnknownUnit,
    UnitRemoved,
    UnitAlreadyConfirmed,
    UnitOutsideDeploymentZone,
    DestinationNotStandable,
    DestinationOccupied,
    DeploymentIncomplete,
    CommandUnavailable,
    NoStateChange
    // steps 07-08 append resource/range/path/target errors here
};

struct RejectedCommand
{
    CommandRejection reason;
};

struct AcceptedCommand
{
    BattleActionId actionId;
    BattleBatchId batchId;
    EventRange events;
};

struct AbortedCommand
{
    BattleAbortReason reason;
    BattleBatchId batchId;
    EventRange events;
};

using CommandResult = std::variant<AcceptedCommand, RejectedCommand, AbortedCommand>;
```

Equivalent expected/error organization is acceptable. Stable enum codes drive tests/UI;
human text is diagnostic only.

Validation is pure and complete before mutation. A rejected command:

* changes no unit, board, phase, confirmation flag, or counter;
* advances no RNG draw;
* appends/commits/publishes no event or empty batch;
* does not call a status/object hook;
* returns the same rejection for the same snapshot/command.

`AbortedCommand` is not a rejection. If a live required-terrain check or another closed
technical invariant fails while `submit` is being attempted, roll back/discard any
uncommitted command transaction and scratch IDs, commit one no-action `TechnicalAbort`
batch from the last trustworthy state, append the structural active-turn close when needed
then `BattleAborted`/`BattleEnded{Aborted}`, and return its reason/batch/range. It consumes no
action ID and never reports the requested gameplay command as accepted. Repeating submit
afterward returns ordinary `RejectedCommand{BattleTerminal}` without another abort batch.

This disposition is only for a **pre-commit** command-transaction failure. If the requested
command batch commits successfully and a synchronous post-commit derived consumer introduced
in step 16 later fails while consuming it, the action and source batch are immutable facts:
return `AcceptedCommand` for that source batch, then expose the separately committed
no-action TechnicalAbort tail through `takeCommittedBatches()` and the now-terminal session
outcome. Never relabel or roll back an accepted source action and never use `AbortedCommand`
to carry both an accepted action and a later system failure. Timeline/TamingSystem source
batches have no command result and report the same failure through their pump stop plus the
committed abort batch.

Guard `submit` against re-entry. A callback cannot exist inside headless resolution in this
step, but the guard prevents future presentation/event subscribers from synchronously
submitting while a batch is still being finalized.

---

# 11. Deployment state and commands

Track side confirmation explicitly. Enemy auto-deployment is completed during transactional
session creation; player confirmation remains false.

## 11.1 Player placement

`PlaceUnitCommand` is legal only when:

* phase is `Deployment`;
* issuer controls the unit's side;
* that side is not confirmed;
* unit exists and has `RemovalReason::None`;
* destination is a standable cell in its side's deployment zone;
* destination is not occupied by an opposing/confirmed unit.

Behavior:

* an undeployed unit is placed;
* an already placed unit moves to an empty legal cell;
* selecting another unconfirmed friendly unit's cell atomically swaps the two;
* selecting the unit's current cell is rejected as `NoStateChange`;
* failed placement/swap preserves both unit flags and both occupancy maps.

All non-empty player roster units must be deployed; there are no reserves or mid-battle
swaps in v1.

## 11.2 Confirmation

`ConfirmDeploymentCommand{Player}` succeeds only when every player unit is placed exactly
once inside the player zone and at least one player/enemy unit exists. It prevents further
player placement. Step 07 moves the phase to `AwaitingActivation` once both sides are
confirmed.

Duplicate confirmation is rejected. Confirmation never fills AP/MP or turn bars.

## 11.3 Enemy placement policies

Consume the exact step-04 policy variant. Preserve these algorithms:

* `fixed`: resolve each authored `[x,z]` to the first standable enemy-zone support cell by
  increasing y then `BoardCellLess`; reject a missing column, duplicate resolved cell,
  non-zone cell, or insufficient list before placing any unit;
* `byLine`: validate `rowsFromEnemyEdge` against the actual deployment depth; enumerate
  rows starting at that outer-edge offset and moving toward centre without wrap. Within a
  row use the authored `leftToRight`, `rightToLeft`, or midpoint-distance/smaller-coordinate
  `centerOut` order from step 04, then increasing y/`BoardCellLess` for stacked supports.
  Assign enemy roster order to the first N cells and fail transactionally if insufficient;
* `seededRandom`: first sort every free enemy-zone cell canonically, prove capacity, then
  perform a partial Fisher-Yates selection using `BattleRng::uniformBelow` for each roster
  entry. The same seed/board/team produces the same placements and draw count.

Plan all enemy destinations before mutation. Random draws happen only after all non-random
validation/capacity checks succeed. Apply planned placements through the same internal
command validator/resolver with `CommandIssuer{CommandController::System}`, then confirm Enemy. If any command
unexpectedly fails, discard the entire not-yet-published session.

Do not call `std::shuffle`, `random_device`, or choose from unordered occupancy output.

---

# 12. Typed event model

Define one append-only vocabulary now. Later steps fill its runtime payloads; they must not
create parallel callback-specific event classes.

## 12.1 Header and origin

Use value-owned metadata equivalent to:

```cpp
enum class BattleEventCategory
{
    Gameplay,
    Lifecycle,
    Taming,
    Diagnostic
};

struct BattleEventOrigin
{
    std::optional<BattleUnitId> sourceUnit;
    std::optional<BattleUnitId> targetUnit;
    std::optional<std::string> abilityId;
    std::optional<std::string> effectId;
    std::optional<std::string> statusId;
    std::optional<BattleObjectId> objectId;
};

struct BattleEventHeader
{
    BattleId battleId;
    BattleEventSequence sequence;
    BattleBatchId batchId;
    BattleTime battleTime;
    std::optional<TurnIndex> turn;
    std::optional<BattleActionId> action;
    BattleEventCategory category;
    BattleEventOrigin origin;
};

struct BattleEvent
{
    BattleEventHeader header;
    BattleEventPayload payload;
};
```

All strings/IDs/cells/numbers are copied values. No payload contains `BattleUnit*`,
`AbilityDefinition*`, a component/entity/widget, `std::reference_wrapper`, span into a
temporary command plan, or a callback.

## 12.2 Required gameplay payloads

Define the closed per-transition cause once here:

```cpp
enum class SpatialMoveCause { Voluntary, Displacement, Teleport, Swap };
```

Declare cohesive payload structs with exactly the semantic noun names below—no `Event`
suffix. `BattleEvent` is the sole header-plus-payload wrapper. Names may be mechanically
adapted only if the prerequisite code already established an equivalent convention;
meanings may not change.

```text
BattleStarted
    descriptor IDs/kind, copied BoardSourceDescriptor, seed-derived battle ID,
    ordered participant IDs

UnitDeploymentChanged
    unit, previous optional cell, new cell, swap partner optional
DeploymentConfirmed
    side, ordered placed unit IDs
DeploymentCompleted
    both sides' ordered placed unit IDs; emitted once when Placement closes

BattleTimeAdvanced
    previous time, delta, new time
ActivationStarted
    unit, turn index, AP/MP after refill+penalty,
    optional closest ally/enemy Manhattan distance
ActivationEnded
    unit, turn index, optional final cell/resources,
    optional closest ally/enemy Manhattan distance, closed ActivationEndReason

ResourceSpent
    unit, actionPoints/movementPoints resource, requested, applied,
    before, after, reason (abilityCost|movementCost)
ResourceChanged
    unit, resource, requestedDelta, appliedDelta, before, after,
    reason (effect|activationRefill|nextActivationPenaltyConsumption|
            effectiveMaximumClamp)
NextActivationPenaltyApplied
    unit, resource, requested, applied, accumulatedBefore, accumulatedAfter

UnitMovementStep
    unit, from, to, entered-cell MP cost, SpatialMoveCause;
    presentation/trap/topology-replay ordering only
UnitMoved
    unit, command-start/command-end cells, actual entered-cell count, total MP spent,
    voluntary=true; exactly one aggregate event per accepted voluntary move
UnitDisplaced
    source optional, target, toward/away direction, requestedDistance, appliedDistance
UnitTeleported
    unit, from, to, Manhattan distance
UnitsSwapped
    first/second IDs and both before/after cells

AbilityCast
    source, ability ID, source cell, exactly one anchor cell,
    targetDistance as source-to-anchor X/Z Manhattan distance,
    canonical captured affectedCells and deduplicated affectedUnits;
    its header.action is the BattleActionId used as the ability-condition window ID

Damage
    source optional, target, ability/effect IDs optional, damage kind,
    computedDamage, appliedToShield, appliedToHealth,
    targetHadAnyShield before the hit, targetHadMatchingShield before the hit,
    HP before/after
Healing
    source optional, target, ability/effect IDs optional,
    requestedHealing, appliedHealing, HP before/after
ShieldApplied
    immediate applier as source optional, target, shield ID, kind, requested/applied amount,
    duration, source ability/effect IDs optional
ShieldAbsorbed
    immediate incoming-damage source optional, target, shield ID, kind, requested/applied
    amount, original shield applier as shieldAppliedBy optional, shield source ability/effect
    IDs optional
ShieldBroken
    immediate incoming-damage source optional, target, shield ID, kind, original shield
    applier as shieldAppliedBy optional, shield source ability/effect IDs optional
ShieldRemoved
    target, shield ID, kind, closed non-depletion reason, remaining amount, original shield
    applier as shieldAppliedBy optional, shield source ability/effect IDs optional

StatusApplied
    immediate applier as source optional, target, status ID, requested/applied stacks,
    resulting stacks/duration, copied definition tags
StatusRemoved
    immediate cleanser/removing-effect source optional, original applier optional as a
    separate field, target, status ID, requested/removed stacks, remaining stacks,
    removal reason, copied definition tags; expiry/unit cleanup has no immediate source

BattleObjectPlaced / BattleObjectRemoved / BattleObjectTriggered
    object instance/definition IDs, creator/trigger unit optional, cell,
    trigger ID/count/removal reason as applicable

UnitRemoved
    unit, previous cell optional, RemovalReason
UnitDefeated
    unit, killer optional, ability/effect IDs optional, previous cell

BattleEnded
    BattleOutcome, active and not-defeated Player/Enemy counts
BattleAborted
    one closed BattleAbortReason value (no exception text as logic)
```

`BattleAbortReason` is the closed enum in section 8. Use
`RequiredTerrainChanged` only for a pinned required chunk disappearing/changing after a
session exists, `SchedulerNoFutureProgress` and `TimelineBoundaryMadeNoProgress` for the two
step-07 stalls, `NumericInvariant` for checked-arithmetic failure during resolution,
`EffectChainDepthExceeded` for the step-10 resolver guard, and
`InternalInvariantViolation` only when a supposedly prevalidated transaction reaches an
impossible internal state. Construction/command validation errors, a user closing the
window, and ordinary mode teardown are not technical battle outcomes. Every technical abort
appends exactly one Lifecycle-category `BattleAborted`, then exactly one Lifecycle-category
`BattleEnded{outcome=Aborted}`, and stores the same reason in the final result.

Step 15 conditions consume effective/applied values, never an authored request that was
clamped. Keep both where diagnostics need both. Distance is X/Z Manhattan unless a payload
explicitly says weighted path cost.

Only simulation-time Taming diagnostics plus their core removal/outcome facts enter this
log, using the same header/category and step-16 batch rules. Feat progression, result,
save/commit, and presentation notifications occur after result finalization; they are
value-owned external domain notices, not `BattleEvent` alternatives, and are never appended
to `BattleEventLog`. There is therefore no `Progression` battle-event category.

---

# 13. Event log and batch commit

Use one append-only owner:

```cpp
enum class BattleBatchKind
{
    Construction,
    Command,
    Timeline,
    TamingSystem,
    TechnicalAbort
};

struct BattleUnitSnapshot
{
    BattleUnitId id;
    BattleSide side;
    std::uint32_t rosterOrder;
    std::optional<CreatureInstanceId> persistentCreatureId;
    std::optional<std::string> encounterSpawnMemberId;
    std::vector<std::string> inheritedCompletedFeatNodeIds;
    std::string speciesId;
    std::string formId;
    int health, maxHealth;
    int actionPoints, maxActionPoints;
    int movementPoints, maxMovementPoints;
    BattleTime stamina, turnBarFill;
    int range;
    bool placed;
    std::optional<BoardCell> cell;
    std::optional<BoardCell> lastOccupiedCell;
    RemovalReason removalReason;
};

struct BattleSnapshot
{
    BattleId battleId;
    BoardSourceDescriptor boardSource;
    BattlePhase phase;
    BattleOutcome outcome;
    std::optional<BattleAbortReason> abortReason;
    BattleTime elapsed;
    std::optional<TurnIndex> turn;
    std::optional<BattleUnitId> activeUnit;
    std::vector<BattleUnitSnapshot> units; // Player roster then Enemy roster
    // Step 10 adds status/shield/object snapshots.
};

struct CommittedBattleBatch
{
    BattleBatchId id;
    BattleBatchKind kind;
    std::optional<BattleActionId> action;
    EventRange events;
    BattleSnapshot before;
    BattleSnapshot after;
};

class BattleEventLog
{
public:
    [[nodiscard]] std::span<const BattleEvent> events() const noexcept;
    [[nodiscard]] std::vector<BattleEvent> copy(EventRange) const;
};
```

Only a private `BattleEventWriter` owned by `BattleSession` may append. `kind` is explicit
authoritative metadata: never infer it from nullable action IDs or the contained event
categories. For one accepted command:

```text
validate completely
capture before snapshot
allocate action ID
open Command batch ID
resolve state and stage/append ordered events
finalize all command-owned transitions
capture after snapshot
commit non-empty batch descriptor
make batch available to the outer presentation/condition pump
return AcceptedCommand
```

A timeline transition in step 07 follows the same process in a `Timeline` batch with no
action ID. Initial construction commits one `Construction` lifecycle batch containing
`BattleStarted` and any system placement events. Every accepted public command uses
`Command` and has the same non-null action ID in its descriptor and event headers. Step 16
alone adds `TamingSystem`, which has no action ID and may intentionally contain Taming,
Gameplay, and Lifecycle event categories. Copy the kind through transcript, replay, and
diagnostics; never reconstruct it from payloads.

Every closed technical-abort path uses `TechnicalAbort`, has no action ID, and contains the
canonical closure/abort/end sequence. A failed command transaction never converts its
partially staged `Command` batch into this kind; discard it and open a fresh batch.

Do not call `EventCenter`, `spk::ContractProvider::trigger`, widgets, or Feat/taming services
from `BattleEventLog::append`. `ContractProvider` suppresses same-provider reentrant
triggers. The safe integration is explicit log/queue first, completed batch second, external
publication afterward.

Keep two distinct structures:

* an append-only authoritative committed-batch archive, retained through result
  finalization, containing each ID/kind/action/event range and immutable before/after
  simulation snapshot;
* a drainable presentation-publication queue/cursor containing indices or copies of those
  already archived descriptors.

`takeCommittedBatches()` drains only the second structure and returns descriptors in order;
it never erases or changes the authoritative archive. The future `BattleMode` triggers
presentation events only after the session is no longer resolving. If a subscriber causes
another command to be queued, it is processed after the current provider callback returns.
Result transcripts and replay inspect the archive, never reconstruct batch boundaries from
events and never depend on whether presentation already drained its queue.

Batch/event order is an observable deterministic contract. No empty batch is committed.
Later post-resolution consumers may respond to one immutable committed batch only by
opening a **new** no-action system batch whose `before` equals the prior `after`; they may
never extend a committed descriptor. Step 16 uses that seam before external publication.
Even if the prior batch chose a terminal outcome, commands/time remain rejected while the
internal derived-batch queue quiesces; result/log freezing occurs only afterward.

---

# 14. Event ordering for this step

Lock deployment event order:

```text
new placement:
    UnitDeploymentChanged(unit, null -> destination)

move within zone:
    UnitDeploymentChanged(unit, old -> destination)

friendly swap:
    UnitDeploymentChanged(lower BattleUnitId, old -> new, partner)
    UnitDeploymentChanged(higher BattleUnitId, old -> new, partner)

confirmation:
    DeploymentConfirmed(side, unit IDs in roster order)
```

The state mutation is complete before its event is appended, so a snapshot after each event
would agree with it. Swaps mutate atomically, then emit the pair ordered by unit ID.

Initial construction batch order is:

1. `BattleStarted` with descriptor IDs, board source, and all participant IDs;
2. Enemy system placement events in enemy roster order;
3. Enemy `DeploymentConfirmed`.

Player commands each form their own batch. When player confirmation later makes both sides
ready, step 07 appends the phase transition/timeline events to the same confirmation batch
or an immediately following no-action timeline batch; choose one there and golden-test it.

---

# 15. Snapshots and deterministic state digest

Use the exact copied/value `BattleUnitSnapshot` and `BattleSnapshot` declared in section 13
before `CommittedBattleBatch`; that declaration order is required because each batch owns
its before/after snapshots by value. These views serve tests, AI, conditions, and
presentation.

With every public command/result/event/batch/snapshot value now complete, declare the one
session facade:

```cpp
class BattleSession
{
public:
    [[nodiscard]] static BattleConstructionResult create(...);

    [[nodiscard]] CommandResult submit(
        const BattleCommand &p_command,
        CommandIssuer p_issuer);

    [[nodiscard]] BattleSnapshot snapshot() const;
    [[nodiscard]] std::vector<BattleEvent> copyEvents(EventRange p_range) const;
    [[nodiscard]] std::vector<CommittedBattleBatch> takeCommittedBatches();
};
```

Now define `BattleConstructionResult`'s two alternative constructors, move operations, and
destructor out of line, after `BattleSession` is complete. Each alternative constructor
populates exactly its named value and rejects a null success pointer; the declarations above
keep ownership move-only and unambiguous without instantiating `unique_ptr` deletion against
an incomplete `BattleSession`.

`boardSource` is copied from the immutable descriptor and must equal the owned board's source
descriptor on every snapshot. `cell` is present exactly while `placed` is true.
`lastOccupiedCell` is absent until the
unit has occupied a battle cell, equals `cell` while placed, and retains the final occupied
cell after removal. Snapshot construction must validate occupancy/unit-position agreement,
that invariant, abort-reason/outcome agreement, and player-versus-enemy provenance
nullability, then produce stable order. A copied snapshot remains valid after later commands
and session destruction.

Add the deterministic `gameplayProgressDigest` (the material-state digest) for tests/AI
guards. Hash canonical scalar bytes/strings with the existing stable deterministic hash
helper. Include `BoardSourceKind`, optional handcrafted definition ID with an explicit
presence marker, immutable board extent/topology identity, phase, outcome, elapsed,
active/turn, units, occupancy, resources, and later statuses/objects/RNG draw count.
The topology identity canonically folds extent and traversal bounds; every bounded source
voxel's stable semantic definition/state/orientation/flip in X/Y/Z order; sorted navigation
nodes/edges with movement costs; both ordered deployment zones; and sorted border cells.
For `LiveWorld`, additionally fold the authoritative `worldAnchor` and the complete
`LiveBoardTerrainStamp` as its coordinate-sorted required chunk coordinates and captured
`contentRevision` values. For `Handcrafted`, fold explicit absence of both values; its
definition ID above still distinguishes two authored arenas that share geometry. Never hash
`presentationOrigin` as an independent renderer transform, a source/world/grid pointer, the
stamp's borrowed world pointer/lifetime token, or any *current* live revision read after
construction. The live presentation origin currently equals `worldAnchor`, but encode that
value exactly once under its authoritative terrain-provenance meaning. This lets equivalent
reconstructed boards compare by value while preserving every captured fact that can change
the result of `terrainIsCurrent()`.
Exclude event/action/batch counters, log length, presentation state, diagnostic traces, and
memory addresses. A zero-cost no-op cast in step 11 must not look like gameplay progress
merely because it appended an action event.

---

# 16. Removal and defeat seam

Implement one internal removal primitive now, even though damage arrives in step 09:

```cpp
void BattleContext::removeUnit(
    BattleUnitId p_unit,
    RemovalReason p_reason,
    const BattleEventOrigin &p_origin);
```

It must:

* do nothing/reject if the same terminal reason was already applied;
* preserve the previous cell as `lastOccupiedCell`;
* set the reason and remove board occupancy exactly once before emitting removal facts;
* append `UnitDefeated` first only for `Defeated`, with killer/source metadata (step 16
  analogously appends `UnitImpressed`);
* run the step-10 owner-cleanup seam in stable order: transient statuses, shields, then only
  owner-activation-duration objects, with removal events but every removal hook suppressed;
* append `UnitRemoved` last with the same batch/action/turn header context;
* if this was the active actor, mark one pending removed-actor structural close and discard
  its owner-duration capture, but do not necessarily append `ActivationEnded` or clear turn
  ownership inside this primitive. The enclosing action owns the stable boundary: a cast
  closes after every captured effect and after-cast seam; a voluntary move closes after its
  aggregate/ended-move seams. Only a removal caller with no continuing enclosing action
  (Taming derived batch, outcome-only system boundary, or abort closure) may close
  immediately after `UnitRemoved`;
* never append defeat/kill credit for `Impressed`.

Before step 10 there are no cleanup instances, so the two removal facts may be adjacent.
After step 10, only the whitelisted cleanup payloads for that same owner may occur between
the reason-specific fact and `UnitRemoved`; suppressed hooks prevent nested removal
sequences. Timeline/infinite objects may survive their creator by retaining snapshotted
creator side; only owner-activation-duration objects require and follow their owner.

The pending close stores unit, `TurnIndex`, and exact
`ActiveUnitDefeated|ActiveUnitImpressed` reason and is consumed exactly once. Same-action
events continue carrying the open TurnIndex until that close, so the step-15 turn window
contains the complete final action. Consuming it uses the common structural-close seam:
append the reasoned event, reset only that removed owner's turn-bar fill, clear ownership,
and never refill AP/MP or decrement discarded owner durations. A terminal outcome cannot leave the marker/open turn
behind: the action boundary consumes the removed-owner close first, then evaluates/appends
the terminal sequence without a second `ActivationEnded`.

The public session does not expose an arbitrary remove command. Step 09 invokes this after a
pending HP-zero transition; step 16 invokes it through `TamingService` after a committed
condition batch.

---

# 17. Error handling and invariants

Session construction errors are expected values and must identify the descriptor, side,
roster entry/unit ID, and violated definition/board contract. Runtime invariant failures
such as stale required live terrain or counter overflow use the later aborted path; they do
not continue with partially trustworthy rules.

After every accepted debug/test command, assert in debug builds:

* unit IDs and roster orders are unique;
* ordered side lists and lookup storage contain the same units;
* placed flags, unit cells, and BoardOccupancy forward/reverse maps agree;
* descriptor, board, current snapshot, and `BattleStarted` board-source values agree;
* active predicate agrees with placement/HP/removal state;
* confirmed deployment contains every side unit exactly once in its zone;
* event, batch, and action sequences are strictly contiguous;
* snapshots contain no duplicate ID;
* current resources stay inside their current effective bounds;
* no definition or persistent creature was mutated.

Do not catch invariant errors and convert them to a successful empty command.

---

# 18. Required tests

Add headless tests under `Playground/tests/battle/` and extend the board fixture.

## 18.1 Projection and construction

Test:

* player IDs precede enemy IDs and start at 1;
* empty player team slots preserve roster-order gaps without allocating IDs;
* two same-species creatures remain distinct by persistent/battle IDs;
* enemy units have no fabricated persistent ID;
* baseline/effective attributes and loadouts match `DerivedCreatureState` exactly;
* HP starts full, AP/MP/bar/penalties start zero;
* no level/random scaling;
* source `CreatureUnit`, definitions, and Feat state remain byte/value unchanged;
* duplicate/missing IDs, bad references, invalid stats, over-capacity, passive stun, and
  insufficient zones fail transactionally;
* invalid or mismatched board-source descriptor and illegal encounter-kind/source pair fail
  transactionally;
* the same valid projection/deployment construction succeeds on equivalent live-world and
  handcrafted boards without reading source-specific internals;
* handcrafted construction/session use no `VoxelWorld` and a terrain-current check cannot
  manufacture `RequiredTerrainChanged`;
* a failed construction leaves the input BoardData/source reusable or clearly moved only on
  success according to the chosen result API.

## 18.2 Stable active/removal semantics

Test undeployed, active, defeated, and impressed combinations. Assert `isActiveCombatant`
is the only truth table and impressed removal clears occupancy without a defeat event.

## 18.3 Enemy deployment

Test fixed, by-line, and seeded-random policies; exact by-line order; fixed-cell validation;
same seed identical placement/draw count; different seed can diverge; random capacity
failure consumes zero draws and publishes nothing; enemy placement events follow roster
order; enemy confirmation is automatic.

## 18.4 Player placement commands

Test placement, reposition, friendly swap, all four zone orientations, wrong side,
nonstandable, occupied enemy, current-cell no-op, confirmed-side mutation, incomplete
confirmation, successful all-unit confirmation, and duplicate confirmation.

For every rejection snapshot/digest all mutable state, action/batch/event counters, log
size, and RNG draw count before/after and assert exact equality.

## 18.5 Event values/order/lifetime

Golden-test the complete initial batch and several player command batches. Assert:

* action/batch/event IDs start at 1 and are contiguous;
* rejected commands create no gaps;
* headers carry battle/time/action/category/origin correctly and `BattleStarted` copies the
  exact board source;
* event ranges are inclusive/exclusive as documented;
* swapped events are ID ordered;
* copied events remain valid after commands/session destruction;
* payload strings are copies, not views into setup temporaries;
* taking committed batches returns each batch once in commit order;
* no `ContractProvider` is triggered from resolution.

Add fixture seams that initialize each checked counter near its maximum. Prove action,
batch, event, object, status, shield, and turn exhaustion rolls back the attempted ordinary
transaction and consumes only the reserved no-action TechnicalAbort batch. Cover an open
activation (three-event tail) and no active unit (two-event tail). Prove rejected commands
and failed ordinary reserve preflights leave gameplay, RNG, and ordinary counters untouched,
and no normal transaction consumes the one-batch/three-event reserve.

## 18.6 Snapshot/digest

Test canonical unit order, occupancy consistency, old snapshot immutability, and exact
descriptor/board/snapshot source agreement for both source kinds. Prove digest equality for
equivalent construction orders and independently rebuilt equivalent handcrafted boards;
digest inequality when source kind or handcrafted definition ID changes even if geometry is
identical; digest change for each material mutation; and digest stability when only
diagnostic/log counters differ.

## 18.7 Headless trace

Build a flat owned-grid/handcrafted fixture, create a seeded two-versus-two session,
auto-place enemies, manually place/confirm players, then compare the exact textual trace
including board-source provenance against a golden value. Repeat the construction/placement
portion on an equivalent live fixture to prove the session path is source-agnostic. The trace
is a test diagnostic, not a runtime logging dependency.

---

# 19. Expected file changes

Add cohesive files equivalent to:

```text
Playground/srcs/battle/battle_unit.hpp
Playground/srcs/battle/battle_unit.cpp
Playground/srcs/battle/battle_context.hpp
Playground/srcs/battle/battle_context.cpp
Playground/srcs/battle/battle_session.hpp
Playground/srcs/battle/battle_session.cpp
Playground/srcs/battle/battle_command.hpp
Playground/srcs/battle/battle_command_result.hpp
Playground/srcs/battle/battle_event.hpp
Playground/srcs/battle/battle_event_log.hpp
Playground/srcs/battle/battle_event_log.cpp
Playground/srcs/battle/battle_snapshot.hpp
Playground/srcs/battle/battle_snapshot.cpp
Playground/srcs/battle/battle_state_digest.hpp
Playground/srcs/battle/battle_state_digest.cpp
Playground/srcs/battle/placement/placement_resolver.hpp
Playground/srcs/battle/placement/placement_resolver.cpp

Playground/tests/battle/battle_construction_test.cpp
Playground/tests/battle/battle_unit_test.cpp
Playground/tests/battle/battle_placement_command_test.cpp
Playground/tests/battle/battle_event_log_test.cpp
Playground/tests/battle/battle_snapshot_test.cpp
Playground/tests/battle/battle_session_trace_test.cpp
```

Modify test fixture/CMake integration and implemented-system documentation. Equivalent
cohesive grouping is acceptable. Avoid one-class-per-trivial-event `.cpp` files.

Do not modify Sparkle engine code, `GameSceneWidget`, modes, rendering, resources, or JSON
schemas in this step unless a prerequisite handoff explicitly changed.

---

# 20. Documentation requirements

Document:

* definition/persistent/session ownership boundaries;
* source-agnostic construction and live-world versus owned-handcrafted board lifetime;
* board-source propagation through descriptor, start event, snapshot, digest, and future
  result/replay contracts;
* projection and unit ID allocation order;
* support-cell placement and active predicate;
* final command shapes and single gateway;
* enemy placement algorithms/RNG draw policy;
* event header/payload/batch vocabulary;
* append/commit/queue-before-publish integration;
* snapshot and material-digest inclusion rules;
* why impressed removal is not defeat;
* runtime state deliberately not copied to `CreatureUnit`.

Mark scheduler/effect/event emissions not yet exercised as step-07-10 contracts, not
implemented behavior.

---

# 21. Non-goals

Do not implement time advancement, AP/MP refill, movement, targeting, ability resolution,
damage/healing, shields, status/passive hooks, battle objects/traps, AI execution, outcome
commit, taming evaluation, Feat evaluation, mode switching, world pin integration, entities,
overlays, input, HUD, result screen, saving, or authoring tools.

Do not add reserves, battle swapping, summons, multi-cell units, revive, fleeing, items,
accuracy, critical hits, elemental types, or mid-battle persistence.

---

# 22. Acceptance criteria

This step is complete when:

* one `BattleSession` owns all mutable state for one battle;
* the same construction/gateway path accepts live-world and handcrafted `BoardData` without
  source-specific battle rules;
* `BoardSourceDescriptor` is copied consistently into descriptor/events/snapshots/digest and
  retained for later result/replay provenance;
* unit projection uses step-04 derived data without mutating persistent state;
* stable unit IDs follow exact side/roster allocation order;
* active-combatant/removal semantics are locked for defeat and impression;
* `BattleContext` cannot be mutated by UI/tests outside the command path;
* final command values include destination-only move and one-anchor cast;
* invalid commands mutate nothing, emit nothing, allocate nothing, and draw no RNG;
* fixed/by-line/random enemy placement is transactional and deterministic;
* manual player deployment/rearrangement/confirmation works for up to six units;
* the typed event vocabulary is value-owned and append-only;
* every accepted command commits one ordered before/after batch;
* no `ContractProvider` is invoked from inside resolution;
* snapshots/digests are stable and headless;
* live terrain drift can abort, while immutable handcrafted terrain is always current and
  has no world/chunk dependency;
* all new and existing tests pass and both Playground targets build.

---

# 23. Required handoff report

Report the exact BattleUnit/Context/Session/command/event/snapshot APIs, complete board-source
propagation and staleness behavior, projection and ID orders, enemy placement policy
algorithms and RNG draw counts, placement/event golden traces, files changed, any repository
drift, and commands/tests actually run. Explicitly state that scheduler, movement/casting,
effects/statuses/AI, external event publication, and rendered battle mode remain
unimplemented.
