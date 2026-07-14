# Implement battle-result transactions, persistence, and loss recovery

Freeze a terminal headless battle's copied record/transcript/result inputs into one immutable
`BattleResult`, then turn it into one atomic persistent-player update: retain Feat Progress
after every completed battle, recruit impressed creatures only after victory,
clear Trainer/Gym/Special encounters only after victory, write a versioned save safely, present a
complete result summary, and return the player either to the encounter cell or the last heal
point.

This is an implementation task. Add the result transaction and the minimum battle-relevant
save/load path. Do not implement mid-battle saving or a complete world-plan serializer.

---

# 1. Repository baseline

Implement after steps 01-17 and re-read the live types before editing.

Required prior contracts:

* `GameContext`/`PlayerData` own the world seed, player world cell, last heal point, six-slot
  team, PC storage, cleared encounter IDs, and deterministic counters introduced by earlier
  steps;
* `BattleTerminalRecord`, `BattleConditionTranscript`, immutable descriptor/participant
  snapshots, impressed-unit summaries, and provisional-recruit inputs remain owned and
  copied after the step-16 derived-batch fixed point; this step validates and combines them
  into the full `BattleResult` exactly once;
* step 17 can evaluate pure `CreatureProgressionDelta` values without mutating PlayerData;
* step 16 never adds a recruit directly to the roster;
* BattleMode keeps the exploration world/session alive until the user confirms a successfully
  committed result;
* mode changes and teleports are queued until the safe post-engine-update boundary.

There is no reliable current save service in Playground. Build the narrow service specified
here in `pg::`; do not import Unity's `Newtonsoft.Json`, GUID registry, static service
locator, or stale save format.

---

# 2. Required final behavior

At a terminal result:

1. After the authoritative command/timeline/Taming derived-batch queue reaches quiescence,
   freeze further battle/log mutation and construct the immutable result immediately.
2. Resolve the exact standable return cell and build every progression/recruit/clear/return
   change against a copy of `PlayerData`, including that cell.
3. Validate the complete candidate state.
4. If autosave is enabled, serialize and durably replace the slot file.
5. Only after successful validation/write, publish the copied state as the new PlayerData.
6. Publish value-owned result-domain notifications and populate the result screen while any
   already queued cosmetic battle presentation finishes.
7. Wait for the presentation gate and then Continue; cosmetic completion never delays or
   changes result preparation/commit.
8. Tear down battle presentation/session in the safe order.
9. Restore exploration; on defeat/draw, queue a teleport to the last heal point.

No subsystem may observe a roster where progress is applied but recruits/clears are not, or
a recruit whose derived state was not built.

If save writing fails, keep the result screen and old PlayerData intact, show an actionable
error, and allow retry. An explicit development-only "continue without saving" path may
publish the already validated candidate to memory, mark the run dirty/unsaved, and warn
that closing the process will lose the result; it must never silently discard progress or
pretend the slot was written.

---

## 2.1 Pre-encounter checkpoint boundary

Complete the pre-battle half of the README's save-boundary rule by integrating step 12's
resolver with this step's `SaveService`. `PlayerData.encounterSerial` is the next
unconsumed ordinal. One `EncounterResolutionCheckpoint` owns:

```cpp
struct EncounterResolutionCheckpoint
{
    std::uint64_t consumedOrdinal;
    PlayerData candidatePlayerData;              // serial advanced exactly once
    std::optional<BattleStartRequest> request;   // absent when Bush chance failed
};
```

For every resolved Bush arrival and accepted named-debug request:

1. Copy current PlayerData and the actor's exact current support cell into
   `candidatePlayerData.playerCell`.
2. Read `encounterSerial` as the attempt ordinal and checked-increment it on the copy.
3. Perform the one immutable encounter-resolution attempt with that ordinal and construct
   the optional copied request; do not publish the copy yet.
4. Validate and, when autosave is enabled, durably write the candidate to the configured
   active slot.
5. Publish candidate PlayerData only after the write succeeds.
6. If chance failed, clear the checkpoint and resume Exploration. If it succeeded, only
   now queue/stage `BattleMode` construction using the already resolved request.

A failed Bush chance therefore consumes and checkpoints its ordinal too. A BattleMode entry
failure after this checkpoint does not refund it. Retrying an in-memory entry reuses the
copied `ResolvedEncounter` and `combatSeed`; it never resolves again. A crash during battle
reloads Exploration at the checkpointed cell with the ordinal already consumed—there is no
mid-battle resume and no reroll of that encounter instance.

On validation/write failure, leave PlayerData and the old slot byte-for-byte unchanged,
remain in Exploration, freeze additional encounter-producing movement/input, and retain the
checkpoint for retry without RNG calls. The ordinary game path requires successful
checkpoint persistence before battle entry. A development-only continue-unsaved override
may publish the candidate and mark the run dirty, but must explicitly warn that crash
determinism is forfeited. With autosave globally disabled, the same transaction commits to
memory and carries that same dirty limitation.

`SaveService`/`GameContext` owns one validated configured `SaveSlotId activeSlot` (use
`autosave` as the bootstrap default); every pre-encounter and post-result write in this
step uses it. Do not infer a slot from display text or add save-slot UI here.

---

# 3. Battle outcome policy

Lock the outcome consequences to this table:

| Result | Existing participants' Feat Progress | Impressed recruits | Clear trainer/gym/special ID | Return world cell |
|---|---|---|---|---|
| PlayerVictory | apply | commit | yes for Trainer/Gym/Special | encounter return cell |
| PlayerDefeat | apply | forfeit | no | last heal point |
| Draw | apply | forfeit | no | last heal point |
| Aborted/error | none | forfeit | no | encounter return cell after cleanup/error handling |

Wild random encounters and Debug definitions are repeatable and do not add a cleared ID.
Trainer/Gym/Special definitions carry stable encounter IDs and are schema-locked
`repeatable:false` in step 04/12.

Battle HP, AP, MP, temporary statuses, shields, traps, and turn-bar fill are session-local
and are not copied back to `CreatureUnit` or saved. The next battle projects full base
health/resources. This plan intentionally has no between-battle injury attrition because
the GDD's required save list does not include current combat resources. A later design may
add it as an explicit migration.

The last heal point still matters as the world-position recovery destination on loss.

---

# 4. Transaction model

Define result identity without using the 32-bit encounter-local `BattleId`:

```cpp
struct BattleResultToken
{
    std::uint64_t worldSeed;
    std::uint64_t encounterOrdinal;
    std::string stableEncounterInstanceId;
    auto operator<=>(const BattleResultToken &) const = default;
};
```

Step 12 already preserves the three source fields: `GameContext.worldSeed`, the consumed
ordinal, and `stableEncounterInstanceId`. In this step, add `BattleResultToken resultToken`
to `BattleStartRequest` and `BattleDescriptor`; construct it exactly once in the
battle-entry transaction from those source fields, validate it against the descriptor, and
return it unchanged in the terminal `BattleResult`. Its ordinal must be strictly below
`PlayerData.encounterSerial`. It is collision-free within a run by the monotonic ordinal
contract; do not reduce it to a hash or `BattleId`, and do not rebuild it at finalization.

Add `std::optional<BattleResultToken> lastCommittedResultToken` to PlayerData/save. Results
are committed sequentially because only one battle may exist. `prepare` rejects an older
ordinal as stale, rejects an exactly equal token as already consumed, and treats an equal
ordinal with different seed/identity as a hard invariant error. The active
`BattleModeRuntime` owns the one pending result/token until successful commit or teardown.
Persisting the complete last token preserves that distinction across crash/reload without
an unbounded token set.

Consolidate the final value-owned result shape here; do not let steps 12/16/17 invent
parallel result bags:

```cpp
struct BattleParticipantResult
{
    BattleUnitId battleUnitId;
    BattleSide side;
    std::uint32_t rosterOrder;
    std::optional<CreatureInstanceId> persistentCreatureId; // player only
    std::string speciesId;
    std::optional<std::string> encounterSpawnMemberId;      // enemy only
    std::optional<RemovalReason> removalReason;              // null when still active
    std::optional<ParticipantFeatSnapshot> featAtBattleStart; // confirmed player participant only
};

struct ImpressedCreatureResult
{
    BattleUnitId battleUnitId;
    std::string speciesId;
    std::string encounterSpawnMemberId;
    std::uint32_t enemyRosterOrder;
};

struct BattleResult
{
    BattleResultToken token;
    BattleTerminalRecord terminal;

    std::string encounterDefinitionId;
    std::string encounterDisplayName;
    EncounterKind encounterKind;
    bool repeatable;
    bool allowsTaming;
    std::uint32_t resolvedTier;
    std::string resolvedTeamId;
    std::uint64_t encounterResolutionSeed;
    std::uint64_t combatSeed;
    spk::Vector3Int encounterReturnWorldCell;
    BoardSourceDescriptor boardSource;

    std::vector<BattleParticipantResult> participants; // player then enemy, roster order
    std::vector<ImpressedCreatureResult> impressed;     // enemy roster order
    std::vector<ProvisionalRecruit> provisionalRecruits; // victory-eligible, roster order
    BattleSnapshot initialSnapshot;
    BattleSnapshot terminalSnapshot;
    BattleConditionTranscript transcript;
};
```

Only the fully confirmed player roster and deployed enemies enter `participants`; v1
requires every non-empty player roster unit and has no unplace/reserve command. Placement
reposition/swap does not change participant identity. All fields are copied values with no
registry/session/component references. `provisionalRecruits` is populated
only according to step 16's outcome policy, while `impressed` remains available for a loss
summary.

Every enemy participant's `encounterSpawnMemberId` is copied from its projected
`BattleUnit`, is non-empty, and matches the selected team's member at the same roster order;
player values are null. Copy resolved tier/team, both child seeds, flags, return cell, and
the exact live-world/handcrafted `boardSource` descriptor from the immutable descriptor
rather than querying registries again. Require
`terminal.abortReason.has_value()` exactly when
`terminal.outcome == BattleOutcome::Aborted`; its value must match the sole `BattleAborted`
event and `terminal.battleId` must match descriptor/snapshots. Every non-aborted result has a
null abort reason.
Also reassert the copied kind/repeatability invariant: Wild/Debug are true and
Trainer/Gym/Special are false. A mismatch is an invalid result, not permission to alter clear
policy.

After the ordinary terminal batch and every permitted post-resolution Taming system batch
(whether Taming caused termination or merely recorded final diagnostics) are fully
committed, `BattleSession::finalizeResult()`/equivalent executes
exactly once: verify Terminal, freeze command/log mutation, capture the terminal snapshot,
copy every committed batch/event into the transcript, build participants/impressed/recruits
in canonical order, copy all encounter provenance above, validate token/descriptor/member/
seed/count consistency, and return the immutable result to `BattleModeRuntime`.
`initialSnapshot` is the first committed battle snapshot
retained by the session. An aborted partial writer is never copied. Subsequent calls return
the same stored value/view or a typed AlreadyFinalized result; they cannot rebuild from a
changed context.

Add a service equivalent to:

```cpp
struct BattleOutcomeTransaction
{
    BattleResult result;
    std::vector<CreatureProgressionDelta> progression;
    std::vector<RecruitCommit> recruits;
    std::optional<std::string> clearedEncounterId;
    BattleReturnDirective returnDirective;
    PlayerData candidatePlayerData;
    BattleResultSummary summary;
};

class BattleOutcomeService
{
public:
    [[nodiscard]] BattleOutcomeTransaction prepare(
        const PlayerData &p_current,
        const BattleResult &p_result,
        const Registries &p_registries,
        const BattleReturnCellResolver &p_returnCells) const;

    void validate(const BattleOutcomeTransaction &p_transaction,
                  const Registries &p_registries) const;

    void commit(GameContext &p_context,
                BattleOutcomeTransaction p_transaction,
                SaveService *p_saveService) const;
};
```

Equivalent result/error types are acceptable. Exceptions may be used consistently with
current JSON/load code, but `prepare` must be pure and `commit` must have a clear all-or-none
boundary.

`prepare` order:

1. Reject non-terminal/previously-consumed results.
2. Copy current PlayerData.
3. Evaluate/apply step-17 deltas to the copy in participant roster order, passing each
   participant snapshot with `result.terminal` and `result.transcript` through the exact
   step-17 seam.
4. For player victory, materialize provisional recruits in enemy roster order.
5. Route recruits to team/storage.
6. For victory, add the stable cleared encounter ID for Trainer/Gym/Special; assert its
   copied `repeatable` flag is false.
7. Select the return kind, resolve its exact currently standable destination (including
   deterministic heal-point fallback), and assign that exact cell to both the return
   directive and `candidatePlayerData.playerCell`.
8. Re-derive and validate every changed creature and the resolved return cell.
9. Build the presentation summary from before/after values.
10. Return the complete candidate without touching GameContext.

`prepare` assigns the complete token to
`candidatePlayerData.lastCommittedResultToken` only after every result change validates.
`commit` serializes the candidate first when saving is configured. Before writing, perform
every potentially throwing validation/allocation/serialization step. After durable replace,
the remaining publish path must be no-throw: move/swap the already built PlayerData into
GameContext, mark the active pending result consumed, then queue value-owned notifications.
A crash after replace but before in-memory publication reloads the saved high-water mark;
the result still cannot apply twice. A repeated commit of the same or older token is
rejected before any reward or write.

---

# 5. Recruit materialization and routing

Convert each step-16 `ProvisionalRecruit` to a fresh persistent unit inside the transaction:

```text
id             = allocate from PlayerData.nextCreatureSerial
speciesId      = provisional species ID
feat progress  = fresh board progress + provisional completed-node IDs applied directly
derived state  = rebuild through step-17 derivation
form           = therefore inherited from completed rewards, never copied independently
```

Do not inherit battle-local resources, statuses, shields, taming progress, enemy AI,
position, or partial enemy Feat progress.

ID allocation must be deterministic and collision-checked. Use the validated
`CreatureInstanceId` format fixed in step 01/04; do not call `random_device` or depend on
`spk::UUID` text parsing that does not exist.

Routing:

1. Find the lowest-index empty team slot.
2. Put the recruit there when available.
3. Otherwise append to PC storage.
4. Continue in enemy roster order for multiple recruits.

Record a `RecruitCommit` with creature ID, species ID, and destination
`TeamSlot{index}`/`Storage{index}` for the result summary. Team capacity remains exactly the
configured six slots; do not resize it to accommodate recruits.

On defeat/draw/abort, allocate no IDs and advance no creature serial counter.

---

# 6. Cleared encounter state

Store stable semantic IDs, not actor pointers, plan vector indices, display names, or world
coordinates:

```cpp
std::set<std::string> clearedTrainerIds;
std::set<std::string> clearedGymIds;
std::set<std::string> clearedSpecialEncounterIds;
```

Equivalent one-map storage with a validated kind is acceptable. Ordered storage makes save
output stable.

Only a PlayerVictory for a matching Trainer/Gym/Special encounter adds the ID. Adding an already
cleared ID is an idempotent no-op but should be caught earlier because step 12 must suppress
starting it again.

Defeating a random wild team never clears its encounter table. Taming is disabled for
trainer/gym/special battles, so there is no interaction between impressed removal and
cleared flags.

Badge/gym count used by encounter tiers is the number of cleared gym IDs after commit.
Do not store a second badge-count integer that can drift.

---

# 7. Loss and return flow

`PlayerData.lastHealPoint` is initialized to the generated spawn standable support cell and
updated only by a future explicit heal-point interaction. This step does not invent heal
centers; it implements the state and loss directive.

Add:

```cpp
enum class BattleReturnKind
{
    EncounterCell,
    HealPoint
};

struct BattleReturnDirective
{
    BattleReturnKind kind;
    spk::Vector3Int targetCell;
};
```

The return region may be far outside the still-pinned battle region. Before calling
`prepare`, enter a terminal `PreparingReturn` state and acquire a separate RAII
`ReturnRegionLease` from the streamer:

1. choose the authored return origin (encounter cell or last heal point);
2. plan the chunks covering X/Z Manhattan recovery radius 16, the current
   `WorldNavigation` vertical bounds, and required head-clearance cells with checked
   arithmetic;
3. request/load and pin those chunks under a distinct owner while retaining the battle
   lease and global fluid pause;
4. verify they are active, then build a temporary read-only traversal/standability view
   over that region using `gameRules.maxVerticalTraversalGap`;
5. resolve and capture the exact target plus chunk content revisions.

Unrelated chunk loads may change global world revision and must not stale the frozen battle
board. Retain `ReturnRegionLease` through result save, Continue, and actor placement. A
load/revision/resolution failure keeps the result flow available for retry and leaves old
PlayerData/save untouched.

Return-cell resolution is then a pure/read-only input to `prepare`. Apply one algorithm to
both origins: if the exact heal point or encounter cell is not standable, search the loaded
temporary view in a deterministic expanding X/Z Manhattan ring from distance 0 through 16
and choose the first standable support cell by distance, then x, z, y. This is essential for
`RequiredTerrainChanged`, where the authored encounter support may be the cell that became
invalid. Store the resolved exact/fallback cell—not the stale origin—in
`returnDirective.targetCell` and `candidatePlayerData.playerCell`; keep `kind` as the
semantic HealPoint/EncounterCell source. If no candidate exists, retain the typed retryable
PreparingReturn error and old PlayerData/save unchanged.

After Continue and battle teardown, realize the already persisted directive:

* queue teleport using the same safe-after-update mechanism as portals;
* clear player path/segment/progress;
* verify the retained return-region content stamp, place the actor, then re-center streamer,
  navigation, camera, and fluid simulation before releasing `ReturnRegionLease`;
* clear pending hover/encounter state;
* restore exploration only when the actor is placed at a valid standable cell.

Log deterministic recovery. If no cell is found within radius 16, fail with an actionable
error rather than placing the player inside terrain.

On victory, restore the exact validated encounter return support cell; battle-unit
positions never move the exploration actor. The later actor placement is only realization
of the state already committed to `PlayerData` and the slot file. It must not select a new
fallback or change `playerCell` after saving; if the world unexpectedly disagrees, keep the
result flow blocked with a technical diagnostic instead of producing a second unsaved
position.

This return policy is identical for a handcrafted Gym/Special battle. Isolated arena
coordinates are never candidates for `PlayerData.playerCell`: the immutable
`encounterReturnWorldCell` remains the exploration origin captured at entry. Teardown removes
the arena lease, restores exploration presentation, and realizes only the already validated
world return directive.

---

# 8. Save schema

Create a battle-relevant version-1 save at `Playground/saves/<slot>.json` (or the existing
writable save-root convention if a prior step established one). Definitions remain under
`resources/data`; saves never do.

Use a schema equivalent to:

```json
{
  "version": 1,
  "world": {
    "seed": "1844674407370955161",
    "generatorVersion": 1,
    "playerCell": [12, 7, -4],
    "lastHealPoint": [0, 5, 0],
    "encounterSerial": "9",
    "lastCommittedResult": {
      "worldSeed": "1844674407370955161",
      "encounterOrdinal": "7",
      "stableEncounterInstanceId": "battle/wild/meadow-lumenwing-wild/12/7/-4/7"
    }
  },
  "player": {
    "nextCreatureSerial": "2",
    "team": ["creature-0000000000000001", null, null, null, null, null],
    "storage": [],
    "creatures": [
      {
        "id": "creature-0000000000000001",
        "species": "cinderkit",
        "featBoard": {
          "board": "cinderkit-growth",
          "chosenExclusiveGroups": {},
          "nodes": [
            {
              "id": "root",
              "completed": true,
              "requirements": []
            },
            {
              "id": "warm-up",
              "completed": false,
              "requirements": [
                {
                  "id": "deal-eighteen",
                  "conditions": [
                    {
                      "id": "deal-eighteen",
                      "completed": false,
                      "qualifyingWindows": 0,
                      "consecutiveWindowStreak": 0,
                      "gameMetric": 6,
                      "gameBuckets": {}
                    }
                  ]
                }
              ]
            },
            {
              "id": "flare-path",
              "completed": false,
              "requirements": [
                {
                  "id": "ignite-three",
                  "conditions": [
                    {
                      "id": "ignite-three",
                      "completed": false,
                      "qualifyingWindows": 0,
                      "consecutiveWindowStreak": 0,
                      "gameMetric": 0,
                      "gameBuckets": {}
                    }
                  ]
                }
              ]
            },
            {
              "id": "ash-path",
              "completed": false,
              "requirements": [
                {
                  "id": "travel-eight",
                  "conditions": [
                    {
                      "id": "travel-eight",
                      "completed": false,
                      "qualifyingWindows": 0,
                      "consecutiveWindowStreak": 0
                    }
                  ]
                },
                {
                  "id": "pounce-twice",
                  "conditions": [
                    {
                      "id": "pounce-twice",
                      "completed": false,
                      "qualifyingWindows": 0,
                      "consecutiveWindowStreak": 0
                    }
                  ]
                }
              ]
            }
          ]
        }
      }
    ],
    "clearedTrainers": [],
    "clearedGyms": [],
    "clearedSpecialEncounters": []
  }
}
```

Adapt field names to step-03/15 ID and advancement types, but preserve these rules:

* world seed, encounter serial, every numeric field of optional
  `lastCommittedResult` (the whole object is JSON null before any result), and
  next-creature serial are decimal strings so the full unsigned 64-bit range survives JSON
  tooling; the token's inner world seed must equal the save world's seed;
* arrays/sets are serialized in deterministic order;
* team always has exactly six nullable entries;
* creature definitions are referenced by filename ID;
* each requirement's `conditions` array stores the top-level condition and every nested
  child by stable ID in current definition traversal order; composites are never collapsed.
  Loading maps the unique set by ID before rebuilding current order, so array reordering is
  not data loss;
* persist stable condition completion for every window, open game metrics/buckets, and
  incomplete fight qualifying-window counts/streaks; omit open ability/turn/fight metrics
  and incomplete ability/turn window counts;
* derived stats/abilities/passives/form are omitted;
* no active battle/session state is written;
* no full WorldPlan is written in this battle-focused step.

`generatorVersion` makes the limitation explicit. Full persistence of generated POI layout
across future generator changes is a later world/save task; do not claim it here.

---

# 9. Save parsing and reconciliation

Use `spk::JSON::Reader` strict parsing and contextual errors. Load into temporary values,
reconcile definitions, validate the entire graph, then publish.

Rules:

* unsupported save version: hard error;
* missing/unknown field: hard error unless explicitly documented migration metadata;
* unknown species/board: hard error naming creature ID;
* a saved board must exactly equal the referenced species' current `featBoard`;
* duplicate creature ID or roster reference: hard error;
* team size not six, storage duplicates, missing referenced creature, or an orphan creature
  catalog entry not referenced exactly once by team/storage: hard error;
* `nextCreatureSerial` must be non-zero and strictly greater than every used creature
  serial; encounter/result ordinals must parse over their full unsigned range without a
  JSON double; `lastCommittedResult`, when present, must have the save world seed, a
  non-empty canonical stable identity, and an ordinal strictly less than the next-unconsumed
  `encounterSerial`;
* unknown completed node/requirement ID from an older content revision: drop with a
  structured warning, then re-derive;
* new definition node/requirement absent from save: initialize fresh/locked state;
* for a still-known requirement, index its saved flat condition entries by stable ID and
  inflate through step 15: a pure array reorder is accepted and rewritten in current
  definition pre-order; if the unique ID set or leaf/composite shape changed because a
  nested condition was added, removed, or retyped, reset that entire requirement's
  advancement to fresh with a structured `ConditionTreeChanged` warning containing
  creature/node/requirement and old/current ID sets. Do not partially salvage child metrics;
  duplicate IDs or malformed values remain hard save errors, not migrations;
* contradictory exclusive evolution completions: hard error, do not guess;
* a known cleared encounter ID must have the matching trainer/gym/special kind and the
  schema-required `repeatable:false`; a now-unknown cleared ID is dropped with a structured
  content-migration warning rather than silently reclassified;
* derived-state failure: hard error;
* player/heal cell numeric overflow: hard error; standability recovery occurs after world
  load, not in JSON parsing.

Warnings must be collectable/testable, not only printed. Never index progress by array
position.

---

# 10. Safe file replacement

Use a validated value such as `SaveSlotId` with the same conservative content-ID grammar
and no path separators. `SaveService` is constructed with one active slot (`autosave` by
default) and exposes `activeSlot()` plus `writeActive(const PlayerData&)`; both the
pre-encounter checkpoint and result commit call that API. `writeSlot(slot, ...)` may remain
an internal/test primitive, but no caller silently chooses a different default.

`SaveService::writeSlot` must:

1. serialize to a deterministic JSON value/string;
2. create the writable save directory when absent;
3. write to a sibling temporary file;
4. flush and close successfully;
5. replace/rename to the final slot path using the safest available same-filesystem path;
6. remove the temporary file on failure where safe;
7. leave the previous valid save readable until replacement succeeds.

Do not write directly over the only valid save. Do not place saves in the resource tree.
Sanitize slot names to a small accepted character set and prevent path traversal.

Exact byte output for identical state must be stable enough for golden tests (consistent
key/array order and formatting).

---

# 11. Result screen

Replace any provisional result banner with a panel bound to immutable
`BattleResultSummary` data:

```text
Outcome: Victory / Defeat / Draw / Technical error
Encounter name

For Technical error:
    stable BattleAbortReason code + short user-facing explanation
    explicit: no progress, recruit, or encounter-clear changes

For each confirmed player participant, roster order (non-aborted outcomes only):
    requirement progress changes
    completed node names
    stat / ability / passive / form rewards

Impressed creatures:
    recruited -> Team slot N / PC storage
    or forfeited after defeat

Cleared trainer/gym/special entry, when applicable
Return destination summary on defeat
Validated encounter return destination on Technical error
Save state: Saved / Error with Retry

[Continue]
```

Continue is disabled until the transaction commits or the user explicitly chooses the
development-only unsaved path after an error. It is also disabled while the presentation
queue still owns battle views needed for final feedback.

The screen subscribes/stores RAII contracts and never owns/mutates PlayerData.
Map every closed `BattleAbortReason` to a stable localized/user-facing message plus a
developer diagnostic code; never display exception text as the logical reason. An Aborted
result still follows the same retry/save/Continue state machine and returns to its validated
encounter cell, but its summary must contain no progression, recruit, or clear rows.

---

# 12. Lifecycle and ownership order

Replace step 12's temporary “stage result, request exit at next boundary” behavior with this
final result-flow state machine:

```cpp
enum class BattleResultFlowState
{
    None,
    ResultStaged,
    PreparingReturn,
    CommitPending,
    CommitError,
    AwaitingContinue,
    ExitQueued
};
```

The one terminal transition freezes commands, constructs/stores the immutable result,
emits `ResultReady`, and enters `ResultStaged`; it does **not** call
`requestBattleExit`/`takeTerminalRecord` at the next boundary. The runtime embeds that
already-stable terminal record in the full `BattleResult`, then acquires the
return-region lease (`PreparingReturn`), prepares and attempts the outcome/save transaction
(`CommitPending`), retains everything with Retry in `CommitError`, and reaches
`AwaitingContinue` only after the candidate is durably/in-memory committed. The result,
session, board, presentation, battle lease, and return lease remain owned and queryable
through every one of these states.

Expose value/typed methods equivalent to `retryResultCommit()` and
`continueAfterCommittedResult()`. Retry is legal only in `CommitError` and reuses the same
validated result/token; Continue is legal only in `AwaitingContinue`, is accepted once, and
sets `ExitQueued`. Only `ExitQueued` causes the next safe frame boundary to emit `WillExit`
and begin teardown. Double-clicks, stale UI callbacks, and the old step-12 automatic exit
return a typed no-op/rejection and cannot consume the result or write twice.

On successful Continue:

```text
deactivate battle input
deactivate result HUD and release its contracts
drain/cancel presentation queue
unregister battle unit and overlay entities from GameEngine
destroy BattlePresenter
destroy BattleSession / live BoardData borrow
release the battle-region lease while retaining ReturnRegionLease
apply the already persisted queued return placement
unpause fluids and restore exploration hover/player renderer/camera/input
hand destination chunks to normal player streaming, then release ReturnRegionLease
re-enable encounter resolution after leaving the triggering step
```

Destroy the session before `GameSceneWidget` can reset `WorldContext`. Do not deactivate the
whole player entity because it also owns streaming/fluid components.

Ensure a second battle starts with a fresh result token, event log, trackers, presenter
maps, HUD contracts, and no staged recruits.

---

# 13. Required tests

Add tests under `Playground/tests/save/`, `tests/battle/results/`, and integration fixtures.

## 13.1 Outcome matrix

Test the full table for victory/defeat/draw/abort: progression, recruit, clear, serial
allocation, and return directive. Assert loss progress persists and recruits are forfeited.

## 13.2 Atomicity

Inject failures at progression validation, recruit derivation, cleared-ID validation,
serialization, temporary write, and replacement. In every case original PlayerData and
previous save remain unchanged. Retry succeeds exactly once. Reusing a consumed result token
cannot duplicate anything. Cover same/older ordinals, an identity mismatch at the same
ordinal, token/descriptor mismatch, crash/reload after durable replace, and a throwing test
seam before replace; prove the post-replace PlayerData publication/active-token consumption
path is `noexcept`.

Also inject pre-encounter checkpoint validation/write/replacement failures for both a
chance-failed Bush arrival and a successful/debug request. Assert no PlayerData publication,
no battle staging, no additional input, and no reroll on retry; successful retry writes the
actual actor cell and advances the next ordinal exactly once. Crash/reload after the
checkpoint and before/inside battle must retain the advanced ordinal. Assert every path
uses the configured `autosave` `SaveSlotId` and rejects an invalid active slot at bootstrap.

## 13.3 Team/storage routing

Test empty team, one slot, full team, multiple recruits crossing the boundary, stable order,
unique IDs, unchanged serial on loss, and inherited completed-node-derived form.

## 13.4 Save round trip

Round-trip world fields including the optional complete last-result token, six-slot
team, storage, creature IDs/species, completed nodes,
condition completion, game-scope advancement, cross-fight counts/streaks, exclusive
choices, and cleared sets. Assert transient open window state and derived fields are absent
from JSON and derived values are correct after load.

## 13.5 Invalid/migration data

Test version, seed range/string, slot traversal, duplicate/missing IDs, unknown species,
species/board disagreement, orphan catalog creatures, invalid next serial, unknown progress
IDs warning/drop, new node initialization, reordered nested condition IDs preserving
progress, added/removed/retyped nested condition trees resetting exactly one requirement
with `ConditionTreeChanged`, exclusive conflict, cleared-ID kind/unknown reconciliation,
malformed vectors, unknown fields, and transactional publication.

## 13.6 File safety/determinism

Use a temporary directory. Assert stable bytes, previous file survival on injected failure,
no orphan temp after success, and slot-name validation.

## 13.7 Loss return

Test valid heal point, blocked point deterministic nearest recovery, no-cell error, queued
post-update teleport, path/hover clearing, streamer/navigation recenter, and restoration of
fluid/input state. For both a direct heal point and a recovered fallback, assert before the
write that `candidatePlayerData.playerCell == returnDirective.targetCell`, assert the slot
contains that exact cell, then reload before Continue and prove it restores the same cell.
Assert the later queued teleport does not rewrite or re-resolve the persisted destination.
Cover the same candidate/save/directive equality for victory and abort return cells. Force
`RequiredTerrainChanged` by invalidating the authored encounter support cell, require the
exact radius/order fallback above, persist/reload it, and prove retry does not loop on the
stale original. Also cover no encounter-cell fallback as a typed retryable error.
Use a heal point outside the battle lease to prove the return region loads/pins before
prepare, unrelated global revision churn does not abort battle, content-revision drift is
diagnosed, the lease survives battle teardown until actor placement, and all failure paths
release only their own lease.
Run victory/abort return coverage once from a live-world battle and once from a handcrafted
Gym fixture. The latter must prove no board-local coordinate leaks into the save/directive
and that its immutable result retains the Handcrafted board-source descriptor.

## 13.8 Repeated lifecycle

Run win -> continue -> loss -> continue -> save/reload -> win in one process/fixture. Assert
no duplicate subscriptions/entities/rewards and exact cleared-encounter suppression.
Drive every final result-flow state explicitly. Assert a frame boundary in
`ResultStaged`/`PreparingReturn`/`CommitPending`/`CommitError`/`AwaitingContinue` never emits
`WillExit` or destroys the session; Retry is state-gated; only one accepted Continue enters
`ExitQueued`; teardown then emits `WillExit`/`Exited` once with both leases released in the
specified order.

Visual checks: a victory with team recruit, victory with PC overflow, defeat with progress
but forfeited tame, save-error retry, and heal-point return.

---

# 14. Expected file changes

Add cohesive files equivalent to:

```text
Playground/srcs/battle/results/battle_outcome_transaction.hpp
Playground/srcs/battle/results/battle_outcome_service.hpp
Playground/srcs/battle/results/battle_outcome_service.cpp
Playground/srcs/battle/results/battle_result_summary.hpp
Playground/srcs/save/save_data.hpp
Playground/srcs/save/save_parser.hpp
Playground/srcs/save/save_parser.cpp
Playground/srcs/save/save_service.hpp
Playground/srcs/save/save_service.cpp
Playground/srcs/ui/battle_result_widget.hpp
Playground/srcs/ui/battle_result_widget.cpp

Playground/tests/battle/results/battle_outcome_service_test.cpp
Playground/tests/save/save_round_trip_test.cpp
Playground/tests/save/save_validation_test.cpp
Playground/tests/save/save_file_safety_test.cpp
Playground/tests/integration/battle_result_lifecycle_test.cpp
```

Modify PlayerData/GameContext, BattleMode teardown/return handling, main/bootstrap load path,
event/domain notification types, relevant HUD composition, and tests. Equivalent cohesive
organization is acceptable.

---

# 15. Documentation requirements

Document the outcome table, transactional prepare/commit boundary, no persistent battle
resources, recruit routing, cleared-ID source of badge count, save schema/reconciliation,
safe file replacement, heal-point recovery, result-screen gating, and explicit WorldPlan
persistence limitation.

Do not claim mid-battle resume or full generated-layout persistence.

---

# 16. Non-goals

Do not implement mid-battle save/resume, save-slot selection UI, cloud saves, encryption,
compression, rollback history, full WorldPlan/POI serialization, persistent combat injury,
items/currency, a heal-center interaction, Elite Four gauntlet state, production main menu,
or resource authoring tools.

---

# 17. Acceptance criteria

This step is complete when:

* terminal outcomes produce one pure, complete candidate PlayerData;
* progress commits after victory, defeat, and terminal draw but not abort;
* recruits commit only after victory and route team-first then PC;
* Trainer/Gym/Special encounter IDs clear only after victory;
* the full update/save is all-or-none and retry-safe;
* result tokens prevent duplicate commits;
* save JSON uses stable semantic IDs and omits derived/battle state;
* save parsing is strict, transactional, and reconciles progress IDs as specified;
* loss returns to a valid last heal point through a queued safe transition;
* result UI accurately distinguishes progress, recruit, forfeit, clear, and save status;
* repeated battle/lifecycle tests find no stale state;
* all builds/tests pass and controlled visual checks are honestly reported.

---

# 18. Required handoff report

Report the final transaction and save schemas, exact outcome table, file-replacement method,
tests/builds and visual scenarios actually run, failure injections covered, warnings or
skipped validation, and the full-world persistence limitations still outside this battle
plan.
