# Implement live wild-creature taming

Implement the GDD's passive taming mechanic on top of the shared battle-condition engine:
each tameable wild opponent tracks its own conditions, becomes impressed and leaves the
board as soon as all conditions are fulfilled, and produces a value-owned provisional
recruit only if the player ultimately wins.

This is an implementation task. Modify the headless battle runtime, connect the existing
battle presentation, add tests, and keep the complete Playground build green. Do not add a
capture command, item, probability roll, or authoring tool.

---

# 1. Repository baseline

Implement against the live repository after steps 01-15. Reinspect it before editing.

The required prior architecture is:

* step 03 parses a `TamingProfileDefinition` embedded in each species and validates every
  condition through the shared condition-specification catalog;
* step 04 supplies immutable species definitions, persistent `CreatureUnit` values, the
  six-slot team, PC storage, and encounter metadata distinguishing wild/trainer/special;
* steps 06-10 supply stable `BattleUnitId`, `BattleContext`, removal reasons, the typed
  battle log, outcome evaluation, status/trap events, and the sole command resolver;
* steps 12-14 supply the live BattleMode, battle presenter, result staging seam, and HUD;
* step 15 supplies `BattleConditionEvaluator`, explicit ability/turn/fight/game windows,
  subject/team filters, aggregators, streaks, end-state predicates, and serializable
  advancement values.

There must still be no taming-specific command. If a prior step introduced one, remove it
and route all acquisition through this passive tracker.

The current Unity draft is useful only as a warning: its `HasLeftBattle` units can still be
scheduled because initiative checks only defeat. Playground must use the common active-unit
predicate everywhere.

---

# 2. Required final behavior

For a battle with `encounterKind == Wild` and `allowsTaming == true`:

1. Each enemy whose species has a non-empty taming profile receives an independent tracker.
2. The tracker observes committed battle-event batches in sequence order.
3. Its conditions evaluate relative to that wild unit and to the player team, using the
   shared condition engine; there is no duplicated taming-only scope implementation.
4. Partial progress is visible to presentation but never saved.
5. When every condition completes, the unit becomes `Impressed` immediately.
6. An impressed unit is removed from occupancy and initiative without becoming defeated.
7. Battle continues while any active enemy remains. If none remains, the ordinary outcome
   rule yields player victory.
8. A wild unit reduced to zero HP before impression is permanently failed for the encounter.
9. At terminal player victory, impressed units become provisional recruit records. At
   defeat or draw, they are forfeited.
10. Step 18 will atomically put provisional recruits into the team or PC. This step must not
    mutate persistent `PlayerData` while battle resolution is still in progress.

Trainer, gym, boss, scripted non-wild, and debug encounters with `allowsTaming == false`
must not create trackers even when their species normally has a taming profile.

---

# 3. Locked terminology and state model

Use these distinct terms:

```text
taming profile       immutable species-authored list of conditions
taming tracker       mutable, battle-local progress for one wild BattleUnit
impressed            all conditions completed; unit voluntarily left battle
failed               unit was defeated before impression; terminal for this encounter
provisional recruit  value record produced by a won battle, not yet committed to PlayerData
recruited            provisional recruit committed by the result transaction in step 18
```

Do not use `Captured`, `CaptureAction`, `captureRate`, `catchChance`, or `pokeball` in C++
or JSON.

Add a state equivalent to:

```cpp
enum class TamingState
{
    Tracking,
    Impressed,
    Failed,
    Forfeited
};

struct TamingConditionMaterialState
{
    ConditionAdvancement advancement;
    std::optional<BattleId> currentBattle;
    std::vector<OpenConditionWindow> openWindows;
};

struct TamingConditionProgress
{
    std::string conditionId;
    std::size_t definitionOrder = 0; // presentation ordering only; never identity
    TamingConditionMaterialState material;
    BattleEventSequence nextExpectedSequence; // orchestration scratch; never snapshotted
};

class TamingTracker
{
private:
    BattleUnitId _subject;
    const TamingProfileDefinition *_definition = nullptr;
    std::vector<TamingConditionProgress> _conditions;
    TamingState _state = TamingState::Tracking;

public:
    // read-only accessors
    // evaluateBatch(...) is callable only by TamingService
    // markFailed(...) is idempotent and terminal
};
```

Equivalent organization is acceptable, including an optional tracker stored directly on
`BattleUnit`. Do not derive `WildBattleUnit` from `BattleUnit` solely to add this data; the
unit kind plus optional state is simpler, avoids polymorphic ownership, and keeps unit
storage homogeneous.

When invoking step 15, compose its ordinary `ConditionEvaluationState` from `material` plus
`nextExpectedSequence`, then split the pure result back into the same two domains. Material
state contains every value that can alter later qualification; the delivery cursor only
guards batch sequencing.

`BattleUnit::isActiveCombatant()` must remain the single predicate used by scheduler,
targeting, AI, occupancy, and outcome rules:

```text
placed
and health > 0
and removalReason == None
```

An impressed unit has `removalReason == Impressed`; it is never active even though its HP
may be positive.

---

# 4. Taming evaluation context

Every tracker evaluates conditions with a context equivalent to:

```cpp
struct TamingEvaluationContext
{
    BattleUnitId wildSubject;
    BattleSide performerSide = BattleSide::Player;
    BattleSide observedSide = BattleSide::Enemy;
    const BattleSnapshot *beforeBatch = nullptr;
    const BattleSnapshot *afterBatch = nullptr;
};
```

The shared condition spec decides which actors and targets qualify. The context only binds
relative vocabulary such as `subject`, `subjectTeam`, and `opponentTeam`:

```text
subject       = this wild unit
subjectTeam   = enemy side
opponentTeam  = player side
```

This supports conditions such as:

* player side deals at least 80 magical HP damage in one ability window;
* any player ends a move adjacent to this wild unit;
* player side casts the same ability three times during the fight;
* at least two player creatures remain active at battle end;
* player side completes two consecutive own turns without physical HP damage.

Do not implement special-case `if` statements for these examples in `TamingService`.
They must be ordinary condition definitions evaluated by step 15's catalog.

Only committed gameplay events enter the evaluator. Preview queries, rejected commands,
presentation animation, hover changes, and AI candidate enumeration never affect taming.

---

# 5. Event-batch ordering and defeat priority

The command resolver already commits one ordered event batch per successful command or
timeline transition. `TamingService` consumes that immutable committed batch after all of
its effects, pending defeats, outcome evaluation, `after` snapshot, and event range are
final, but before external publication or acceptance of the next command.

Use this order:

```text
validate command
resolve all authored effects in their defined order
after each complete authored EffectApplication and all of its permitted reactions, finalize
    every HP-zero defeat; no defeat remains pending at batch commit
evaluate ordinary battle outcome
capture after snapshot and commit the command/timeline batch unchanged
release the command-resolution guard
pre-scan and purely evaluate that committed batch against copied material tracker/cursor state
after the complete tracker pass validates, atomically commit its delivery-cursor advances
if no material tracker state changes: publish the original batch; create no derived batch
otherwise open one no-action Taming system batch whose before == original after
    apply staged material/failure state and append its diagnostic events
    remove newly Impressed units in stable enemy-roster order
    if an impressed unit owned the activation, run removed-actor cleanup/activation closure
    evaluate ordinary outcome again
    if it newly becomes terminal while another actor owns the turn, structurally close it
    append BattleEnded only on that new transition
    capture after and commit the non-empty system batch
feed that system batch to every still-Tracking evaluator while ignoring Taming-category
    payloads; repeat derived batches until no tracker changes
publish all committed batches in batch-ID order
```

Defeat wins a same-effect race. A killing hit that also crosses the final taming threshold
does not tame the zero-HP unit. Pre-scanning `UnitDefeated` before scoring the rest of that
batch makes this deterministic and matches the GDD's "defeated before all conditions are
met" rule.

The original committed descriptor is immutable: never extend its event range or replace its
`after` snapshot. Separate material tracker state from the evaluator's delivery cursor:

* `BattleSnapshot`/`TamingTrackerSnapshot` and `battleSnapshotDigest` contain advancement,
  completion/failure/forfeit, open material metrics, and impression state, but not
  `nextExpectedSequence`;
* `nextExpectedSequence` is authoritative orchestration scratch. Stage it on copies and
  advance it atomically only after every tracker has validated the safe input span; include it in the full
  `authoritativeBattleStateDigest` and replay-pump state, but emit no event/batch for a
  cursor-only change;
* therefore an original batch's material `after` still deep-equals the next derived batch's
  material `before`. The permitted cursor advance between them cannot affect
  `battleSnapshotDigest` or `gameplayProgressDigest`.

The derived batch has `kind == BattleBatchKind::TamingSystem`, `action == null`, the next
contiguous batch/event IDs, and snapshots that include changed material taming tracker state.
It has no issuer because it is an internal no-action batch, not a submitted command. This
keeps step 06's snapshot-continuity and step 15's replay checks true without recursively
creating cursor-only Taming batches.

An ordinary gameplay batch that just set `BattlePhase::Terminal` is still consumed once by
this post-resolution service before result finalization. Terminal already rejects every new
command and time advance, and its outcome is immutable, but the session keeps the internal
system-batch seam open until Taming quiesces. Branch before staging impressions:

* when `batch.after.outcome == Undecided`, use the normal completion/removal algorithm;
* for PlayerVictory/PlayerDefeat/Draw, evaluate the final safe span so cursors, progress, and
  defeat priority are exact, then transition every remaining Tracking tracker to
  `Forfeited`. A newly completed profile is still Forfeited, never Impressed. A derived batch
  may record final progress/failure/forfeit diagnostics after the existing `BattleEnded`, but
  it may not append `UnitImpressed`, cleanup/removal, a provisional recruit, another
  `BattleEnded`, or any core gameplay mutation;
* Aborted follows the separate no-evaluation rule below.

If Taming itself removes the last opponent from an undecided state, its system batch performs
the one ordinary outcome transition and appends the sole `BattleEnded`. Only after the
derived fixed point does step 18 freeze the log and construct `BattleResult`.

Exception: never evaluate a `BattleBatchKind::TechnicalAbort` or an after-snapshot whose
outcome is Aborted. The state is no longer a safe gameplay input and step 15 grants no
advancement for aborted transcripts. Commit no Taming system batch, progress, impression,
removal, or provisional recruit from that aborting span; freeze existing trackers and move
directly to result finalization. Impressions completed in earlier safe batches may remain in
the historical summary, but Aborted outcome still commits no recruit. Do not turn the abort
itself into a failure/progress event.

Do not recursively publish through the same `ContractProvider` while it is dispatching.
Queue every committed descriptor and let the normal presentation pump publish them only
after the derived fixed point is complete. Taming-category diagnostics advance evaluator
cursors but never score, so they cannot feed their own condition; Gameplay/Lifecycle facts
in a `TamingSystem` batch remain visible and may legitimately close another tracker.

---

# 6. Live tracker algorithm

Implement one `TamingService` owned by the active `BattleSession`, not a global singleton.

At battle construction:

1. Check encounter kind and `allowsTaming`.
2. Walk enemy units in stable roster order.
3. Look up each unit's species profile.
4. If the profile is absent or empty, leave its tracker empty; it cannot be tamed.
5. Otherwise construct progress entries in authored condition-array order.
6. Reset all scopes, including `game`, because taming progress never persists between
   encounters even though it reuses the same evaluator shape.
7. Complete this initialization after unit projection but before the Construction batch's
   snapshots are captured, so both sides of that first batch contain the same fresh material
   tracker values. After the committed `BattleStarted` span arrives, invoke step 15's
   `beginBattle` exactly once and stage its opened-window/material result through the normal
   non-empty TamingSystem batch; do not mutate the committed Construction descriptor.

For every newly committed batch whose explicit `BattleBatchKind` is `Construction`,
`Command`, `Timeline`, or `TamingSystem` (never `TechnicalAbort`):

1. Ignore terminal trackers. A fully processed duplicate span returns unchanged; a partial
   overlap, future gap, or foreign BattleId is a typed error exactly as specified in step 15.
2. Feed the exact event span and snapshots to every condition on copied material tracker/
   cursor state, applying step 15's category mask so Taming diagnostics do not score. Stage
   each cursor across every event sequence even when no material condition changes. Only
   after the complete pass over every eligible tracker validates, atomically publish all
   staged cursor advances; this cursor write is not a snapshot mutation.
   Use `beginBattle` for the first Construction/`BattleStarted` span, `finishBattle` for a
   safe span containing the one `BattleEnded`, and `evaluateBatch` for every intervening
   Command/Timeline/TamingSystem span. Never call two of these for the same event range.
3. Stage every changed material condition state—including advancement, current-battle
   identity, opened/closed windows, metric/sample flags, and invariant flags—without
   changing live state or the definition.
4. A tracker with no fight-close-dependent leaf completes when every top-level condition is
   complete. A tracker blocked only on fight-close leaves enters the prospective-close
   candidate algorithm below; do not mark it complete prematurely.
5. Stage failed and newly impressed IDs; do not mutate unit storage while iterating
   trackers.
   If the input after-outcome is already a non-Aborted terminal outcome, stage no impressed
   ID: retain final advancement for presentation, mark every still-Tracking tracker
   `Forfeited`, and emit only Taming diagnostics.
6. If any material state changed, commit all staged tracker changes and their events inside the one
   no-action Taming system batch described above.
7. Process staged impressed IDs in enemy roster order through
   `BattleContext::removeUnit` with `RemovalReason::Impressed` inside that same batch.
8. Feed the newly committed `TamingSystem` batch back through steps 1-7. Stop only when the
   pass changes no tracker and stages no removal/outcome. Every non-final derived pass either
   removes at least one previously active tracked enemy or consumes the one terminal close;
   a diagnostics-only pass is therefore final. Assert a structural budget of
   `initialTrackedEnemyCount + 2` derived passes; exceeding it is an
   `InternalInvariantViolation` technical abort, not silent truncation.

Evaluator failure is transactional at the complete pass boundary. If any copied evaluator
returns `ArithmeticOverflow`, `EventSequenceGap`, a non-duplicate
`DuplicateOrOverlappingRange`, `ForeignBattle`,
`BatchEnvelopeMismatch`, `WindowBoundaryMismatch`, `ReplayStateMismatch`, `LimitExceeded`,
or another typed step-15 failure:

1. discard every cursor/material/event/impression/outcome change staged by that pass; no
   tracker cursor advances across the failing source span and no partial `TamingSystem`
   batch is committed;
2. retain the already committed source batch exactly—it remains the last trustworthy
   gameplay/material state;
3. from that batch's immutable `after` state, open one fresh no-action
   `BattleBatchKind::TechnicalAbort` and apply step 06's canonical active-turn close,
   `BattleAborted`, and `BattleEnded{Aborted}` tail;
4. map `ArithmeticOverflow` to `BattleAbortReason::NumericInvariant`; map malformed envelope/
   cursor/battle/window/replay state, definition inconsistency, `LimitExceeded`, and derived-
   pass budget exhaustion to `BattleAbortReason::InternalInvariantViolation`;
5. never feed the abort batch back into Taming and never fabricate progress/failure/
   forfeiture/impression from it.

When the trustworthy source was a Command batch, `BattleSession::submit` still returns its
ordinary `AcceptedCommand`: the action already committed before this derived evaluation.
The separately queued TechnicalAbort batch and terminal outcome expose the later failure.
Do not return step 06's `AbortedCommand`, which is reserved for a command transaction that
never committed. Timeline/TamingSystem sources instead surface the abort through the battle
pump result and committed-batch queue.

If the failing source is a previously committed TamingSystem batch, changes safely committed
in earlier passes remain; only the current incomplete pass rolls back. The source batch's
material `after` is still the abort batch's `before`.

Removing an impressed unit must:

* erase its board occupancy;
* leave its immutable unit identity and source spawn record available for the result;
* exclude it from scheduler and AI immediately;
* append the exact per-unit sequence, completing one unit before the next enemy-roster ID:
  `UnitImpressed`; transient `StatusRemoved{UnitRemoved}` by instance ID; non-depleted
  `ShieldRemoved{UnitRemoved}` by shield ID; `BattleObjectRemoved{OwnerRemoved}` for its
  owner-activation objects by object ID; `UnitRemoved{reason=Impressed}`; and, only when it
  owned the activation, `ActivationEnded{reason=ActiveUnitImpressed}` followed by bar reset
  and active-unit/turn clear. Hooks and captured-duration decrement are suppressed and the
  captured IDs are discarded;
* not append `UnitDefeated`, increment kill conditions, grant defeat credit, or run
  death hooks;
* be idempotent if the service sees the event range twice due to a caller bug (assert in
  debug, no duplicate removal in release).

## 6.1 Prospective fight close for terminal taming conditions

Step 03 deliberately allows fight-close conditions such as keeping two player creatures
active or winning the fight. Waiting for an actual `BattleEnded` is circular when the
tracked wild unit is the last opponent: the battle cannot end until that unit is impressed,
but it cannot be impressed until its fight window closes.

Use step 15's pure `previewFightClose` seam together with a scratch battle transaction. It
evaluates copied advancement against the exact prospective after-state/event sequence,
appends nothing to the live log, advances no real cursor/window, consumes no RNG, and
performs no live mutation. A plain active-count snapshot is insufficient because exact
reason-specific/cleanup/`UnitRemoved`/`ActivationEnded` ordering must match the committed
transcript even though owner-removal hooks and duration expiry are suppressed.

After ordinary batch evaluation and defeat pre-scan:

1. Build `C` in enemy roster order from Tracking units whose non-fight-close leaves are all
   ready and whose only remaining blockers require fight close.
2. From the immutable original batch's `after` state, create a full value-owned scratch
   transaction with copied instance/sequence counters and tracker state. Apply the same
   tracker events and impression removals that a live `TamingSystem` batch would apply.
3. If a candidate is the active unit, run the exact `ActiveUnitImpressed` closure on the
   scratch state, including hook-suppressed owner cleanup and capture discard. Then run
   ordinary outcome evaluation.
4. If the copied outcome is terminal while a different active actor remains, perform step
   07's hook-free/no-duration-decrement `BattleTerminal` structural close and append the
   prospective `BattleEnded`. If outcome is not terminal, discard the scratch transaction;
   continued play or defeat of another enemy may make a later attempt terminal.
5. Preview-close every candidate's fight windows against that exact scratch after-snapshot
   and scratch event sequence. Let `P` be candidates whose complete profile passes.
6. If `P != C`, set `C = P` and repeat from step 2 using a fresh transaction from the same
   base. If `C` becomes empty or its removal no
   longer makes the copied battle terminal, commit nothing. The finite roster makes this
   fixed point terminate.
7. When a non-empty stable `C` both produces a terminal ordinary outcome and every preview
   passes, commit that already-validated scratch transaction atomically as the one real
   `TamingSystem` batch. Do not rerun a looser active-count-only path. Its exact event order
   is tracker diagnostics, then for each candidate in enemy roster order the complete
   `UnitImpressed -> status cleanup -> shield cleanup -> owner-activation-object cleanup ->
   UnitRemoved -> optional ActivationEnded(ActiveUnitImpressed)/bar reset/turn clear`
   sequence, then optional structural `ActivationEnded{BattleTerminal}`, then the sole
   `BattleEnded`.

Never append scratch preview events to the live `BattleEventLog`. Never preserve a failed preview's
window close/reset. Never group a merely partial tracker into `C`. A same-batch real defeat
was already marked Failed and cannot enter the candidate set.

This permits two last-side wild creatures with compatible terminal profiles to become
impressed together. If one fails its prospective terminal predicate and leaving it active
means the fight is not over, none of the circular candidates commits; the player must
continue normal play. Definition/roster order, not container order, controls every pass.

---

# 7. Provisional recruit records

Add a value type equivalent to:

```cpp
struct ProvisionalRecruit
{
    BattleUnitId sourceBattleUnit;
    std::string speciesId;
    std::vector<std::string> inheritedCompletedFeatNodeIds;
    std::string encounterDefinitionId;
    std::string encounterSpawnMemberId;
    std::uint32_t enemyRosterIndex = 0;
};
```

Use the stable node identifier type chosen in step 03 rather than raw `std::string` if that
step introduced a validated wrapper.

The record deliberately does not contain:

* a pointer/reference to `BattleUnit` or `CreatureSpeciesDefinition`;
* current battle HP/AP/MP, statuses, shields, turn-bar fill, or board position;
* partial taming-condition progress;
* partial Feat Requirement progress earned by the enemy during the fight;
* a generated persistent creature ID (step 18 creates it inside the commit transaction).

It does inherit the encounter spawn's pre-completed Feat node IDs. Form, attributes,
abilities, and passives are derived by replaying those rewards, so a tamed evolved wild
creature remains in the form the player encountered.

At terminal outcome:

```text
PlayerVictory -> one ProvisionalRecruit for every impressed enemy, roster order
PlayerDefeat  -> empty committed recruit list; retain impressed IDs only for result text
Draw/Aborted  -> empty committed recruit list
```

Store these records as copied, immutable result inputs owned by the session/runtime. Step 18
freezes them together with the terminal record and transcript into `BattleResult`. Never add
directly to the active team from an event callback inside resolution.

---

# 8. Presentation integration

Extend the existing presenter/HUD without making it authoritative:

* Condition progress rows come from a read-only `TamingTrackerSnapshot`.
* On `UnitImpressed`, play a simple deterministic placeholder presentation: tint or
  raise/fade the cube, show `"<name> was impressed"`, then hide/remove its view.
* Do not play the defeat presentation or label the unit KO.
* If the impressed unit was the last opponent, finish this short presentation before
  showing the existing victory result panel; the headless result is already terminal.
* Enemy creature details may show condition names and current progress. If discovery/hidden
  lore is desired later, it is a UI policy; do not hide data in the simulation.
* The result staging view lists impressed creatures even on a loss, followed by a clear
  `"forfeited after defeat"` status. It must not imply recruitment occurred.

Presentation delays do not change battle time and do not block creation of the headless
result. They may gate accepting the user's Continue click until their queue is drained.

---

# 9. Required events and snapshots

Add or finalize value-owned events equivalent to:

```text
TamingConditionStateChanged
    category=Taming; wildUnitId, conditionId, definitionOrder, change kind,
    previous TamingConditionMaterialState, new TamingConditionMaterialState
UnitTamingFailed
    category=Taming; wildUnitId, reason=Defeated
UnitTamingForfeited
    category=Taming; wildUnitId, reason=BattleAlreadyTerminal
UnitImpressed
    category=Taming; wildUnitId, speciesId
UnitRemoved
    category=Gameplay; unitId, previousCell, reason=Impressed
BattleEnded (only when impression creates the ordinary terminal transition)
    category=Lifecycle; outcome/count payload from step 07
```

Emit `TamingConditionStateChanged` whenever and only when the complete material state
changes, including a window open/close, intermediate metric/sample/invariant change, or
durable advancement. Use a closed change-kind summary such as
`WindowOpened|WindowUpdated|WindowClosed|AdvancementChanged`; the copied previous/new values
remain authoritative when several aspects change together. Presentation may label an
advancement delta as progress but must not infer that every window sample is durable
progress. This event guarantees every material-only derived batch is non-empty.

It is diagnostic/presentational; condition evaluators advance their sequence cursor across
but never score events from the `Taming` category. A `TamingSystem` batch deliberately mixes
categories when it removes a unit or ends the battle, so every still-Tracking evaluator must
consume it: Taming payloads cannot feed themselves, while Gameplay/Lifecycle facts may close
windows or advance another tracker. Fixed-point quiescence and the structural pass budget,
not wholesale exclusion of the batch kind, prevent recursion.

Snapshots expose state by value or immutable view for the duration of one update. Do not
expose mutable trackers to widgets.

---

# 10. Error handling and determinism

Battle construction must fail before entering Placement when:

* an enabled wild tracker references a missing/invalid condition definition (normally
  prevented at registry load; retain a defensive assertion);
* condition progress count differs from the species profile;
* two enemy roster entries produce the same `BattleUnitId`;
* a provisional recruit cannot identify its species or encounter spawn.

A non-wild battle carrying `allowsTaming=true` is invalid encounter data and must have been
rejected by step 04. Defensively return a construction error rather than silently enabling
it.

Tracker traversal, material-state events, impression removal, and provisional recruits use enemy
roster order. Do not depend on an unordered map. Taming consumes no RNG.

---

# 11. Required tests

Add headless tests under `Playground/tests/taming/` and extend battle integration fixtures.

## 11.1 Construction and gating

Test:

* wild + non-empty profile creates one tracker;
* wild + empty/absent profile creates none;
* trainer/gym/debug-no-taming creates none for the same species;
* two same-species wild units get independent progress;
* construction order is roster order.

## 11.2 Condition progress

Use typed event batches, not direct mutation, to prove:

* one incomplete condition retains partial progress;
* all conditions are ANDed;
* ability, turn, and fight windows behave exactly as step 15 specified;
* adjacency is measured relative to the correct wild unit;
* a single player event can advance multiple wild trackers when its filters qualify;
* rejected commands and preview queries produce no progress;
* replaying presentation notifications cannot advance progress.

## 11.3 Impression and removal

Assert the exact two-batch golden sequence for the final qualifying command: its ordinary
command batch remains unchanged, followed by one no-action Taming system batch containing:

* one or more ordered material-state-change events, including the final advancement;
* impressed event;
* status/shield/owner-activation-object cleanup in stable ID order when present;
* removal event after cleanup and optional active-owner closure after removal;
* no defeated/kill event;
* occupancy cleared;
* scheduler excludes the unit;
* AI never receives another turn for it.

Assert the first batch's `after` equals the second batch's `before`, both event ranges are
final/contiguous, the second batch's `after` includes tracker/removal/outcome mutations,
and neither descriptor is extended after commit. If evaluation produces material progress
but no impression, require the same system-batch contract for its state mutation/event. If it
produces no material tracker change, require no empty system batch.

Add a cursor-only continuity case: consume a contiguous gameplay batch that matches no
condition. Assert `nextExpectedSequence` and `authoritativeBattleStateDigest` advance, while
the material tracker snapshot, original batch `after`, next batch `before`,
`battleSnapshotDigest`, and `gameplayProgressDigest` remain unchanged; no TamingSystem batch
or diagnostic event is created, and the next contiguous gameplay batch is accepted.
Re-submit the fully consumed span directly to the service and require a successful unchanged
duplicate no-op: no cursor/digest/event/batch mutation. A partial overlap and a future gap
take the transactional evaluator-error/TechnicalAbort policy instead.

Also cover material changes with no `ConditionAdvancement` delta: consuming the Construction
batch opens a fight window in one non-empty TamingSystem batch, and a later throughout-window
sample updates its metric/sample/invariant state in another. Each emits the exact previous/
new `TamingConditionStateChanged`, preserves original-after/derived-before equality, advances
the delivery cursor without snapshotting it, and reaches quiescence without recursively
emitting a cursor-only batch.

Test last-enemy impression yields player victory even though defeated-enemy count is zero.

Add prospective-close cases:

* one last wild with a survivor-count/fight-outcome leaf passes from a pure prospective
  snapshot and is impressed without a real pre-existing `BattleEnded`;
* a failed preview changes no tracker/window/log/context state and can be retried later;
* two ready last-side wilds whose terminal leaves pass reach the stable candidate set and
  commit in roster order;
* when one candidate fails and the passing subset would not make outcome terminal, neither
  impression commits;
* a non-ready or already defeated tracker never enters the candidate set;
* preview calls are deterministic, append no synthetic event, and consume no RNG.

## 11.4 Failure priority

Test:

* defeat before completion marks failed;
* later qualifying events cannot revive taming progress;
* a killing hit that also reaches the threshold fails rather than impresses;
* an already impressed unit cannot later be defeated;
* failure is idempotent.

For both a PlayerDefeat and Draw final command, make the terminal span complete an otherwise
Tracking profile. Assert final progress diagnostics and cursor advancement are retained,
the tracker becomes `Forfeited`, and there is no `UnitImpressed`, cleanup/removal,
provisional recruit, second `BattleEnded`, or post-terminal gameplay mutation. Also cover an
incomplete surviving tracker becoming Forfeited.

For each of `RequiredTerrainChanged`, `NumericInvariant`, and
`EffectChainDepthExceeded`, start with a tracker one safe event from completion, trigger the
technical abort, and assert no new tracker advancement, impression, removal, or provisional
recruit and no `TamingSystem` batch.

Inject an evaluator arithmetic overflow and separately a malformed envelope/sequence gap
while another tracker in the same pass would have advanced. Assert the committed source
batch survives, every cursor/material change from the failing pass is discarded, no partial
TamingSystem batch/event/removal exists, and exactly one fresh canonical TechnicalAbort tail
uses `NumericInvariant` for overflow and `InternalInvariantViolation` for malformed state.
Assert neither abort is fed back into the evaluators.
For a Command source, also assert the gateway result remains Accepted with its original
action/batch/range while the drained committed-batch sequence contains source then abort;
repeat with a Timeline source and assert the pump reports Terminal without a command result.

## 11.5 Results

Test:

* victory produces provisional recruits in roster order;
* defeat/draw produces none;
* result still records which units had been impressed for presentation;
* recruit inherits only completed node IDs, not battle resources/statuses/partial progress;
* multiple recruits with identical species remain distinct records.

## 11.6 Mode/presentation lifecycle

Enter/exit two wild battles in one `GameSceneWidget` lifetime and assert no stale tracker,
contract, view, or provisional record survives. Visually validate one impression and one
loss-forfeit flow with placeholder cubes.

---

# 12. Expected file changes

Add cohesive files equivalent to:

```text
Playground/srcs/taming/taming_tracker.hpp
Playground/srcs/taming/taming_tracker.cpp
Playground/srcs/taming/taming_service.hpp
Playground/srcs/taming/taming_service.cpp
Playground/srcs/taming/taming_snapshot.hpp
Playground/srcs/taming/provisional_recruit.hpp

Playground/tests/taming/taming_tracker_test.cpp
Playground/tests/taming/taming_service_test.cpp
Playground/tests/taming/taming_result_test.cpp
```

Modify at least the battle unit/context/result/event-log, outcome integration, BattleMode
presentation pump, unit presenter, HUD/result staging view, and relevant fixture files.
Equivalent organization is acceptable; avoid one-file wrappers and avoid placing taming
rules in widgets.

---

# 13. Documentation requirements

Update the implemented-system documentation to explain:

* the difference between profile, tracker, impressed, provisional, and recruited;
* why taming reuses the shared condition engine but never persists condition progress;
* defeat priority within one event batch;
* why impressed removal is not defeat;
* victory when every enemy leaves impressed;
* completed-node inheritance and excluded state;
* trainer/gym gating;
* the step-18 transaction boundary.

Do not describe a recruit as added to team/PC until step 18 implements that commit.

---

# 14. Non-goals

Do not implement capture actions/items, random tame chance, feeding/gifts, taming minigames,
mid-battle recruitment, enemy recruitment on loss, persistent partial taming progress,
species discovery/lore progression, production animation/VFX/audio, team/PC commit, save
serialization, or taming in trainer/gym battles.

---

# 15. Acceptance criteria

This step is complete when:

* each eligible wild unit owns independent battle-local progress;
* every condition is evaluated through step 15's engine;
* defeat in the same effect batch takes priority over impression;
* an impressed unit leaves occupancy, initiative, AI, and targeting immediately;
* impression emits no defeat or kill credit;
* all-impressed enemies yield ordinary player victory;
* loss/draw forfeits all provisional recruits;
* victory returns value-owned provisional records with completed-node inheritance;
* no persistent player state is mutated during resolution;
* trainer/gym battles cannot instantiate trackers;
* deterministic ordering and all required tests pass;
* the visible placeholder flow distinguishes impression from defeat;
* the complete SparklePlayground and PlaygroundTests targets build and pass.

---

# 16. Required handoff report

Report the tracker/result APIs, exact final-event ordering, tests and commands actually run,
the visible scenarios actually checked, any skipped checks, and the precise result fields
step 18 must consume. Explicitly state that team/PC commit remains unimplemented at this
step.
