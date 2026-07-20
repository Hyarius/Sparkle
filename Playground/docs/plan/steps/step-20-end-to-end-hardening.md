# Step 20 - Harden and sign off the complete battle vertical slice

Implement this step only after steps 01 through 19 are complete. This is the final
integration and verification step for the battle-mode vertical slice. Do not expand the
design. Make the implemented loop deterministic, diagnosable, lifecycle-safe, documented,
and honestly verifiable from a clean checkout.

---

# 1. Current state at the start of this step

The repository should now contain a playable path from exploration through battle and back:

* strict immutable JSON registries and a three-species reference content pack;
* persistent creature/team/storage state and result-boundary save handling;
* tactical board, deployment, stamina turns, commands, targeting, effects, statuses,
  passives, traps, AI, defeat, taming, Feat Progress, and evolution;
* safe live-world Battle mode, tactical input, placeholder creature views, HUD, result
  summary, and exploration recovery;
* headless tests for the modules introduced by each preceding step.

Individual modules being green is not proof that the complete feature is reliable. The
remaining risks are interactions: different call sites producing different commands,
noncanonical iteration, lifecycle leaks after repeated mode switches, presentation lag
changing rules, impossible content, result commits occurring twice, malformed resources
partially publishing, and documentation claiming behavior that was never run.

Read all 19 prerequisite handoff reports before editing. Build an actual discrepancy list
between those reports, the current tree, and this prompt. A missing prerequisite is not a
hardening detail: finish its owning step first.

---

# 2. Final outcome

After this step, a lower-risk development build must provide all of the following:

1. A clean resource load either publishes the complete registry set or publishes nothing.
2. The same seed, definitions, roster, deployment, and command transcript always produce
   the same typed events, terminal outcome, and authoritative state digest.
3. Player, AI, debug autoplay, golden tests, and replay tests all use the one public command
   gateway.
4. Hundreds of repeated Battle-mode round trips leave exploration in the same ownership
   state as before entry.
5. Thousands of headless AI battles terminate within explicit safety caps without invalid
   state or no-progress loops.
6. Wild recruit, trainer victory, loss, Feat reward, evolution, save, and reload flows work
   together using the real shipped JSON.
7. Invalid data and commands fail with contextual diagnostics and no partial mutation.
8. Performance is measured against explicit, non-flaky budgets and obvious hot paths are
   corrected without compromising deterministic rules.
9. Current documentation describes only implemented behavior and gives exact build/run/
   verification instructions.

The battle vertical slice is then ready for the user's resource-authoring phase. That later
phase may improve content workflow; it must not be required to play this slice.

---

# 3. Definition of authoritative state

Before writing more tests, freeze what is and is not simulation state.

## 3.1 Included in a battle state digest

Use three separately named/domain-prefixed digests:

* `gameplayProgressDigest` is step 06/11's material mutation guard: frozen board identity,
  source-specific live terrain provenance, gameplay/RNG state, live taming advancement,
  completion/failure, open-window metrics, and other condition values that can alter later
  scoring/impression. It excludes event/action/batch counters,
  the activation's generic `resolvedNonEndCommands`, log/archive, delivery-only
  `nextExpectedSequence`/battle cursor, presentation cursors, and the AI guard's unit/turn/
  count/observed-digest sequence;
* `battleSnapshotDigest` hashes exactly one immutable `BattleSnapshot` value in its declared
  canonical field order and nothing outside that value;
* `authoritativeBattleStateDigest` is the full replay/equality digest described below,
  including live authoritative context, permanent archive metadata, log, counters, and
  pending derived/result state.

The permanent batch archive stores snapshots, not a full state digest. Each committed
batch's ID/kind/action/range and two `battleSnapshotDigest` values are folded exactly once
into the domain-prefixed incremental archive accumulator; the full state digest hashes only
that archive count/tip. It must never include its own current or previously formatted
`authoritativeBattleStateDigest` as input; that would be circular. Replay
`afterSnapshotDigest` and operation `stateDigest` deliberately mean different domains.

Serialize/hash in an explicitly documented canonical order:

* battle schema/digest version; the complete result token/source tuple; encounter definition
  and display IDs, stable instance ID, kind, taming/repeatability flags, resolved tier/team,
  world/resolution/combat seeds, validated return cell, and exact `BoardSourceDescriptor`
  (kind plus handcrafted definition ID when present); then `BattleId`, phase, outcome,
  optional abort reason, simulated fixed-point time, active unit ID, activation counter,
  generic `resolvedNonEndCommands`, and scheduler tie metadata;
* source-specific terrain provenance: for `LiveWorld`, the authoritative `worldAnchor` and
  complete `LiveBoardTerrainStamp` encoded as coordinate-sorted required chunk coordinates
  and captured `contentRevision` values; for `Handcrafted`, explicit absence of both. Never
  encode the stamp's borrowed `VoxelWorld*`, lifetime token, or a revision sampled after
  board construction;
* ordered board cell definitions needed by rules, occupancy, and ordered battle objects,
  using board-local coordinates for both sources. A renderer translation or
  `presentationOrigin` never enters independently; the live anchor is encoded exactly once
  above because it is authoritative terrain provenance, not cosmetic placement;
* every battle unit sorted by `BattleUnitId`: side, encounter roster order, species/form,
  persistent source ID or enemy encounter-spawn member ID, inherited completed enemy Feat
  node IDs, position/removal reason, complete baseline `CreatureAttributes` and complete
  effective `CreatureAttributes` in `CreatureStat` enum-code order, current HP/AP/MP,
  pending next-activation penalties, effective maxima,
  shields in resolution order, statuses and their remaining durations/stacks/sources,
  stamina fill, activation state, ability snapshot, passive snapshot, and AI scratch state
  that can affect a later decision; full-digest-only AI activation guard scratch is encoded
  once after the units and is absent from `BattleSnapshot`/`battleSnapshotDigest`;
* battle-local RNG state or an equivalent draw counter plus seed;
* immutable `ParticipantFeatSnapshot` records in participant order, including board and
  active-node IDs; the live taming evaluator's delivery cursor, current battle identity,
  open/pending window IDs and material metrics; and all other fight-local taming aggregates
  that can alter acceptance or later scoring. Feat evaluation remains the post-battle
  transcript replay from steps 15/17 and is represented only once it exists as pending
  result/progression state;
* the complete ordered authoritative batch archive represented by its incrementally
  maintained count and canonical archive-prefix digest (each appended record contains ID,
  explicit kind, optional action, event range, and canonical before/after
  `battleSnapshotDigest` values), the analogous typed-event prefix count/digest, and next
  event, batch, action, object, status, and shield IDs;
* any in-flight simulation writer or pending post-resolution/Taming derivation input that
  can still mutate authoritative state; public command/pump replay checkpoints must be
  quiescent and therefore have none;
* pending terminal/result information that affects future mutation.

Use integer/fixed-point values and stable UTF-8 strings. Sort maps/sets before hashing.
Never hash object addresses, RTTI names, `unordered_*` bucket order, render transforms,
localized text, widget selection, animation progress, wall-clock timestamps, log formatting,
or floating-point camera state.

Explicitly exclude the completed-batch publication queue drained by
`BattleSession::takeCommittedBatches()` and cursors owned only by outer observational or
presentation consumers. That queue contains immutable descriptors already represented by
the authoritative archive; it only controls when outer consumers animate/publish them and
cannot affect later simulation mutation. Draining it before or after digesting must produce
the same digest. The live taming evaluator is authoritative, not an outer consumer: its
processed cursor, `nextExpectedSequence`, battle identity, open-window IDs, and accumulated
window metrics are included. Do not confuse either cursor class with the
internal, not-yet-quiesced Taming derivation seam above. That delivery cursor is orchestration
scratch and is deliberately absent from `BattleSnapshot`/`battleSnapshotDigest`; material
taming tracker values remain in the snapshot.

## 3.2 Persistent-state digest

Add a separate canonical digest for result tests covering:

* save schema version, full-width world seed, and `generatorVersion`;
* exact `playerCell`, `lastHealPoint`, next-unconsumed `encounterSerial`, optional complete
  `lastCommittedResultToken`, and `nextCreatureSerial`;
* team slot order and storage order;
* each creature's stable ID, species/form derivation inputs, completed/progressed Feat nodes,
  and other persistent fields established in step 18;
* cleared encounter keys and other battle-result flags in sorted order.

Do not mix session-local HP/AP/MP/status/turn-bar values into the persistent digest. They
reset at the next battle according to step 18.

## 3.3 Canonical bytes and hash

Do not feed formatted C++ text or `std::hash` into checked-in digests. Define digest schema
version 1 as a byte stream with:

* the matching ASCII domain prefix `pg-battle-progress`, `pg-battle-snapshot`,
  `pg-battle-state`, `pg-player-state`, `pg-battle-archive`, or `pg-battle-event`, followed
  by a zero byte and a little-endian `uint32` digest-schema version;
* fixed-width signed/unsigned integers in little-endian two's-complement representation;
* booleans as exactly `0x00`/`0x01`;
* enums through explicit versioned `uint32` codes, never compiler underlying values;
* UTF-8 strings as little-endian `uint32` byte length followed by exact bytes;
* optionals as one presence byte followed by the value; sequences as a `uint32` count then
  canonical values; no native padding or object-memory dumps;
* fixed-point ticks and integer cell coordinates directly; no locale text or float.

Hash the completed byte stream with the existing
`pg::deterministic::fnv1a64::hash(std::string_view)` algorithm and render the `uint64`
result as exactly 16 lowercase hexadecimal digits. Do not add the avalanche step or silently
change algorithms. Golden-test at least one hand-constructed byte stream. Any encoding,
field-order, enum-code, or algorithm change increments the digest schema version.

## 3.4 Digest contract tests

Prove that:

* semantically equal states built through different insertion orders hash equally;
* changing every authoritative field changes either the digest or a documented enclosing
  value that does;
* presentation-only changes never alter the battle digest;
* digest generation itself does not mutate state or advance RNG;
* changing only a live evaluator's delivery cursor changes
  `authoritativeBattleStateDigest` but not `gameplayProgressDigest`; changing its material
  taming advancement/completion/failure changes both;
* a zero-cost cast that matches no condition and changes only event delivery cursors leaves
  `gameplayProgressDigest` unchanged, while an otherwise identical cast that advances a
  taming condition changes it;
* a zero-cost applied-no-change AI cast increments generic/AI command counts and changes the
  full digest but not `gameplayProgressDigest`, so the next plan detects the repeated
  material state; initializing/appending/clearing the guard never breaks adjacent batch
  snapshot continuity;
* changing only a baseline/effective stat (for example a strength-only Feat reward or a live
  armor modifier) changes the appropriate snapshot/full digest even when current pools do
  not change;
* changing a live board's `worldAnchor`, one required chunk coordinate, or one captured
  content revision changes the material/full digest and causes the expected terrain-current
  comparison behavior; reconstructing an equivalent frozen live fixture produces the same
  digests;
* a handcrafted board carries explicit absence of live terrain provenance, and changing
  only a presentation renderer transform changes no simulation digest;
* changing the digest schema requires changing its explicit version.

The digest is a diagnostic/equality contract, not save serialization and not a network
protocol.

## 3.5 Incremental history folds

Never rescan the complete growing event/batch history for every replay checkpoint. Maintain
one versioned FNV-1a accumulator for the canonical archive byte stream and one for the event
stream. Initialize each once with its domain prefix/version; on commit append the next
length-delimited canonical record bytes to the accumulator exactly once, then increment its
checked record count. `authoritativeBattleStateDigest` hashes those counts and current tips,
not the whole archive again. The archive remains stored for transcript/result/replay output.

Tests rebuild each accumulator from the complete stored archive/event log in scratch and
require equality with the incremental tip after every batch kind. Rejected commands,
presentation-queue drains, and outer cursor changes advance neither fold. This makes a full
battle's history hashing O(total history bytes), not O(history squared).

---

# 4. Deterministic replay fixture

Add a value-owned replay DTO in the headless battle/test layer. It is not a content authoring
tool and must not become a second game API.

```cpp
struct ReplaySubmission
{
    BattleCommand command;
    CommandIssuer issuer;
};

struct ReplaySubmissionResult
{
    enum class Disposition { Accepted, Rejected, Aborted } disposition;
    std::optional<CommandRejection> rejection;
    std::optional<BattleAbortReason> abortReason;
};

struct ReplayLiveWorldFixtureSource
{
    std::string fixtureId; // versioned test-owned frozen live-world recipe
};

struct ReplayHandcraftedBoardSource
{
    std::string definitionId;  // production battle-board registry ID
    std::string contentDigest; // canonical board + expanded prefab content
};

using ReplayBoardSource = std::variant<
    ReplayLiveWorldFixtureSource,
    ReplayHandcraftedBoardSource>;

struct ReplayBattleConstruction
{
    BattleDescriptor descriptor;
    ReplayBoardSource boardSource;
    std::vector<BattleParticipantSeed> participants; // complete canonical Player/Enemy order
    std::vector<ParticipantFeatSnapshot> playerFeatSnapshots;
};

struct ReplaySubmitOperation
{
    ReplaySubmission submission; // player/debug/explicit invalid submission
};

struct ReplayPumpOperation
{
    // Run the production BattlePump until player input or terminal. AI planning is replaced
    // only by this recorded command source; every value still traverses the real gateway.
    std::vector<ReplaySubmission> recordedAutomaticCommands;
    std::uint32_t maximumInternalTransitions;
};

using ReplayOperation = std::variant<ReplaySubmitOperation, ReplayPumpOperation>;

struct ReplayBatchCheckpoint
{
    BattleBatchId id;
    BattleBatchKind kind;
    std::optional<BattleActionId> action;
    std::vector<std::string> eventDigests;
    std::string afterSnapshotDigest;
};

struct ReplayCheckpoint
{
    // One entry for Submit; zero or more for the recorded commands consumed inside Pump.
    std::vector<ReplaySubmissionResult> submissionResults;
    std::uint64_t rngDrawCount = 0;
    std::vector<ReplayBatchCheckpoint> newBatches;
    std::string stateDigest;
    BattlePhase phase;
    std::optional<BattleUnitId> activeUnit;
};

struct BattleReplay
{
    std::string id;
    std::uint32_t schemaVersion;
    ReplayBattleConstruction construction;
    std::vector<ReplayOperation> operations;
    std::vector<ReplayCheckpoint> checkpoints; // exactly one per operation
    BattleOutcome expectedOutcome;
    std::string expectedFinalStateDigest;
};
```

These names deliberately reuse the canonical step-06/17 value types. For each
`ReplaySubmissionResult`, Accepted requires both optionals null, Rejected requires only the
exact gateway `CommandRejection`, and Aborted requires only the exact
`BattleAbortReason`; action/batch IDs are verified by the batch checkpoint.
An Accepted submission may be followed in that same operation by a no-action TechnicalAbort
batch when a post-commit derived consumer fails: the checkpoint records the accepted source
batch then the abort tail and Terminal phase. Aborted disposition remains exclusive to a
pre-commit command failure whose requested action batch never committed.
Do not invent a parallel validation-code enum. The replay contains definition IDs, exact
participant projection seeds, frozen active-Feat snapshots, and value commands, not
pointers or a serialized mutable `BattleContext`. `ReplayLiveWorldFixtureSource` resolves a
versioned, test-owned frozen recipe containing the live anchor, complete required chunk/
voxel layers, captured content revisions, named cells, and deployment expectations. The
replay harness owns a minimal test `VoxelWorld`/required active chunks for the entire session,
populates them through existing test seams, then calls the normal `WorldCellSource` live-board
builder and terrain-stamp path. It does not use `GridCellSource`, label an owned synthetic
grid LiveWorld, or serialize mutable production chunks. The resulting BoardData and
`BattleDescriptor` both carry genuine LiveWorld provenance.

`ReplayHandcraftedBoardSource` instead resolves the production battle-board registry and
materializes its referenced production prefab through the normal handcrafted builder. Its
`contentDigest` is a versioned canonical digest of the board definition fields plus the
expanded prefab's ordered local cell positions/voxel IDs (not raw JSON bytes); mismatch
fails before participant/session construction. Its ID and the descriptor's Handcrafted
source must agree. Neither alternative accepts pointers or a prebuilt mutable BoardData,
and unknown IDs/schema versions/digests fail contextually.

`Submit` records player/debug placement and activation commands plus deliberate invalid
submissions. `Pump` is an explicit temporal operation: it calls the production
`BattlePump::advanceUntilPlayerInput`/equivalent, advancing no-command scheduler boundaries,
activation starts, timeline expiries, automatic ends, and any Taming system batches. During
replay, inject a `RecordedAiCommandSource` in place of only the planner's decision function;
the pump consumes the operation's AI-origin values one by one through the normal gateway.
It errors if it needs an unrecorded command or leaves one unused. Separate planner goldens
prove the live ordered-rule planner chooses those values. Never both regenerate and replay
an AI command.

The checkpoint captures every committed batch since the preceding operation, including
no-action timeline/activation/Taming batches, in order. An operation that legitimately
changes nothing records an empty batch vector. This makes the next submitted command legal
for the same reason it was legal during recording; replay must not jump directly from one
command to another while omitting pump state.

Provide a test-only JSON reader/writer or checked-in C++ fixture representation. If JSON is
used, keep it in `Playground/tests/resources/battle_replays/`, version it, parse strictly,
and clearly distinguish it from game-authored resources. Do not expose replay fields in the
production registry set. Encode `descriptor.combatSeed` and every other full-width unsigned 64-bit
value as a canonical decimal string in JSON, then range-check it on parse.

Require `checkpoints.size() == operations.size()`. Each checkpoint stores the exact
post-operation facts listed below; final-only expectations cannot diagnose or verify an
earlier divergence.

Replay assertions must compare after each operation:

1. ordered Accepted/Rejected/Aborted dispositions for every gateway submission made by that
   operation, exact `CommandRejection` only for Rejected, exact `BattleAbortReason` only for
   Aborted, and no action ID for Aborted;
2. RNG draw count;
3. ordered newly committed batches, explicit batch kinds, and typed events/after digests;
4. state digest;
5. phase/active-unit transition.

On mismatch, print the replay ID, operation index/value, internal command index when
applicable, first divergent batch/event, old
and new digest, simulated time, phase, and active unit. A bare `expected != actual` is not
an actionable diagnostic.

Check in these single-session replays using a versioned `showcase-v1` frozen-live-world
fixture plus the
real step-19 encounter/team definitions except where explicitly stated:

* `placement-and-first-activation`;
* `status-trap-displacement`;
* `impressed-wild-result`;
* `handcrafted-gym-first-activation`, resolving production
  `verdant-trial-arena` and its checked content digest rather than a fixture board;
* `simultaneous-terminal-edge`, using a test-owned synthetic effect/session fixture from the
  step-09 outcome goldens because shipped step-19 effects cannot reduce the final active
  units on both sides within one resolution boundary. Keep it outside production registries.

Result commit, save/reload, and “reward visible next battle” cross the BattleSession
boundary and therefore do not belong in `BattleReplay`. Add a separate test-owned scenario
DTO:

```cpp
struct RunBattleReplayStep { std::string replayId; };
struct CommitPendingResultStep { BattleOutcome expectedOutcome; };
struct ReloadActiveSlotStep {};
using PersistentScenarioStep = std::variant<RunBattleReplayStep,
                                            CommitPendingResultStep,
                                            ReloadActiveSlotStep>;
struct PersistentBattleScenario
{
    std::string id;
    PlayerData initialPlayer;
    std::vector<PersistentScenarioStep> steps;
    std::vector<std::string> expectedPersistentDigestAfterEachStep;
};
```

The scenario harness uses the real step-18 outcome/save services and feeds current
PlayerData into subsequent battle construction; it does not serialize a BattleContext.
Check in `player-loss-progress-commit` and `feat-reward-next-battle` scenarios, with the
latter running battle -> commit -> reload -> next battle and proving the newly derived
reward is absent before commit and present in that next session.

---

# 5. Central invariant auditor

Create a debug/test-only `BattleInvariantAuditor` that reads public/explicit friend test
state and returns structured violations. Do not scatter assertions that abort production
builds or expose mutable internals solely for tests.

Run it after session construction, every accepted command/effect boundary, scheduler
transition, result construction, and replay step in tests. Check at minimum:

## 5.1 Identity and ownership

* all unit, status-instance, shield, battle-object, event, and command IDs are unique where
  their contract requires uniqueness;
* every stored source/target reference is a stable ID with a defined missing-source policy;
* player projections link to at most one persistent creature and wild units link to none;
* definition pointers/references, where allowed, resolve into registries that outlive the
  session.

## 5.2 Board and occupancy

* every active on-board unit occupies exactly one standable cell;
* each cell has at most one unit; occupancy and unit position agree both ways;
* removed/defeated/impressed units are absent from occupancy, traversal blockers, scheduler,
  AI candidate lists, and target queries;
* battle objects refer to existing board cells and respect their stacking/uniqueness policy;
* paths contain four-neighbor legal transitions and their recomputed cost matches the
  accepted command.

## 5.3 Numeric state

* maximum HP/AP/MP are positive where required and effective maxima obey modifier clamps;
* current HP/AP/MP and shield values are within documented ranges;
* fixed-point times/durations/fill are finite representable integers and never overflow;
* no negative damage/healing is smuggled through an unsigned conversion;
* status stack counts and remaining durations satisfy their definitions;
* RNG draw count only changes at documented selection points.

## 5.4 Phase and terminal state

* phase-required fields are present and phase-forbidden fields are absent;
* only the active unit may submit activation commands;
* AP/MP reset occurs exactly once at activation start, not at battle start/end;
* a terminal session accepts no further gameplay command and commits results at most once;
* scheduler membership equals active combatants able to remain in the fight;
* a battle cannot be both terminal and waiting indefinitely for an activation.

## 5.5 Event consistency

* event sequence numbers are strictly increasing and never reused;
* applied amounts reflect actual clamped/mitigated mutation, not requested values;
* each accepted mutation has its required event and invalid commands have none;
* defeat/impressed/removal and outcome events follow the locked ordering;
* event consumers cannot mutate or clear authoritative history unexpectedly.

Return all violations found in one audit pass where practical. Include stable IDs and cells
in diagnostic text.

---

# 6. Close the interaction test matrix

Review all earlier tests and add missing cross-system cases. Prefer small table-driven tests
over a single enormous fixture, then cover the full loops with replay/end-to-end tests.

## 6.1 Commands and atomicity

For placement, move, cast, and end-turn commands, cover:

* wrong phase, wrong active unit, removed unit, unknown ID, unowned ability;
* insufficient AP, insufficient MP, and both insufficient;
* a stale destination/preview whose recomputed canonical path is now blocked or exceeds
  current MP; `MoveCommand` never carries a caller-supplied path;
* target outside min/max range, invalid elevation-independent range, blocked 3D LoS;
* invalid anchor affiliation versus invalid affected-target affiliation;
* empty AoE, duplicate target capture, dead target, and target removed during an earlier
  effect in the same captured cast;
* zero-cost ability and zero-MP move cannot create a no-state-change infinite activation;
* invalid commands leave state digest, RNG, event count, resources, and scheduler unchanged;
* a pre-commit technical invariant failure returns Aborted rather than Rejected, rolls back
  every staged command mutation/event/scratch ID, commits only a no-action `TechnicalAbort`
  batch from the last trustworthy state, and a later submission is Rejected with
  `BattleTerminal`;
* a post-commit Taming/derived evaluator failure keeps and reports the source command as
  Accepted, appends the separately checkpointed no-action TechnicalAbort tail, exposes a
  terminal pump/session, and never partially advances tracker cursors/material state;
* accepted casts pay once even with multiple affected cells/targets and effects execute at
  their declared `sourceUnit`, `primaryUnit`, `affectedUnits`, `anchorCell`, or
  `affectedCells` scope.

## 6.2 Timeline and phase edges

Cover:

* bars start empty and lower stamina seconds fill sooner;
* exact ties use player side, encounter roster order, then `BattleUnitId`;
* very large simulated jumps stop exactly at the next event instead of overshooting;
* all bars temporarily unable to progress produces a diagnosed terminal/invalid-content
  path, never a hang;
* daze/stun pauses bar growth as defined and timeline duration still expires;
* AP/MP current penalties versus next-activation penalties occur at distinct times;
* defeat during an activation, active-unit displacement, and no legal commands;
* already captured effects continue after source defeat only under the step 09 policy;
* simultaneous final removals resolve to the one documented victory/loss/draw policy.

## 6.3 Effects, hooks, and credit

Cover:

* physical/magical mitigation boundaries, rounding halves, zero defense, large defense;
* shield channel matching, multiple shields in ascending `BattleShieldId` (oldest-first)
  order, overflow to HP;
* damage/healing condition credit uses actual HP/shield changes, not requested amounts or
  overheal;
* DOT source defeated/removed before tick; HOT at full HP; cleanse before scheduled hook;
* refresh/replace/stack policy at exact limit and stable instance ordering;
* status modifier decreases effective maxima and clamps current pools exactly once; later
  maximum increases do not heal, refill, or restore those pools;
* movement and displacement both call enter hooks; failed displacement calls neither enter
  nor leave success hooks;
* one-charge trap triggers once, is removed in order, and cannot trigger on deployment;
* push into invalid, occupied, missing, too-high, and trap cells;
* no v1 payload/command can revive a removed unit; unknown revive-like data is rejected and
  no alive off-board unit can be fabricated.

## 6.4 AI

Cover every selector and action type used by shipped profiles:

* candidate generation uses the validator, not a looser approximation;
* equal scores use the explicit definition/cell/unit ordering; v1 AI schema has no random
  tie-breaking mode and consumes no RNG;
* AI re-evaluates after each mutation and never executes a cached illegal target;
* command cap, repeated-state digest guard, and end-turn fallback;
* target selectors ignore impressed/defeated units and respect side/profile filters;
* AI owns the named ability before casting it;
* no reachable action and all-zero-cost action sets both terminate safely.

## 6.5 Taming, progression, and persistence

Cover:

* condition window boundaries: ability, activation, fight, and persistent game totals;
* actual applied amount versus requested amount in every aggregate;
* tame completion and lethal damage in the same event batch use the locked defeat priority;
* impressed removal gives no defeat/kill progress and cannot act again;
* provisional recruit discarded on loss and committed exactly once on victory;
* a full team routes recruits to the unbounded PC-storage vector in deterministic order
  without partial save mutation;
* every confirmed non-empty roster creature participates, including defeated creatures, and
  can gain eligible progress; storage creatures and inactive nodes do not;
* nodes active at battle start consume events, newly unlocked nodes do not consume earlier
  events from that battle;
* losing commits eligible progress; reward availability changes only after result commit;
* exclusive branch ties use authored order; reload derives the same form/stats/loadout;
* save failure preserves the pre-result in-memory and on-disk state as specified;
* result accept/cancel/double-click cannot commit twice.

---

# 7. Strict resource failure suite

Build table-driven malformed-resource fixtures for every registry introduced in steps
02-04. Each load starts from a known published registry set, attempts to load an invalid
complete set into locals, and proves the old set remains published.

At minimum test:

* unreadable directory/file, invalid JSON, wrong root type, unknown field, missing required
  field, wrong scalar type, non-integral integer, invalid enum discriminator;
* empty filename ID, duplicate derived ID under platform case rules, invalid ID syntax;
* negative/zero/overflow stats, costs, ranges, durations, stack limits, weights, and fixed-
  point decimals according to each field's domain;
* unknown cross-reference in every category;
* missing/multiple roots, disconnected/asymmetric/self Feat edges, invalid exclusive
  groups, duplicate local node IDs, unsupported reward/condition combinations;
* effect discriminator with missing payload, forbidden payload, invalid execution scope, or
  incompatible target profile;
* resource condition with missing/empty/duplicate/unknown/action-incompatible reason
  filters;
* owner-turn stun duration, `maxTriggers:0` combined with `removeWhenExhausted:true`, AI
  without end-turn fallback, AI rule naming an unavailable ability, encounter roster above
  six;
* handcrafted-board unknown prefab, wrong version/field set, even or out-of-range X/Z,
  invalid Y/deployment depth/approach, checked volume failure, prefab cell outside declared
  bounds, insufficient standable deployment capacity, and encounter kind/board-policy
  mismatch (Wild/Trainer must liveWorld; Gym/Special must handcrafted; Debug may use either);
* every encounter kind/repeatability mismatch: Wild/Debug false and
  Trainer/Gym/Special true are all rejected transactionally;
* empty/invalid-reference taming profiles and the step-19 exact-roster reachability test;
* unknown/non-monotonic form rewards, duplicate reward IDs, and final ability union above
  capacity.

Also add positive regressions proving a connected cyclic Feat adjacency graph, an unlimited
non-exhausting trap, duplicate ability unlocks across distinct rewards, and equal positive
team weights remain valid and deterministic. These are documented features, not malformed
data.

Diagnostics must include registry category, filename, JSON path, offending value/ID, and
the unresolved referenced category where relevant. Snapshot only stable diagnostic codes or
meaningful substrings; avoid brittle full compiler/library exception text.

Run the shipped step-19 pack through the same validator. Test fixtures must not get a more
permissive path than production resources.

---

# 8. Headless soak and determinism campaign

Create a bounded test/benchmark driver around the normal `BattleSession`. It may be a test
helper or a development-only command-line mode, but it must not bypass placement, command
validation, effects, AI, events, result construction, or invariant checks.

## 8.1 Required campaign

In a local validation run, execute at least:

| Campaign | Count | Variation |
|---|---:|---|
| exact replay repeat | 10 per checked-in replay | none |
| wild starter vs Lumenwing | 1,000 | sequential deterministic seeds |
| trainer trio | 1,000 | seeds and deterministic legal deployments |
| handcrafted Gym trio | 1,000 | combat/team seeds; identical authored board geometry |
| AI mirror trio | 2,000 | seeds and both side assignments |
| synthetic status/trap fixture | 1,000 | seeds, legal target/cell tie cases |
| result/save/reload loop | 250 | win/loss/impressed/full-team variants |

The normal per-commit automated suite may run a documented smaller deterministic subset if
the full campaign exceeds the project's test-time budget. Register the full campaign under
a separate CTest label such as `playground-soak`; document and actually run it for this
step's sign-off.

## 8.2 Caps and failure artifacts

Each harness run must have explicit test-only caps, generous enough for real play:

* maximum accepted commands;
* maximum scheduler activations;
* maximum typed events;
* maximum same-state AI iterations;
* maximum simulated battle time.

These belong to `SoakLimits`, not `BattleGameRules`; v1 intentionally has no production
whole-battle event/activation/time cap. The harness stops driving and reports the still-
valid session without appending a fabricated `BattleAborted`/`BattleEnded`. Exceeding a cap
is a failed diagnostic result, not a fabricated victory. Emit seed,
encounter ID, roster/deployment, last commands/events, phase, active unit, RNG count, state
digest, and invariant report so the case is immediately reproducible.

## 8.3 Assertions

Across the campaign assert:

* every case terminates or fails explicitly within a cap—no hang;
* no invariant violation, exception escaping the harness, NaN/float-derived rule state,
  negative pool, duplicate occupancy, or stale scheduler member;
* replaying every failed or sampled seed yields the identical trace;
* two independently constructed sessions for a sample of every encounter yield identical
  per-command digests;
* two world seeds/ordinals resolving the same handcrafted board yield identical ordered
  board cells/source-content digest and zero board-builder RNG draws, even though their
  combat/team seeds and resulting combat may differ;
* destroying/reconstructing presentation has no effect on headless results;
* outcome distribution is reported for information but not constrained to a fabricated
  balance percentage.

Do not hide failures by increasing caps without analyzing the trace.

---

# 9. Live lifecycle stress

Add an integration harness at the `GameSceneWidget`/`ModeManager` boundary using the
smallest available engine test seam. If a window/OpenGL context is unavoidable, separate
the logic-level automated test from the controlled manual run.

## 9.1 Ownership counters

Expose debug snapshots, not mutable access, for:

* current game mode and pending transition count;
* active Battle runtime/session/presenter/view counts;
* lifecycle and event subscription/RAII contract counts owned by each component;
* battle entity/widget count;
* pinned/leased world region count;
* active isolated handcrafted-arena renderer/lease count and captured exploration-visibility
  state;
* player path length and exploration avatar renderer visibility;
* fluid pause lease count and streamer region/owner count;
* pending presentation-event queue size.

Prefer existing engine diagnostics where available. Do not add global counters that become
new ownership authorities.

## 9.2 Repeated transition matrix

Run at least 100 sequential cycles covering:

* debug battle victory;
* debug battle loss;
* Bush battle with impressed recruit;
* handcrafted Gym victory/loss and entry failure while materializing its arena;
* entry construction failure (bad board/insufficient deployment cells);
* exit requested while presentation still has animations queued;
* scene/widget destruction while Battle mode is active;
* rapid repeated F8/start requests in Exploration and Battle;
* result acceptance invoked twice;
* world reset/new game after a completed battle.

After each exit, compare the ownership snapshot to the pre-entry baseline, allowing only
documented persistent changes such as roster/progression/cleared state. Specifically prove:

* mode is Exploration and no transition remains queued;
* session/presenter/views/contracts/leases are gone;
* fluid simulation and exploration input are restored exactly once;
* avatar renderer and hover behavior are restored;
* the old click path is cleared and does not resume;
* for live-world battles, streaming remains alive, the region is unpinned, and terrain was
  neither copied nor reverted; for handcrafted battles, the isolated arena is destroyed and
  the previously captured exploration renderer/stream visibility is restored exactly;
* no callback can reach a destroyed session after an engine update.

Use AddressSanitizer or the platform's available memory diagnostics in a separate build if
the repository supports it. Do not claim leak freedom solely from stable visible behavior.

---

# 10. Performance and responsiveness budgets

Correct pathological work discovered by measurement, while preserving canonical output.
Do not hard-fail normal CI on tight wall-clock thresholds from unknown hardware.

## 10.1 Deterministic work budgets

Enforce algorithmic caps in tests:

* board radius/policy limits bound cell count before allocation;
* movement flood/path expansion visits each reachable board node no more than the
  documented algorithmic bound;
* target preview deduplicates cells/targets once and returns canonical order;
* AI evaluates no more than its configured rule/candidate/command caps;
* status/passive/object hook dispatch has a recursion/depth or event-expansion guard;
* presentation consumes a bounded number of queued events per frame without dropping the
  authoritative log;
* state digest/event snapshot construction is linear or documented `O(n log n)` for
  canonical sorting, never accidentally quadratic in event history.

## 10.2 Measured development targets

Add a non-gating benchmark report for a release-like build on the developer machine. For a
normal radius-6 board and at most 12 units, target:

* board construction below 50 ms;
* move/target preview below 5 ms per user query;
* one AI decision below 10 ms at the 95th percentile;
* accepted headless command resolution below 2 ms at the 95th percentile excluding file
  I/O and invariant auditing;
* no sustained gameplay-frame regression below 60 FPS caused by placeholder battle
  presentation on the reference scene.

Record hardware/build type, sample count, median, p95, and maximum. Treat these as diagnostic
targets: if missed, profile, fix clear regressions, and report the remainder. Do not weaken
rules, remove validation, use nondeterministic parallel resolution, or add flaky timing
assertions to satisfy a number.

## 10.3 Allocation/lifetime review

Inspect hot commands for avoidable full-registry copies, repeated JSON parsing, per-frame
board reconstruction, unbounded event copies, and widget/entity churn. Definitions load
once; board traversal data builds once per battle; presentation may cache immutable meshes.
Never cache a query whose invalidation depends on mutable occupancy/status state without an
explicit revision key.

---

# 11. Presentation reconciliation

Presentation remains a projection and may lag behind fast simulation. Harden this boundary:

* every lifecycle notice, visual/event item, and deferred callback carries the
  presentation-only `BattleRuntimeGeneration` plus `BattleId` and event sequence where
  needed;
* an item is accepted only when generation and BattleId both match the active presentation;
  an event for a destroyed/old battle is ignored safely;
* skipped/fast-forwarded animations apply the latest authoritative snapshot rather than
  leaving a unit between cells;
* terminal resolution waits only for a bounded cosmetic drain or lets the user skip it;
  gameplay never waits indefinitely for animation callbacks;
* a removed unit cannot remain selectable, block hover, or retain an ability panel;
* HUD selection is repaired when the active unit/target disappears;
* resizing and camera changes do not alter board picking or simulation;
* hidden exploration avatar continues owning streaming/fluid helpers as designed;
* no UI callback stores a raw reference into a session beyond its synchronous lifetime.

Add snapshot reconciliation tests for out-of-order duplicate presentation notices, battle
teardown with queued events, and immediate result skip. Force two consecutive presentation
runtimes to use the same folded `BattleId` and the same early event sequences through a test
seam, but distinct generations. Queue callbacks/publications from the first, tear it down,
enter the second, then release the old queue; every old item must be ignored and every new
item accepted once. Assert generation never enters simulation/result/replay/save digests.
These tests assert presentation state only; they must not modify authoritative event ordering
to make animations easier.

---

# 12. Controlled manual sign-off

Automated tests cannot prove readability, picking, camera comfort, or visible teardown.
Perform and record the following with a real development executable and the shipped
resources.

## 12.1 Exploration and entry

* boot a new game from the documented command;
* walk to a Bush and enter a live-world battle;
* verify the same terrain/chunks/light remain visible, fluid is paused, and the old path/
  hover/avatar presentation is suppressed appropriately;
* launch `gym-verdant-trial`, verify the exploration render is hidden and the registered
  isolated arena is visible/pickable through the same scene/camera, then return and verify
  the exact captured exploration visibility/stream state is restored;
* force one invalid-board entry and verify exploration remains fully usable.

## 12.2 Placement and control

* place and repeatedly reposition/swap every mandatory roster unit without marking any as
  having left battle; confirmation remains disabled until all are placed;
* test mouse hover/click and keyboard cancel/end-turn across cells at different elevations;
* verify the eight locked deployment/movement/path/ability-range/AoE/LoS-blocked/
  hover-or-selection/invalid masks are distinct and match command validation;
* verify camera pan/orbit/zoom and window resizing do not shift picking.

## 12.3 Battle readability

* observe AP/MP refill, turn bars, tie order, statuses/durations, shield and HP changes;
* trigger damage, healing, DOT/HOT, cleanse, fortify, daze, trap, push, blocked push,
  impressed removal, defeat, and AI decisions;
* use animation skip/fast-forward and verify final views snap correctly;
* confirm typed feedback reports actual applied values.

## 12.4 Complete result loops

* win and recruit Lumenwing, then save/reload;
* lose with a deployed defeated creature and confirm progress plus heal-point return;
* unlock/evolve and verify the reward only in the next battle;
* win the non-repeatable trainer battle, return to exploration, then attempt its named
  launch again and verify it is suppressed with a cleared diagnostic, consumes no ordinal/
  RNG, and queues no transition; use `debug-content-showcase` for repeatable reproduction;
* win `gym-verdant-trial`, verify the same clear/suppression rule and world-cell return, and
  repeat it in a copied fresh player state under a different world seed to verify identical
  arena geometry/deployment/picking;
* close/reset the scene while a battle is active and watch for errors or stale callbacks.

Capture concise notes and, if the project workflow supports it, screenshots/log traces for
failures. Never mark a visual item passed because its headless counterpart passed.

---

# 13. Diagnostics and failure UX

Make development failures actionable without exposing internal exceptions to normal game
flow.

* Resource-load failure reports the full contextual diagnostic and prevents boot/publication
  according to the existing startup policy.
* Battle-entry failure emits `EntryFailed`, releases every acquired lease, restores
  exploration, and shows/logs a short reason plus stable diagnostic code.
* Invalid player commands provide a typed reason suitable for HUD feedback; expected invalid
  previews do not spam error logs.
* AI invalid-command bugs include AI profile/rule/unit/command and terminate that activation
  safely after the established retry policy.
* Safety-cap/invariant/replay failures print the reproduction payload described above.
* Save/result failure leaves the result screen in a retryable state and does not pretend the
  commit succeeded.

Separate user-facing messages from developer detail, but preserve both. Do not catch all
exceptions and continue with partially mutated state.

---

# 14. Build, test, and configuration matrix

Run from a clean configure, not only an incremental build. At minimum:

```powershell
cmake --preset test
cmake --build build/test --target PlaygroundTests SparklePlayground
ctest --test-dir build/test --output-on-failure -L playground
ctest --test-dir build/test --output-on-failure -L playground-soak
./build/test/Playground/SparklePlayground.exe --resource-path Playground/resources
```

Also build the project's normal development preset used in `Playground/docs/README.md`.
If multiple compilers/configurations are already supported by CI, run or preserve them; do
not invent unsupported platform claims.

Check:

* clean resource root and an intentionally missing/invalid resource root;
* Debug/test configuration with invariant auditing;
* release-like configuration for performance sampling;
* deterministic tests at least twice in separate processes;
* full existing Sparkle/Playground tests, not only newly filtered battle tests.

Fix preset/documentation drift. Tests must actually be enabled in the preset claimed by the
docs, and CTest labels must discover both the normal and optional soak groups. A zero-test
CTest run is a failure even if its exit code is zero.

---

# 15. Documentation reconciliation

Audit, update, or clearly mark stale every battle-related claim in:

* `Playground/docs/README.md`;
* `Playground/ERELIA.md`;
* `Playground/docs/01-architecture.md`;
* `Playground/docs/02-data-model.md`;
* `Playground/docs/03-systems/board.md`;
* `Playground/docs/03-systems/ui-hud.md`;
* `Playground/docs/decision-log.md` where a new locked choice needs a durable record;
* the battle content/schema documents created in prior steps.

Do not rewrite the GDD as if every future feature exists. Distinguish:

* implemented vertical-slice behavior;
* reference JSON content that is intentionally provisional;
* explicit non-goals/limitations;
* future resource-authoring workflow.

The final docs must include:

1. exact configure/build/test/run commands;
2. executable flow and controls for exploration, placement, targeting, end turn, result,
   and named debug encounter;
3. module ownership diagram showing definition, persistent, session, and presentation
   lifetimes;
4. registry directories, filename-ID rule, strict validation, and cross-reference phase;
5. deterministic scheduler/command/event contracts and state-digest version;
6. save/result boundary and the absence of mid-battle save;
7. shipped content ID manifest and manual extension example;
8. how to run normal tests, full soak, and replay a reported seed/fixture;
9. known limitations: placeholder art, no authoring tools, no final balance, no trainer NPC,
   and any honestly unresolved visual/performance issue.

Use Mermaid only if the existing docs renderer supports it; otherwise use compact text
diagrams. Ensure copied JSON examples are valid against the real loader.

---

# 16. Cleanup and code-quality audit

Before sign-off:

* remove obsolete fixture IDs/resources superseded by the step-19 pack unless a test still
  clearly owns them;
* remove temporary debug mutation paths, duplicate command helpers, hidden autoplay
  shortcuts, TODOs that describe already implemented behavior, and dead historical battle
  code;
* retain the named debug encounter and bounded diagnostics needed to reproduce behavior;
* confirm all new game code is under `Playground/srcs` and namespace `pg`, not `spk`;
* inspect includes and CMake source lists for accidental Unity paths or absolute developer
  paths;
* use value IDs and RAII ownership; search for raw owning pointers, untracked subscriptions,
  `random_device`, wall-clock rule time, and unordered iteration in deterministic paths;
* keep test-only helpers out of the production executable unless deliberately exposed as a
  development diagnostic;
* format according to repository style without mechanically rewriting unrelated user code.

Useful searches should include terms for old temporary types, direct HP/AP/MP setters from
widgets/AI, hard-coded content IDs in C++, and `TODO`/`FIXME` in the new battle directories.
Review every match; do not blindly delete unrelated work.

---

# 17. Expected files

Exact names depend on earlier steps, but expect focused changes/additions such as:

* `Playground/srcs/battle/battle_state_digest.*`
* `Playground/srcs/battle/battle_invariant_auditor.*` if test/debug code shares the module,
  or the equivalent under `Playground/tests/support/`;
* `Playground/tests/support/battle_replay.*`
* `Playground/tests/support/persistent_battle_scenario.*`
* `Playground/tests/resources/battle_replays/*` if JSON replay fixtures are chosen;
* focused cross-system, content failure, replay, lifecycle, and soak test sources;
* CMake/CTest registration and labels;
* small generic bug fixes in the modules that own demonstrated failures;
* current Playground architecture/data/board/UI/readme documentation.

Do not centralize unrelated systems into a giant `battle_manager.cpp` during cleanup. Fix a
bug in the module that owns its invariant and add the narrow regression test there.

---

# 18. Non-goals

Do not add:

* resource editors, exporters, schema generators, hot reload, or Unity conversion;
* more species, abilities, encounters, story, trainer NPCs, or production balance;
* final models, animation rigs, VFX, sound, music, icons, or localization;
* multiplayer, rollback networking, a public replay product, or cross-version replay
  compatibility;
* mid-battle saving/resume;
* new combat mechanics such as levels, XP, items, elements, crits, accuracy, equipment,
  summons, reserves, or fleeing;
* engine-wide refactors or promotion of Playground gameplay into `spk::`;
* strict CI wall-clock assertions tied to one developer machine.

If hardening uncovers a missing required rule, implement the smallest generic correction in
its owning module and update the relevant earlier contract documentation. Do not disguise
feature expansion as a bug fix.

---

# 19. Final acceptance checklist

The 20-step battle implementation series is complete only when all statements are true:

## Architecture and data

* [ ] Definitions, persistent state, battle session, and presentation have distinct tested
      ownership/lifetimes.
* [ ] All shipped JSON loads strictly and transactionally; malformed sets cannot partially
      publish.
* [ ] No production C++ rule names a vertical-slice content ID.
* [ ] Player, AI, debug, tests, and replay use one command gateway and typed event vocabulary.

## Rules and determinism

* [ ] Canonical digest coverage and insertion-order tests pass.
* [ ] Every checked-in replay matches validation, RNG, event, phase, outcome, and digest at
      every command boundary.
* [ ] All command/effect/timeline/status/trap/AI interaction matrices pass.
* [ ] Invalid commands mutate nothing and advance neither RNG nor gameplay events.
* [ ] Full soak counts were actually run, all cases terminated within caps, and any failure
      artifact reproduced identically.

## Game loop

* [ ] Bush and every named entry selecting `liveWorld` are transactional frozen overlays,
      while Gym/Special (and any Debug selecting `handcrafted`) transactionally use the
      registered isolated arena in the same scene; both paths restore exploration exactly.
* [ ] Manual deployment, tactical input, masks, camera, views, HUD, AI, result, and teardown
      form a playable loop.
* [ ] Wild taming/recruit, trainer win, loss progress, evolution reward, save, and reload all
      pass end-to-end with the shipped pack.
* [ ] One hundred repeated lifecycle cycles return ownership/control/leases to baseline.
* [ ] Result state commits at most once and failure remains retryable/atomic.

## Quality and handoff

* [ ] Normal clean build, complete Playground tests, optional full soak, and release-like
      performance report were run with exact results recorded.
* [ ] Manual sign-off records every requested scenario as passed, failed, or not run—never
      inferred from tests.
* [ ] Performance work budgets hold and measured misses are profiled/documented rather than
      hidden by flaky thresholds.
* [ ] Current docs match the executable, JSON examples, controls, commands, and limitations.
* [ ] The tree contains no accidental absolute Unity path, stale temporary battle path, or
      resource-authoring tool.

---

# 20. Final implementation handoff

Lead the final report with what can now be played. Then include:

1. final module/resource manifest and noteworthy generic fixes;
2. digest/replay schema versions and checked-in replay IDs;
3. exact build/test/CTest/soak commands and counts, durations, pass/fail results;
4. any failed seed and its resolution;
5. lifecycle cycle count and before/after ownership snapshot;
6. performance build/hardware/sample table with median/p95/max;
7. manual scenario checklist with honest visual results;
8. save compatibility consequence and shipped content IDs;
9. remaining limitations assigned to the later resource-authoring/art phase.

Do not say “battle complete” if a required check was skipped or failing. State the precise
remaining blocker. If every item passes, this report is the handoff point for creating
resource-authoring tools and expanding content without redesigning the battle runtime.
