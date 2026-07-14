# Implement deterministic rule-driven battle AI

Execute the JSON AI decision lists created in step 04 against the complete headless battle
runtime. Add one pure planner that reads snapshots/query results and one bounded driver that
submits ordinary `MoveCommand`, `CastAbilityCommand`, and `EndTurnCommand` values through
`BattleSession::submit`. Re-evaluate rules from the top after every successful command,
with explicit command-count and repeated-state guards.

The AI receives no privileged mutation, target, path, resource, effect, RNG, scheduler, or
outcome API. This step does not add behavior trees, utility scripting, machine learning,
random choices, difficulty cheating, or presentation.

---

# 1. Repository baseline and prerequisites

Implement after steps 01-10 and read every handoff report. Reinspect the live repository.
Preserve:

* strict ordered `AIBehaviourDefinition` values with closed selectors, conditions,
  anchors, and decisions;
* each encounter enemy's immutable `aiBehaviourId`;
* one active-combatant predicate excluding undeployed, defeated, and impressed units;
* one immutable `BattleSnapshot` and const `BattleQueryService`;
* authoritative move/cast planning shared with player preview and gateway validation;
* exactly one anchor per cast and canonical target capture;
* the common command issuer/controller checks;
* fixed deterministic scheduler time and no frame-time rule state;
* material gameplay digest that excludes event/action/log noise;
* `maxCommandsPerActivation` and `maxAiCommandsPerActivation`;
* complete status/object/effect runtime and append-complete-batch-before-publish ordering.

The original Unity AI may reveal desired personality but is not an authority for direct
unit mutation, coroutines, random target selection, NavMesh paths, physics queries, or a
separate "AI ability resolver."

Step 04 already validates profile syntax/references/final termination fallback. This step
implements runtime evaluation; do not create a second JSON schema.

---

# 2. Required final behavior

At the end of this step:

1. the active enemy resolves its profile's rules in JSON order;
2. conditions are pure, integer, and AND-combined;
3. unit selectors ignore inactive/removed units and have explicit tie order;
4. cast anchors are chosen only from step-08 legal cast plans;
5. movement destinations are chosen only from step-08 legal move plans and use weighted MP
   budgets;
6. a matching rule whose decision cannot produce a legal command falls through to the next
   rule;
7. after every accepted move/cast, evaluation restarts at rule zero on a fresh state;
8. every chosen command is revalidated and resolved by `BattleSession::submit` with
   `CommandIssuer{CommandController::EnemyAi}`;
9. rejected planned commands terminate/retry safely without spinning;
10. an AI-specific command cap and repeated material-state guard force an ordinary end turn;
11. no AI definition/evaluation consumes `BattleRng`;
12. a bounded battle pump exposes stable `WaitingForPlayer`, `WaitingForAI`, and terminal
    points for step 12;
13. a fixed AI-vs-fixture run produces the same command/event/digest trace every time.

The visible checkpoint is a fully headless battle played by deterministic profiles, not a
rendered battle.

---

# 3. Architecture and ownership

Add modules equivalent to:

```text
Playground/srcs/battle/ai/battle_ai_planner.hpp
Playground/srcs/battle/ai/battle_ai_planner.cpp
Playground/srcs/battle/ai/battle_ai_driver.hpp
Playground/srcs/battle/ai/battle_ai_driver.cpp
Playground/srcs/battle/battle_pump.hpp
Playground/srcs/battle/battle_pump.cpp
```

Use exact repository conventions if paths/names differ.

Responsibilities:

| Type | Responsibility |
|---|---|
| `BattleAIPlanner` | pure selectors/conditions/candidate scoring; returns one proposed ordinary command plus trace |
| `BattleAIDriver` | activation guard state, submission, rejection policy, one AI logical operation |
| `BattlePump` | scheduler versus player wait versus one AI operation versus terminal |
| `BattleSession` | authoritative validation, action mutation, events, phases, outcomes |
| `BattleQueryService` | legal move/cast candidates and plans |
| later mode/UI | call pump/read trace; never implement rules again |

The planner borrows only const session/query/registry values for the duration of one call.
It retains no unit pointers, definition pointers, candidate spans, or preview objects across
a successful command.

---

# 4. Public planner values

Use values equivalent to:

```cpp
enum class AIPlanFailure
{
    None,
    WrongPhase,
    UnitNotActive,
    MissingBehaviour,
    NoRuleProducedCommand
};

struct AIRuleTrace
{
    std::string ruleId;
    bool conditionsMatched = false;
    bool decisionProducedLegalCommand = false;
    std::optional<std::string> detailCode;
};

struct AIPlannedCommand
{
    BattleUnitId unit;
    std::string behaviourId;
    std::string ruleId;
    BattleCommand command;
    std::vector<AIRuleTrace> evaluatedRules;
};

using AIPlanResult = std::variant<AIPlannedCommand, AIPlanFailure>;

class BattleAIPlanner
{
public:
    [[nodiscard]] AIPlanResult chooseNextCommand(
        const BattleSession &p_session,
        BattleUnitId p_unit,
        const AIBehaviourDefinition &p_behaviour) const;
};
```

Diagnostic trace values are owned copies and do not enter gameplay conditions. They may be
compiled/recorded only in tests/debug builds if desired, but the selected rule/command must
remain inspectable in focused tests.

The planner does not submit. The driver submits the returned ordinary command immediately
against the same session thread; the gateway still replans it.

---

# 5. Eligible unit set and universal order

Every selector starts from `BattleUnit::isActiveCombatant()` and current placement. It
must ignore:

* undeployed units;
* HP-zero/pending/defeated units;
* `RemovalReason::Impressed`;
* stale historical cells;
* units from another battle.

Candidate selector tie order is:

1. smaller encounter/team roster order;
2. smaller `BattleUnitId`.

All candidates for one relationship already have the intended side, so do not add pointer,
map insertion, species ID, display name, or position as an undocumented tie.

`Self` is exactly the acting unit. Every `*Ally` selector means another active unit on
the acting unit's side and excludes self; content uses `self` explicitly when self is
intended.

---

# 6. Unit selectors

Implement:

## 6.1 Nearest

`NearestEnemy` and `NearestAlly` minimize current x/z Manhattan distance from the acting
unit's support cell. Elevation and weighted path cost do not participate. Use roster/ID ties.

## 6.2 Lowest/highest health

Compute current health ratio:

```text
healthPermille = currentHealth * 1000 / effectiveMaxHealth
```

with checked integer multiplication and truncation toward zero. Effective max is at least
one. `LowestHealth*` minimizes this permille; `HighestHealth*` maximizes it; ties use
roster/ID. This is percentage health, not raw HP, so species with different max HP compare
meaningfully.

No selector falls back to a different relationship when its candidate set is empty. It
returns no unit and its consuming condition/decision handles that deterministically.

---

# 7. Condition evaluation

Conditions in one rule are logical AND in JSON order. Empty is true. Short-circuit on the
first false value for stable diagnostics. An explicit `always` is true and was already
validated not to coexist with other conditions.

## 7.1 Health ratio

Resolve the authored selector, compute the section-6 permille, then compare inclusively:

```text
atMost:  actual <= authored
atLeast: actual >= authored
```

No selected unit means false.

## 7.2 Resource at least

`resourceAtLeast` reads the acting unit's current AP or MP pool and compares
`current >= amount`. It does not inspect maximum/refill/penalty or target another unit.

## 7.3 Distance

Resolve the selector and compare current x/z Manhattan distance inclusively with
`atMost|atLeast`. No selected unit means false. Do not use path MP cost.

## 7.4 Status/status tag

Resolve the selector and call the step-10 status query. Compare actual presence to authored
`present`. No selected unit is false even when `present == false`; absence of a candidate
is not evidence that a target lacks a status.

## 7.5 Ability affordable

The acting unit must own the ability and resolve its definition. Return true only when
current AP and current MP cover authored cast costs. As locked in step 04, this condition
does not test range, LoS, anchor filter, affected targets, or whether a decision's selector
exists. Full legality belongs to decision planning/gateway.

Conditions mutate no state, allocate no runtime IDs, append no events, and draw no RNG.

---

# 8. Rule evaluation

For each `AIRuleDefinition` in authored order:

1. evaluate all conditions;
2. if false, continue;
3. ask the decision visitor to produce one currently legal ordinary command;
4. if no legal command is produced, record the fallthrough and continue;
5. return the first produced command immediately.

Do not reorder rules based on cost, perceived utility, C++ variant type, or prior success.
Do not cache the previously successful rule across commands.

The final unconditional end-turn fallback should make `NoRuleProducedCommand` unreachable
for a valid active profile. Keep a driver safety fallback anyway; runtime corruption must
end safely instead of hanging.

---

# 9. Cast-decision common rules

`AICastAbilityDecision` first requires that the actor owns the named ability, can afford
it, and that the complete effect runtime supports it. Enumerate/legalize anchors through
`BattleQueryService`; never duplicate range, Range bonus, LoS, filters, AoE, or target
capture in AI.

The selected command contains only:

```text
CastAbilityCommand { acting unit, authored ability ID, selected anchor }
```

Do not pass the preview's target list/effect outcomes into submission.

If an anchor spec cannot produce a legal anchor, this decision produces no command and rule
evaluation continues. It does not automatically end, move, or choose another ability.

---

# 10. Unit anchor

For `AIUnitAnchor`:

1. resolve its unit selector using section 6;
2. read that unit's exact current support cell;
3. call `planCast` for that one cell;
4. return it only when legal.

Do not iterate second-nearest/second-lowest candidates when the selector's single winner is
not a legal anchor. The profile can express another rule/anchor fallback. This keeps selector
meaning independent of ability geometry.

---

# 11. Self-cell anchor

`AISelfCellAnchor` proposes the acting unit's exact current support cell and calls
`planCast`. It does not bypass min range, self/empty filters, LoS, or ability ownership.

---

# 12. Best-area anchor

For `AIBestAreaAnchor`, enumerate all currently legal anchors/plans for the ability.
For each plan, count only captured active affected units:

* enemy-preferred: preferred = opposing side; non-preferred = acting side;
* ally-preferred: preferred = acting side, including self when captured; non-preferred =
  opposing side.

The ability's `affectedFilter` already decided membership. Do not add units that the plan
excluded and do not penalize/credit empty affected cells.

Choose lexicographically:

1. largest preferred affected-unit count;
2. smallest non-preferred affected-unit count;
3. canonical board-cell anchor order.

This exact score comes from the step-04 schema. Do not add damage estimates, missing HP,
distance, AP efficiency, kill prediction, status value, or random ties.

A legal zero-unit area can win only when no legal candidate affects a preferred unit and its
score/order wins normally.

---

# 13. Nearest-legal-cell-to anchor

Resolve the authored unit selector. Enumerate all legal anchors/plans and choose:

1. minimum x/z Manhattan distance from anchor to selected unit's current support cell;
2. canonical board-cell anchor order.

Do not use path cost or choose the selected unit's cell without validation. No selected
unit/legal anchor means no command.

---

# 14. Movement-decision candidate set

For `AIMoveTowardDecision` and `AIMoveAwayDecision`:

1. resolve the selector and its current support cell;
2. compute budget:

```text
maximumMovementPoints == 0:
    budget = actingUnit.currentMP
otherwise:
    budget = min(currentMP, authored maximumMovementPoints)
```

3. enumerate step-08 legal non-origin `MovePlan` values whose total weighted MP cost is
   within budget;
4. score only plans whose destination strictly improves the intended objective compared
   with the actor's current x/z Manhattan distance.

Strict improvement means:

* toward: destination distance < origin distance;
* away: destination distance > origin distance.

This prevents an "approach" rule from spending MP sideways or an "away" rule from moving
closer when blocked. If none improves, the rule produces no command and falls through.

Terrain cost is a budget/tie value, not the distance objective. Never submit a path or cost;
return `MoveCommand { unit, selected destination }`.

---

# 15. Movement scoring and ties

Choose among improving candidates:

For `moveToward`:

1. smallest destination Manhattan distance to selected unit;
2. smallest total weighted MP cost;
3. fewest entered cells;
4. canonical destination cell.

For `moveAway`:

1. largest destination Manhattan distance;
2. smallest total weighted MP cost;
3. fewest entered cells;
4. canonical destination cell.

The selected `MovePlan` already has canonical path ties. AI does not prefer spending all
MP, avoid traps, predict reactions, or reserve AP/MP unless encoded by ordered rules.

---

# 16. End-turn decision

`AIEndTurnDecision` always produces
`EndTurnCommand{actingUnit, EndTurnRequestCause::Explicit}` when that unit currently owns a
valid activation. It is submitted through the same gateway with `EnemyAi`; it receives
ordinary action/event/phase behavior.

There is no direct `endActivation()` call for AI.

---

# 17. Re-evaluation after successful commands

The driver executes at most one AI command per logical operation:

1. confirm the same active AI unit/TurnIndex;
2. run safety guards;
3. call planner from rule zero;
4. submit immediately through `BattleSession::submit`;
5. return control to the pump/caller with the accepted batch;
6. if the same AI activation remains, the next operation starts at rule zero with a new
   snapshot/query.

Do not plan an entire turn in advance. A movement trap, status hook, resource drain,
teleport, defeat, or changed legal anchor from the previous command must affect the next
choice.

This segmentation also lets step 12 publish/animate complete batches without exposing
intermediate resolver state.

---

# 18. AI activation guard state

Store deterministic scratch for the currently AI-driven activation, equivalent to:

```cpp
struct AIActivationGuardState
{
    TurnIndex turn;
    BattleUnitId unit;
    std::uint32_t acceptedNonEndCommands = 0;
    std::vector<std::uint64_t> observedProgressDigests;
};
```

Initialize it when AI first pumps that activation; clear it when active unit/TurnIndex
changes or the activation ends. Debug autoplay may explicitly attach the same guard to a
player activation with an override profile; production waiting-for-player state does not.

Maintain two digest concepts if necessary:

* `gameplayProgressDigest`: authoritative material gameplay state excluding events/action/
  batch counters, the session's generic `resolvedNonEndCommands`, and this guard's count/list;
  used to detect no progress;
* full `authoritativeBattleStateDigest`: includes generic `resolvedNonEndCommands` plus guard
  unit/turn/count/observed digest sequence because those values can affect the next cap/
  command and therefore matter to replay/determinism.

Do not make digest computation recursive by including the observed set in the digest being
observed.

The guard is authoritative orchestration scratch, not a `BattleSnapshot` field. Its
initialization, observed-digest append, accepted-command increment, and activation-end clear
occur between committed simulation batches without gameplay events; include them only in the
full authoritative digest and replay pump checkpoint. Exclude them from
`BattleSnapshot`/`battleSnapshotDigest` so `previousBatch.after == nextBatch.before` remains
exact. AI behavior still cannot mutate material battle state except through the gateway.

Preserve observed digests in insertion order; membership lookup acceleration may not change
serialized order.

---

# 19. Repeated-state guard

Immediately before planning a non-end command:

1. compute current `gameplayProgressDigest`;
2. if it already appears in this activation's observed sequence, submit an ordinary
   `EndTurnCommand{unit, EndTurnRequestCause::AiRepeatedStateGuard}`;
3. otherwise append it to observed scratch and continue.

This catches a successful zero-cost/no-applied-change ability because its event/action IDs
do not make gameplay progress. It also catches longer deterministic cycles that return to a
prior material state.

The forced end is not a fabricated gameplay effect. It uses the gateway and produces the
exact `ActivationEndReason::AiRepeatedStateGuard` payload.

Do not merely compare adjacent snapshots and do not include trace/log counters, or a no-op
cast would appear productive forever.

---

# 20. AI command cap

`acceptedNonEndCommands` increments after each accepted AI move/cast and not for rejected
commands or the terminating end command.

Before planning, if it is at least
`gameRules.battle.maxAiCommandsPerActivation`, submit ordinary `EndTurnCommand` with
`EndTurnRequestCause::AiCommandCap`, producing the same-named activation-end reason.

The session's generic `maxCommandsPerActivation` remains authoritative for all
controllers. When that generic cap is reached by an accepted command, the session
automatically ends the activation in the same batch as established by the command layer;
the AI driver must observe the changed phase and stop. Since AI max is less than or equal to
generic max, neither guard can spin trying to submit an illegal extra command.

Every accepted zero-cost cast counts. Do not exempt "support" rules.

---

# 21. Planned-command rejection policy

The planner uses current query results, and submission is immediate on one simulation
thread, so a rejection normally indicates a bug, stale terrain, or invariant drift.

Policy:

1. a rejected submission appends no battle event and changes no authoritative battle state,
   per the gateway contract;
2. record a separate owned `AICommandDiagnostic` containing battle/turn/unit/profile/rule/
   command/rejection/current digest;
3. if the progress digest changed between planning and rejection due to an explicitly
   supported outer orchestration boundary, replan once on the fresh state;
4. if unchanged, or if a second rejection occurs, submit ordinary `EndTurnCommand` with
   `EndTurnRequestCause::AiInvalidPlannedCommand`;
5. if even EndTurn is rejected because state already became terminal/changed activation,
   return the new stable pump state; otherwise a same-state EndTurn rejection is a technical
   abort/invariant, not a retry loop.

Do not try the next rule after gateway rejection: legal-decision fallthrough happened
inside the planner. Do not append a fake gameplay event for a rejected command.

---

# 22. Missing/corrupt profile fallback

Session construction already rejects missing enemy profiles. Defensively, if an active AI
unit has no resolvable profile or the planner reaches `NoRuleProducedCommand`, request
ordinary EndTurn with `EndTurnRequestCause::AiNoRuleProducedCommand`. In debug/tests,
assert/report the definition invariant. No free-form diagnostic string becomes the
authoritative activation-end reason.

Do not select a hard-coded attack, nearest move, or profile ID in C++.

---

# 23. No RNG and deterministic candidate limits

No v1 AI condition, selector, anchor, or decision is random. The planner must not call:

* `BattleRng`;
* `std::shuffle`, random engine/device, or hash-order pseudo-randomness;
* wall clock/frame counter;
* renderer/physics/camera state.

Assert the RNG draw count is unchanged by planning and by AI-specific orchestration.
Commands themselves currently draw no combat RNG either; future authored random decisions
would require an explicit schema/version/stream contract and are outside this series.

Candidate work is bounded by current active units, board cells, and legal query results.
Visit each rule/condition/candidate no more than the documented straightforward bound. Do
not repeatedly recompute the same legal-anchor vector inside one anchor decision.

---

# 24. Battle pump seam

Provide a headless, bounded orchestration value:

```cpp
enum class BattlePumpState
{
    NeedsDeployment,
    WaitingForPlayer,
    WaitingForAI,
    Progressed,
    Terminal
};

struct BattlePumpResult
{
    BattlePumpState state;
    std::size_t logicalOperations = 0;
    std::vector<AICommandDiagnostic> diagnostics;
};

class BattlePump
{
public:
    BattlePumpResult advanceUntilPlayerInput(
        BattleSession &p_session,
        std::size_t p_maxLogicalOperations);
};
```

One logical operation is one scheduler boundary/activation selection or one submitted AI
command. The pump:

* returns `NeedsDeployment` during manual deployment;
* in `AwaitingActivation`, calls the deterministic scheduler advance;
* on Player activation, returns `WaitingForPlayer` without mutation;
* on Enemy activation, invokes at most budgeted AI driver operations;
* returns `WaitingForAI` when the operation budget ends mid-AI activation;
* returns `Terminal` without further mutation once terminal.

`Progressed` may be returned when the budget ends immediately after a scheduler/action
boundary and the caller should pump again.

A budget of zero mutates nothing. Splitting N deterministic logical operations across
several calls must produce identical battle state/events to one call with budget N. Render
delta is not an input.

Step 12's `ModeManager::updateBattle` calls
`BattlePump::advanceUntilPlayerInput` with a small per-frame logical budget,
publishes completed batches after each call, waits for player input, and queues terminal
exit only at a safe frame boundary.

---

# 25. Debug autoplay seam

Allow the planner/driver to accept an explicit AI behavior override ID for development-only
player autoplay. The debug launch/configuration supplies that non-empty ID, resolves it
through the ordinary AI registry, and copies it into the driver; it is not written into the
`BattleUnit` or species and C++ supplies no content-ID default.

Debug autoplay:

* places/confirms player units through ordinary commands;
* on a Player activation, invokes the same planner and guard with
  `CommandIssuer{CommandController::DebugAutoplay}`, never the UI's `Player` issuer;
* on Enemy activation, uses its encounter profile and `EnemyAi`;
* on every Player activation, uses the one explicitly selected debug override profile;
  unavailable cast rules simply produce no legal candidate and continue through the
  profile's ordinary fallback—there is no per-species hard-coded substitution;
* never calls resolver/context/outcome setters;
* is disabled for production/manual battles unless explicitly requested later in step 12.

Controller issuer is a driver parameter derived from the side; the AI planner itself is
side-agnostic.

---

# 26. Required selector tests

Test every selector with:

* single and multiple candidates;
* no candidate;
* y differences ignored for nearest;
* ratio versus raw-HP disagreement;
* exact permille truncation;
* roster-order tie then unit-ID tie;
* ally selectors excluding self;
* impressed/defeated/undeployed/pending-HP-zero exclusion;
* repeat calls producing identical ID and no RNG/state change.

Use insertion-order permutations to prove lookup container order does not matter.

---

# 27. Required condition tests

Test:

* empty AND identity and explicit always;
* ordered short-circuit diagnostics;
* health atMost/atLeast exact boundaries and truncation;
* AP/MP current resource exact boundary;
* Manhattan distance exact boundary;
* present/absent status and tag;
* missing selector false even for `present:false`;
* ability ownership and AP-only/MP-only/both affordability;
* affordable but no legal anchor remains true;
* no snapshot, ID, event, digest, or RNG mutation.

Parser tests remain in step 04; do not duplicate JSON parsing here except integration
resources.

---

# 28. Required anchor-decision tests

Test:

* unit anchor success and selected-unit illegal-anchor fallthrough without trying
  second-nearest;
* self cell through the full validator;
* bestArea preferred-max/nonpreferred-min/canonical tie;
* self counted as same-side for ally preference only when captured;
* affected filter excludes friendly-fire score;
* legal empty AoE;
* nearestLegalCellTo distance/canonical tie;
* range bonus, LoS, filters, height, objects, status-changed resources all come from the
  query service;
* command contains one anchor and no captured target/path values.

Assert candidate enumeration is performed once per decision and consumes no RNG.

---

# 29. Required movement-decision tests

Test toward/away with:

* weighted terrain budget versus cell distance;
* maximum 0 using all current MP;
* positive maximum clamped to current MP;
* strict improvement and blocked/no-improvement fallthrough;
* primary distance objective;
* lower weighted cost, fewer steps, canonical cell ties;
* occupancy/object/trap-updated candidates after a prior command;
* no destination/path/cost direct mutation;
* gateway recomputing the final selected path.

Include equal x/z distances on different y levels and prove y is ignored in objective but
the exact board cell remains canonical.

---

# 30. Required rule/re-evaluation tests

Build in-memory profiles and assert:

* first matching legal rule wins;
* false condition falls through;
* true condition plus illegal decision falls through;
* final end fallback;
* after a move, rule zero is reconsidered and a newly legal cast wins;
* after a status/resource/trap reaction, the next choice uses the new snapshot;
* no whole-turn cached command list exists;
* profile/rule trace IDs are exact;
* missing/corrupt profile safety end;
* planner output submitted with the appropriate ordinary issuer.

Golden-test the command and event sequence, not only the terminal outcome.

---

# 31. Required safety tests

Create:

1. a zero-cost ability whose resolved state does not materially change;
2. two abilities/moves that return to a prior material state;
3. a profile that can issue productive commands beyond AI cap;
4. a fixture forcing planned-command rejection through a controlled stale-state seam;
5. a no-rule result behind a fixture-only invalid profile.

Assert:

* observed progress digest detects both same-state and cycle;
* event/action counters do not fool the guard;
* forced end uses the ordinary command path and exact reason;
* every accepted non-end action counts toward AI cap;
* generic command cap remains authoritative;
* one replan maximum, then safe end/technical invariant;
* no infinite loop and no RNG draw;
* full state digest includes guard scratch and generic resolved-command count while progress
  digest includes neither and does not recurse;
* a zero-cost applied-no-change command increments both command counters and changes the full
  digest, but repeats the progress digest; the next plan therefore submits
  `EndTurn{AiRepeatedStateGuard}` rather than being fooled by those counters;
* initialization/append/increment/clear around split pump budgets never changes either
  adjacent batch snapshot, and batch `after`/next `before` continuity remains exact.

---

# 32. Pump/determinism tests

Test all pump states and budgets:

* zero budget no mutation;
* deployment wait;
* one scheduler operation to Player wait;
* one scheduler operation to Enemy then one AI command;
* budget ending mid-AI activation returns WaitingForAI;
* terminal idempotence;
* split budgets versus one larger budget yield identical events/full digest/RNG;
* committed batches become externally publishable only after each operation resolves;
* no frame time affects scheduler or AI.

Run the same fixed profile battle at least twice in separate session instances and compare:

* selected rule IDs and ordinary commands;
* accepted/rejected results;
* all typed events and action/turn IDs;
* per-command progress/full digests;
* terminal outcome and simulated time;
* zero AI RNG draws.

---

# 33. Headless AI-vs-fixture checkpoint

Add a deterministic debug/test scenario:

1. construct/deploy the synthetic training battle through ordinary placement;
2. attach `training-aggressive` to the enemy;
3. attach an explicit debug profile to the player only for autoplay;
4. pump with a bounded operation budget;
5. trace time/turn/unit/profile/rule/command/result/digest at every operation;
6. demonstrate movement making a cast legal, a status/trap affecting reevaluation, and
   explicit/end-guard behavior;
7. reach an ordinary terminal outcome;
8. repeat and compare the exact trace.

Keep the trace development-only and headless. Normal exploration still boots and no battle
starts automatically until lifecycle work in step 12.

---

# 34. Event, snapshot, and diagnostics policy

AI decisions are not gameplay effects and need not add `BattleEvent` payloads. Ordinary
submitted commands already create the authoritative gameplay events with action/turn
windows. If an `AIDecisionMade` diagnostic event is added, categorize it Diagnostic and
exclude it from condition evaluation/progress digest; rejected submissions still must not
append a command batch.

Prefer `AICommandDiagnostic` returned/stored outside the gameplay log for planner bugs.
It must own IDs/command/error/digests and never contain pointers.

Extend only the full authoritative digest/replay-pump checkpoint—not `BattleSnapshot` or
`battleSnapshotDigest`—with guard scratch that affects future deterministic behavior. Do not
store a cached planned command/target in authoritative state.

---

# 35. Expected file changes

Adapt paths to repository conventions, with files equivalent to:

```text
Playground/srcs/battle/ai/battle_ai_planner.hpp
Playground/srcs/battle/ai/battle_ai_planner.cpp
Playground/srcs/battle/ai/battle_ai_driver.hpp
Playground/srcs/battle/ai/battle_ai_driver.cpp
Playground/srcs/battle/battle_pump.hpp
Playground/srcs/battle/battle_pump.cpp
Playground/srcs/battle/battle_session.cpp
Playground/srcs/battle/battle_snapshot.hpp
Playground/tests/battle/ai_selector_condition_tests.cpp
Playground/tests/battle/ai_anchor_tests.cpp
Playground/tests/battle/ai_movement_tests.cpp
Playground/tests/battle/ai_rule_driver_tests.cpp
Playground/tests/battle/ai_safety_tests.cpp
Playground/tests/battle/battle_pump_tests.cpp
Playground/tests/battle/ai_fixture_trace_tests.cpp
Playground/docs/battle.md
```

Update CMake source/test lists. Modify step-04 AI parsing only if a real handoff defect is
demonstrated; do not change JSON version/literals. Do not add new content profiles beyond a
minimal test/debug fixture unless step 19 owns them.

---

# 36. Documentation requirements

Document:

* planner/driver/session/pump ownership;
* every selector, condition, anchor, decision, and missing-candidate rule;
* health ratio and Manhattan distance formulas;
* rule fallthrough and top-down reevaluation;
* exact best-area/nearest-legal/movement scores and ties;
* weighted MP budget versus Manhattan objective;
* common gateway/issuer requirement;
* AI and generic command caps;
* progress-digest cycle guard and full-digest scratch;
* rejected-command retry/end policy;
* complete absence of RNG/cheats;
* battle pump wait states/logical budget and frame-delta independence;
* debug player autoplay override versus production manual control.

Remove stale docs that say the AI directly executes an ability, selects a random target,
plans a whole turn once, or mutates `BattleUnit`.

---

# 37. Non-goals

Do not implement new AI JSON alternatives, behavior trees, utility scoring, scripts,
blackboards, learning, difficulty scaling, random selection, fog/hidden information,
predictive damage simulation, trap avoidance, path reservation, multi-turn plans, retreat/
flee, player battle input, battle rendering/HUD, lifecycle mode transitions, authoring
tools, or balance tuning.

Do not give AI free resources, extra range, hidden targets, direct resolver access, or a
different command error policy.

---

# 38. Acceptance criteria

This step is complete only when:

* [ ] one pure planner implements the exact step-04 closed AI catalog;
* [ ] all unit selectors filter active combatants and use roster/ID ties;
* [ ] conditions use checked integer/current-state semantics and AND order;
* [ ] cast anchors come only from shared legal plans with locked scores;
* [ ] movement comes only from shared legal plans, weighted budget, strict improvement, and
      locked ties;
* [ ] true-but-illegal decisions fall through to later rules;
* [ ] every accepted command goes through `BattleSession::submit` with ordinary issuer;
* [ ] rules restart at zero after each accepted state change;
* [ ] no planned target/path survives across commands;
* [ ] AI command cap, progress-digest cycle guard, and invalid-plan fallback terminate safely;
* [ ] full digest captures future-affecting AI scratch without recursive progress hashing;
* [ ] AI planning/orchestration draws no RNG;
* [ ] bounded pump states are stable and frame-budget segmentation is outcome-neutral;
* [ ] repeated AI fixture traces match command/event/digest/outcome exactly;
* [ ] full Playground build/tests and normal exploration smoke run stay green.

---

# 39. Required handoff report

Report:

1. exact planner/driver/pump types and public methods;
2. exact selector eligibility/ratio/distance/tie algorithms;
3. condition semantics and missing-candidate behavior;
4. every anchor score/tie and movement objective/budget/tie;
5. rule fallthrough/re-evaluation behavior;
6. common command submission and rejected-plan policy;
7. AI/generic command caps and repeated-state guard;
8. progress versus full digest coverage;
9. pump wait states and logical-operation budgeting;
10. diagnostics/event policy and RNG proof;
11. files created/materially changed/deliberately untouched;
12. repository drift/adaptations;
13. commands actually run and results;
14. exact repeated headless trace summary;
15. work explicitly deferred to lifecycle/presentation/later content steps.
