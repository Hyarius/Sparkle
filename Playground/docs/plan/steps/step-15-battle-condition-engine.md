# Implement the shared battle-condition engine

Implement one deterministic, headless evaluator for the structured conditions authored in
Feat Board requirements and inline species taming profiles. It must consume only committed
typed battle batches, use explicit ability/turn/fight/game window identities, support live
incremental evaluation and post-battle replay with identical results, retain serializable
advancement, and express every Feat/taming example in the GDD without a special-case script
or service callback.

This is an implementation task. Complete the condition-definition semantics where the GDD
exposes missing fields, audit the event vocabulary from steps 06-10, add the evaluator and
golden tests, update documentation, and keep the complete build green. Do not implement the
taming state machine or apply Feat rewards here; steps 16 and 17 consume this engine.

---

# 1. Repository baseline

Implement after steps 01-14. Reinspect the exact prerequisite types and mechanically adapt
names, but do not create a second battle log, snapshot, condition schema, or amount type.

Step 03 supplies a strict closed `ConditionSpec` variant with:

* relative `ConditionUnitSet` values `subject`, `subjectAllies`, `subjectTeam`,
  `opponentTeam`, and `any`;
* event roles `source`, `target`, and `either`;
* windows `ability`, `turn`, `fight`, and `game`;
* `requiredWindowCount` and `windowMode` `cumulative|consecutive`;
* aggregations `count|sum|maximum|minimum`;
* comparisons `atLeast|greaterThan|atMost|lessThan|equal|between`;
* leaf variants `damage`, `healing`, `abilityCast`, `status`, `shield`,
  `movement`, `resource`, `position`, `unitRemoval`, `battleOutcome`,
  `survivorCount`, and `eventAbsence`;
* recursive `allOf` and `anyOf` with stable semantic IDs.

Step 06 supplies:

* `BattleId`, `BattleActionId`, `BattleBatchId`, `BattleEventSequence`,
  `TurnIndex`, and fixed-millisecond `BattleTime`;
* value-owned `BattleEvent{header, payload}` in one append-only log;
* `CommittedBattleBatch{id, action, events, before, after}`;
* a copied `BattleSnapshot` before/after every committed batch;
* typed effective damage/healing/shield/status/resource/movement/removal/outcome payloads;
* rejected commands that allocate no action/batch/event IDs and append nothing.

Steps 07-10 establish exact resolution ordering:

* `ActivationStarted`/`ActivationEnded` and one `TurnIndex` per activation;
* one accepted `CastAbilityCommand` with exactly one anchor and one action ID;
* the full synchronous cast/effect/status/passive/trap/defeat chain is committed before its
  batch is published;
* effective applied amounts are recorded after mitigation/clamping;
* voluntary movement has ordered `UnitMovementStep` events and one aggregate `UnitMoved`;
* no presentation callback is an authoritative event.

Step 14 consumes the same batches for cosmetics. The condition evaluator must be a sibling
headless consumer, never read a cube, widget, overlay, or formatted HUD string.

---

# 2. Required final behavior

After this step:

1. Every leaf is evaluated relative to one bound subject and stable subject/opponent sides.
2. Every eligible ability, turn, fight, and game window has an explicit typed identity.
3. A leaf can qualify at most once per eligible window.
4. Incomplete ability/turn/fight metrics never bleed into the next window.
5. Game metrics retain checked partial accumulation across battles for Feat progress.
6. `cumulative` counts qualifying windows; `consecutive` resets on the first failed eligible
   window using precisely defined window-owner filtering.
7. Amount conditions consume effective/applied values, never requested or authored values.
8. One multi-hit cast can satisfy “total in one ability” with `sum`, while “one hit” uses
   `maximum`.
9. “Same ability three times” uses per-ability buckets, not adjacent event guesses.
10. Position evaluation uses event-time board state and catches intermediate movement
    steps, displacement, teleport, swaps, defeat, and removal.
11. An event-absence turn can qualify even though it contains zero matching events.
12. Fight-end predicates use the terminal after-snapshot and distinguish defeated from
    impressed/removed units.
13. `allOf`/`anyOf` recursively retain child progress by stable ID.
14. Live batch-by-batch evaluation and replay of the archived batches produce byte/value
    equivalent advancement.
15. Duplicate batch delivery is a no-op; a gap, foreign battle, malformed boundary, or
    overflow returns a typed error without partial mutation.
16. Rejected commands, previews, AI candidate enumeration, animation, and diagnostic events
    cannot advance a condition.
17. Advancement is value-owned and serializable; it contains no definition/event/snapshot
    pointer.
18. All GDD examples are represented by ordinary strict JSON and pass golden tests.
19. A pure candidate-set prospective fight-close query breaks terminal-profile circularity
    for one or several wilds without
    fabricating an event or mutating/closing live advancement.

---

# 3. Verify the three final-purpose schema seams before runtime implementation

Step 03 already closes the three gaps required here: explicit window ownership, proof that
an ability affected a required unit set, and an invariant held throughout a finite window.
Before implementing evaluation, verify that its parser, examples, strict-field tests, and
concrete payload types contain the exact fields below. If repository drift removed one,
restore the settled step-03 contract rather than adding an evaluator-only side table or a
second schema.

## 3.1 Required windowActor on every leaf

Use the settled `ConditionLeafHeader`:

```cpp
struct ConditionLeafHeader
{
    ConditionWindow window;
    ConditionUnitSet windowActor;
    std::uint32_t requiredWindowCount = 1;
    WindowCountMode windowMode = WindowCountMode::Cumulative;
};
```

`windowActor` is required in every leaf JSON:

* `ability`: an eligible window is a committed cast whose `AbilityCast.source` belongs to
  this set;
* `turn`: an eligible window is an activation whose owner belongs to this set;
* `fight`: the only accepted value is explicitly `any`;
* `game`: the only accepted value is explicitly `any`.

Do not infer ownership from the first matching event. Absence conditions need eligible
zero-match windows, and target-role conditions may observe an actor other than the caster.
Requiring the field makes those semantics authored and inspectable.

Composites have no window and therefore no `windowActor`.

## 3.2 AbilityCast affected-unit filter

Use the required `AbilityCastConditionSpec` fields:

```cpp
ConditionUnitSet affected;
std::uint32_t minimumAffectedUnits = 0;
```

Semantics:

* deduplicate the event's captured `affectedUnits` by stable ID;
* resolve each ID against the bound relative unit sets;
* require at least `minimumAffectedUnits` members of `affected`;
* `affected:any` plus zero disables the filter;
* non-`any` with zero is rejected as meaningless;
* the upper bound is the configured maximum participants, not an arbitrary unbounded count.

This records a successful targeting result, not nonzero damage. A shielded/immune target
may still have been hit by an ability. Content that requires effective damage uses
`DamageConditionSpec` instead.

Without these fields, “cast at distance” could count an empty ground cast and “get hit by N
attacks” would count damage effects rather than distinct attacks.

## 3.3 throughoutWindow position sample

Use the `ThroughoutWindow`/`throughoutWindow` `PositionSample` alternative declared in step
03. It is valid with ability, turn, or fight and
invalid with game, whose lifetime window has no meaningful close.

For one eligible window:

1. sample at window open;
2. sample after every event-time position or active-membership change;
3. sample once at close;
4. compare every comparable sample;
5. one failed sample irreversibly fails that window;
6. a state with no active actor/relative pair is neutral, but at least one comparable
   sample is required to qualify.

At each sample, test every active member of `actor` against its closest active member of
`relativeTo` using X/Z Manhattan distance. This catches an actor that briefly walks too
close and then moves away before the aggregate command ends.

The ordinary `afterMove` sample still tests only the unit that completed a voluntary move.
`turnStart`/`turnEnd` test the eligible activation owner. For these discrete samples, the
window qualifies if at least one required sample passes; `throughoutWindow` alone uses the
all-samples invariant.

---

# 4. Explicit condition-window identities

Do not identify windows by vector position, “events since last cast,” current UI phase, or
adjacent log records. Define typed keys:

```cpp
struct AbilityConditionWindowId
{
    BattleId battle;
    BattleActionId action;
    auto operator<=>(const AbilityConditionWindowId&) const = default;
};

struct TurnConditionWindowId
{
    BattleId battle;
    TurnIndex turn;
    auto operator<=>(const TurnConditionWindowId&) const = default;
};

struct FightConditionWindowId
{
    BattleId battle;
    auto operator<=>(const FightConditionWindowId&) const = default;
};

struct FeatGameWindowId
{
    CreatureInstanceId subject;
    auto operator<=>(const FeatGameWindowId&) const = default;
};

struct TamingGameWindowId
{
    BattleId battle;
    BattleUnitId wildSubject;
    auto operator<=>(const TamingGameWindowId&) const = default;
};

using ConditionWindowId = std::variant<
    AbilityConditionWindowId,
    TurnConditionWindowId,
    FightConditionWindowId,
    FeatGameWindowId,
    TamingGameWindowId>;
```

Equivalent strong types are acceptable. Raw `uint64_t` is not.

## 4.1 Ability window

An ability window is exactly one successfully committed `CastAbilityCommand` action batch:

* ID is `{BattleId, BattleActionId}`;
* the batch must contain exactly one matching `AbilityCast` payload;
* the event source must satisfy `windowActor`;
* the window sees the complete committed batch, including synchronous effects, reactions,
  traps, displacement, defeat, and automatic activation end;
* resource events that precede `AbilityCast` in that same accepted cast batch are still in
  the ability window;
* a later no-action timeline/status batch is not part of it;
* movement/end-turn/placement action IDs are not ability windows;
* a rejected cast has no action ID/window.

The committed batch boundary is the explicit open/close boundary. Do not add synthetic
`WindowOpened` events to the battle log.

## 4.2 Turn window

A turn window is one activation:

* ID is `{BattleId, TurnIndex}`;
* open on `ActivationStarted` for an owner matching `windowActor`;
* include subsequent events bearing that TurnIndex;
* close on the matching `ActivationEnded`, including automatic end in a cast/move batch;
* active-owner removal does not close the window at `UnitRemoved`: the enclosing command
  retains the TurnIndex through every remaining captured-cast or movement-aggregate event
  and appends the one matching `ActivationEnded` only at that command's stable boundary;
* refill/resource events emitted before `ActivationStarted` are outside the semantic turn
  window even if the header already carries the assigned TurnIndex;
* a unit may act multiple times in one fight, producing distinct IDs;
* enemy/player phases are not separate window types.

Require one open and one close, matching owner/index, with no overlap. A terminal outcome
must not leave an open activation.

## 4.3 Fight window

A fight window:

* is keyed by `BattleId`;
* opens at committed `BattleStarted`;
* includes deployment and every committed gameplay/timeline batch;
* closes at one `BattleEnded`;
* never qualifies on `BattleAborted`;
* uses the terminal after-snapshot for outcome/survivor predicates.

For `requiredWindowCount > 1`, distinct terminal battles are distinct qualifying windows.

## 4.4 Game window

Game is a logical persistence window, not a BattleSession lifetime:

* Feat domain key is the persistent creature subject and carries across terminal battles,
  wins and losses;
* taming domain key is the current battle plus wild subject and resets for every encounter;
* `requiredWindowCount` must be one and `windowMode` must be cumulative;
* metric qualification may occur immediately after any committed batch;
* an aborted battle contributes nothing to persistent Feat progress;
* `eventAbsence` and `throughoutWindow` are invalid with game because no finite close can
  prove the invariant.

The condition ID keys advancement within its owning node/profile; it need not be duplicated
inside the window ID.

---

# 5. Subject binding and relative unit sets

Evaluation receives a binding, not absolute player/enemy literals:

```cpp
enum class ConditionDomain
{
    Feat,
    Taming
};

struct ConditionSubjectBinding
{
    ConditionDomain domain;
    BattleUnitId subject;
    BattleSide subjectSide;
    BattleSide opponentSide;
    std::variant<FeatGameWindowId, TamingGameWindowId> gameWindow;
};
```

Validate the subject exists in the battle participant identity table even if it is later
defeated or impressed. Side membership is immutable for the fight.

Resolve sets:

```text
subject        exactly bound BattleUnitId
subjectAllies  same side, excluding subject
subjectTeam    same side, including subject
opponentTeam   opposite side
any            every present stable battle-unit identity
```

Removal does not change which relative set an identity belongs to. “Active” is a separate
state predicate.

For source/target payloads:

* role `source`: actor tests source; counterpart tests target;
* role `target`: actor tests target; counterpart tests source;
* role `either`: test source/target orientation, then target/source orientation;
* one self-source/target event can contribute at most once;
* a missing optional source cannot match `subject`/team sets; `any` means any present unit,
  not a fabricated null actor;
* if `counterpart` is omitted by a payload spec, do not invent a constraint.

Use the same resolver for Feat and taming. For a player Feat, subjectTeam is Player. For a
wild taming tracker, subjectTeam is Enemy and opponentTeam is Player.

---

# 6. Batch input and event-time replay state

Define an immutable call input:

```cpp
struct ConditionBatchView
{
    const CommittedBattleBatch& batch;
    std::span<const BattleEvent> events;
};
```

Validate before scoring:

* event range exactly matches the descriptor;
* sequences are contiguous and all headers use the same battle/batch ID;
* action header agrees with the batch action;
* `before`/`after` use the same battle;
* the evaluator cursor expects the first sequence;
* events are already committed and the session is not resolving.

Batch before/after snapshots alone cannot reconstruct the position at an intermediate
movement step. Create a lightweight value-owned `ConditionReplayState` from `before`:

```cpp
struct ConditionReplayUnit
{
    BattleUnitId id;
    BattleSide side;
    std::optional<BoardCell> cell;
    std::optional<BoardCell> lastOccupiedCell;
    std::int64_t health;
    RemovalReason removalReason;
};
```

Apply relevant events in exact sequence:

* an unpaired `UnitDeploymentChanged` changes one optional cell immediately; when its
  `swapPartner` is present, buffer the first event, require the adjacent reciprocal partner
  event, validate both old/new cells, apply both cell changes atomically, and take one
  semantic topology sample after the pair;
* each non-Swap `UnitMovementStep` changes its unit's `cell` and `lastOccupiedCell` to the
  destination and takes a semantic topology sample; this covers Voluntary, Displacement,
  and Teleport;
* `UnitsSwapped` is the one aggregate topology exception: validate both current source
  cells, apply both destinations atomically, and take one semantic topology sample; buffer
  and validate the following exact two Swap-cause `UnitMovementStep` details in unit-ID
  order without moving or sampling either unit again;
* aggregate `UnitMoved`, `UnitDisplaced`, and `UnitTeleported` verify their recorded final
  cells/distances against `cell` when the mover is still placed or `lastOccupiedCell` when a
  trigger removed it after entry, but do not move a unit a second time;
* Damage/Healing update health from recorded before/after values;
* defeat/removal copies the previous `cell` to `lastOccupiedCell`, clears `cell`, and changes
  active membership/reason;
* other events leave replay topology unchanged.

After the span, validate this lightweight state against relevant fields of `after`. A
mismatch is an event-contract error, not an excuse to guess from the final snapshot.

Use this replay state for discrete position samples and `throughoutWindow`. In particular,
sample after every non-Swap `UnitMovementStep` so passing through a forbidden adjacent cell
fails the invariant even if the final destination is far away. Atomic deployment and battle
swaps each produce exactly one post-pair sample; no impossible intermediate double-occupied
topology is observable. A missing/misordered/non-reciprocal pair is an event-contract error.

---

# 7. Serializable advancement and transient evaluation state

Separate durable progress from an open battle cursor. Both are plain values.

An equivalent durable model:

```cpp
struct AbilityBucketAdvancement
{
    std::string abilityId;
    std::int64_t value = 0;
};

struct MetricAdvancement
{
    bool hasValue = false;
    std::int64_t value = 0;
    std::vector<AbilityBucketAdvancement> buckets; // sorted by ability ID
};

struct LeafConditionAdvancement
{
    bool completed = false;
    std::uint32_t qualifyingWindowCount = 0;
    std::uint32_t consecutiveWindowCount = 0;
    MetricAdvancement persistentGameMetric;
};

struct ConditionAdvancement;

struct CompositeConditionAdvancement
{
    bool completed = false;
    std::vector<ConditionAdvancement> children; // definition order, keyed by child ID
};

struct ConditionAdvancement
{
    std::string conditionId;
    std::variant<LeafConditionAdvancement, CompositeConditionAdvancement> value;
};
```

Step 04 already established the flat save-facing `PersistentConditionAdvancement`. Define
one canonical adapter rather than introducing a second persistence shape:

```cpp
std::vector<PersistentConditionAdvancement> flattenPersistentAdvancement(
    const ConditionSpec& definition,
    const ConditionAdvancement& advancement);

ConditionAdvancement inflatePersistentAdvancement(
    const ConditionSpec& definition,
    std::span<const PersistentConditionAdvancement> flat);
```

Traverse root first, then composite children recursively in authored definition order. Emit
exactly one flat entry for the root and every nested child. For a leaf, map completion,
qualifying/consecutive window counts, optional game metric, and sorted ability buckets
losslessly. For a composite, persist only completion; its counters are zero and metric/
buckets are empty. Inflation first creates the recursive shape from the immutable
definition, indexes the flat input by stable condition ID, then rebuilds current authored
order. Require the exact unique ID set and matching leaf/composite shape, but accept an older
array order: definition reordering must not lose progress. Reject duplicates, missing/extra
IDs, shape changes, a non-game leaf with a game metric/buckets, illegal cross-battle
ability/turn transient counts, overflow, or a stored composite completion inconsistent with
its reconstructed allOf/anyOf children.

`sanitize` runs before flattening at a battle result boundary: it retains completed state,
game accumulators, and permitted incomplete fight counts/streaks, while clearing the
documented transient ability/turn/fight-open values. Save JSON writes this flat pre-order
array; load inflates it. Golden-test `inflate(flatten(x)) == sanitized(x)` for nested allOf/
anyOf trees, successful inflation from a permuted unique flat array, and exact failure
diagnostics for every malformed ID set/shape.

Add direct accessors/snapshots for current value, target, successful windows, streak, and
completion so steps 16-18 do not inspect the variant ad hoc.

Transient live/replay state is equivalent to:

```cpp
struct OpenConditionWindow
{
    std::string conditionId;
    ConditionWindowId id;
    MetricAdvancement metric;
    bool sawComparableSample = false;
    bool invariantFailed = false;
    bool absencePredicateMatched = false;
};

struct ConditionEvaluationState
{
    ConditionAdvancement advancement;
    std::optional<BattleId> currentBattle;
    BattleEventSequence nextExpectedSequence;
    std::vector<OpenConditionWindow> openWindows;
};

enum class ConditionEvaluationErrorCode
{
    DefinitionProgressMismatch,
    UnknownSubject,
    ForeignBattle,
    DuplicateOrOverlappingRange,
    EventSequenceGap,
    BatchEnvelopeMismatch,
    WindowBoundaryMismatch,
    ReplayStateMismatch,
    MissingTerminalEvent,
    AbortedTranscript,
    ArithmeticOverflow,
    LimitExceeded
};

struct ConditionEvaluationError
{
    ConditionEvaluationErrorCode code;
    std::string conditionId;
    std::optional<ConditionWindowId> windowId;
    std::optional<BattleId> battleId;
    std::optional<BattleBatchId> batchId;
    std::optional<BattleEventSequence> sequence;
    std::string payloadKind;
};

struct ConditionEvaluationSuccess { ConditionEvaluationState state; };
struct ConditionReplaySuccess { ConditionAdvancement advancement; };
struct ConditionPreviewSuccess { ConditionEvaluationState hypotheticalState; };

using ConditionEvaluationResult = std::variant<
    ConditionEvaluationSuccess,
    ConditionEvaluationError>;
using ConditionReplayResult = std::variant<
    ConditionReplaySuccess,
    ConditionEvaluationError>;
using ConditionPreviewResult = std::variant<
    ConditionPreviewSuccess,
    ConditionEvaluationError>;
```

No field contains a pointer/span/reference. Sorted vectors are preferable to unordered
maps so serialization and diagnostics are stable. The condition ID distinguishes several
composite children observing the same ability/turn/fight ID; keep at most one open entry
per `(conditionId, windowId)`.

Persistence policy:

* completed state persists;
* completed conditions persist regardless of their authored window;
* incomplete ability/turn qualifying-window counts and streaks reset at fight close;
* incomplete fight qualifying-window count and fight-consecutive streak persist across
  distinct terminal battles, so `requiredWindowCount:2` can mean two fights;
* open ability/turn/fight metric state is discarded after its close;
* only the game metric accumulator crosses a battle boundary;
* taming keeps everything battle-local and discards it on encounter end;
* a completed condition freezes; later events cannot change its report.

Do not store a numeric/encounter-local `BattleId` as a durable replay stamp. The evaluator
is a pure prepare operation: the same initial advancement plus transcript yields the same
result. Step 18's stable consumed-result transaction token owns apply-once idempotency
across save/reload. Feeding one terminal transcript a second time as if it were a new result
is a caller error and is prevented at that boundary, not by polluting every condition value
with an incomplete battle ledger.

All additions and counters are checked before commit. On overflow, return an error and
leave the caller's state byte/value equivalent to its input.

---

# 8. Evaluator API and transactional algorithm

Create one service. Forward-declare `ProspectiveFightClose` and
`BattleConditionTranscript` before this declaration; their complete value shapes are locked
in sections 8.1 and 12:

```cpp
struct ProspectiveFightClose;
struct BattleConditionTranscript;

class BattleConditionEvaluator
{
public:
    [[nodiscard]] static ConditionEvaluationResult beginBattle(
        const ConditionSpec& definition,
        const ConditionSubjectBinding& subject,
        const ConditionAdvancement& initial,
        const ConditionBatchView& battleStartedBatch);

    [[nodiscard]] static ConditionEvaluationResult evaluateBatch(
        const ConditionSpec& definition,
        const ConditionSubjectBinding& subject,
        const ConditionEvaluationState& current,
        const ConditionBatchView& batch);

    [[nodiscard]] static ConditionEvaluationResult finishBattle(
        const ConditionSpec& definition,
        const ConditionSubjectBinding& subject,
        const ConditionEvaluationState& current,
        const ConditionBatchView& terminalBatch);

    [[nodiscard]] static ConditionReplayResult replay(
        const ConditionSpec& definition,
        const ConditionSubjectBinding& subject,
        const ConditionAdvancement& initial,
        const BattleConditionTranscript& transcript);

    [[nodiscard]] static ConditionPreviewResult previewFightClose(
        const ConditionSpec& definition,
        const ConditionSubjectBinding& subject,
        const ConditionEvaluationState& current,
        const ProspectiveFightClose& prospective);
};
```

`beginBattle` requires a `ConditionBatchView`, not a bare descriptor, so it can validate and
consume the actual `BattleStarted` payload while establishing the sequence cursor. The exact
split may otherwise differ, but it must support both:

```text
live:
    state = begin(...)
    for each newly committed batch:
        state = evaluateBatch(copy of old state, batch)
    inspect completion after each batch

replay:
    create same initial state
    call the same batch evaluator in transcript order
    finish at BattleEnded
    extract sanitized durable advancement
```

Do not implement separate “fast post-battle” scoring rules.

## 8.1 Non-mutating prospective fight close

A taming profile can require a fight-end fact such as survivor count while impressing the
last active wild would itself cause `BattleEnded`. Waiting for the real terminal event is
circular. Supply one narrow pure query:

```cpp
struct ProspectiveFightClose
{
    BattleSnapshot beforeTransaction;
    BattleSnapshot afterTransaction;
    BattleOutcome prospectiveOutcome;
    std::vector<BattleUnitId> impressedCandidates; // canonical enemy-roster order
    std::vector<BattleEvent> scratchEvents; // exact uncommitted Taming-system sequence
};
```

The caller builds `afterTransaction` and `scratchEvents` with the exact normal Taming
removal/active-actor closure/outcome rules on a full scratch transaction. Candidate
impressions begin with unchanged positive HP and remain notDefeated, but captured-duration
state is discarded—not ticked—when closing an impressed active actor. The exact cleanup and
turn-close facts still occur before outcome. The query is legal only when:

* the candidate vector is non-empty, duplicate-free, and in enemy-roster order;
* the bound subject appears in the candidate vector;
* the actual fight window is open;
* all remaining incomplete leaves are fight-close dependent;
* executing the complete candidate scratch transaction makes ordinary outcome terminal;
* the scratch sequence has canonical Taming-system order, closes any activation, ends with
  the one prospective `BattleEnded`, and its replay matches `afterTransaction`;
* `beforeTransaction` otherwise matches current authoritative state.

On a temporary condition-state copy, consume the already validated scratch terminal facts
through the exact same internal fight-close routine used for a real `BattleEnded`, including
outcome, survivor, invariant-close, and composite semantics. The query:

* appends no scratch event/batch to the live log;
* advances no event cursor or live window;
* consumes no RNG;
* returns the would-complete advancement only as diagnostics;
* leaves input state byte/value unchanged on pass, failure, or error.

Step 16 calls this query for each candidate against one shared scratch transaction and may
commit normal impressed removals only when the candidate set reaches this deterministic
fixed point:

```text
C = roster-ordered Tracking wilds ready except for fight-close leaves
loop:
    start a fresh scratch transaction from the authoritative base
    prospectively remove C and run exact active-impressed/structural-terminal closure
    if C is empty or the scratch outcome is not terminal: commit none; stop
    P = members of C whose profiles preview complete against that same close
    if P == C: atomically stage C; stop
    C = P                              # strict subset; bounded convergence
```

On a stable non-empty set, apply impressed removals in roster order in one internal
transaction/batch, then evaluate the real ordinary outcome. Do not accept input or run
outcome selection between members. If the set shrinks to non-terminal or empty, commit
none and leave every actual window open. A candidate dropped in this pass is not re-added;
new committed gameplay may trigger a fresh calculation.

For each call:

1. validate definition/state IDs and the complete batch envelope;
2. copy the current evaluation state;
3. establish/close eligible windows from explicit IDs;
4. build replay state from `before`;
5. visit events in sequence, updating topology and each open metric;
6. close windows at exact boundaries and test them once;
7. recursively refresh composites in authored order;
8. validate replay state against `after`;
9. advance the cursor;
10. return the copy plus a value-owned delta.

If any step fails, return a stable error code/context and no new state.

Do not throw from a normal nonqualifying window. Definition errors should already be
startup validation failures.

---

# 9. Window qualification, counts, and streaks

A metric window begins empty:

* `count` = 0 but `hasValue` becomes true only on a qualifying event;
* `sum` = 0 with the same nonempty rule;
* `maximum/minimum` are undefined until the first qualifying event;
* a normal empty metric window fails;
* event absence is the explicit exception: an eligible empty window qualifies.

At close:

1. derive the metric result;
2. apply the authored comparison;
3. produce one boolean qualified/not-qualified;
4. update counts once.

For `cumulative`:

* qualified increments `qualifyingWindowCount` checked;
* failure leaves it unchanged;
* completion is count >= `requiredWindowCount`.

For `consecutive`:

* qualified increments `consecutiveWindowCount`;
* failure resets it to zero;
* completion is streak >= `requiredWindowCount`;
* report total qualifying windows separately only if useful for diagnostics; it must not
  substitute for the streak.

Between is inclusive at both ends. `atLeast/atMost/equal` are exact integer comparisons.
No epsilon/floating percentage exists.

For a game metric, update/check after each qualifying event/batch because the logical
window does not close during normal play. It completes once its comparison is true and then
freezes.

---

# 10. Effective-value rules by leaf

Implement a closed visitor over every step-03 alternative. Unknown types are impossible
after parsing; do not use a map of string callbacks.

## 10.1 Damage

Match `Damage` after source/target/team, kind, source-ability, and pre-hit-shield
filters.

Selected amount:

```text
component health -> appliedToHealth
component shield -> appliedToShield
component total  -> checked(appliedToHealth + appliedToShield)
```

An amount of zero is not a qualifying event. `count` counts matched positive effective
damage events. `sum/max/min` use the selected effective amount.

Read JSON `targetHadShield` from the step-09
`Damage::targetHadAnyShield` pre-hit fact. Keep the separate
`targetHadMatchingShield` for mitigation diagnostics; it cannot substitute for “any
positive shield before this hit.” Never inspect the after-snapshot to guess; the first hit
may break the shield before a second hit.

Each `Damage` is one resolved hit/effect application. Therefore:

* `maximum > 80` in an ability window means one hit exceeded 80;
* `sum >= 80` means the cast's combined matched hits reached 80.

## 10.2 Healing

Use `appliedHealing` after max-HP clamp. Overheal contributes zero and does not increment a
count. Source is the immediate healer. `sum` across game supports the healer Feat.

## 10.3 AbilityCast

Visit each successful `AbilityCast` once:

* source must match `actor`;
* ability ID filter must match;
* source-to-anchor X/Z Manhattan `targetDistance` must be within inclusive min/max;
* deduplicated affected IDs must satisfy `affected/minimumAffectedUnits`.

`sameAbility:false` increments one count. `sameAbility:true` increments the bucket for that
ability and compares the greatest bucket; break reporting ties by semantic ability ID.
Buckets are sorted and use checked values.

Do not count Damage/Healing child events as casts. A multi-effect cast is one cast.

## 10.4 Status

Use effective `StatusApplied`/`StatusRemoved` stack deltas and copied normalized definition
tags.

Lock source meaning:

* on apply, source is the immediate applier;
* on removal, source is the immediate cleanser/removing effect owner;
* expiry, death cleanup, dispel-without-owner, and battle cleanup use no source;
* original applier, if retained for status behavior, is a different field.

This lets “subject removes Poison from allies” exclude natural expiry. `count` counts one
positive status event; `sum` counts actual stacks changed.

## 10.5 Shield

Use only the matching shield payload family:

* applied -> effective amount placed;
* absorbed -> effective compatible damage absorbed;
* broken -> one event, count only.

Do not also consume `Damage` for a `shield` leaf, which would double count absorption.
`DamageCondition{component:shield}` is a separate authored interpretation.

## 10.6 Movement

Use aggregate events, not both step and aggregate:

* voluntary -> one `UnitMoved`, actual entered-cell count;
* displacement -> applied distance and authored toward/away direction;
* teleport -> actual X/Z Manhattan distance;
* any -> the appropriate one of these.

`UnitMovementStep` is used for event-time position replay/traps only and contributes no
second movement metric. The aggregate still contributes its applied prefix when an enter
trigger removed the mover before the aggregate was appended. Placement is not movement.

## 10.7 Resources

Use applied pool changes:

* `spent` from accepted move/ability costs with normalized reason `movementCost` or
  `abilityCost`;
* `gained/lost` from the effective changed delta with normalized reason `effect`,
  `activationRefill`, `nextActivationPenaltyConsumption`, or `effectiveMaximumClamp`;
* `nextActivationPenalty` from the effective accumulated penalty with normalized reason
  `effect`.

Requested/clamped values do not contribute. Normalize the typed event families to the
closed step-03 reason literals, then require membership in the leaf's non-empty `reasons`
filter. A start-of-turn refill is therefore `gained` only when `activationRefill` is
explicitly authored; do not silently call it spending or let an `effect`-only requirement
count it. Penalty accumulation and later pool consumption are distinct and cannot both
count for one action literal.

## 10.8 Position

Distance is X/Z Manhattan and ignores elevation, matching GDD range rules.

Discrete samples:

* `turnStart`: eligible activation owner at `ActivationStarted`;
* `turnEnd`: owner at `ActivationEnded`;
* `afterMove`: unit at the final aggregate voluntary `UnitMoved` point, only when that mover
  is still active and placed at the aggregate; a removed mover's retained
  `lastOccupiedCell` is available for contract validation but is not a comparable sample;
* no relative active unit means no comparable sample and cannot qualify by itself;
* a window with multiple after-move samples qualifies if at least one passes.

`throughoutWindow` follows section 3.3 and event-time replay. Do not evaluate it only from
batch before/after snapshots.

## 10.9 Unit removal

Count one semantic removal by stable unit ID and reason:

* Defeated satisfies `defeated`;
* Impressed satisfies `impressed`;
* `any` accepts either;
* credited source filtering for Defeated uses the matched `UnitDefeated` killer/source
  metadata; Impressed has no defeating credit and therefore fails any non-`any` credit
  requirement;
* an impressed removal never increments a kill/defeated condition.

The pairing contract is exact: `removeUnit(Defeated)` emits `UnitDefeated`, then zero or more
hook-suppressed owner-cleanup payloads for that same unit in stable status/shield/object-ID
order, then `UnitRemoved{reason=Defeated}`, all in the same battle/batch/action/turn. Hold the
first payload as pending attribution, validate/ignore only that whitelist while pending,
count on the matching final payload, take reason/removed identity from `UnitRemoved`, and
take killer/source credit from `UnitDefeated`. Suppressed hooks forbid a nested removal
sequence. A missing, duplicated, cross-unit, header-mismatched, non-whitelisted, or
out-of-order sequence is an event-contract error and commits no condition state.
`UnitRemoved{Impressed}` uses the analogous preceding `UnitImpressed` plus cleanup sequence
for validation but has no defeat credit; Taming-category `UnitImpressed` itself never scores.
Golden-test cleanup interposition, attribution, and no double count.

Core `UnitRemoved{Impressed}` remains a gameplay-state event. Step-16 domain events such as
`UnitImpressed` use category Taming and are ignored by the evaluator, preventing recursive
condition scoring.

## 10.10 Battle outcome

Valid only in a fight window and only at non-aborted `BattleEnded`. Compare outcome relative
to subject side. `requireSubjectActive` reads terminal `isActiveCombatant`, not HP alone.

## 10.11 Survivor count

Valid only in a fight window and terminal after-snapshot:

* `active` means placed, HP > 0, removal reason None;
* `notDefeated` includes Impressed units retained in identity/result records;
* select units by relative set;
* compare exact count once.

## 10.12 Event absence

Open an explicit eligible window even when it will contain no matches. Mark
`absencePredicateMatched` when a committed payload matches the closed predicate visitor.
At close, qualify iff it remains false.

For a damage predicate:

* component health matches only `appliedToHealth > 0`;
* component shield only positive absorption;
* total either positive;
* a physical hit fully absorbed by shield therefore does not break “no physical HP
  damage.”

Support only the parsed predicate alternatives damage, abilityCast, status, and movement.
Do not add general negation or nested thresholds.

---

# 11. Composition

`allOf` and `anyOf` have no independent window/metric.

Initialize one child advancement per definition child in authored order and validate stable
IDs. For each committed batch:

1. visit incomplete children in authored order through the same evaluator;
2. commit all child results together;
3. `allOf` completes when every child is complete;
4. `anyOf` completes when at least one child is complete;
5. once the composite completes, freeze all child state.

Do not flatten children into one percentage or let one child's open window share an
accumulator with another. Recursion depth remains bounded by the parsed game rule.

---

# 12. Event-category and transcript policy

The authoritative input is the battle log plus committed batch descriptors.

Condition-relevant categories/payloads:

* Gameplay payloads from steps 06-10;
* Lifecycle `BattleStarted`, activation boundaries, `BattleEnded`, and `BattleAborted`;
* core `UnitRemoved` state even when reason is Impressed.

Ignore:

* Diagnostic;
* presentation/cosmetic notifications (never in the log);
* Taming domain progress/impressed notices;
* Progression/Feat notices;
* result/save notices.

Never feed an evaluator-generated progress delta back into the same evaluator.

Copy from step 06's permanent authoritative batch archive for post-battle replay; never use
or depend on the drainable `takeCommittedBatches()` publication queue:

```cpp
struct BattleConditionTranscript
{
    BattleId battle;
    std::vector<CommittedBattleBatch> batches;
    std::vector<BattleEvent> events;
};
```

Equivalent shared immutable storage is acceptable. It must:

* preserve exact batch/event order and before/after snapshots;
* remain valid through step-17 result preparation;
* contain no pointers into a destroyed session;
* reject checked container/sequence-size overflow; v1 deliberately has no production
  max-battle-event/activation/time rule beyond the per-activation/no-progress guards, so do
  not reference a nonexistent game-rules field;
* exclude uncommitted/aborted partial writer state.

Presentation draining must not destroy the only replay metadata.

---

# 13. Every GDD example as strict JSON

Add parser/evaluator fixtures for each example. IDs/numbers below are illustrative; fields
and semantics are normative.

## 13.1 Hit an enemy with named spell between Y and Z cells

```json
{
  "id": "long-range-hit",
  "type": "abilityCast",
  "description": "Hit an enemy with ember-arc from 3 to 6 cells.",
  "window": "ability",
  "windowActor": "subject",
  "requiredWindowCount": 1,
  "windowMode": "cumulative",
  "actor": "subject",
  "abilities": ["ember-arc"],
  "sameAbility": false,
  "affected": "opponentTeam",
  "minimumAffectedUnits": 1,
  "minimumTargetDistance": 3,
  "maximumTargetDistance": 6,
  "comparison": "atLeast",
  "threshold": 1
}
```

Distance is source-to-anchor; affected enemy proves it was not an empty cast.

## 13.2 Take total damage while shielded

```json
{
  "id": "shielded-damage",
  "type": "damage",
  "description": "Take 100 total effective damage while shielded.",
  "window": "game",
  "windowActor": "any",
  "requiredWindowCount": 1,
  "windowMode": "cumulative",
  "actor": "subject",
  "role": "target",
  "counterpart": "any",
  "kind": "any",
  "component": "total",
  "targetHadShield": "required",
  "sourceAbilities": [],
  "aggregation": "sum",
  "comparison": "atLeast",
  "threshold": 100
}
```

## 13.3 Remove Poison from allies X times

```json
{
  "id": "cleanse-poison",
  "type": "status",
  "description": "Remove Poison from allies three times.",
  "window": "game",
  "windowActor": "any",
  "requiredWindowCount": 1,
  "windowMode": "cumulative",
  "actor": "subject",
  "role": "source",
  "counterpart": "subjectAllies",
  "action": "removed",
  "statuses": [],
  "tags": ["poison"],
  "aggregation": "count",
  "comparison": "atLeast",
  "threshold": 3
}
```

Natural expiry has no remover source and does not count.

## 13.4 Heal allies for total HP

```json
{
  "id": "heal-allies",
  "type": "healing",
  "description": "Restore 250 effective HP to allies.",
  "window": "game",
  "windowActor": "any",
  "requiredWindowCount": 1,
  "windowMode": "cumulative",
  "actor": "subject",
  "role": "source",
  "counterpart": "subjectAllies",
  "sourceAbilities": [],
  "aggregation": "sum",
  "comparison": "atLeast",
  "threshold": 250
}
```

Overheal is excluded.

## 13.5 Stay away from enemies for X fights

```json
{
  "id": "keep-distance",
  "type": "position",
  "description": "Remain at least four cells from every enemy for three fights.",
  "window": "fight",
  "windowActor": "any",
  "requiredWindowCount": 3,
  "windowMode": "cumulative",
  "sample": "throughoutWindow",
  "actor": "subject",
  "relativeTo": "opponentTeam",
  "comparison": "atLeast",
  "distance": 4
}
```

One intermediate step at distance three fails that fight even if the subject ends far away.
The healer evolution node authors cleanse, healing, and keep-distance as three top-level
requirements; the node service ANDs them in step 17.

## 13.6 Get hit by N attacks

```json
{
  "id": "take-attacks",
  "type": "abilityCast",
  "description": "Be affected by 20 enemy attacks.",
  "window": "game",
  "windowActor": "any",
  "requiredWindowCount": 1,
  "windowMode": "cumulative",
  "actor": "opponentTeam",
  "abilities": [],
  "sameAbility": false,
  "affected": "subject",
  "minimumAffectedUnits": 1,
  "minimumTargetDistance": 0,
  "maximumTargetDistance": 64,
  "comparison": "atLeast",
  "threshold": 20
}
```

One multi-hit ability counts as one attack, not one per `Damage`.

## 13.7 Deal more than 80 magical damage in one hit

For a wild subject, player units are `opponentTeam`:

```json
{
  "id": "big-magic-hit",
  "type": "damage",
  "description": "Deal more than 80 magical HP damage to this wild creature in one hit.",
  "window": "ability",
  "windowActor": "opponentTeam",
  "requiredWindowCount": 1,
  "windowMode": "cumulative",
  "actor": "opponentTeam",
  "role": "source",
  "counterpart": "subject",
  "kind": "magical",
  "component": "health",
  "targetHadShield": "any",
  "sourceAbilities": [],
  "aggregation": "maximum",
  "comparison": "greaterThan",
  "threshold": 80
}
```

Use `sum` instead if authored wording is “80 total in one ability.” Do not conflate them.

## 13.8 Keep at least two allies alive at fight end

```json
{
  "id": "two-allies-active",
  "type": "survivorCount",
  "description": "Keep at least two player creatures active.",
  "window": "fight",
  "windowActor": "any",
  "requiredWindowCount": 1,
  "windowMode": "cumulative",
  "units": "opponentTeam",
  "state": "active",
  "comparison": "atLeast",
  "threshold": 2
}
```

## 13.9 Move a creature adjacent to the wild creature

```json
{
  "id": "approach-wild",
  "type": "position",
  "description": "End a voluntary move adjacent to this wild creature.",
  "window": "turn",
  "windowActor": "opponentTeam",
  "requiredWindowCount": 1,
  "windowMode": "cumulative",
  "sample": "afterMove",
  "actor": "opponentTeam",
  "relativeTo": "subject",
  "comparison": "atMost",
  "distance": 1
}
```

Deployment, push, pull, swap, and teleport do not produce the afterMove sample.

## 13.10 Use the same ability three times in one fight

```json
{
  "id": "repeat-one-ability",
  "type": "abilityCast",
  "description": "Use one ability three times in this fight.",
  "window": "fight",
  "windowActor": "any",
  "requiredWindowCount": 1,
  "windowMode": "cumulative",
  "actor": "opponentTeam",
  "abilities": [],
  "sameAbility": true,
  "affected": "any",
  "minimumAffectedUnits": 0,
  "minimumTargetDistance": 0,
  "maximumTargetDistance": 64,
  "comparison": "atLeast",
  "threshold": 3
}
```

Cast order A, B, A, A qualifies A's bucket. Three arbitrary abilities do not.

## 13.11 Avoid physical HP damage for two consecutive own turns

```json
{
  "id": "avoid-physical-damage",
  "type": "eventAbsence",
  "description": "Complete two consecutive player turns without physical HP damage.",
  "window": "turn",
  "windowActor": "opponentTeam",
  "requiredWindowCount": 2,
  "windowMode": "consecutive",
  "predicate": {
    "type": "damage",
    "actor": "opponentTeam",
    "role": "target",
    "counterpart": "any",
    "kind": "physical",
    "component": "health",
    "sourceAbilities": []
  }
}
```

Enemy/wild activations are not eligible windows and neither increment nor reset this
streak. A player activation with zero matching damage qualifies.

---

# 14. Idempotency, replay, and persistence boundaries

## 14.1 Live duplicate protection

`ConditionEvaluationState` stores current battle and next expected sequence:

* a batch ending before next expected is a duplicate and returns unchanged;
* partial overlap is an error;
* a future gap is an error;
* a foreign BattleId is an error;
* no event is applied twice.

Compare full strong IDs, not only numeric sequences.

## 14.2 Replay equality

For every golden transcript:

```text
liveResult =
    fold(evaluateBatch, initialState, batches as committed)

replayResult =
    BattleConditionEvaluator::replay(initialAdvancement, archivedTranscript)

sanitize(liveResult) == replayResult.advancement
```

Test every variant and composite, not only damage.

## 14.3 Feat persistence

Step 17 supplies only battle-start-active requirements. It:

1. copies persistent advancement;
2. replays the complete terminal transcript;
3. receives a new sanitized advancement/delta;
4. persists completion for every scope, incomplete closed fight counts/streaks, and the
   game metric/buckets; it clears incomplete ability/turn counts/streaks at battle close;
5. atomically applies it with rewards/results later.

Aborted transcripts return no advancement. Wins, losses, and rule-defined terminal draws
are eligible.

## 14.4 Taming live state

Step 16 creates a fresh `ConditionEvaluationState` per top-level profile condition and wild
unit. All scopes, including game, start empty. It feeds every complete committed batch after
defeat pre-scan, including `TamingSystem` batches; the evaluator advances its cursor through
ignored Taming-category diagnostics while Gameplay/Lifecycle facts still score. It reads
completion after each batch. A `TechnicalAbort` explicitly abandons/finalizes the live
evaluator without attempting to score or bridge that terminal span. Nothing is saved after
encounter.

For batch-snapshot continuity, step 16 stores `advancement`, `currentBattle`, and
`openWindows` as material tracker state but keeps `nextExpectedSequence` as full-digest-only
orchestration scratch. It composes this complete type for each pure evaluator call and splits
the result again; that storage decision does not change duplicate/gap validation semantics.

The condition engine reports completion only. `TamingService` owns impression/removal
policy; the evaluator never removes a unit.

---

# 15. Error model and diagnostics

Use the `ConditionEvaluationErrorCode`/`ConditionEvaluationError` value types declared with
the result variants in section 7; do not invent an exception-only or boolean error channel.

Include stable context: condition ID, window ID when known, battle/batch/sequence, and
payload kind. Do not put pointer values or localized prose in deterministic digests.

Duplicate fully processed delivery is a successful no-op, not an error. `AbortedTranscript`
is a normal “no progress” result for step 17 but should remain distinguishable from a
loss/draw.

Add optional F7/debug trace counters:

```text
condition batches processed
events visited
open ability/turn/fight windows
last window ID/result
last evaluation error
```

Do not add a production condition UI in this step.

---

# 16. Complexity and determinism

The expected content scale is small. Prefer a clear closed visitor over a premature rule VM.

Requirements:

* condition/child iteration follows authored order;
* event iteration follows sequence;
* affected unit IDs are deduplicated/sorted;
* ability buckets are sorted by semantic ID;
* all arithmetic is signed 64-bit checked with configured caps;
* no wall time, render delta, pointer order, unordered iteration, or RNG;
* evaluating conditions consumes no battle RNG;
* definitions are immutable;
* advancement mutation uses copy/validate/commit.

An O(number of active conditions × relevant events) replay is acceptable under configured
event/condition limits. If indexing is later needed, build a deterministic typed event
index; do not change semantics.

---

# 17. Expected files

Adapt names to the established tree:

```text
Playground/srcs/conditions/
├── condition_definition.hpp/.cpp          # final schema fields if not already reconciled
├── condition_parser.cpp                   # strict new fields/literals
├── condition_window.hpp/.cpp
├── condition_advancement.hpp/.cpp
├── condition_subject_resolver.hpp/.cpp
├── condition_replay_state.hpp/.cpp
├── condition_event_matcher.hpp/.cpp
├── battle_condition_evaluator.hpp/.cpp
├── battle_condition_transcript.hpp/.cpp
└── condition_progress_snapshot.hpp/.cpp

Playground/tests/conditions/
├── condition_window_tests.cpp
├── condition_subject_resolver_tests.cpp
├── condition_replay_state_tests.cpp
├── condition_metric_tests.cpp
├── battle_condition_evaluator_tests.cpp
├── condition_gdd_golden_tests.cpp
├── condition_replay_idempotency_tests.cpp
└── condition_schema_completion_tests.cpp
```

Retain and run the step-03 parser tests proving every JSON fixture includes `windowActor`
and AbilityCast's affected fields. Add the evaluator/runtime tests here and register their
sources in the existing CMake targets.

---

# 18. Automated verification

## 18.1 Window identity and boundaries

Prove:

* two accepted casts get distinct ability IDs from action IDs;
* move/end/placement actions create no ability window;
* rejected casts create no action/window/event;
* one turn spans multiple command batches with one `TurnIndex`;
* automatic activation end closes the same turn;
* enemy turns are ignored for a player `windowActor`;
* fight opens/closes exactly once;
* BattleId scopes reused numeric action/turn sequences across fights;
* Feat/taming game IDs differ by domain/subject.

Malformed open/close/action/batch combinations must return typed errors without progress.

## 18.2 Metrics and comparisons

Table-test count/sum/max/min and all comparisons, including:

* empty normal window fails;
* absence empty window succeeds;
* between is inclusive;
* requested versus applied damage/heal/shield/resource;
* zero effective values do not count;
* checked overflow;
* one cast with two 50-damage hits: sum 100, max 50;
* shield-before fact across a breaking multi-hit;
* same-ability bucket tie/order.

## 18.3 Relative filtering

For Feat and wild-subject bindings, test every unit set and role:

* subject versus ally exclusion;
* subjectTeam inclusion;
* opponent inversion in taming;
* source, target, either without self double count;
* null source behavior;
* removed identity retains side relation.

## 18.4 Event-time position

Use multi-level board-local cells and prove:

* distance ignores elevation;
* afterMove samples only voluntary aggregate completion;
* deployment/displacement/teleport do not satisfy afterMove;
* throughoutWindow fails on an intermediate `UnitMovementStep` then remains failed;
* swap/displacement/teleport can fail the invariant;
* HP crossing zero changes active membership at the exact Damage event;
* removed relative units are absent from later closest-distance checks;
* a voluntary, displacement, and teleport fixture whose enter trigger removes the mover
  validates the aggregate against `lastOccupiedCell`; only the voluntary aggregate counts
  its applied movement prefix, and none produces an `afterMove` sample for the removed unit;
* no comparable pair alone cannot qualify an otherwise empty window;
* replay topology matches batch after-snapshot.

## 18.5 Persistence/streaks

Across at least two transcripts prove:

* game sum carries effective remainder;
* open ability/turn/fight metrics never carry;
* an ability/turn condition completed inside the fight stays completed;
* an incomplete ability/turn qualifying count/streak resets at fight close;
* fight required-window count counts distinct terminal battles;
* fight-consecutive streak crosses battles and resets on a failed eligible fight;
* taming resets every field for a new encounter;
* aborted battle contributes nothing.

## 18.6 Catalog

Golden-test every leaf:

* damage kind/component/shield/ability filters;
* healing and overheal;
* cast affected count/distance/same ability;
* status action/ID/tag/stacks/source semantics;
* shield apply/absorb/break;
* voluntary/displacement/teleport direction/distance;
* resources and penalty, including every valid reason and refill/effect/penalty/max-clamp
  near misses;
* all position sample modes;
* defeat versus impression without double count;
* relative outcome and subject active;
* active/notDefeated survivor counts;
* every absence predicate;
* nested allOf/anyOf and depth boundary.

## 18.7 GDD golden suite

Create one named test for every JSON example in section 13. For each include near misses:

* wrong ability, empty affected set, below/above distance;
* damage before shield and one below threshold;
* Poison expiry versus actual ally cleanse;
* overheal;
* one too-close intermediate step;
* multi-hit attack counted once;
* two 50 hits versus one >80 hit;
* impressed versus active survivor;
* displacement adjacent versus voluntary move;
* three casts split across different abilities;
* physical shield absorption versus physical HP loss;
* enemy turns interleaved between consecutive player turns.

This suite is the acceptance proof that the catalog expresses the GDD.

## 18.8 Live/replay/idempotency

For all variants:

* fold live batches and replay transcript; compare sanitized advancement/deltas;
* deliver a completed batch twice; second is unchanged;
* deliver a gap/overlap/foreign battle; typed error and unchanged state;
* replay the same transcript twice from the same initial advancement; results are equal;
* assert advancement contains no encounter-local replay stamp and document that the stable
  step-18 result transaction token prevents double application;
* shuffle definition storage in an unordered fixture but retain authored vectors; output
  remains stable;
* snapshot/event/log/RNG digests remain unchanged by evaluation.

## 18.9 Prospective close

Build one- and two-wild fixtures and prove:

* the actual tracker remains incomplete before terminal closure;
* copied Impressed removal of the candidate set yields a normal terminal outcome;
* prospective survivor/outcome leaves use that copied terminal state and can pass;
* pass and failure mutate no advancement, cursor, log, snapshot, or RNG;
* a non-terminal prospective removal is rejected;
* a foreign/duplicate/out-of-order candidate or mismatched snapshot is rejected;
* prospective output equals the same leaves' output when an equivalent real terminal
  transcript is constructed;
* two wilds whose non-close leaves are ready and fight-close leaves pass form a stable set
  and both can be staged;
* when one of two candidates fails, the set shrinks and is recomputed; if the remainder is
  non-terminal, neither is committed and all actual windows remain open;
* candidate application order is enemy roster order regardless of container insertion.

Run the complete Playground suite, not only condition tests.

---

# 19. Runtime smoke verification

No new production HUD is required. Use the normal debug battle plus deterministic trace:

1. enable a debug-only condition trace for one player subject;
2. cast a multi-hit ability and verify one ability window ID with multiple Damage events;
3. move through several support cells and verify position replay sees every step;
4. end two own turns with an enemy turn interleaved and inspect eligible window IDs;
5. finish the battle and compare live trace advancement with replay output;
6. repeat with the same initial advancement/transcript in a headless command and verify an
   equal pure result; separately verify the result-transaction seam rejects double apply;
7. verify battle rendering/input/HUD behavior from steps 12-14 is unchanged.

Do not expose hidden taming requirements yet; step 16 owns presentation policy.

---

# 20. Documentation

Update:

* the implemented step-03 progression schema against its documented `windowActor`,
  `affected/minimumAffectedUnits`, and `throughoutWindow` contract;
* battle event documentation with pre-hit shield/source/affected-unit semantics and
  transcript retention;
* a new `Playground/docs/03-systems/battle-conditions.md` covering IDs, boundaries,
  persistence, effective values, relative binding, replay, and GDD mappings;
* architecture diagrams showing battle log/batches -> shared evaluator -> taming live or
  Feat post-battle consumers;
* test/how-to documentation with the GDD golden suite.

Correct any stale Unity-era wording that refers to a separate `FeatEvent` list or
ScriptableObject subclass evaluator. JSON definitions and the typed battle log are the
Playground source of truth.

---

# 21. Non-goals

Do not implement:

* `TamingService`, impression/removal priority, recruitment, or taming HUD;
* Feat node activation, reward application, evolution, or result summaries;
* save-file parsing/commit;
* a general expression language, scripting runtime, reflection registry, or arbitrary
  negation;
* custom user-authored condition code;
* condition authoring/editor tools;
* polling presentation state;
* inferred windows based on event adjacency;
* floats for progress;
* condition-driven mutation inside an unresolved battle batch;
* balance content beyond deterministic fixtures.

---

# 22. Acceptance criteria

This step is complete only when:

* the strict JSON schema has explicit window owners, affected-unit cast filters, and a
  throughout-window position invariant;
* all fixtures/content use the final schema and unknown fields remain rejected;
* ability/turn/fight/game windows use explicit scoped IDs and exact boundaries;
* event-time replay catches intermediate topology/active-state changes;
* effective value, source/target/team, aggregation, comparison, absence, bucket, streak,
  and composite semantics are implemented as specified;
* advancement/runtime state are value-owned, serializable, bounded, checked, and stable;
* live and replay paths call the same evaluator and produce equal output;
* prospective fight close calls the same closure logic without mutating or fabricating log
  state;
* duplicate/gap/foreign/malformed/overflow behavior is transactional and tested;
* aborted/rejected/preview/presentation/domain events cannot grant progress;
* every leaf and every GDD example plus near misses has a golden test;
* Feat persistence and battle-local taming initialization seams are ready for steps 16-17;
* full builds/tests pass and battle presentation still works;
* documentation records exact schema, windows, IDs, persistence, and event semantics.

---

# 23. Handoff to steps 16 and 17

Report:

* final `ConditionSpec` field names/literals and schema version implications;
* exact window ID types, open/close rules, and `windowActor` semantics;
* `ConditionAdvancement` versus transient `ConditionEvaluationState`;
* batch/replay API and typed errors;
* persistence sanitization and the step-18 result-token idempotency boundary;
* event payload additions/clarifications made in steps 06-10;
* position replay and throughout-window rules;
* all GDD example fixture paths and test commands.

Step 16 stores one fresh evaluation state per wild/profile condition, feeds committed
batches live, and owns impression policy. Step 17 binds active player Feat requirements,
replays the retained terminal transcript, persists only permitted advancement, and owns
reward application. Neither may reinterpret a matcher or window.
