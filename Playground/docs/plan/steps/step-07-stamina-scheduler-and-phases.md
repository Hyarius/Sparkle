# Implement the stamina scheduler and battle phase machine

Implement deterministic simulated-time advancement, stamina-bar readiness, activation
selection, start-of-activation AP/MP refill with one-shot penalties, explicit multi-command
activations, turn indices, terminal outcomes, and a non-hanging stall/abort policy.

This is an implementation task. Extend the headless `BattleSession` and event batches, add
tests, and keep Playground green. Do not implement movement/cast validation, effects,
status/object runtime, AI, modes, rendering, HUD, persistence, or progression evaluation in
this step.

---

# 1. Repository baseline and prerequisites

Implement after steps 01-06 and read their final handoff reports. Reinspect the live types
before editing.

The required prior contracts are:

* `BattleTime` is signed integer milliseconds; authored stamina is exact fixed time;
* `CreatureAttributes::stamina` is seconds-to-fill expressed as `BattleTime`; lower is
  faster. There is no second floating stamina-rate clock;
* `BattleSession` is the sole mutation facade and rejects commands without mutation, RNG,
  counter, or event changes;
* `BattleContext` owns ordered units, board, RNG, phase/outcome/time/counters, and the typed
  event log;
* bars/AP/MP/penalties start at zero and both sides are deployed before scheduling;
* `BattlePhase` has `Deployment`, `AwaitingActivation`, `Activation`, and `Terminal`;
* event batches have before/after snapshots and optional action IDs;
* `BattleUnit::isActiveCombatant()` means placed, HP positive, and no removal reason.

The current Unity scheduler is evidence only. It uses floats/epsilon, ignores taming-left
units in some paths, refills AP/MP at turn end, has no timed-status deadline in its jump,
and can return no next unit without a terminal policy. Do not port those defects.

---

# 2. Required final behavior

After this step, a headless placed session can:

1. Start with every turn bar empty at battle time zero.
2. Jump simulated time exactly to the next readiness/timeline boundary.
3. Select the ready unit by the locked side/roster/ID tie order.
4. Assign a monotonic `TurnIndex` and refill that unit's AP/MP at activation start.
5. Consume explicit next-activation penalties after refill.
6. Pause all battle time while any activation is awaiting/resolving commands.
7. Accept explicit `EndTurnCommand` through the common gateway.
8. Reset only the actor's bar at activation end, then schedule again.
9. Preserve partially filled bars of every other unit.
10. End with PlayerVictory, PlayerDefeat, Draw, or an explicit technical Aborted result.
11. Produce the same event trace/digest for the same setup and end-turn sequence.

Movement/cast commands remain unavailable until step 08, but an activation may already wait
for multiple commands conceptually; ending it is explicit and does not advance real time.

---

# 3. Locked stamina model

Use these exact semantics:

```text
effective bar maximum = unit.effectiveAttributes.stamina
initial fill          = 0 ms
normal fill rate      = 1 ms of fill per 1 ms of simulated battle time
ready                 = fill >= effective maximum and unit is not stunned
time to ready         = effective maximum - fill
```

Lower authored stamina is faster. Effective stamina is always clamped to at least
`gameRules.battle.minimumStamina`. No float ratio or epsilon is involved.

When simulated time advances by `delta`:

```text
for each active, non-stunned unit in canonical unit order:
    fill = min(effectiveStamina, checked(fill + delta))

for each active stunned unit:
    fill is unchanged

timeline durations:
    advance by the same delta even for stunned/defeated owners as specified in step 10
```

Bars start empty. A stunned unit keeps its fill exactly. If it was full when stunned, it is
not selectable until stun expires; then it is immediately ready at the same battle time.
Timeline-duration stun is the only authored stun form, so simulated time can normally reach
its expiration even while every unit is stunned.

Do not advance bars from `spk::UpdateContext`, animation duration, UI delay, wall time, or
render delta. `BattleMode::update` later calls an instantaneous scheduler pump, not
`advance(frameSeconds)`.

---

# 4. Timeline boundary seam

The scheduler must account for timed status/shield/object expiration before those runtime
containers exist. Add final-purpose context queries that step 10 fills in, for example:

```cpp
[[nodiscard]] std::optional<BattleTime>
BattleContext::timeUntilNextTimelineBoundary() const;

void BattleContext::advanceTimeline(
    BattleTime p_delta,
    BattleEventWriter &p_events);
```

In this step the empty runtime returns `nullopt` and performs no duration mutation. Do not
add a generic public callback list or a temporary fake status system.

`timeUntilNextTimelineBoundary()` returns a non-negative duration relative to the current
battle time. Step 10 guarantees authored finite durations are positive and processes a
zero result immediately before another scheduler calculation. It returns the minimum over
all timeline statuses, shields, and objects; equal expirations resolve later in stable
instance order.

The next scheduler delta is:

```text
minimum(
    time-to-ready for every active non-stunned unit not already ready,
    next positive timeline boundary)
```

If a ready eligible unit exists now, select it without advancing time. If a timeline
boundary occurs first, commit that time-advance/expiration batch, recalculate readiness and
deadlines, and continue. Never compute one large jump past a status expiration that changes
stun or stamina.

---

# 5. Scheduler API

Use a headless service owned/called by the session:

```cpp
enum class SchedulerStop
{
    ActivationReady,
    Terminal,
    Aborted
};

struct SchedulerAdvanceResult
{
    SchedulerStop stop;
    std::optional<BattleUnitId> activeUnit;
    std::vector<BattleBatchId> committedBatches;
};

enum class SchedulerRejection
{
    WrongPhase,
    SessionBusy
};

struct RejectedSchedulerAdvance
{
    SchedulerRejection reason;
};

using SchedulerCallResult =
    std::variant<SchedulerAdvanceResult, RejectedSchedulerAdvance>;

class StaminaScheduler
{
public:
    [[nodiscard]] static SchedulerCallResult advanceUntilActivation(
        BattleContext &p_context,
        BattleEventWriter &p_events);
};
```

Equivalent placement within `BattleSession` is acceptable. The public call should be:

```cpp
[[nodiscard]] SchedulerCallResult BattleSession::advanceUntilActivation();
```

It is legal only in `AwaitingActivation`. `Deployment`, `Activation`, or `Terminal` returns
`RejectedSchedulerAdvance{WrongPhase}` with no mutation; re-entry returns `SessionBusy`.
Guard it exactly like `submit`. A successfully executed scheduler call that discovers a
technical stall returns `SchedulerAdvanceResult{stop=Aborted}` after committing the one
`TechnicalAbort` batch; it is not a scheduler-call rejection.

Each distinct positive time jump/timeline boundary commits a no-action event batch with
before/after snapshots. Starting an activation at the current boundary may be included in
that final scheduler batch. If a unit was already ready and no batch is open, commit one
no-action batch containing start/refill events. No empty batch is created.

---

# 6. Ready selection and ties

Collect only units for which all are true:

```text
isActiveCombatant()
not currently stunned
turnBarFill >= effective stamina
```

Sort/select by this exact tuple:

```text
(side priority: Player before Enemy,
 rosterOrder ascending,
 BattleUnitId ascending)
```

Do not use map/vector insertion order as an undocumented shortcut, even though normal ID
allocation currently follows roster order. Explicitly compare the tuple and test a fixture
whose storage insertion order differs.

Only one unit becomes active. Other tied units retain full bars and will be selected at the
same battle time after the chosen unit ends; player commands do not let time advance ahead
of those ties.

Removed (`Defeated` or `Impressed`) and undeployed units are never candidates. This closes
the Unity issue where a wild unit that left can still receive initiative.

---

# 7. Activation start and resource refill

Assign `TurnIndex` values starting at 1, checked and monotonically increasing. Allocate the
index only once the selected unit is known and the transition will commit.

Activation start ordering is exact:

```text
1. set phase = Activation and activeUnit = selected unit
2. allocate/attach TurnIndex
3. refill current AP to effective max AP
4. refill current MP to effective max MP
5. apply and consume the stored one-shot AP penalty
6. apply and consume the stored one-shot MP penalty
7. append ActivationStarted with final post-penalty pools and position distances
8. step 10 runs activation-start status/passive/object hooks at the defined insertion point
9. after every hook/trigger effect boundary, finalize pending defeat/removal; if the active
   owner was removed, stop the remaining start pipeline and consume its pending structural
   close as `ActivationEnded{ActiveUnitDefeated|ActiveUnitImpressed}` in this same no-action
   batch, reset only its bar, clear ownership, then evaluate outcome
10. only a surviving actor proceeds to the ordinary legal-command check
```

The locked refill formula for each resource is:

```text
refilled = effective maximum
final    = max(0, refilled - accumulated next-activation penalty)
pending penalty becomes 0 regardless of how much could be subtracted
```

Use checked integer arithmetic. A penalty may exceed the maximum; it yields zero but never a
negative pool and never carries excess into a second activation.

Emit `ResourceChanged` for an effective refill change and another for an effective
penalty subtraction, with before/after and exact reasons `activationRefill` and
`nextActivationPenaltyConsumption`. The earlier `NextActivationPenaltyApplied` was emitted
when the effect accumulated the penalty. Emit no separate penalty-consumed payload; condition
progress distinguishes the effective accumulation from later resource-pool consumption by
the typed families and reasons.

Append `ActivationStarted` after both pools have their final values. Its optional
closest-active-ally/enemy distances are minimum X/Z Manhattan distances from the actor's
current support cell; absent if no qualifying unit exists.

This start refill overrides the older decision-log sentence and Unity implementation that
refilled at turn end. The GDD and current plan index lock **start-of-activation** refill.
Do not reset/refill AP or MP at activation end.

---

# 8. Phase transitions

Use this state machine:

```text
Deployment
    both sides confirmed
        -> AwaitingActivation

AwaitingActivation
    scheduler boundary + ready selection
        -> Activation(active unit, TurnIndex)
    outcome/technical failure
        -> Terminal

Activation
    successful move/cast (steps 08-10)
        -> Activation (same unit, no time advancement)
    EndTurn
        -> AwaitingActivation or Terminal
    active unit removed / no legal command remains
        -> automatic activation end -> AwaitingActivation or Terminal

Terminal
    every command/scheduler call rejected; state immutable
```

When the Player confirmation command makes both deployment confirmations true, complete
that command's deployment event batch, set phase to `AwaitingActivation`, and append a
`DeploymentCompleted` in the same batch. Do not automatically call the scheduler from
inside `submit`; the caller/pump invokes `advanceUntilActivation()` next. This keeps command
and timeline batches explicit.

There is no separate PlayerTurn/EnemyTurn phase or AI-only phase. `Activation` plus the
active unit's side is sufficient and prevents divergent rules.

---

# 9. Multi-command activation and end-turn command

Time is frozen for the whole `Activation` phase. Successful movement/ability commands in
steps 08-10 do not alter `BattleContext::elapsed` or any other unit's bar.

`EndTurnCommand{unit, cause}` is now available and validates:

* phase is `Activation`;
* command unit exists, is the active unit, and issuer controls its side;
* issuer/cause is one of the exact combinations locked in step 06;
* session is not already resolving/terminal.

It has no AP/MP cost and is always legal for a still-active actor. It receives an action ID
and commits a non-empty batch because `ActivationEnded`/phase/bar state change.

Activation-end ordering is:

```text
1. step 10: run activation-end object triggers in locked order; after every trigger effect,
   finalize pending defeat/removal and jump to step 4 if the owner was removed
2. run activation-end status/passive hooks in locked order with the same per-effect check
3. decrement/expire captured owner durations in stable order with the same per-effect check
4. on owner removal, stop the pipeline, discard every unprocessed capture, and replace the
   requested/automatic reason with `ActiveUnitDefeated|ActiveUnitImpressed`
5. compute final position distances if the unit is still placed
6. append exactly one ActivationEnded with the final reason
7. reset this unit's turn-bar fill to zero
8. clear active unit/current TurnIndex
9. evaluate outcome
10. set Terminal, otherwise AwaitingActivation
```

An activation ended early by the actor's defeat or impression still closes its semantic
turn, but it does not count as an owner-activation duration tick. Removal cleanup destroys
that owner's transient statuses/shields and owner-activation objects with hooks suppressed;
discard the capture and do not emit duration-expiry effects after the actor has left.
The structural close keeps the same TurnIndex and takes precedence over
`NoLegalCommands`, Explicit, every AI end cause, and the generic command cap. A fatal start
hook closes in the scheduler/timeline batch with no action ID. A fatal end hook closes in the
already accepted EndTurn/action batch; it does not create a second command or append the
original requested reason first.

The reset affects only the unit that owned the activation. Other units keep exact fill,
including full tied bars. AP/MP retain their final values until the next activation's start
refill (or effects change them while waiting).

Steps 08-10 may end automatically when the active unit is removed or no legal move/cast
exists. That automatic end is part of the successful command's same action batch and uses
the same ordering/reason; it does not fabricate a second `EndTurnCommand`/action ID.

Track `resolvedNonEndCommands` on the activation, initialized to zero. Every accepted move
or cast increments it exactly once, including a zero-cost cast whose applied effects happen
to make no material change. Rejected commands and `EndTurn` do not increment it. After an
accepted non-end command finishes all reactions, if the count reaches
`gameRules.battle.maxCommandsPerActivation` and the same actor still owns a non-terminal
activation, end that activation automatically in the **same action batch** with reason
`CommandCap`. Do not accept one command beyond the cap and do not allocate a second action
ID for the automatic end.

Step 11 adds the lower/equal AI-specific cap. The generic cap is authoritative for Player,
AI, debug, and tests and is also the final protection against repeated zero-cost actions.

The generic count is simulation state mutated inside the accepted command transaction:
extend `BattleSnapshot` and the full authoritative digest with it so batch replay sees the
future cap boundary. Deliberately exclude it from `gameplayProgressDigest`; otherwise a
zero-cost applied-no-change cast would look materially productive and defeat step 11's
repeated-state AI guard.

---

# 10. Status-hook insertion points reserved now

Step 10 implements the actual hook engine. Preserve these insertion points:

```text
ActivationStarted/refill complete
    -> status/passive ActivationStart hooks
    -> object UnitActivationStartedOnCell triggers
    -> after each effect boundary: if owner removed, stop and structural-close now

explicit/automatic end requested
    -> object UnitActivationEndedOnCell triggers
    -> status/passive ActivationEnd hooks
    -> owner-duration decrement/expiration
    -> after each effect boundary: if owner removed, stop/discard capture and override reason
    -> ActivationEnded
```

If step 10's final event ordering needs an event-before-hook signal for condition windows,
update both files together; never fire hooks by subscribing reentrantly to the public event
provider. The resolver calls the hook/object services explicitly and they append to the
current batch.

Stun controls only bar fill/ready selection. Applying stun to a unit whose activation has
already begun does not retroactively revoke that activation. A stunned active unit may end
its turn; authored v1 content should normally apply stun before readiness.

---

# 11. Outcome semantics

Evaluate outcomes only after Deployment completes and at a stable resolution boundary,
never halfway through an effect list. Use `isActiveCombatant()` counts:

```text
player active > 0, enemy active == 0 -> PlayerVictory
player active == 0, enemy active > 0 -> PlayerDefeat
player active == 0, enemy active == 0 -> Draw
otherwise                           -> Undecided
```

Impressed enemies are inactive but not defeated. If every enemy leaves impressed while a
player remains, the ordinary rule yields PlayerVictory with zero defeat credit.

Define the permanent small terminal value used by lifecycle code and later embedded in the
full step-18 result:

```cpp
struct BattleTerminalRecord
{
    BattleId battleId;
    BattleOutcome outcome;
    std::optional<BattleAbortReason> abortReason;
};
```

It is constructible exactly once when phase becomes Terminal. `abortReason` is present if
and only if outcome is Aborted and matches the sole `BattleAborted` payload. This is not a
progression/recruit/result-summary bag; it is the stable core terminal identity.

On a terminal transition:

1. close any active activation using its appropriate typed reason;
2. set outcome and phase `Terminal` exactly once;
3. append one `BattleEnded` with active and not-defeated counts for both sides;
4. commit the batch, after which all commands/time advancement are rejected.

There are two distinct closure paths and they must not be conflated:

* If the active actor was defeated or impressed before outcome evaluation, close it first
  with `ActiveUnitDefeated` or `ActiveUnitImpressed`: skip ordinary end-on-cell and
  ActivationEnd hooks because the actor is removed; run the stable hook-suppressed owner
  cleanup, append `UnitRemoved`, discard the captured duration IDs, append
  `ActivationEnded`, reset its bar, and clear turn ownership before evaluating outcome.
* If ordinary outcome evaluation becomes terminal while an otherwise active actor still
  owns the turn, close the semantic turn structurally with `BattleTerminal` immediately
  before `BattleEnded`. This terminal-only close runs no end-on-cell/status/passive hooks
  and does not decrement owner-activation durations: the fight is already decided and no
  post-decision gameplay mutation is allowed. It still appends `ActivationEnded`, resets the
  actor's bar, and clears turn ownership so step 15 has no open turn window.

A technical abort during an activation uses the same structural path with
`TechnicalAbort`. These structural policies make the selected terminal outcome immutable
and make prospective taming equivalence finite; do not run arbitrary activation-end effects
after selecting a terminal result.

Step 09 finalizes HP-zero removals after each effect application but captures the cast's
targets in advance and waits until the complete command batch to choose the terminal
outcome. Thus a simultaneous all-zero result becomes Draw deterministically.

`BattleOutcome::Aborted` is technical, not a draw/loss. Step 18 commits no progression,
recruits, or encounter clear for it.

---

# 12. Zero-progress stall and technical abort

The scheduler must never spin when no unit can fill and no timeline duration can expire.

After processing all due zero-time boundaries, if all are true:

```text
outcome is still Undecided
no eligible ready unit exists
no active non-stunned unit has positive time-to-ready
no positive timeline boundary exists
```

then transition to `BattleOutcome::Aborted` with stable reason
`SchedulerNoFutureProgress`:

```text
BattleAborted(reason=SchedulerNoFutureProgress)
BattleEnded(outcome=Aborted, counts...)
```

Commit one no-action `BattleBatchKind::TechnicalAbort` batch and return
`SchedulerStop::Aborted`. Do not pick a winner, treat
it as a normal draw, advance an arbitrary millisecond, poll frames, or loop until a cap.

The step-02 validator forbids owner-turn/infinite stun, and effective stamina has a positive
minimum, so ordinary authored battles should never reach this path. Keep it for corrupt
runtime state and future modifiers. Test it explicitly with a narrow fixture-only blocked
fill seam; do not add invalid production JSON merely to trigger it.

Also guard repeated zero-time boundary processing. If a supposedly due duration remains due
without a state/digest change, abort with `TimelineBoundaryMadeNoProgress` rather than spin.

---

# 13. Event batching and ordering

Scheduler/event order is externally observable. Golden-test it.

For a positive time jump that produces readiness:

```text
BattleTimeAdvanced(previous, delta, new)
[timeline expiration/removal events in step-10 canonical order]
[resource refill events AP then MP]
[resource penalty-consumption events AP then MP]
ActivationStarted
[activation-start hooks/triggers in step 10]
```

For a boundary that does not yet produce readiness, commit after expiration events and
start a fresh batch for the next jump. For an already ready unit, omit
`BattleTimeAdvanced`; the batch starts with resource/start events.

For explicit `EndTurn`, all events carry the command action ID. Automatic end after a
move/cast also carries that command action ID. Pure scheduler batches have no action ID.

`ActivationStarted` and `ActivationEnded` carry the assigned `TurnIndex`; every event inside
that activation uses it in the header. The turn header becomes empty only after end
finalization.

No event is published through `ContractProvider` until its entire batch and after snapshot
are committed and the session resolution guard is released.

---

# 14. Deterministic algorithms and overflow

All selection and arithmetic use integer values:

* checked `BattleTime` subtraction/addition;
* no epsilon comparisons;
* explicit canonical unit tuple;
* checked turn/action/batch/event counters;
* checked AP/MP penalty accumulation/application;
* stable event order even when values do not change (omit zero-effective
  `ResourceChanged`, but preserve `ActivationStarted`).

`advanceUntilActivation()` consumes no RNG. Initiative ties never draw randomness.

Changing effective stamina in step 10 clamps current fill immediately to the new maximum.
If the new maximum is at/below fill, the unit becomes ready at the next stable scheduler
selection; it cannot preempt a currently active command.

---

# 15. Required tests

Add headless tests under `Playground/tests/battle/scheduler/` and extend session fixtures.

## 15.1 Basic readiness

Test:

* bars start at zero;
* a 2.000-second unit readies at exactly 2000 ticks;
* lower stamina acts before higher stamina;
* no render/update delta is accepted by the API;
* only active/non-stunned units fill;
* fill clamps exactly at effective stamina;
* other bars preserve partial fill when one activates/ends;
* actor bar resets to zero only at activation end.

## 15.2 Tie order

Test Player before Enemy, roster order within side, then BattleUnitId for deliberately equal
roster order in an invariant-test fixture. Reinsert storage/maps in different orders and
assert the same activation trace. A second ready tied unit must activate at the same battle
time after the first ends.

## 15.3 Repeated schedule trace

Use stamina values 2s and 3s and golden-test several activations/timestamps, proving the
faster unit acts more often and integer partial fill is retained. Repeat from identical
setup and compare full events/digest; use a different command timing only to prove waiting
for UI does not change it.

## 15.4 Refill and penalties

Test:

* AP/MP are zero before first activation and max at start;
* refill events are AP then MP;
* current resource drain before a future activation is overwritten by refill;
* AP/MP next penalties subtract once after refill;
* penalties over maximum clamp to zero and discard excess;
* both penalties apply independently;
* changing effective max before activation changes refill target;
* end turn does not refill either pool;
* next activation refills from the then-current effective maxima.

The direct drain/penalty setup may use a fixture-only context helper until step 09 executes
the effects; it must call the same internal resource functions, not mutate private fields by
`const_cast`.

## 15.5 Phase/command behavior

Test deployment completion to `AwaitingActivation`, wrong-phase scheduler calls, start to
`Activation`, explicit `EndTurnCommand`, wrong actor/controller, repeated end, terminal
rejection, action/turn IDs, and no time advancement while activation waits.
Table-test every issuer/request-cause combination: Player accepts only Explicit; EnemyAi and
DebugAutoplay accept Explicit plus the four `Ai*` causes for their controlled side; System
and every spoofed automatic/safety cause reject atomically. Assert exact mapped
`ActivationEndReason` in the accepted event.

Unit-test the generic command-count transition at `max-1` and `max` through a narrow
fixture-only call to the same post-accepted-non-end-command boundary used later by move/cast.
Rejected-command simulation does not count; reaching max appends one automatic
`ActivationEnded(CommandCap)` to the already open fixture action batch and allocates no
second action ID. Do not add or accept a public cast in this step; step 09 owns the first
gateway-level zero-cost cast integration test.

Submit several rejected/unavailable commands while an actor waits and assert elapsed time,
all bars, counters, RNG, events, and digest do not change.

## 15.6 Outcomes

Through an internal resolver fixture, test PlayerVictory, PlayerDefeat, simultaneous Draw,
impressed-last-enemy victory without defeat credit, terminal event counts, one terminal
event only, and immutable terminal state.

## 15.7 Timeline/stall seam

Test an earlier timeline boundary is chosen before readiness, a due boundary that unblocks
a full bar yields same-time readiness, and multiple boundaries commit in order. Until step
10 provides real instances, use a small deterministic fake owned by the scheduler test
fixture behind the final context seam.

Test no eligible fill/no deadline produces exact `BattleAborted` then `BattleEnded` events,
and a zero-time boundary with no digest change aborts instead of looping.

## 15.8 Event golden sequence

Assert exact headers/payloads for:

```text
deployment completion
time jump + refill + activation start
explicit activation end
same-time tied next activation
terminal outcome
technical stall
```

Verify timeline batches have no action ID, end-turn has one, all activation events share
the correct TurnIndex, and no same-provider notification is triggered during resolution.

---

# 16. Expected file changes

Add cohesive files equivalent to:

```text
Playground/srcs/battle/scheduler/stamina_scheduler.hpp
Playground/srcs/battle/scheduler/stamina_scheduler.cpp
Playground/srcs/battle/scheduler/scheduler_result.hpp
Playground/srcs/battle/battle_phase.hpp
Playground/srcs/battle/battle_outcome_rules.hpp
Playground/srcs/battle/battle_outcome_rules.cpp
Playground/srcs/battle/activation/activation_rules.hpp
Playground/srcs/battle/activation/activation_rules.cpp

Playground/tests/battle/scheduler/stamina_scheduler_test.cpp
Playground/tests/battle/scheduler/stamina_tie_test.cpp
Playground/tests/battle/scheduler/activation_resource_test.cpp
Playground/tests/battle/scheduler/phase_machine_test.cpp
Playground/tests/battle/scheduler/outcome_test.cpp
Playground/tests/battle/scheduler/scheduler_trace_test.cpp
```

Modify `BattleUnit`, `BattleContext`, `BattleSession`, commands/results/events/snapshots,
fixtures, test CMake integration, and implemented battle documentation. Equivalent cohesive
organization is acceptable.

Do not modify engine code, widgets, `GameSceneWidget`, JSON schemas/resources, world
streaming/fluid code, or presentation.

---

# 17. Documentation requirements

Document:

* stamina as fixed seconds-to-fill with no rate/epsilon;
* bar start/reset/stun behavior;
* timeline-boundary-aware jump algorithm;
* complete phase transition table;
* exact tie tuple;
* start refill and one-shot penalty formula/order;
* time freeze throughout activation;
* end-turn ordering and no end refill;
* active/impressed outcome semantics;
* technical stall abort versus gameplay Draw;
* event batch/action/TurnIndex assignments.

Correct stale docs that say AP/MP refill at turn end, stamina uses floats, or a render-frame
tick advances initiative.

---

# 18. Non-goals

Do not implement movement, target previews, casts, effect formulas, statuses/passives,
actual timeline instances, traps, AI execution, modes, encounter triggers, world pinning,
entities, input, UI, progression/taming evaluation, results, saving, authoring tools, real
time animation, reserves/swaps, fleeing, revive, summons, or mid-battle persistence.

Do not introduce a second Player/Enemy turn controller or an AI-only scheduler.

---

# 19. Acceptance criteria

This step is complete when:

* all battle time/bar math is exact integer `BattleTime`;
* lower stamina readies sooner and partial bars are preserved;
* stun eligibility and timed-boundary seams are final-purpose;
* ties use explicit Player/roster/ID order;
* TurnIndex is assigned once per activation;
* AP/MP refill only at activation start and penalties apply/clear exactly once;
* time never advances while commands are awaited/resolved;
* explicit EndTurn uses the common command gateway;
* the generic non-end command cap terminates in the reaching command's batch;
* only the actor's bar resets at end and AP/MP do not refill there;
* terminal victory/defeat/draw uses active-combatant counts and impression is not defeat;
* no-future-progress produces a deterministic technical Aborted result, never a hang;
* scheduler/action event batches have exact IDs/order/snapshots;
* deterministic traces and all required tests pass;
* complete Playground targets still build.

---

# 20. Required handoff report

Report the exact scheduler/phase APIs, time-to-ready and tie algorithms, start refill/
penalty formula, end ordering, outcome/stall policies, event golden traces, files changed,
repository drift, and commands/tests actually run. Explicitly state that movement/casting,
effect/status/object runtime, AI, modes, external publication, and presentation remain for
later steps.
