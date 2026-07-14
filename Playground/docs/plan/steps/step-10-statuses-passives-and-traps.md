# Implement statuses, passives, durations, hooks, and battle objects

Complete the v1 effect runtime. Add battle-local status/passive instances, deterministic
stacking and duration refresh, effective-stat modifiers, post-event hooks, stun, cleanse,
shield expiration, and cell battle objects/traps. Integrate all reactions explicitly into
the current command/timeline batch through the existing `EffectResolver`; do not dispatch
reentrantly through the public event provider.

This step produces the first complete headless battle rules loop. It does not add AI,
presentation, taming/progression evaluation, content authoring tools, or a general
expression/scripting engine.

---

# 1. Repository baseline and prerequisites

Implement after steps 01-09 and read every handoff report. Reinspect exact names before
editing. Preserve:

* immutable status/passive and battle-object definitions from strict JSON registries;
* a passive as an infinite status definition listed by a creature, not a separate
  executable class/registry;
* explicit effect scopes and the single shared `EffectResolver`;
* effective runtime stats read at each concrete effect;
* fixed `BattleTime`, exact timeline-boundary scheduling, and lower Stamina being faster;
* activation-start refill/penalty before hooks;
* activation-start order: status/passive hooks, then object-on-cell triggers;
* activation-end order: object-on-cell triggers, status/passive hooks, captured
  owner-activation duration decrement, then `ActivationEnded`;
* movement occupancy committed before `unitLeftCell`, leave before enter, and no
  deployment/removal movement triggers;
* a complete event batch appended before external `ContractProvider` publication;
* same-provider reentrant dispatch suppression in the existing event system;
* the game-rule `maxEffectChainDepth` safety bound.

Step 09 deliberately rejects any ability containing `applyStatus`, `removeStatus`,
`cleanse`, `placeObject`, or `removeObjects`. Implement all five through the same
visitor, then remove the complete-effect-runtime guard for the now-complete closed payload
catalog.

Do not copy the Unity draft's MonoBehaviour status ownership, coroutine timers, frame-time
DOT ticks, unordered callback lists, ScriptableObject mutation, or physics-trigger traps.
The battle session is simulated deterministic state, not scene behavior.

---

# 2. Required final behavior

At the end of this step:

1. creature passives project as immutable-definition-backed infinite runtime status
   instances;
2. transient statuses have stable instance IDs, stacks, applier, and explicit duration state;
3. applying/reapplying a status obeys exact stack and duration-refresh rules;
4. remove-status and cleanse remove only transient instances in stable order;
5. effective stats derive from baseline plus ordered modifiers and immediately reconcile
   HP/AP/MP/bar bounds;
6. all ten authored hook kinds execute at fixed insertion points;
7. effects produced during a hook/trigger do not recursively schedule further hooks or
   triggers;
8. timeline expiration participates in scheduler boundaries before ready selection;
9. owner-activation durations decrement only instances captured at that activation's start;
10. stun pauses bar fill without clearing accumulated fill;
11. shields expire through the same duration machinery;
12. battle objects have stable IDs, cell/creator/duration/trigger-count state, movement/LoS
    blocking, five trigger kinds, and deterministic trigger filters;
13. each successful spatial transition invokes old-cell leave before destination enter;
14. full real-resource abilities, statuses, and training traps execute headlessly;
15. all events contain actual stack/amount/count changes and are appended before publication.

There is still no tick-every-frame status callback. DOT/HOT are ordinary authored effects on
timeline/activation hooks.

---

# 3. Runtime instance model

Add strong `BattleStatusInstanceId` (zero invalid), allocated monotonically from one and
never reused. Use values equivalent to:

```cpp
enum class BattleStatusOrigin
{
    PassiveProjection,
    TransientEffect
};

// DurationState and DurationSnapshot are the shared values defined in step 09.

struct BattleStatusInstance
{
    BattleStatusInstanceId id;
    std::string definitionId;
    BattleStatusOrigin origin;
    std::uint32_t stacks = 1;
    DurationState duration;
    std::optional<BattleUnitId> appliedBy;
    std::optional<std::string> sourceAbilityId;
    std::optional<std::string> sourceEffectId;
};
```

Runtime values reference definitions by stable ID; never copy executable callbacks or
store registry reader nodes.

Each battle unit may have:

* zero or one passive-projection instance for each derived passive ID, preserving derived
  passive order;
* zero or one transient instance for each status definition ID;
* both a passive and transient instance of the same definition at once.

Transient reapplication finds only the transient instance. It never adds stacks to, changes
duration of, removes, or cleanses an innate passive. Both instances contribute modifiers
and make a status/tag presence query true.

Store passive order explicitly or derive it from the creature's immutable derived list.
Store transient instances by stable instance ID; a lookup map may accelerate status-ID
access but cannot define iteration order.

---

# 4. Passive projection and battle-start timing

During `BattleUnit` projection:

1. resolve every derived passive ID in authored derived order;
2. reject the reserved `stun` tag on a passive defensively;
3. allocate one passive instance with one stack, infinite duration, and
   `appliedBy == unit.id`;
4. make its modifiers active immediately;
5. do not emit a transient `StatusApplied` event and do not execute its `Applied` hook
   during partial session construction.

When both sides finish deployment and every participant is placed, invoke each passive's
`Applied` hook exactly once in normal unit tie order, then passive authored order, inside
the deployment-completion batch after `DeploymentCompleted` and before the first scheduler
advance. Finalize pending defeats/outcome before entering a usable
`AwaitingActivation` state.

This delayed hook rule prevents an enemy passive from attacking a player that has not yet
been deployed while still giving `Applied` an exact meaning for passives. Modifier
presence itself is not delayed.

If prerequisite implementation already established an equivalent single battle-start
hook boundary, use it and document the adaptation. Do not run passive `Applied` once per
placement edit or emit it again on future activation.

---

# 5. Duration materialization

Convert immutable `DurationSpec` at application/placement time:

| Definition value | Runtime state |
|---|---|
| timeline duration `d` | `expiresAt = checked(context.elapsed + d)` |
| owner activations `n` | `remainingActivations = n` |
| infinite | infinite |

For statuses/shields, "owner" is the bearer/target. For a battle object, "owner" is its
creator. All authored finite values are positive.

Timeline expiration occurs at the exact deadline. A value created at time T with duration D
is active for boundaries strictly before T+D and is removed when the scheduler reaches
T+D, before choosing a ready unit at that same time.

No wall clock, frame delta, coroutine, OS timer, or float comparison participates.

---

# 6. Apply-status semantics

For `ApplyStatusEffectSpec`, require a current active target and resolve the immutable
definition. Let:

```text
requestedStacks = authored positive stacks
incomingStacks  = min(requestedStacks, definition.maxStacks)
incomingDuration = materialized authored duration
```

## 6.1 First transient instance

If no transient instance of this definition exists:

1. allocate a new status instance ID;
2. use `incomingStacks`;
3. store incoming duration and source origin;
4. reconcile effective stats/bounds;
5. append `StatusApplied` with previous 0, requested, signed applied delta, resulting
   stacks/duration, and copied tags;
6. invoke the instance's `Applied` hook through the guarded reaction service;
7. finalize pending defeats at the enclosing authored effect boundary.

## 6.2 Existing transient instance

Compute stacks exactly:

```text
AddStacks:
    resulting = min(maxStacks, previous + requestedStacks)

ReplaceStacks:
    resulting = incomingStacks

appliedStackDelta = signed(resulting) - signed(previous)
```

Use checked addition before the max clamp. A replace may produce a negative stack delta.
Events and later conditions must retain this signed actual delta rather than calling the
requested positive number "applied."

Refresh duration using section 7, update source origin to this latest successful
application, reconcile effective stats, and append `StatusApplied` with both old/new
state. Run `Applied` when stack state or duration actually changed. If neither changed,
emit at most one diagnostic no-change skip and do not invoke the hook.

Reapplication never allocates a new transient instance ID.

---

# 7. Duration-refresh policy

The registry's final graph validator must inspect every `applyStatus` reference. For a
status using `keepLonger` or `extend`, all finite transient applications of that status
must use the same duration kind (all timeline or all owner-activation). Infinite may coexist
as the absorbing longest case. Reject mixed finite kinds at startup with both effect paths;
timeline seconds and activation counts are not comparable.

Apply:

| Policy | Same finite kind | Infinite participation |
|---|---|---|
| `replace` | incoming replaces current exactly | incoming/current replacement is literal |
| `keepLonger` | timeline max deadline; owner max remaining count | infinite wins |
| `extend` | timeline current deadline + incoming duration; owner counts add | infinite wins/stays |

For `extend`, add the authored incoming duration to the current deadline, not to
`context.elapsed`. Use checked `BattleTime`/integer addition. If a current finite status
is already at its expiration boundary, the scheduler must have removed it before commands
can run, so it is a first application rather than extension.

`replace` intentionally permits a different duration kind because no comparison/addition
occurs. Stun references remain finite timeline only under step-02 validation.

---

# 8. Remove-status and cleanse

`RemoveStatusEffectSpec` looks up only the target's transient instance for the named
definition:

```text
removedStacks = min(requestedStacks, currentStacks)
remainingStacks = currentStacks - removedStacks
```

Append `StatusRemoved` with requested, actual removed, remaining, tags, and reason
`ExplicitEffect` whenever an instance was found. Reconcile effective stats immediately.
If stacks remain, retain the same duration/source/instance ID and do not invoke the
`Removed` hook.

If the instance reaches zero:

1. copy the definition, instance origin, and removed-hook effect list needed for resolution;
2. erase the instance and lookup entry;
3. reconcile stats;
4. append `StatusRemoved`;
5. invoke the copied `Removed` hook after the instance is absent;
6. never let that removed hook find/re-enter itself as an active instance.

The removal primitive receives two distinct origins:

* `immediateRemover` is the current effect frame's source for explicit remove/cleanse and
  becomes `StatusRemoved`'s event source;
* `originalAppliedBy` is copied from the instance into a separate payload field and remains
  available for status behavior/diagnostics.

Timeline expiry, owner-activation expiry, and unit cleanup have no immediate remover even
though `originalAppliedBy` may still be present. Never report the original applier as if it
performed a later cleanse. Raw session/battle teardown emits no gameplay `StatusRemoved`
sweep; authoritative removals happen before result freeze, while object destruction after
freeze is unlogged lifetime cleanup.

`CleanseEffectSpec`:

* matches a transient status if its definition contains at least one authored cleanse tag;
* never matches passives;
* snapshots matching instance IDs in ascending order;
* removes each full instance once with reason `Cleanse`;
* appends/invokes each removal completely before the next ID;
* does not double-remove a status matching multiple tags.

Every cleanse removal uses the cleanse effect's immediate source as remover; if that effect
has no source, the remover remains null. The original applier stays a separate field.

An absent named status or empty match set is a legitimate no applied change. No status ID
or hook is fabricated.

---

# 9. Status removal reasons and owner cleanup

Use a closed reason enum equivalent to:

```cpp
enum class StatusRemovalReason
{
    ExplicitEffect,
    Cleanse,
    TimelineExpired,
    OwnerActivationsExpired,
    UnitRemoved
};
```

Normal explicit/cleanse/duration removal runs the `Removed` hook under the reaction guard.
When a battle unit is defeated/impressed and leaves battle, after the reason-specific
`UnitDefeated`/`UnitImpressed` fact and before final `UnitRemoved`, remove all its transient
status instances and shields in ascending instance order, append their removal events with
reason `UnitRemoved`, but suppress `Removed` hooks. Then remove its owner-activation-duration
objects by object ID with reason `OwnerRemoved`; timeline/infinite objects persist using the
snapshotted creator side. Innate passive instances may remain in the
historical unit snapshot but are inactive because their owner is not an active combatant.

Suppressing hooks on owner removal prevents post-defeat "removed" explosions from acting
after outcome credit and makes unit-removal cleanup distinct from an authored cleanse.
Document this content rule.

Only explicit/cleanse reasons can carry an immediate remover. All expiration/cleanup
reasons carry null immediate remover regardless of the stored original applier.

---

# 10. Effective-stat evaluation

Add one pure `EffectiveStatsEvaluator` used everywhere; do not maintain independent
formula copies in UI, scheduler, resolver, and AI.

Start with the battle unit's derived baseline attributes. Build a deterministic ordered
modifier stream:

1. passive instances in derived passive order, then modifier definition order;
2. transient instances by `BattleStatusInstanceId`, then modifier definition order.

Evaluate in two passes:

1. apply/sum all `Add` contributions in the stream order with checked arithmetic;
2. apply all `MultiplyPermille` operations in the same stream order, truncating toward
   zero after every multiplication.

For an additive modifier with `perStack`, contribution is
`value * currentStacks`; otherwise it is value.

For a multiplier with `perStack == false`, apply its authored factor once:

```text
value = value * factorPermille / 1000
```

For `perStack == true`, apply that factor once per current stack, truncating after each
application. This is multiplicative stacking; do not multiply the full permille factor by
the stack count.

Finally clamp:

* MaxHealth, MaxActionPoints, and MaxMovementPoints to at least one;
* Stamina to `gameRules.battle.minimumStamina` (the exact positive `BattleTime` floor from
  steps 01/02/07), never merely one tick;
* Strength, MagicPower, Armor, Resistance, and Range to at least zero.

Use the established exact `BattleTime` unit for Stamina; additive authored values are
milliseconds converted exactly to ticks. Lower effective Stamina is faster.

No definition/container iteration outside the explicit stream may affect results.

---

# 11. Reconcile current state after modifier changes

Whenever a status stack/instance changes, recompute old and new effective stats and apply
these bound rules immediately:

* if effective MaxHealth decreases below current HP, clamp HP down to the new max;
* increasing MaxHealth does not heal;
* decreasing MaxActionPoints/MaxMovementPoints clamps current pools down;
* increasing a maximum does not refill the current pool;
* if effective Stamina decreases below current bar fill, clamp fill down;
* increasing Stamina leaves fill unchanged;
* Strength/MagicPower/Armor/Resistance/Range need no stored-pool reconciliation.

Append `EffectiveStatChanged` for each effective bound that changes. If AP or MP current
pool is reduced by the new maximum, append `ResourceChanged` with exact reason
`effectiveMaximumClamp`; this is resource `lost` only for a step-03 leaf that explicitly
allows that reason. Use the existing bar-adjustment payload for a stamina-fill clamp and a
distinct maximum-health adjustment payload/reason for an HP clamp, so neither is reported
as damage. Snapshots and the material digest reflect every adjustment. Never mislabel a
maximum clamp as voluntary resource spending or an effect-origin change.

Because effective maximum HP is at least one, lowering a max cannot by itself defeat a
living unit. A unit already pending at zero follows normal defeat finalization.

Invalidate/recompute scheduler readiness after Stamina/stun changes. Do not advance time
during a command.

---

# 12. Status and tag queries

Provide pure queries used by effects, AI, and scheduler:

```cpp
bool hasStatus(BattleUnitId, std::string_view p_statusId) const;
bool hasStatusTag(BattleUnitId, std::string_view p_tag) const;
bool isTurnBarPaused(BattleUnitId) const;
CreatureAttributes effectiveAttributes(BattleUnitId) const;
```

`hasStatus`/tag is true when at least one active passive or transient instance matches.
Removed units are never valid AI/turn candidates even if their historical passive list
remains.

`isTurnBarPaused` is true when an active instance definition has reserved tag `stun`.
Multiple stun statuses do not stack a separate pause count; the bar resumes only after the
last one is gone.

Extend `BattleUnitSnapshot` with complete copied `baselineAttributes` and
`effectiveAttributes` values in addition to current HP/AP/MP/bar pools and the detailed
status/shield snapshots below. Existing convenience maxima/range/stamina fields, if retained,
must equal the effective value. The snapshot/material/full digests encode every attribute in
the step-01 `CreatureStat` order, so a strength-only or armor-only modifier cannot collide
with a state that will resolve future damage differently.

Do not cache pointers into registry values in a way invalidated by registry replacement.
Registries are immutable for a running battle.

---

# 13. Hook service and final non-reentrancy rule

Implement an internal `StatusHookService` invoked explicitly by the scheduler/session/
resolver. It never subscribes to the public event provider.

Use a battle-resolution guard equivalent to:

```cpp
enum class ReactionKind { None, StatusHook, BattleObjectTrigger };

struct ReactionGuard
{
    ReactionKind active;
    std::size_t effectDepth = 0;
};
```

Final v1 rule:

* a top-level command/timeline event explicitly invokes its applicable status hooks and
  battle-object triggers;
* effects executed while a status hook or object trigger is active append all ordinary
  damage/heal/status/object/movement events;
* those reaction-produced effects do **not** schedule any further status hook or
  battle-object trigger;
* this includes movement produced by a reaction: occupancy changes and movement events
  occur, but leave/enter/end traps are suppressed for that nested movement;
* the outer captured hook/trigger iteration continues in stable order;
* `maxEffectChainDepth` is still checked on every nested resolver entry and a breach
  discards the complete uncommitted command/timeline transaction and every staged reaction,
  restores the last trustworthy committed state, then opens a fresh no-action
  `TechnicalAbort` batch with `BattleAbortReason::EffectChainDepthExceeded` and the
  canonical structural-close/`BattleAborted`/`BattleEnded{Aborted}` sequence. A submitter
  receives `AbortedCommand`; no partial gameplay batch or action ID is committed.

This deliberately shallow reaction model prevents poison-heal loops, thorn recursion, and
trap-push-trap cycles. It is the implementation of the decision that effects applied during
a hook do not trigger hooks. Do not rely on `ContractProvider`'s accidental reentrant
suppression as game logic.

---

# 14. Hook instance order and snapshot rule

For one hook dispatch on one owner, snapshot eligible instance IDs at dispatch start:

1. passives in derived passive order;
2. transient statuses by ascending instance ID.

For each snapshot entry, before execution:

* require that instance still exists and is active;
* resolve its definition;
* if it contains the requested hook, execute its authored hook effects in definition order
  through `EffectResolver`; v1 emits no separate hook-trigger diagnostic payload;
* finalize pending defeats after each authored effect application as usual.

A status applied during this dispatch waits until the next matching game event. A status
removed before its snapshot turn does not execute. Reapplying an existing status does not
move its order.

The hook definition contains at most one entry per hook literal by prior validation.

---

# 15. Hook effect frame

Hook effects need the same explicit scope vocabulary as abilities. Build a captured
`EffectExecutionFrame` for each hook:

* `sourceUnit` is the status instance's `appliedBy` when present, otherwise the hook
  owner (passives use the owner);
* `primaryUnit` is the hook owner for Applied, ActivationStart, ActivationEnd, and Removed;
* for after-dealt/done hooks, primary is the original damage/healing target;
* for after-taken/received hooks, primary is the original damage/healing source when it
  still has a battle-unit ID, otherwise the hook owner;
* for AfterAbilityCast/AfterVoluntaryMove, primary is the hook owner;
* `anchorCell` is the hook owner's current cell, or the owner's last occupied cell for a
  copied Removed hook when available;
* `affectedUnits` contains primary once when present;
* `affectedCells` contains the primary's current/last cell once when present, otherwise
  anchor once.

All lists are copied/canonical. `requiresLivingSource` refers to this frame's source
(therefore an applier must still be active when authored true). An enduring DOT whose
applier may leave must author `requiresLivingSource: false`.

Events retain source/status/hook origin IDs for attribution. Do not recapture an ability AoE
inside a status hook.

---

# 16. Exact hook insertion points

Implement all ten:

| Hook | Invocation point |
|---|---|
| `Applied` | after new/reapplied status state/event/reconciliation; passive once at deployment completion |
| `ActivationStart` | after refill+penalty and `ActivationStarted`, before object start-on-cell |
| `ActivationEnd` | after object end-on-cell, before owner-duration decrement and `ActivationEnded` |
| `AfterAbilityCast` | after all cast effects/defeats, before outcome/auto-end |
| `AfterDamageDealt` | after `Damage`, on current active source |
| `AfterDamageTaken` | after dealt hook, on current active target |
| `AfterHealingDone` | after `Healing`, on current active source |
| `AfterHealingReceived` | after done hook, on current active target |
| `AfterVoluntaryMove` | after aggregate `UnitMoved` and ended-move object trigger |
| `Removed` | after transient instance removal event/state, except unit cleanup |

Damage/healing hooks run even when computed/requested reached a valid target but applied
amount is zero; authored hook conditions are not a scripting feature, so content should use
effects appropriately. The reaction guard prevents nested repetitions.

If the event source/target is absent or removed before its hook turn, skip that owner's
dispatch. Finalize pending defeats after every hook effect boundary before continuing.

Activation start/end pipelines have an additional owner-removal precedence rule. After each
individual start hook, start-on-cell object trigger, end-on-cell object trigger, end hook, or
Removed hook caused by owner-duration expiry, finalize pending defeats immediately. If the
active owner was removed:

1. stop every later owner hook/trigger in that start/end pipeline;
2. discard the complete remaining owner-duration capture and perform no later decrements;
3. consume the pending structural close in the current batch as exactly one
   `ActivationEnded{ActiveUnitDefeated|ActiveUnitImpressed}` with the same TurnIndex;
4. reset the removed owner's bar, clear activation ownership, and evaluate outcome.

This close outranks `NoLegalCommands`, the submitted Explicit/AI end reason, and command-cap
reasons. Fatal ActivationStart resolution stays in the scheduler/timeline batch and has no
action ID. Fatal ActivationEnd or duration-expiry resolution stays in the already accepted
EndTurn action batch; never append the requested end reason and then a second close. This
special early stop applies to the activation-start/end owner pipeline, not to the separate
step-09 rule that an already captured ability continues permitted later effects after its
source is removed.

---

# 17. Timeline-boundary integration

Extend the scheduler's boundary source to return the minimum of:

* next active unit readiness;
* transient status timeline deadline;
* shield timeline deadline;
* battle-object timeline deadline.

When several deadlines/readiness share the same exact `BattleTime`:

1. advance once and append `BattleTimeAdvanced`;
2. expire timeline statuses by normal unit order then status instance ID;
3. expire shields by normal unit order then shield ID;
4. expire battle objects by object ID;
5. run allowed Removed hooks/defeat finalization after each status;
6. evaluate outcome;
7. only if non-terminal, select a ready unit using the step-07 tie order.

Each scheduler boundary is one complete no-action batch. Newly created already-due
deadlines are impossible because authored durations are positive. Removed/expired entries
must be ignored by stale deadline-heap records if a heap is used; never let heap insertion
order choose expiration order.

If expiration reactions create another exact boundary at current time, process it within
the same bounded resolution loop and apply the scheduler's no-progress guard. There must be
no infinite zero-time loop.

---

# 18. Owner-activation duration capture

At activation start, after the active unit is selected but before start hooks create new
instances, snapshot IDs of:

* that unit's transient owner-activation statuses;
* that unit's owner-activation shields;
* owner-activation battle objects whose creator is that unit.

Store this capture on the activation state. At an ordinary activation end while the actor
remains in battle:

1. run eligible end object/status hooks;
2. iterate each captured category in stable ID order;
3. if the captured instance still exists and still has owner-activation duration,
   decrement once;
4. when count reaches zero, remove/expire it with reason
   `OwnerActivationsExpired`;
5. run normal status Removed hooks under the guard;
6. after every decrement/removal/Removed-hook effect boundary, finalize pending defeats;
7. if the owner was removed, discard all remaining captured IDs and take the defeat/
   impression structural-close path above; otherwise continue in stable order;
8. append the ordinary requested `ActivationEnded` only for a surviving owner and finish the
   phase transition.

An instance created during that activation is not captured and does not immediately lose a
turn. Refreshing a captured instance during the activation does not remove it from the
capture; its resulting duration decrements once. Removing/recreating the same definition
allocates a new instance ID, so the new instance is not accidentally decremented.

If the active owner is defeated or impressed, do not run this decrement algorithm: the
unit-removal primitive first emits the reason-specific fact, performs hook-suppressed owner
cleanup, emits `UnitRemoved`, discards the captured IDs, and marks a pending structural
close. The enclosing cast or voluntary-move boundary appends the appropriate
`ActivationEnded` only after its remaining captured effects/after-cast or aggregate/
ended-move seams; a Taming/system removal with no continuing action may close immediately.
Step 07's `BattleTerminal` and `TechnicalAbort` structural closures also run neither hooks
nor captured-duration decrement; they exist only to close turn ownership after the fight is
already decided or the state is no longer safe to mutate.
Owned objects with remaining owner-activation duration whose creator has permanently left
are removed during unit cleanup with reason `OwnerRemoved`; they cannot wait forever for
an activation that can never occur.

---

# 19. Shield expiration

Use the `DurationState` implementation shared with statuses. Add a closed shield removal
reason:

```text
Depleted
TimelineExpired
OwnerActivationsExpired
UnitRemoved
```

Damage depletion emits `ShieldBroken` in step-09 order and no `ShieldRemoved`. Every other
removal emits exactly one `ShieldRemoved{shieldId, target, reason, remainingAmount}`:
timeline expiry, owner-activation expiry, and unit cleanup use the matching closed reason.

Expired shields do not absorb damage later at the same boundary. No hook is attached to a
shield in v1.

---

# 20. Battle-object runtime model

Use the strong `BattleObjectId` from step 01, allocated monotonically from one. Store:

```cpp
struct BattleObjectTriggerState
{
    std::string triggerId;
    std::uint32_t timesTriggered = 0;
};

struct BattleObjectInstance
{
    BattleObjectId id;
    std::string definitionId;
    std::optional<BattleUnitId> creator;
    BattleSide creatorSide;
    BoardCell cell;
    DurationState duration;
    std::vector<BattleObjectTriggerState> triggerStates;
    std::optional<std::string> sourceAbilityId;
    std::optional<std::string> sourceEffectId;
};
```

The context owns instances by ID. The board owns/queries stable per-cell object ID lists in
ascending ID and derives:

* movement blocked when any present definition has `blocksMovement`;
* LoS blocked when any present definition has `blocksLineOfSight`.

Creator side is snapshotted so relationship filters remain meaningful after creator
removal. Trigger-state vector preserves definition trigger order; lookup acceleration must
not reorder it.

Include instances, durations, cells, and trigger counts in snapshots/material digest.

---

# 21. Place-object semantics

`PlaceObjectEffectSpec` executes for each explicit cell scope entry:

1. require the cell exists/standable;
2. materialize the duration in scratch. An `ownerActivations` duration requires the copied
   creator ID to resolve to an active combatant before any object ID is allocated; otherwise
   append `EffectApplicationSkipped{SourceNotLiving}`. Timeline/infinite objects may be
   created after their creator leaves and retain the already snapshotted creator side;
3. reject a duplicate instance with the same definition ID, creator ID, and cell;
4. if the new definition blocks movement, require no unit and no existing
   movement-blocking object on the cell;
5. allocate one object ID;
6. initialize every trigger count to zero;
7. insert context and board-cell indices atomically;
8. append `BattleObjectPlaced`.

A nonblocking object may coexist with a unit and with other object definitions. An object
placed under a unit does not retroactively emit entered/left/ended movement. It can trigger
on the unit's later activation-start/end stay hooks.

Duplicate/blocked placement is a concrete effect no-change/skip inside an already accepted
cast; it does not roll back earlier effects or costs and does not consume an object ID.

---

# 22. Remove-object semantics

`RemoveObjectsEffectSpec` examines only its explicit captured cell entries in canonical
order. Snapshot matching object IDs on each cell in ascending order. Match when the
definition has at least one requested tag; multiple matching tags still remove once.

For each still-existing match:

1. remove context and board indices atomically;
2. append `BattleObjectRemoved` with reason `ExplicitEffect`, origin, definition,
   creator, cell, and final trigger counts;
3. do not execute any removal hook (none exists in v1).

An empty match is a valid no applied change. Removing an object never emits unit movement
triggers.

Other removal reasons are `TimelineExpired`, `OwnerActivationsExpired`,
`TriggerExhausted`, and `OwnerRemoved`.

---

# 23. Trigger relationship filters

Object trigger filters are evaluated against the current triggering unit relative to the
instance's snapshotted creator identity/side:

* `allowSelf`: triggering unit ID equals still-recorded creator ID;
* `allowAllies`: different active unit with creator side;
* `allowEnemies`: active unit with opposing side;
* `allowDefeated`: never qualifies for movement/activation triggers in v1;
* `allowEmptyCell`: never qualifies because all five runtime triggers carry a unit.

If creator ID is removed, self cannot qualify but ally/enemy still use snapshotted side.
The triggering unit must be active and placed at the trigger dispatch's expected state.

Do not reinterpret a leaving unit as empty. `unitLeftCell` observes the unit after its
occupancy transition committed while the object remains anchored to its own old cell.

---

# 24. Trigger execution frame and counts

For every qualifying object/trigger pair, build:

* source = creator ID when present;
* primary unit = triggering unit;
* anchor cell = object cell;
* affected units = triggering unit once;
* affected cells = object cell once;
* origin = object ID, definition ID, trigger ID, creator, source ability/effect.

Before effects:

1. check `maxTriggers == 0` or current count below max;
2. increment the matching trigger count with checked arithmetic;
3. append `BattleObjectTriggered` with new count/max;
4. enter `ReactionGuard::BattleObjectTrigger`;
5. execute authored effects in order through the shared resolver.

After effects, if the object still exists and this finite trigger reached max with
`removeWhenExhausted == true`, remove it with reason `TriggerExhausted`. Incrementing
before effects prevents the same trigger from exceeding its limit if future code adds
nested calls; the current reaction guard already suppresses them.

Objects on one cell execute by ascending object ID; matching trigger specs within one
object execute in definition order. Snapshot object IDs at dispatch start. A new object
placed by an earlier trigger does not join that same dispatch; an object removed before its
turn is skipped.

---

# 25. Five trigger insertion points

Implement exact semantics:

## 25.1 `unitLeftCell`

For every successfully departed cell during voluntary movement, displacement, teleport, or
swap:

* occupancy/unit position transition is already committed;
* dispatch objects anchored on the old cell;
* the object anchor remains the old cell;
* dispatch happens before any destination-enter dispatch;
* failed movement, deployment, and removal do not dispatch it.

## 25.2 `unitEnteredCell`

After leave dispatch, only if the moving unit remains active/placed at the expected
destination, dispatch objects anchored there. A reaction that removed or moved the unit
prevents the stale enter.

## 25.3 `unitEndedMoveOnCell`

Dispatch once at the actual final cell after any spatial operation that applied at least
one transition:

* one voluntary move command, after its aggregate `UnitMoved`;
* one displacement effect, after `UnitDisplaced`;
* one teleport effect, after `UnitTeleported`;
* each unit in a swap, after all swap leaves/enters.

No dispatch for a failed/same-cell operation. This is the movement "stay" trigger.

## 25.4 `unitActivationStartedOnCell`

After `ActivationStarted` and all eligible status/passive ActivationStart hooks, dispatch
objects on the active unit's current cell if it remains active/placed.

## 25.5 `unitActivationEndedOnCell`

Before status/passive ActivationEnd hooks and duration decrement, dispatch objects on the
active unit's current cell if it remains active/placed. An actor already removed does not
run ordinary end-on-cell/status hooks or duration decrement: removal cleanup has already
destroyed its transient owner-bound instances and discarded the captured duration IDs.

The two activation triggers are stay-like; they do not imply movement.

---

# 26. Multi-unit and multi-cell movement ordering

Voluntary/displacement/teleport transitions process per unit/per cell:

```text
commit transition
append UnitMovementStep
old cell objects: unitLeftCell
if still expected: new cell objects: unitEnteredCell
```

Then append the cause-specific aggregate and ended-move dispatch.

For atomic swap:

```text
commit both unit/cell swaps atomically
append UnitsSwapped
append both UnitMovementStep events by unit ID
all old-cell unitLeftCell dispatches by unit ID
all destination unitEnteredCell dispatches by unit ID
all unitEndedMoveOnCell dispatches by unit ID
```

This all-leaves-before-any-enters contract preserves the atomic swap. If a trigger reaction
relocates/removes one unit, verify expected position before its later dispatch.

For a multi-cell voluntary path, a trap interruption stops further planned steps. In
particular, step 08 rechecks MP before every next destination after enter triggers; a trap
that leaves less MP than the next cell cost ends the accepted move successfully at the
current cell. The one aggregate `UnitMoved` records actual original/last-entered/final
state, applied cells, and MP. A unit removed after successfully entering a cell retains that cell as
`lastOccupiedCell`.

---

# 27. Stun integration

A unit's bar-fill rate is paused whenever `hasStatusTag(unit, "stun")` is true:

* current fill is preserved exactly;
* scheduler time may advance for other units and timeline expirations;
* timeline stun expiration resumes fill from the preserved value;
* readiness selection excludes a still-stunned full-bar unit;
* applying stun to a unit whose activation already began does not cancel that activation;
* finite owner-activation and infinite stun content is invalid before runtime;
* removed/undeployed units never participate.

The scheduler's next-boundary calculation must consider the stun deadline so a battle with
only stunned survivors can resume. If no future timeline change can unpause/fill any unit,
use the existing stable no-future-progress Aborted policy.

---

# 28. Zero-cost and chain safety

After this step all valid abilities are resolver-supported. Keep:

* the per-activation total command cap from step 07;
* step 11's stricter AI command cap;
* the material-state no-progress concept that excludes event/action counters;
* `maxEffectChainDepth`;
* reaction suppression described above.

A zero-cost ability may be legal and useful, but an accepted command whose only results are
diagnostic/no-change must still count toward the activation command cap. The session may
automatically end when no legal non-end command remains; it must not repeatedly auto-cast.

---

# 29. Event and snapshot extensions

Complete/add typed payloads:

```text
StatusApplied
StatusRemoved
EffectiveStatChanged
ShieldRemoved
BattleObjectPlaced
BattleObjectTriggered
BattleObjectRemoved
```

`StatusApplied` carries previous/requested/signed applied/resulting stacks, old/new
duration, tags, instance ID, source, and origin. `StatusRemoved` carries requested/actual/
remaining stacks, reason, tags, instance ID, optional immediate remover as its event source,
and optional original applier as a separate field. Object events carry instance/definition/
creator/cell/trigger count and stable reason.

Add the exact value projection used by presentation/replay and include it as
`std::vector<BattleObjectSnapshot> objects` in `BattleSnapshot`, ordered by ID:

```cpp
struct BattleObjectSnapshot
{
    BattleObjectId id;
    std::string definitionId;
    std::optional<BattleUnitId> creator;
    BattleSide creatorSide;
    BoardCell cell;
    DurationSnapshot duration;
    std::vector<BattleObjectTriggerState> triggerStates; // definition order
    bool blocksMovement = false;
    bool blocksLineOfSight = false;
};
```

The two block flags are copied from the resolved immutable definition so consumers never
retain a definition pointer. Also extend snapshots with:

* effective stats;
* passive instances in derived order;
* transient status instances in ID order;
* shield instances/durations;
* stun/paused state where useful;
* activation duration-capture IDs if the snapshot is used for exact debug comparison.

Material digest includes gameplay-relevant instance state and excludes log/event/counter
noise as established. Deep atomicity state includes next status/object/shield IDs.

---

# 30. Registry graph validation additions

Before publishing registries:

* enforce finite-kind consistency for keepLonger/extend status references;
* reassert stun is never a passive and all references are finite timeline;
* validate hook effect payload/scope references against the now-complete catalogs;
* validate object trigger effect references and five trigger literals;
* ensure no status/object hook ID/effect ID ambiguity in diagnostics;
* prove every checked-in ability has full runtime support.

Keep two-phase local-then-commit loading. An invalid cross-reference must not partially
replace registries.

---

# 31. Required status stacking tests

Test:

* first instance stable ID/source/duration;
* AddStacks below/exact/over cap;
* ReplaceStacks increasing, equal, and decreasing signed delta;
* reapplication retains instance ID and updates source;
* replace/keepLonger/extend for timeline and owner counts;
* infinite combinations;
* mixed finite kind rejection for keep/extend and allowed literal replacement;
* no-change reapplication avoids Applied hook;
* transient status same definition as passive remains separate;
* absent remove, partial remove, full remove, and Removed hook timing;
* cleanse any-tag matching, no double match, stable ID order, and passive immunity;
* explicit/cleanse immediate remover credit versus separately retained original applier;
* timeline/owner/unit cleanup removal with null immediate remover;
* owner-removal cleanup reason with hook suppression.

Golden-test status events using actual signed stack delta and resulting duration.

---

# 32. Required modifier tests

Table-test:

* passive-before-transient order;
* all additives before multipliers;
* positive/negative add;
* additive per-stack;
* multiplier truncation after every operation;
* multiplier applied once per stack;
* min-one/max-zero clamps for every stat;
* Stamina millisecond conversion and lower-is-faster;
* checked overflow/underflow;
* max-health/resource/bar downward clamp;
* no heal/refill/fill on maximum increases;
* Range changes target maximum only through the step-08 query;
* same effect reads new runtime stat after an earlier status application.
* a strength-only and an armor-only modifier change effective snapshot/material/full digests
  even when no current pool changes; removing each restores the exact prior value/digest.

Prove all subsystems call the same evaluator.

---

# 33. Required hook tests

Create definitions for every hook literal and assert exact insertion/event order. Test:

* passive Applied once at deployment completion;
* transient Applied after state/event;
* activation start after refill and before object start;
* object end before activation-end status and duration decrement;
* after-cast after authored effects;
* dealt before taken; healing-done before healing-received;
* voluntary move after aggregate/ended object;
* Removed after instance absence;
* passive order then transient ID order;
* snapshot rule: newly applied waits, removed-before-turn skips;
* hook execution frame source/primary/anchor/cells;
* original applier removed with requiresLivingSource true versus false;
* reaction damage/heal/status/movement appends events but triggers no nested hook/object;
* effect depth boundary and exact `EffectChainDepthExceeded` technical abort.
* fatal ActivationStart status hook and start-on-cell object trigger: later start dispatches
  stop, the no-action batch keeps one TurnIndex and closes exactly once before outcome;
* fatal ActivationEnd hook and fatal Removed hook while an owner-duration status expires:
  remaining hooks/captures do not run, the accepted EndTurn batch uses
  `ActiveUnitDefeated` rather than its requested reason, and closes exactly once.

Include a deliberately cyclic pair of statuses and prove it terminates deterministically
without depending on public-provider reentrancy behavior.

---

# 34. Required duration/stun tests

Test:

* exact timeline deadline and expiration before same-time readiness;
* status then shield then object category order and stable IDs within category;
* owner-activation capture before start hooks;
* existing/refreshed/newly-created/removal-recreated instance decrement behavior;
* early defeat/impression destroys owner-bound instances, discards captured IDs, and does
  not decrement duration; fatal enter-trigger and source-dies-mid-cast traces close the
  activation exactly once only after all permitted same-action events;
* creator-removal object cleanup;
* shield depletion versus time/owner expiration events;
* one stun, overlapping stuns, preserved full/partial fill, exact resume;
* active-unit stun does not revoke activation;
* all-stunned with a future deadline resumes;
* all-stunned without any future progress aborts stably;
* no zero-time expiration loop.

---

# 35. Required battle-object tests

Test:

* stable ID, duration, creator/side snapshot, and trigger-count initialization;
* nonblocking object under unit; blocking object rejects unit/other blocking object;
* same definition+creator+cell duplicate rejection without ID consumption;
* other nonblocking definitions coexist;
* movement and LoS blocking query integration;
* removeObjects any-tag, canonical cells/object IDs, no duplicate removal;
* self/ally/enemy relationship filters before/after creator removal;
* finite/unlimited count and remove-on-exhaustion;
* increment/event before effects, removal after effects;
* new object does not join current trigger snapshot; removed-before-turn skips;
* all five trigger kinds;
* exact leave-after-commit and leave-before-enter semantics;
* failed move/deployment/removal fire neither;
* multi-cell per-cell order and actual aggregate after interruption;
* an enter-trigger MP drain makes the next unaffordable positive-cost cell stop without
  underflow/free traversal and preserves the applied movement prefix;
* displacement/teleport/swap/end-move semantics;
* all swap leaves before all enters;
* object-triggered movement does not recursively trigger another object;
* activation start/end status/object ordering.

Use golden event sequences, not only final state.

---

# 36. Full headless battle checkpoint

Do not expand the production slice early merely to make this checkpoint reachable. Check in
a strict test-only JSON tree and load it through the same real parsers, registries,
cross-reference validator, and transactional publish path as production data:

```text
Playground/tests/resources/battle_runtime_pack/
├── abilities/runtime-strike.json
├── abilities/runtime-guard.json
├── abilities/runtime-poison.json
├── abilities/runtime-regeneration.json
├── abilities/runtime-stun.json
├── abilities/runtime-cleanse.json
├── abilities/runtime-place-snare.json
├── statuses/runtime-passive.json
├── statuses/runtime-guarded.json
├── statuses/runtime-poison.json
├── statuses/runtime-regeneration.json
├── statuses/runtime-stun.json
├── statuses/runtime-removal-hook.json
├── battle-objects/runtime-snare.json
├── featboards/runtime-board.json
├── creatures/runtime-actor.json
└── creatures/runtime-target.json
```

The definitions collectively cover a passive, finite shield, DOT, HOT, stun, cleanse plus
Removed hook, and finite trap. They use schema version 1 and ordinary cross-references; tests
must not instantiate `AbilityDefinition`, `StatusDefinition`, or `BattleObjectDefinition`
directly in C++. A missing/malformed member makes the complete test-pack registry load fail
without publishing partial state.

Build a deterministic fixture using that loaded pack:

1. project passive-bearing units;
2. deploy and show passive Applied ordering;
3. run scheduler to a player activation;
4. cast guard/poison or equivalent status effects;
5. move through/place a training snare and show leave/enter/end trigger counts;
6. show a DOT/HOT activation hook and integer applied values;
7. show stun pausing/resuming another bar at an exact timeline deadline;
8. show cleanse/removal hook and shield/object expiration;
9. finish via ordinary damage and verify BattleEnded;
10. print/compare exact final event trace and material digest across two runs.

Use no AI or presentation. Keep normal exploration boot unchanged.

---

# 37. Expected file changes

Adapt to repository conventions, with files equivalent to:

```text
Playground/srcs/battle/status/battle_status.hpp
Playground/srcs/battle/status/battle_status.cpp
Playground/srcs/battle/status/duration_state.hpp
Playground/srcs/battle/status/effective_stats.hpp
Playground/srcs/battle/status/effective_stats.cpp
Playground/srcs/battle/status/status_hook_service.hpp
Playground/srcs/battle/status/status_hook_service.cpp
Playground/srcs/battle/objects/battle_object.hpp
Playground/srcs/battle/objects/battle_object.cpp
Playground/srcs/battle/effects/effect_resolver.cpp
Playground/srcs/battle/scheduler/stamina_scheduler.cpp
Playground/srcs/battle/battle_session.cpp
Playground/srcs/battle/battle_context.cpp
Playground/srcs/battle/battle_event.hpp
Playground/srcs/battle/battle_snapshot.hpp
Playground/tests/battle/status_runtime_tests.cpp
Playground/tests/battle/effective_stats_tests.cpp
Playground/tests/battle/status_hook_tests.cpp
Playground/tests/battle/duration_stun_tests.cpp
Playground/tests/battle/battle_object_tests.cpp
Playground/tests/battle/full_headless_battle_tests.cpp
Playground/tests/resources/battle_runtime_pack/...
Playground/docs/battle.md
```

Update CMake lists. Modify step-02 registry validation only for the explicitly required
cross-reference checks; do not create schema version 2. Do not add authoring tooling.

---

# 38. Documentation requirements

Document:

* passive versus transient instance separation;
* all stacking and duration-refresh formulas;
* status removal/cleanse/passive immunity and cleanup reasons;
* effective-stat order, per-stack multiplier meaning, clamps, and current-state reconciliation;
* all hook frames/insertion points/order;
* explicit reaction non-reentrancy and depth cap;
* exact timeline/owner duration boundaries;
* stun pause/resume;
* object instance/placement/removal/relationship/count rules;
* five trigger semantics, including committed-state leave and all swap leaves before enters;
* full event/snapshot ordering and append-before-publish;
* DOT/HOT as data-authored hooks rather than special tick classes.

Remove stale docs describing frame timers, separate passive subclasses, recursive event
subscriptions, or physics-trigger trap authority.

---

# 39. Non-goals

Do not implement AI, battle/player UI, scene object visuals, VFX/audio, taming/progression
evaluation, save/resume, authoring tools, hot reload, general scripts/expressions,
pre-damage interception hooks, immunities not represented by current modifiers/effects,
revive, aura range scans, equipment/items, or random status chance.

Do not make a public callback provider the rules engine. Do not create one class per status
or trap. Do not run duration logic on frame time.

---

# 40. Acceptance criteria

This step is complete only when:

* [ ] passives and transients use one definition catalog with distinct runtime-origin rules;
* [ ] stable instance IDs/order and one transient per definition are enforced;
* [ ] stack/reapply/duration-refresh/removal/cleanse behavior is exact and evented;
* [ ] one effective-stat evaluator owns ordering, per-stack semantics, clamps, and reconciliation;
* [ ] all ten hooks run at final insertion points through the shared resolver;
* [ ] reaction-produced effects cannot recursively schedule hooks/triggers;
* [ ] timeline deadlines and owner-activation captures integrate with the scheduler exactly;
* [ ] stun preserves fill and cannot deadlock a resumable scheduler;
* [ ] shields expire deterministically;
* [ ] objects have stable state, blocking, placement/removal, filters, counts, and durations;
* [ ] all five triggers match committed leave/enter/stay semantics;
* [ ] voluntary/displacement/teleport/swap ordering passes golden tests;
* [ ] all closed effect payloads are now executable; no unsupported skip remains;
* [ ] complete batches append before public publication;
* [ ] full headless battle trace is deterministic across repeated runs;
* [ ] all focused/full Playground tests and exploration smoke run stay green.

---

# 41. Required handoff report

Report:

1. exact status/passive/instance/duration/object types and owners;
2. exact stacking and duration-refresh algorithms;
3. exact remove/cleanse/owner-cleanup behavior;
4. exact effective-stat modifier order/per-stack/clamp/reconciliation behavior;
5. hook frames, insertion points, ordering, snapshot, and non-reentrancy rules;
6. timeline/owner duration scheduler integration and stun behavior;
7. object placement/removal/filter/count/runtime order;
8. exact five movement/activation trigger semantics;
9. event/snapshot/digest additions;
10. registry graph checks added;
11. files created/materially changed/deliberately untouched;
12. repository drift/adaptations;
13. commands actually run and results;
14. deterministic full headless trace actually observed;
15. work explicitly deferred to step 11 and later.
