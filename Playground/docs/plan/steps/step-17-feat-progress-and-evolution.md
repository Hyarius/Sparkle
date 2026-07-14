# Implement Feat Progress, reward replay, and branching evolution

Implement Erelia's no-level progression loop: evaluate the completed battle's typed event
log for the player creatures that participated, advance only Feat nodes that were active at
battle start, complete qualifying nodes, and rebuild each creature's stats, abilities,
passives, and form from immutable species data plus completed rewards.

Progress applies after both victory and defeat. This is an implementation task: add the
headless progression service, integrate it with battle results, add exhaustive tests, and
keep Playground compiling. Do not build a Feat Board editor or runtime graph screen.

---

# 1. Repository baseline

Implement against the repository after steps 01-16 and inspect the actual APIs first.

Required earlier contracts:

* step 03 provides validated `FeatBoardDefinition`, stable board/node/requirement IDs,
  adjacency, evolution choice groups, condition specifications, and reward specifications;
* step 04 provides persistent `CreatureUnit`, species/form/default-loadout definitions,
  in-memory team/storage, and a deterministic derived-loadout seam;
* step 06 maps each deployed player `BattleUnitId` back to a stable persistent creature ID
  and records ordered typed events without raw pointers;
* step 15 evaluates conditions over explicit ability/turn/fight/game windows;
* step 16 adds taming events but keeps provisional recruits out of persistent player state;
* step 07's permanent `BattleTerminalRecord` distinguishes PlayerVictory, PlayerDefeat,
  Draw, and Aborted, while step 15's copied `BattleConditionTranscript` retains the
  authoritative batches/events long enough for post-battle services. Step 18 will combine
  these and the other result inputs into the full `BattleResult`.

No level, XP, XP curve, stat growth roll, or duplicated derived state may be introduced.

The Unity draft's `FeatBoardService` only applies battle progress on wins in some paths.
That behavior is explicitly wrong for Playground: the GDD requires meaningful progress
after losing.

---

# 2. Required final behavior

For each player creature actually deployed in a terminal battle:

1. Capture its active Feat node set when the battle is constructed.
2. At battle end, bind the creature as the condition subject and evaluate only that set
   against the final ordered battle log and terminal snapshots.
3. Retain completed condition state, `game` open advancement, and incomplete closed
   qualifying-fight counts/streaks; discard open ability/turn/fight metrics and incomplete
   ability/turn counts after this result.
4. Complete every active node whose requirements all qualify, in board-definition order.
5. Mark mutually exclusive alternatives blocked when an evolution choice completes.
6. Rebuild the creature's derived attributes, ability pool, permanent passives, and current
   form by replaying all completed rewards from a clean species baseline.
7. Produce a value-owned progression summary for the result screen and step-18 commit.

Defeated participants still progress. An unconfirmed/aborted Placement creates no terminal
participant result; after confirmation every non-empty roster member participates. PC
storage creatures, enemy units, summaries without a persistent player-creature identity,
and provisional recruits do not consume this battle's events.

Newly activated neighbor nodes are available in the next battle. They must not consume
events that occurred before they became active.

---

# 3. Persistent versus derived state

`CreatureUnit` persists only identity and authored-reference/progression state equivalent
to:

```cpp
struct DerivedCreatureState
{
    CreatureAttributes attributes;
    std::vector<std::string> abilityIds;
    std::vector<std::string> passiveStatusIds;
    std::string formId;
};

struct CreatureUnit
{
    CreatureInstanceId id;
    std::string speciesId;
    FeatBoardProgress featProgress;

    // Derived cache rebuilt through CreatureProgressionService only.
    DerivedCreatureState derived;
};
```

`derived` may be cached for convenient battle construction, but it is never the
authoritative save value. It must be reproducible exactly from `speciesId + featProgress +
current registry content`.

Do not persist or directly mutate:

* a level or XP value;
* reward-adjusted stats;
* unlocked ability/passive vectors;
* current form as an independent choice;
* battle HP/AP/MP/status/turn-bar state.

If step 04 temporarily stored form/loadout fields for bootstrapping, migrate callers to the
derived state and ensure serialization in step 18 omits those caches.

---

# 4. Progress value model

Use stable authored IDs, never node indices, as persistent keys. A model equivalent to the
following is required:

```cpp
using FeatNodeId = std::string;
using FeatRequirementId = std::string;

struct RequirementProgress
{
    FeatRequirementId requirementId;
    // Step-15 canonical flat pre-order form for the root and every nested child.
    std::vector<PersistentConditionAdvancement> conditionAdvancement;
};

enum class FeatNodeState
{
    Locked,
    Active,
    Completed,
    BlockedByChoice
};

struct FeatNodeProgress
{
    FeatNodeId nodeId;
    bool completed = false;
    std::vector<RequirementProgress> requirements;
};

class FeatBoardProgress
{
    std::string boardId;
    std::map<FeatNodeId, FeatNodeProgress> nodes;
    std::map<std::string, FeatNodeId> chosenExclusiveGroups;
};
```

Equivalent sorted-vector storage is acceptable and may be preferable. Externally visible
iteration always follows `FeatBoardDefinition::nodes` order, not map hash order.

Do not replace step 04's advancement model with one scalar per top-level requirement. An
`allOf`/`anyOf` may contain several game/fight leaves whose independent metrics, completed
window counts, streaks, and completion flags must survive. Condition IDs are unique across
the board and are the persistent keys; definition order controls presentation/replay order.

Nodes complete once. Do not implement `repeatLimit`, infinitely repeatable nodes, or reward
farming from a completed node. `requiredWindowCount` belongs to individual conditions and
already expresses repeated qualifying abilities, turns, fights, or lifetime thresholds.

On a fresh creature:

* create progress for every current node/requirement ID;
* complete the declared root node exactly once;
* require the root to have no requirements/rewards that depend on battle events;
* derive the initial active neighbors;
* rebuild derived state.

Unknown saved IDs and schema migration are handled in step 18, but this step's in-memory
APIs must be ID-based and robust to definition reordering.

---

# 5. Active-node snapshot

At `BattleSession` construction, after the non-empty player roster is fixed but before
Placement, create one immutable snapshot per roster creature:

```cpp
struct ParticipantFeatSnapshot
{
    CreatureInstanceId creatureId;
    BattleUnitId battleUnitId;
    std::string boardId;
    std::vector<FeatNodeId> activeNodeIds;
};
```

The active set is based on progress at battle start:

```text
not completed
and not blocked by an exclusive choice
and at least one neighbor is completed
and every node-level evolution gate is currently satisfied
```

Evolution gates include the definition fields fixed in step 03:

* `minimumCompletedNodes` counts completed non-root nodes unless the schema explicitly
  documents root inclusion; use non-root for this series;
* `fromForm`, when present, must equal the creature's derived form at battle start;
* an `exclusiveGroup` with an existing different choice blocks the node.

Snapshot the IDs, not condition progress copies. Persistent condition completion, game
metrics, and cross-fight counts continue from `CreatureUnit`; only eligibility is frozen. A node unlocked by this battle is absent
from the snapshot and waits for the next battle.

Manual placement happens after this snapshot. Step 06 requires every non-empty roster unit
to be placed before confirmation; v1 has no unplace/reserve selection. Once both sides
confirm, every snapshotted player unit is a participant. Reposition/swap during Placement
does not change eligibility or emit a leave-battle fact. Cancelling/aborting Placement
produces no terminal participant progress.

---

# 6. Event attribution

Evaluate a creature with:

```text
condition subject = its BattleUnitId
subject team       = Player
opponent team      = Enemy
event stream       = complete BattleConditionTranscript copied from BattleEventLog/archive,
                     including terminal BattleEnded
before/after state = battle snapshots retained by the result
```

Do not pre-filter the log merely to `caster == subject`. Step 15 condition filters must be
able to credit:

* damage dealt as source;
* damage taken as target;
* shield absorption on the subject;
* healing supplied to an ally;
* statuses cleansed from an ally;
* movement and end-turn positioning by the subject;
* survival/win/loss/end-state team predicates.

The condition definition owns actor/target/team filtering. The progression service only
binds the subject context and supplies the full immutable stream.

Feat-domain events produced after battle (`FeatRequirementAdvanced`, `FeatNodeCompleted`,
`DerivedCreatureStateChanged`) are not appended back into the battle log and cannot satisfy
combat conditions.

---

# 7. Scope persistence semantics

Use step 15's window rules without reinterpretation:

* `ability`: one ability/action window can qualify once; incomplete progress does not cross
  into another ability;
* `turn`: one subject turn can qualify once; incomplete progress does not cross turns;
* `fight`: this completed battle can qualify once; the open fight metric is discarded after
  close, while a qualifying-fight count or consecutive-fight streak persists when the leaf
  still needs more fights;
* `game`: contributions accumulate persistently across battles, carry their documented
  remainder, and can eventually satisfy the condition even after losses.

Within a node, all top-level requirements must be complete. A requirement that qualifies
this fight and a game-scope requirement completed earlier may combine to complete the node.

Once any leaf/composite condition completes, persist that completion while its node remains
active, even if sibling requirements are not complete yet. Persist game-scope open metrics
and buckets, and incomplete fight-scope closed-window counts/streaks. Do not save open
ability/turn/fight metrics or incomplete ability/turn counts across battles. The result
summary may show those transient values from the current battle, but after commit they
reset. This distinction is required for definitions such as “survive two fights” without
allowing half of one fight metric to leak into the next.

Terminal outcomes:

```text
PlayerVictory -> evaluate and commit progress
PlayerDefeat  -> evaluate and commit progress
Draw          -> evaluate and commit progress when the battle reached a rules-defined terminal draw
Aborted       -> commit nothing
```

A content/load/runtime failure that aborts a session is not a loss and must not grant
progress.

---

# 8. Node completion algorithm

Implement a `FeatProgressionService` with a pure prepare/apply split compatible with the
transaction introduced in step 18:

```cpp
struct CreatureProgressionDelta;

CreatureProgressionDelta evaluate(
    const CreatureUnit &p_creature,
    const ParticipantFeatSnapshot &p_participant,
    const BattleTerminalRecord &p_terminal,
    const BattleConditionTranscript &p_transcript) const;

void apply(CreatureUnit &p_creature, const CreatureProgressionDelta &p_delta) const;
```

`evaluate` never mutates player state. It returns previous/new advancement, completed node
IDs, blocked alternatives, and a before/after derived-state summary. Step 18 can validate
all deltas and apply them atomically. It first requires a matching nonzero
`p_terminal.battleId`/`p_transcript.battle` plus participant creature/battle-unit identity
agreement with the transcript's construction/final snapshots, then returns no delta for
Aborted. Step 18 passes `BattleResult::terminal` and `BattleResult::transcript` to this exact
seam; step 17 does not introduce a premature partial `BattleResult` type.

Evaluation order:

1. Validate creature/species/board/participant identity agreement.
2. Copy the creature's progress into a working value and inflate every requirement's flat
   entries through step 15's canonical definition-validated adapter.
3. Iterate `activeNodeIds` in board-definition order.
4. Evaluate each requirement using the same unchanged battle-start eligibility snapshot.
5. Sanitize and flatten changed persistent completion/game/fight-window advancement back
   through the same adapter; discard battle-local open ability/turn/fight values at the
   result boundary.
6. Mark the node complete if all requirements are complete.
7. For an exclusive node, record its group choice and block every alternative.
8. Continue evaluating other nodes that were independently active at battle start; do not
   add newly adjacent nodes to this pass.
9. Rebuild derived state once after all completions, not once per reward.
10. Return one immutable delta.

If two independently active nodes in the same exclusive group would both complete from one
battle, the earliest node in definition order wins. Stop evaluating other alternatives in
that group and mark them blocked. This tie-break must be documented and tested.

---

# 9. Reward replay

Reuse and extend the one authoritative function created in step 04; do not add a second
copy under `feats/`:

```cpp
DerivedCreatureState deriveCreatureState(
    const CreatureSpeciesDefinition &p_species,
    const FeatBoardDefinition &p_board,
    const FeatBoardProgress &p_progress,
    const Registries &p_registries);
```

Algorithm:

```text
attributes       = species.baseAttributes
abilityIds       = species.defaultAbilityIds, preserving authored order and uniqueness
passiveStatusIds = species.defaultPassiveStatusIds, preserving authored order and uniqueness
formId           = species.defaultFormId

for node in board definition order:
    if node is completed:
        apply rewards in authored reward order

validate final form/abilities/passives and stat bounds
return value
```

Reward semantics:

## 9.1 `bonusStat`

Add the finite authored amount to the named base/secondary stat. Integer stats require an
integer reward. Fixed-point stats use the step-01 representation. Apply all additions before
final clamping. Health bonuses change max health for future battles; they do not heal an
already completed battle.

## 9.2 `unlockAbility`

Append the referenced ability if absent. Preserve first-unlock order determined by board
definition/reward order. Duplicate unlocks are idempotent.

## 9.3 `removeAbility`

Erase the referenced ability if present. This is useful for form transitions. Removing a
missing ability is a validated no-op, not a crash.

## 9.4 `unlockPassive`

Append the referenced permanent status/passive if absent. Duplicate unlocks are idempotent.

## 9.5 `changeForm`

Set the form to the referenced species-local form. Step-03 validation must already ensure
the reward belongs to this species/board. Definition order makes replay deterministic.

No reward directly mutates a battle unit. The rebuilt state is used when the next battle is
constructed and by exploration/team presentation after result commit.

The function must be pure and idempotent:

```text
derive(species, board, progress) == derive(species, board, progress)
apply delta twice is rejected or a no-op, never doubles rewards
```

---

# 10. Evolution choices and branches

An evolution is a Feat node with a `changeForm` reward plus its authored gates. It is not a
separate XP/level system.

When an evolution node in `exclusiveGroup = G` completes:

* record `chosenExclusiveGroups[G] = nodeId`;
* mark every other node in G `BlockedByChoice` permanently;
* their branch descendants remain locked because step-03 board validation requires them to
  be connected only through that alternative until a later common merge node;
* replay `changeForm` in board order;
* do not remove progress already stored on an alternative (it should never have progressed
  while inactive, but retaining an ignored value makes future migrations safe).

The branch choice is derived from the completed node and can be rebuilt; the explicit group
map is a validation/cache convenience. On load, step 18 reconstructs and verifies it rather
than trusting contradictory saved data.

Respec and reversing a choice are non-goals.

---

# 11. Progression summary and domain events

Produce values equivalent to:

```cpp
struct RequirementProgressDelta
{
    FeatNodeId nodeId;
    FeatRequirementId requirementId;
    ConditionAdvancement before;
    ConditionAdvancement after;
};

struct CreatureProgressionDelta
{
    CreatureInstanceId creatureId;
    std::vector<RequirementProgressDelta> requirementChanges;
    std::vector<FeatNodeId> completedNodes;
    std::vector<FeatNodeId> blockedNodes;
    DerivedCreatureState beforeDerived;
    DerivedCreatureState afterDerived;
};
```

After the transaction applies, publish value-owned domain notifications for presentation.
Do not append them to `BattleEventLog` and do not use recursive `ContractProvider` dispatch.

The result screen needs enough information to say:

* which requirements advanced and by how much;
* which nodes completed;
* which abilities/passives/stats/forms changed;
* that progress was retained after a defeat.

Do not expose mutable `CreatureUnit` through the widget payload.

---

# 12. Error handling and validation

Reject/evaluate no delta when:

* the participant creature no longer exists in PlayerData;
* its species or board ID differs from the battle-start snapshot;
* active node IDs are duplicated or absent from the board;
* progress contains duplicate node/requirement IDs;
* a node references a condition progress entry of another node;
* reward replay yields an unknown form/ability/passive;
* a completed exclusive group contains more than one choice;
* a numeric reward overflows or produces an invalid stat.

Registry structural errors should already fail at boot. These checks defend the result
transaction and must return contextual errors naming creature, board, node, and requirement.
Do not partially mutate a creature when any delta is invalid.

---

# 13. Required tests

Add tests under `Playground/tests/feats/`.

## 13.1 Fresh progress and activation

Test root completion, initial neighbor activation, locked distant nodes, stable ID lookup,
and no duplicate progress entries.

## 13.2 Battle-start eligibility

Test:

* active snapshot is captured before battle;
* an aborted/unconfirmed Placement phase produces no terminal result or progression;
* every confirmed non-empty roster unit is present in the participant snapshot;
* completed and blocked nodes are absent;
* a node unlocked by this result does not consume the same log;
* it can progress in the next battle.

## 13.3 Outcome policy

Use identical event logs with victory and defeat and assert qualifying combat progress is
the same. Assert win-only predicates differ. Test defeated participants progress,
storage creatures do not, inactive nodes do not, terminal draw follows policy, and aborted
commits nothing. There is no v1 undeployed/reserve participant case.

## 13.4 Scope persistence

Golden-test ability, turn, fight, and game requirements over two battles. Assert game open
metrics and incomplete fight qualifying-window counts/streaks cross the boundary, completed
conditions remain complete, and open ability/turn/fight metrics plus incomplete ability/
turn counts reset. Prove fight required-window counts count distinct battles.

## 13.5 GDD examples

Build definitions/events proving the engine can unlock nodes for:

* hitting with a named ability between minimum/maximum distances;
* absorbing cumulative shield damage;
* cleansing Poison, healing allies, and ending turns far from enemies;
* being hit a required number of times for max-health reward.

## 13.6 Reward replay

Test every reward kind, authored order, duplicate unlock idempotence, removal, numeric
boundaries, form changes, clean re-derivation after registry content changes, and two
consecutive calls producing identical values.

## 13.7 Evolution

Test completed-node count/from-form gates, two active alternatives completing from one log
(definition-order winner), permanent sibling blocking, descendant locking, later branch
evolution, and no respec.

## 13.8 Deltas and atomicity seam

Assert evaluation is pure, delta application changes exactly the intended creature, applying
twice cannot double rewards, and any invalid reward causes no mutation. Golden-test summary
ordering/text input data.

## 13.9 Full flow

Run two headless battles: lose the first while earning a new ability node; verify the
ability appears in derived state after result processing and is available when constructing
the second battle. The node activated by that completion may begin progress only in battle
two.

---

# 14. Expected file changes

Add cohesive files equivalent to:

```text
Playground/srcs/feats/feat_progress.hpp
Playground/srcs/feats/feat_progress.cpp
Playground/srcs/feats/feat_progression_service.hpp
Playground/srcs/feats/feat_progression_service.cpp
Playground/srcs/creatures/creature_state_derivation.hpp   # existing; reuse/modify if needed
Playground/srcs/creatures/creature_state_derivation.cpp   # existing; reuse/modify if needed
Playground/srcs/feats/progression_delta.hpp

Playground/tests/feats/feat_progress_test.cpp
Playground/tests/feats/feat_progression_service_test.cpp
Playground/tests/feats/creature_state_derivation_test.cpp
Playground/tests/feats/evolution_branch_test.cpp
```

Modify `CreatureUnit`, battle participant/result construction, the post-battle service
pipeline, result-view data model, registry validation as needed, and shared fixtures.
Equivalent organization is acceptable; do not put progression evaluation in BattleMode or
widgets.

---

# 15. Documentation requirements

Document:

* persistent progress versus derived creature state;
* active-node snapshot timing and why retroactive same-battle progress is forbidden;
* participant and loss-credit policies;
* exact scope persistence;
* reward replay order/idempotence;
* evolution gates and exclusive-group tie-break;
* the prepare/apply transaction seam;
* why Feat domain events are outside `BattleEventLog`.

Update stale docs that imply progress happens only on victory or that form is independently
saved.

---

# 16. Non-goals

Do not implement XP/levels, random stat growth, runtime Feat Board graph UI, editor tools,
respec, repeatable nodes, mid-battle unlocks, progress for storage creatures, enemy Feat
earning, provisional-recruit progress, save-file serialization, or production balance.

---

# 17. Acceptance criteria

This step is complete when:

* fresh creatures start at root with only adjacent nodes active;
* battle-start eligibility is immutable for that battle;
* deployed player participants progress after wins and losses, even when defeated;
* inactive nodes and storage creatures do not progress; every non-empty roster creature is
  a deployed participant in v1;
* only completed conditions, game open advancement, and incomplete fight qualifying-window
  counts/streaks persist between fights; other open/transient window state resets;
* completed nodes never complete twice;
* reward replay deterministically derives all stats, abilities, passives, and form;
* evolution gates and mutually exclusive branches work without level checks;
* a newly unlocked ability is available in the next battle, not the current one;
* progression evaluation produces value-owned atomic deltas and summaries;
* all tests/builds pass and the result UI can present the delta without mutable pointers.

---

# 18. Required handoff report

Report the final progress/delta/derivation APIs, participant and scope policies, the exact
exclusive-branch tie-break, tests and commands actually run, visual result-summary checks
actually performed, skipped validation, and the serialization/atomic-commit work explicitly
left for step 18.
