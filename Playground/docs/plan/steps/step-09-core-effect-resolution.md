# Implement core effect resolution

Make legal ability casts executable through the existing battle command transaction. Add
one deterministic `EffectResolver` for damage, healing, shields, direct resource changes,
next-activation penalties, turn-bar adjustment, displacement, swapping, and source
teleportation. Resolve explicit effect scopes against the cast plan captured in step 08,
record applied rather than merely requested values, and finalize pending defeats at exact
effect boundaries.

This step does not implement statuses, passives, cleanse, or battle objects. An ability that
contains any still-unsupported payload remains atomically unavailable until step 10; never
skip an unsupported payload and call the cast successful.

---

# 1. Repository baseline and prerequisites

Implement after steps 01-08 and read their handoff reports. Reinspect the repository and
adapt names without creating parallel types. The required inherited contracts are:

* checked integer/fixed-point battle primitives, stable runtime/content IDs, and a positive
  integer mitigation scale;
* strict immutable effect definitions with authored order, unique effect IDs,
  `requiresLivingSource`, and explicit `applyTo`;
* `BattleSession::submit` as the only public command mutation path;
* an immutable `CastPlan` containing source, exactly one anchor, optional primary unit,
  canonical captured area/affected cells, and canonical captured affected units;
* AP/MP costs validated before an action and current pools clamped to effective maxima;
* board occupancy/path/LoS values and ordered movement-trigger seams;
* action batches appended to the event log before external publication;
* pending battle outcomes checked through the phase machine, not by an effect callback;
* no revive effect and removed units no longer occupy a board cell.

Inspect the Unity prototype only for player-facing intent. Do not copy floating-point
damage, mutable ScriptableObject state, recursive Unity event dispatch, target recapture,
or separate effect implementations per ability.

Expected baseline limitation: step 08 can plan a cast but rejects it with
`EffectRuntimeUnavailable`. Replace that blanket guard with a complete-payload capability
check backed by the resolver in this step.

---

# 2. Required final behavior

At the end of this step:

1. a cast is planned again at submission, spends its real AP/MP costs, records
   `AbilityCast`, and executes authored effects in order;
2. every effect uses its explicit execution scope; there is no implicit "all targets" loop;
3. source/anchor/target membership is captured once, while current target state and current
   effective stats are read at each effect;
4. damage uses checked integer offense ratios and kind-specific mitigation;
5. matching shields absorb in stable instance order before HP;
6. healing clamps to current effective maximum health and never revives;
7. resource, penalty, and turn-bar effects record requested and applied values;
8. displacement, swap, and teleport update occupancy through the same spatial mutation
   seam as movement and invoke reserved leave/enter/end triggers in final order;
9. HP-zero units become pending defeats and are removed at the end of the current authored
   `EffectApplication`;
10. the captured cast continues after a defeat; only an upcoming effect marked
    `requiresLivingSource` is skipped when the source is no longer living;
11. outcome is evaluated after the complete cast/reaction boundary, including simultaneous
    defeat/draw;
12. unsupported status/object payloads make the whole command unavailable before cost/IDs;
13. scripted headless combat tests can finish a battle without AI or presentation.

All formulas and ordering are deterministic and draw no RNG.

---

# 3. Resolver ownership and mutation authority

Add an internal service equivalent to:

```cpp
class EffectResolver
{
public:
    [[nodiscard]] bool supports(const EffectPayload &p_payload) const noexcept;
    [[nodiscard]] bool supportsAll(const AbilityDefinition &p_ability) const noexcept;

private:
    friend class BattleSession;

    EffectResolutionSummary resolveAbility(
        BattleContext &p_context,
        BattleEventWriter &p_events,
        const CastPlan &p_plan,
        const AbilityDefinition &p_ability,
        const BattleEventOrigin &p_origin);
};
```

Exact signatures may use a private mutation facade instead of raw context, which is
preferred if already established. The non-negotiable boundary is:

* only `BattleSession` begins/commits/publishes a command transaction;
* only the resolver and narrow context primitives may modify combat state inside it;
* definitions, plans, and snapshots remain immutable values;
* no public `dealDamage`, `healUnit`, `applyShield`, `teleport`, or mutable-unit
  method lets UI/AI/tests bypass command/effect ordering;
* status hooks and object triggers in step 10 call this same resolver with an explicit
  non-ability origin; they do not reimplement payloads.

Do not use the external `ContractProvider` during resolution. Stage/append all events,
complete the batch, then let the outer pump publish it.

---

# 4. Runtime effect identity and origin

Every concrete resolution retains:

* battle/action/batch IDs from the active transaction;
* optional source unit;
* ability ID for a cast;
* authored effect ID;
* optional status ID or battle-object instance ID for future reactions;
* one target unit or target cell when a concrete payload is applied.

Extend `BattleEventOrigin` rather than packing debug strings into payloads. Origin IDs are
copied values and remain valid after a unit/status/object is removed.

Add a diagnostic payload equivalent to:

```cpp
enum class EffectSkipReason
{
    MissingScopeValue,
    SourceNotLiving,
    TargetNotLiving,
    TargetNoLongerPlaced,
    DestinationInvalid,
    DestinationBlocked,
    NoDirectionalAxis,
    NoAppliedChange
};

struct EffectApplicationSkipped
{
    std::string effectId;
    std::optional<BattleUnitId> targetUnit;
    std::optional<BoardCell> targetCell;
    EffectSkipReason reason;
};
```

This is diagnostic and must not itself count as damage/healing/movement/status gameplay for
conditions. Emit it only where it makes an authored application observably unable to run;
do not flood the log with an entry for every harmless zero clamp unless a golden contract
below explicitly requires it.

An unknown payload alternative is impossible after strict parsing. It is a programming
invariant failure, not a skip.

---

# 5. Cast transaction and ordering

For `submit(CastAbilityCommand)`, preserve the step-08 validation order, then require
`EffectResolver::supportsAll(ability)`. If any payload is unsupported, reject the entire
command before action/batch ID allocation, costs, events, or RNG.

For a supported cast, resolve exactly:

1. create the before snapshot;
2. allocate one action ID and batch ID;
3. deduct non-zero AP cost and append `ResourceSpent(ActionPoints, AbilityCost)`;
4. deduct non-zero cast MP cost and append
   `ResourceSpent(MovementPoints, AbilityCost)`;
5. append one `AbilityCast` containing the captured source cell, anchor, x/z Manhattan
   anchor distance, captured affected cells, and captured affected unit IDs;
6. for each authored `EffectApplication` in definition order:
   1. test `requiresLivingSource`;
   2. expand only its explicit captured scope;
   3. apply its payload to scope entries in canonical order;
   4. run the direct damage/healing reaction seam for each applied unit payload;
   5. after the full authored application, finalize all pending defeats in canonical unit
      order;
7. run the source's `afterAbilityCast` seam once;
8. finalize any pending defeat produced by that seam;
9. evaluate battle outcome;
10. if non-terminal, evaluate command cap and no-legal-command automatic ending;
11. capture after state, append the immutable batch/log descriptor, then publish outside
    resolution.

Costs are already affordable, so their applied value equals the authored positive cost.
Do not emit a `ResourceSpent` event for a zero cost.

The cast remains accepted when a later effect legitimately has no applied change because a
previous authored effect removed/moved/filled the captured target. The cast has already
spent costs and emitted `AbilityCast`; record a stable skip where useful and continue.

---

# 6. Explicit effect execution scopes

Expand `EffectApplication::applyTo` exactly:

| Scope | Concrete entries |
|---|---|
| `sourceUnit` | captured source ID exactly once |
| `primaryUnit` | captured primary ID once, or no entry plus missing-scope skip |
| `affectedUnits` | captured affected unit IDs in their captured canonical order |
| `anchorCell` | captured anchor cell exactly once |
| `affectedCells` | captured affected cells in canonical board-cell order |

Do not:

* apply a unit payload to every affected unit when its scope says primary/source;
* apply an anchor-cell payload once per affected cell;
* convert an empty primary unit into the source;
* recalculate AoE membership after a push/teleport/defeat;
* stop the entire captured vector because one entry is no longer eligible;
* reorder entries by current position.

Step-02 cross-validation already restricts payload/scope compatibility. Keep a defensive
visitor that treats an impossible pairing as a technical invariant failure.

`requiresLivingSource` is checked once immediately before each authored application.
Living means `BattleUnit::isActiveCombatant()`: HP positive, placed, and no removal
reason. When false, skip the whole application and emit one diagnostic, then proceed to the
next authored application. It is not a command error and does not roll costs back.

Payload-specific unit eligibility is checked at concrete execution time. Damage/heal/
shield/resource/bar/displacement require a current active target unless explicitly stated
otherwise. There is no defeated-unit effect and no revive in v1.

---

# 7. Checked offense formula

Use the same checked integer helper for damage and healing. At the moment a concrete payload
is applied, read the source's current effective Strength and MagicPower. If the origin has
no source unit, both contributions are zero. If a source ID still exists but was removed
and `requiresLivingSource == false`, its current effective stats remain readable from the
stored battle unit.

```text
scaled =
    base * 1000
    + effectiveStrength * strengthRatioPermille
    + effectiveMagicPower * magicPowerRatioPermille

offenseAmount = scaled / 1000
```

All validated inputs are non-negative. Division truncates toward zero. Calculate each
multiply/add with checked wide integer operations. Do not use float, double, rounding to
nearest, hidden level scaling, random variance, critical hits, accuracy, same-type bonus,
or an un-authored minimum of 1.

Runtime effective stats, not species base stats and not values captured at cast selection,
are authoritative. Step 10's status modifiers therefore affect each later effect at its
actual point in the authored sequence.

If an impossible overflow is detected after resolution has begun, discard the complete
uncommitted command transaction—including all staged gameplay events, mutations, tentative
action ID, and scratch IDs—and restore the last trustworthy committed state. Open a fresh
no-action `BattleBatchKind::TechnicalAbort` batch, perform the structural active-turn close
when required, then append the canonical `BattleAborted(NumericInvariant)` and
`BattleEnded{Aborted}` events. Return `AbortedCommand`, not `AcceptedCommand` or
`RejectedCommand`; do not wrap, saturate silently, throw past the mode, or publish any part
of the failed command. Registry/stat bounds should make this unreachable and tests must
exercise both the boundary helper and transactional rollback.

---

# 8. Damage and mitigation

For a `DamageEffectSpec`:

1. compute `offenseAmount` above;
2. choose the target's current effective Armor for physical or Resistance for magical;
3. read positive `gameRules.battle.mitigationScale`;
4. compute:

```text
computedDamage =
    offenseAmount * mitigationScale
    / (effectiveDefense + mitigationScale)
```

Use checked arithmetic and truncation toward zero. Effective defense is non-negative. There
is no defense penetration and no forced minimum damage. `computedDamage` is the
post-mitigation amount presented to shields/HP and stored in `Damage::computedDamage`.

Capture `hpBefore` before shield resolution. Resolve matching shields, then apply the
remaining amount to HP:

```text
appliedToHealth = min(remainingDamage, currentHealth)
currentHealth -= appliedToHealth
```

HP never goes below zero. Before absorption, capture two independent facts:

* `targetHadAnyShield`: at least one positive shield of any kind existed;
* `targetHadMatchingShield`: at least one positive shield matching this damage kind existed.

The first fact is consumed by the authored condition named `targetHadShield`; the second is
useful for combat diagnostics. Preserve both even when computed damage is zero or the hit
breaks the last shield. Nonmatching shields are untouched.

Append one `Damage` for every concrete damage payload that reached an active target,
including computed zero, with:

* source/target/ability/effect origin;
* damage kind;
* computed post-mitigation damage;
* total applied to matching shields;
* applied to health;
* pre-hit any-shield and matching-shield booleans;
* HP before and after.

Conditions later consume applied shield/health values. A zero-applied damage event is useful
diagnostic evidence but is not a qualifying positive-damage occurrence.

If HP changes from positive to zero, add one pending-defeat record carrying the first origin
that caused that transition. Do not immediately erase occupancy while other entries of this
authored `EffectApplication` are still resolving.

---

# 9. Shield runtime and absorption order

Introduce a strong, battle-local `BattleShieldId` (zero invalid), allocated monotonically
from one and never reused. This step also owns the shared duration runtime/snapshot values
used by shields now and statuses/objects in step 10:

```cpp
struct TimelineDurationState { BattleTime expiresAt; };
struct OwnerActivationDurationState { std::uint32_t remainingActivations = 0; };
struct InfiniteDurationState {};

using DurationState = std::variant<
    TimelineDurationState,
    OwnerActivationDurationState,
    InfiniteDurationState>;

struct TimelineDurationSnapshot { BattleTime expiresAt; };
struct OwnerActivationDurationSnapshot { std::uint32_t remainingActivations = 0; };
struct InfiniteDurationSnapshot {};

using DurationSnapshot = std::variant<
    TimelineDurationSnapshot,
    OwnerActivationDurationSnapshot,
    InfiniteDurationSnapshot>;

struct BattleShield
{
    BattleShieldId id;
    DamageKind kind;
    std::int64_t remainingAmount = 0;
    DurationState duration;
    std::optional<BattleUnitId> source;
    std::optional<std::string> sourceAbilityId;
    std::optional<std::string> sourceEffectId;
};
```

Snapshots copy the absolute timeline deadline, not wall-clock time; remaining presentation
time is derived as `max(0, expiresAt - BattleSnapshot.elapsed)`. The duration alternatives
retain the same semantic order/codes across runtime and snapshot serialization.

is owned by the target `BattleUnit` in ascending instance-ID order. Definitions are not
copied wholesale.

`ApplyShieldEffectSpec` against a living target:

1. allocate the next shield ID;
2. create exact positive `remainingAmount == authored amount`;
3. materialize duration relative to current battle time/target owner;
4. append to ordered storage;
5. append `ShieldApplied` with requested/applied amount equal and copied duration value.

There is no aggregate shield cap in v1. Checked totals used for snapshots/diagnostics must
not overflow.

For damage, iterate only matching shields by ascending `BattleShieldId`. For each:

1. `requested = remaining incoming damage`;
2. `applied = min(requested, shield.remainingAmount)`;
3. subtract from shield and incoming damage;
4. append `ShieldAbsorbed` when `applied > 0`;
5. when remaining shield becomes zero, erase it only after copying origin/duration data and
   append `ShieldBroken` immediately after its absorption event;
6. stop when incoming damage is zero.

Do not merge equal shields, use LIFO, choose largest first, or let physical absorb magical.
Timeline/owner-activation expiration is implemented in step 10, but duration state and
stable ordering are final here.

---

# 10. Healing

For `HealEffectSpec`, compute `requestedHealing = offenseAmount` using current effective
source stats. A target must currently be an active combatant. Healing never selects,
re-places, or revives a removed/defeated unit.

```text
missing = effectiveMaxHealth - currentHealth
appliedHealing = min(requestedHealing, max(0, missing))
currentHealth += appliedHealing
```

Append one `Healing` even when the target is already full, with requested/applied and
HP before/after. Current effective max health is read at application time. No overheal
shield, percent heal, healing variance, or hidden bonus exists unless later represented by
an explicit status hook/effect.

The status reaction seam runs after the event. Conditions later use `appliedHealing`, not
the request.

---

# 11. Direct AP/MP changes

`ChangeResourceEffectSpec` changes the current target pool only. It never alters a maximum
or a future refill.

For the selected AP/MP resource:

```text
after = clamp(before + authoredDelta, 0, currentEffectiveMaximum)
appliedDelta = after - before
```

Use checked addition. Append `ResourceChanged` with requested delta, applied delta, before,
after, and exact reason `effect`. A negative requested delta is resource lost, positive
is gained. Conditions distinguish signs and consume effective applied magnitude.

This may drain the currently acting unit during its activation; it does not retroactively
cancel the cast already being resolved. It may make later commands unaffordable. A positive
change cannot exceed the effective maximum.

Do not route this payload through `ResourceSpent(AbilityCost|MovementCost)`; those events
are reserved for voluntary command costs.

---

# 12. Next-activation penalties

`ApplyNextActivationPenaltyEffectSpec` accumulates on the target's matching pending
AP/MP-penalty field:

```text
accumulatedAfter = checked(accumulatedBefore + authoredPositiveAmount)
```

Append `NextActivationPenaltyApplied` with requested amount, applied accumulation amount,
and before/after. It does not change the current pool. Step 07 already consumes the whole
field exactly once after the next activation refill and clamps the refilled pool at zero.

Penalty accumulation has a validated/checked representable upper bound. Overflow is a
technical numeric invariant, never wraparound. Do not reinterpret a penalty as a status,
maximum modifier, positive bonus, or immediate drain.

---

# 13. Turn-bar adjustment

`AdjustTurnBarEffectSpec::delta` is signed exact `BattleTime`:

```text
after = clamp(before + delta, 0, target.effectiveStamina())
appliedDelta = after - before
```

Positive adds fill (readiness closer); negative removes fill (readiness delayed). Append a
typed `TurnBarAdjusted` payload containing requested/applied delta and before/after. Add it
to the common event variant rather than overloading `ResourceChanged`.

The payload does not modify authored/effective stamina. If the target is the currently
active unit, its activation remains active regardless of bar change; normal activation end
will reset its bar according to step 07. A stunned target retains its adjusted fill while
paused.

Use exact `BattleTime` checked addition. No float seconds and no scheduler advancement
occur during the cast.

---

# 14. Shared spatial-effect primitives

Position effects mutate through one internal spatial primitive shared with movement, using
the closed `SpatialMoveCause` declared in step 06.

It must atomically maintain:

* unit cell/placed state;
* occupancy forward/reverse maps;
* object-blocking rules;
* `UnitMovementStep` detail;
* step-10 `unitLeftCell` then `unitEnteredCell` trigger seams;
* final ended-move trigger seam for a completed spatial action.

Position effects spend no MP unless an ability separately has an authored cast MP cost.
They do not append the voluntary aggregate `UnitMoved` and do not invoke
`afterVoluntaryMove`.

For each successfully departed/entered step, occupancy is committed before the leaving
hook; the old object stays anchored to its old cell; all leave processing precedes the
corresponding enter. A failed spatial attempt invokes neither. Deployment/removal remain
non-movement.

Step 10 may let a trigger defeat/remove/relocate a moving unit. Therefore accumulate actual
steps and stop when the unit is no longer active/placed at the expected current cell.

---

# 15. Displacement

For `DisplaceEffectSpec`, both source and target must currently be placed active units.
At the start of this concrete application:

```text
dx = target.x - source.x
dz = target.z - source.z
```

Choose x when `abs(dx) >= abs(dz)`, otherwise z. For `awayFromSource`, use the sign of
the chosen target-minus-source component; for `towardSource`, reverse it. Lock that
cardinal direction for the entire displacement.

If source and target have the same x/z, there is no direction: apply zero cells and emit
`UnitDisplaced` with requested distance and applied distance zero plus the diagnostic
reason. Do not choose arbitrary positive x.

Otherwise, for up to authored distance:

1. find graph neighbors whose x/z changes by exactly the chosen cardinal direction;
2. if more than one y-layer candidate exists, choose canonical board-cell order;
3. require standable, not object-blocked, and unoccupied;
4. perform one spatial transition with cause Displacement;
5. stop at the first missing/invalid/occupied cell or when a reaction interrupts the unit.

Displacement ignores terrain MP cost and cannot push through a unit or push a chain. Append
exactly one `UnitDisplaced` after applied steps, with source, target, toward/away, locked
direction, requested distance, applied distance, original cell, and actual final cell.
Invoke `unitEndedMoveOnCell` once at the actual final cell when at least one cell applied.

---

# 16. Swap with source

`SwapWithSourceEffectSpec` is schema-valid only for `primaryUnit`. At execution:

* source and captured primary both exist, are distinct, placed, and active;
* each still occupies its recorded current cell;
* their two current cells remain standable;
* no third unit occupies either cell.

Blocking objects do not prevent exchanging two already legally occupied cells; their entry
triggers may react afterward. Atomically exchange both occupancy entries and unit cells
before any event/trigger.

Then:

1. append one `UnitsSwapped` with both IDs and before/after cells;
2. append one `UnitMovementStep` for each unit ordered by `BattleUnitId`, cause Swap;
3. invoke both old-cell `unitLeftCell` triggers in unit-ID order;
4. invoke destination `unitEnteredCell` triggers in unit-ID order for units still at the
   expected destinations;
5. invoke `unitEndedMoveOnCell` once per surviving correctly placed unit in unit-ID order.

All leaves occur before any enters for the atomic two-unit transition. No MP is spent and
no voluntary `UnitMoved` is emitted. If preconditions fail, append one stable skip and
change neither unit.

---

# 17. Teleport source to anchor

`TeleportSourceEffectSpec` is schema-valid only for `anchorCell`. At execution, use the
captured source and captured anchor but current occupancy:

* source must be placed and active;
* anchor must still exist and be standable;
* anchor must not be blocked by a blocking object;
* anchor must be unoccupied.

If the anchor equals the source's current cell, treat it as no applied change and do not
fire movement triggers. Otherwise perform one atomic spatial transition, append
`UnitMovementStep{cause=Teleport}` first, invoke old-cell leave then the new-cell enter when
the source remains at the expected destination, append one `UnitTeleported` with x/z
Manhattan distance, then invoke one ended-move trigger if the source still remains there.
This is the shared step-detail-before-seams-before-aggregate order from sections 14 and 25;
teleport does not define a parallel order.

Teleport ignores intermediate cells, terrain cost, LoS at effect time, and MP. Those were
cast-anchor concerns. It does not invoke enter/leave for any cell along a ray.

---

# 18. Direct reaction seams

Install explicit no-op hook calls that step 10 fills:

```text
after damage event:
    source afterDamageDealt
    target afterDamageTaken

after healing event:
    source afterHealingDone
    target afterHealingReceived

after all authored cast effects:
    source afterAbilityCast
```

The order is source hook first, then target hook. If source equals target, the distinct hook
kinds still run in that order. Hooks run only when their owner remains present under the
step-10 hook eligibility contract. In this step they are no-ops.

Do not invoke hooks by triggering the same external provider from inside the resolver.
Step 10 executes their effect lists directly through this resolver under a reaction guard.

---

# 19. Pending defeat and removal

When an effect or future reaction first changes a unit's HP from positive to zero, record:

```cpp
struct PendingDefeat
{
    BattleUnitId unit;
    BattleEventOrigin creditOrigin;
    std::optional<BoardCell> previousCell;
};
```

Do not add it twice and do not overwrite first credit. `isActiveCombatant()` becomes false
at HP zero even before occupancy removal, so later concrete payloads cannot heal or act on
it.

After all canonical scope entries for the current authored `EffectApplication` and its
direct reaction seams finish:

1. sort pending defeats by normal externally visible unit order (Player side first, roster
   order, unit ID);
2. for each still HP-zero/not-yet-removed unit, copy previous cell and remove occupancy;
3. append `UnitDefeated` with credit origin;
4. append `UnitRemoved(RemovalReason::Defeated)`;
5. if this was the active owner, mark its pending structural close; do not append
   `ActivationEnded` or clear ownership until the complete captured-cast boundary;
6. clear the pending list for the next authored application.

Reconcile step 06's removal primitive to this final event order. `UnitDefeated` precedes
`UnitRemoved` for defeat. Later `Impressed` removal emits `UnitImpressed` then
`UnitRemoved` and never `UnitDefeated`.

Already captured remaining targets/effects are not recalculated. If the source was removed,
the next application runs only when its own `requiresLivingSource` is false. No result is
declared in the middle of an ability: after all effects and after-cast reactions, evaluate
active combatants once. Both sides reaching zero in the same cast produces Draw.

If the active unit is defeated, continue all captured effects permitted by their
`requiresLivingSource` flags and run every eligible after-cast seam. Then consume the pending
marker exactly once through the common activation-close seam: append
`ActivationEnded{ActiveUnitDefeated}`, reset only its turn-bar fill, and clear ownership
before outcome/no-legal-command evaluation. Do not refill/reset its AP/MP or grant another
command. Every remaining same-action event keeps that activation's TurnIndex, so a
turn-window condition closes only after observing the complete cast.

---

# 20. Capability boundary for step 10

`supportsAll` returns true in this step only when every payload is one of:

```text
damage
heal
applyShield
changeResource
applyNextActivationPenalty
adjustTurnBar
displace
swapWithSource
teleportSource
```

It returns false if any payload is:

```text
applyStatus
removeStatus
cleanse
placeObject
removeObjects
```

Do not partially execute an ability such as "damage then poison." Step 10 implements the
remaining visitors and then makes the complete ability executable without changing the
command, plan, transaction, or core payload semantics.

---

# 21. Event and snapshot extensions

Add the following to the common event variant if not already declared:

```text
TurnBarAdjusted
EffectApplicationSkipped (Diagnostic)
```

Complete existing `Damage`, `Healing`, `ShieldApplied`,
`ShieldAbsorbed`, `ShieldBroken`, `ResourceChanged`,
`NextActivationPenaltyApplied`, `UnitDisplaced`, `UnitTeleported`, and
`UnitsSwapped` payloads with the exact applied fields described here.

Extend snapshots with shields in stable unit then shield-ID order:

```cpp
struct BattleShieldSnapshot
{
    BattleShieldId id;
    DamageKind kind;
    std::int64_t remainingAmount;
    DurationSnapshot duration;
    std::optional<BattleUnitId> source;
    std::optional<std::string> abilityId;
    std::optional<std::string> effectId;
};
```

Include shields and pending penalties in the material state digest. Do not include next ID
counters in the material progress digest used by AI; continue including counters in
deep-atomicity comparisons.

---

# 22. Required formula tests

Use table-driven tests with literal expected integers.

## 22.1 Offense

Test:

* base-only;
* Strength-only at 1000 and fractional permille;
* MagicPower-only;
* hybrid ratios;
* truncation instead of rounding;
* zero contribution from an absent source;
* current effective stats read after an earlier modifier test seam;
* checked maximum valid value and one synthetic overflow.

## 22.2 Mitigation

For physical/Armor and magical/Resistance, test defense 0, scale boundary, defense equal to
scale, very high defense yielding zero, and exact truncation. Prove the wrong defense stat
does not participate and there is no minimum-one rule.

## 22.3 Healing

Test partial missing HP, exact full, overheal clamp, already-full applied zero, effective max
changed before application, and defeated/removed target skip. Assert no revive/re-placement.

---

# 23. Required shield tests

Test:

* physical and magical isolation;
* oldest matching ID absorbs first;
* one shield partial absorption;
* exact break event immediately after absorption;
* damage spilling through multiple shields to HP;
* insufficient damage leaving later shields untouched;
* `targetHadAnyShield` and `targetHadMatchingShield` for matching-only, nonmatching-only,
  mixed, and no-shield cases;
* computed zero with shield present;
* stable ID allocation and no reuse;
* duration state materialization;
* snapshot/digest order.

Assert exact golden sequence: shield absorb/break details precede the aggregate
`Damage`, then reaction seam, then defeat finalization at application boundary.

---

# 24. Required resource/time tests

Test direct AP/MP positive/negative clamp and requested/applied values, distinction from
cost `ResourceSpent`, accumulated next-activation penalties without current-pool change,
one-shot consumption through the existing activation-start logic, and checked overflow.

Test positive/negative turn-bar delta, both clamps, stunned retention, current-active-unit
behavior, and exact `BattleTime` values. Prove the battle clock does not advance.

---

# 25. Required position tests

## 25.1 Displacement

Test all cardinal directions, toward/away, x winning equal absolute ties, zero source-target
direction, blocked first/later cell, occupied cell, stacked-y canonical neighbor, board
edge, requested versus applied distance, no MP loss, and one aggregate displacement event.

## 25.2 Swap

Test legal atomic swap, same unit/missing/removed/third-occupant failures, exact event order,
all leaves before all enters, no transient occupancy corruption observable to hooks, no MP,
and canonical unit-ID trigger order.

## 25.3 Teleport

Test legal non-adjacent teleport, occupied/object-blocked/missing anchor, same-cell no-op,
height ignored only for distance display (cell remains exact), leave-before-enter, no
intermediate triggers, and no MP.

For every failed spatial application, state remains unchanged except the already accepted
cast's costs/events; a skip never corrupts occupancy.

---

# 26. Required cast/effect-order tests

Create small in-memory definitions and assert:

* every `applyTo` scope executes exactly its intended count;
* empty primary does not fall back to source or affected units;
* cell scopes are never multiplied by unit counts;
* canonical captured order survives earlier movement/removal;
* runtime effective stats/state are read for each later authored effect;
* an AoE effect marks several defeats, finishes its captured scope, then finalizes them in
  canonical unit order;
* later effects continue after captured-target defeat;
* source defeat causes only later `requiresLivingSource == true` applications to skip;
* false allows a base-only effect with the removed source origin;
* simultaneous final defeat yields Draw after the full cast;
* active-source defeat ends activation at the cast boundary;
* a source-dies-mid-cast golden continues allowed later effects and after-cast processing,
  then emits exactly one `ActivationEnded{ActiveUnitDefeated}`; all same-action events share
  its TurnIndex and the condition engine includes them before closing the turn window;
* a real zero-cost cast submitted through the public gateway counts exactly once toward the
  step-07 generic command cap even when its applied payload clamps to no material change;
  the cast that reaches the cap includes one `ActivationEnded{CommandCap}` in the same batch
  and no second action ID;
* no outcome/event batch is externally published until all nested events are appended.

Golden-test AP event, MP event, AbilityCast, authored effects, defeat events, automatic
activation/outcome events, batch commit, then external publication.

---

# 27. Unsupported-effect atomicity tests

Use abilities containing each unsupported payload and mixed lists such as:

```text
damage -> applyStatus
applyShield -> placeObject
heal -> cleanse
```

Every submission must return `EffectRuntimeUnavailable` before:

* AP/MP change;
* action/batch/event/instance ID allocation;
* RNG draw;
* state digest change;
* log append;
* subscriber callback.

The supported prefix must not execute. Legal-anchor preview remains available.

---

# 28. Headless scripted combat checkpoint

Extend the debug fixture with core-only abilities:

1. player activation moves/casts a physical strike through `BattleSession`;
2. enemy receives a physical shield;
3. a second strike demonstrates shield absorption/break and HP spill;
4. a magical effect demonstrates Resistance rather than Armor;
5. resource/bar effects show exact clamps;
6. displacement and teleport show final cells/occupancy;
7. a lethal AoE produces canonical defeat events and a terminal `BattleEnded`.

Print or assert fixed event sequences and final digest. Do not enter this battle
automatically in normal exploration and do not add UI.

---

# 29. Expected file changes

Adapt to repository conventions, with changes equivalent to:

```text
Playground/srcs/battle/effects/effect_resolver.hpp
Playground/srcs/battle/effects/effect_resolver.cpp
Playground/srcs/battle/effects/battle_damage.cpp
Playground/srcs/battle/effects/battle_spatial_effects.cpp
Playground/srcs/battle/battle_shield.hpp
Playground/srcs/battle/battle_runtime_ids.hpp
Playground/srcs/battle/battle_session.cpp
Playground/srcs/battle/battle_context.cpp
Playground/srcs/battle/battle_event.hpp
Playground/srcs/battle/battle_snapshot.hpp
Playground/tests/battle/effect_formula_tests.cpp
Playground/tests/battle/shield_tests.cpp
Playground/tests/battle/resource_effect_tests.cpp
Playground/tests/battle/spatial_effect_tests.cpp
Playground/tests/battle/cast_resolution_tests.cpp
Playground/docs/battle.md
```

Update CMake lists. Do not modify JSON schema except to correct a demonstrated prerequisite
bug and reconcile all downstream prompts. Do not put runtime instances in registries.

---

# 30. Documentation requirements

Document:

* the cast transaction and exact event order;
* explicit scope expansion and captured-membership/current-state distinction;
* offense and mitigation formulas with integer examples;
* shield instance/absorption order and duration ownership;
* healing/no-revive behavior;
* direct resource versus next-refill penalty;
* signed turn-bar adjustment;
* displacement dominant-axis/tie/zero rules;
* atomic swap and teleport trigger order;
* pending defeat boundary, credit, and outcome timing;
* supported versus deferred payload catalog;
* append-complete-batch-then-publish rule.

Remove stale docs/formulas that use floats, level scaling, random variance, target recapture,
or immediate nested callbacks.

---

# 31. Non-goals

Do not implement status instances/modifiers/hooks, DOT/HOT, stun, cleanse, battle-object
placement/triggers, AI, player input, presentation, progression/taming evaluation, revive,
lifesteal as a special formula, immunity/penetration, elemental types, critical hits,
accuracy/evasion, random variance, push chains, collision damage, or authored scripts.

Do not make unsupported effects successful. Do not publish intermediate events. Do not add
public mutation methods for tests.

---

# 32. Acceptance criteria

This step is complete only when:

* [ ] one internal resolver executes all and only the core payload catalog;
* [ ] the common cast command/planner/session path is unchanged and authoritative;
* [ ] complete-payload preflight prevents partial unsupported casts;
* [ ] AP/MP costs and AbilityCast precede authored effects in exact order;
* [ ] explicit scopes execute once per captured canonical entry without recapture;
* [ ] formulas use checked integers, runtime effective stats, exact mitigation, and truncation;
* [ ] shields absorb matching damage by ascending stable ID before HP;
* [ ] healing clamps and never revives;
* [ ] current resources, next penalties, and turn bar remain distinct;
* [ ] displacement/swap/teleport preserve occupancy and final trigger order without MP;
* [ ] pending defeats finalize after each full authored EffectApplication;
* [ ] captured casts continue and living-source guards work per later application;
* [ ] outcome evaluates after the full cast/reaction boundary;
* [ ] all values/events use applied amounts needed by condition consumers;
* [ ] action batches append fully before external publish;
* [ ] formula/effect/spatial/atomicity/integration tests pass;
* [ ] full Playground build/tests and exploration smoke run stay green.

---

# 33. Required handoff report

Report:

1. exact resolver/runtime-shield/ID types and ownership;
2. supported/deferred payload visitor list;
3. cast transaction and event order;
4. explicit-scope expansion and target capture semantics;
5. exact formula/mitigation/rounding/overflow behavior;
6. shield allocation/absorption/expiration seam;
7. resource/penalty/bar semantics;
8. displacement/swap/teleport algorithms and trigger order;
9. pending-defeat/removal/outcome timing and credit;
10. event/snapshot/digest additions;
11. files created/materially changed/deliberately untouched;
12. repository drift/adaptations;
13. commands actually run and results;
14. deterministic scripted trace actually observed;
15. work explicitly deferred to step 10 and later.
