# Battle effect resolution

Step 09 makes a legal, resolver-supported ability cast one atomic command. `BattleSession::submit`
replans the cast, spends non-zero AP/MP costs, records `AbilityCast`, resolves authored effects in
definition order, runs the explicit after-cast seam, then evaluates outcome and automatic activation
end. A batch is appended to the authoritative log before it becomes available to publication.

## Scope and ordering

`CastPlan` is immutable. `sourceUnit`, `primaryUnit`, `affectedUnits`, `anchorCell`, and
`affectedCells` are captured once in canonical order; effects never recapture area membership after
a move, removal, or teleport. Unit payloads can only use unit scopes, while cell payloads can only
use the captured anchor/area cells. `swapWithSource` consumes the captured primary exactly once and
`teleportSource` consumes the captured anchor exactly once. Missing primary, absent/dead targets,
blocked or invalid destinations, a missing axis, and no state change produce
`EffectApplicationSkipped` with its authored effect ID and copied origin.

`requiresLivingSource` is evaluated once per authored application. A false result skips that whole
application, but a previously captured cast continues. Unit-target payloads require a currently
active combatant. Damage that changes positive HP to zero is pending until all entries of that
application finish, then defeat/removal is emitted in canonical session-unit order. An effect with
`requiresLivingSource: false` can still read the removed source's stored effective stats.

## Numeric rules and transaction safety

Damage and healing use one checked integer formula:

```
offense = (base * 1000 + strength * strengthRatioPermille
           + magicPower * magicPowerRatioPermille) / 1000
damage  = offense * mitigationScale / (defense + mitigationScale)
```

Every multiply, addition, denominator, accumulated shield value, AP/MP delta, next-activation
penalty, event narrowing, turn-bar operation, and spatial event distance is checked. Damage is
then resolved against matching shields in ascending `BattleShieldId` order before HP. Healing
clamps to the current effective maximum and does not revive. Resource changes clamp current pools;
next-activation penalties accumulate only when their checked `int` total fits.

Before a cast mutates state, the session captures a private rollback state containing board
occupancy, units/shields, RNG, phase/turn, shield/object allocators, and counters. The exact final
staged-event count is capacity-checked immediately before commit; it is never guessed from a
per-effect estimate. A resolver or capacity `std::overflow_error` restores that state, discards all
staged gameplay events, and commits one no-action `TechnicalAbort` batch. If an activation was live, it first emits
`ActivationEnded{TechnicalAbort}`, followed by `BattleAborted{NumericInvariant}` and
`BattleEnded{Aborted}`. No partial cast, action ID, or shield allocation reaches the log.

## Statuses, hooks, objects, and durations

`applyStatus`, `removeStatus`, `cleanse`, `placeObject`, and `removeObjects` are resolver
payloads, not special commands. A `BattleUnit` owns authored-order passive projections and
ascending-ID transient instances. Reapplying a transient keeps its instance ID, applies the
definition's add/replace stack policy, and applies replace/keep-longer/extend duration policy.
Passives are never a target for remove or cleanse.

`EffectiveStatsEvaluator` is the single modifier calculation: passive modifiers then transient
modifiers, all additions first, then multipliers (once per stack when authored), followed by the
game-rule floors. Reconciliation immediately clamps HP/AP/MP/bar fill only when a new maximum is
lower. Snapshots and the material digest include baseline/effective attributes, status instances,
shield duration, and placed-object state.

`StatusHookService` is explicitly invoked at application/removal, activation start/end, damage,
healing, cast, and voluntary-move seams. It never subscribes to the public event provider. Its
reaction guard means an effect produced by a hook or object trigger records its normal facts but
cannot dispatch another hook or trigger. Passive `Applied` hooks run once after deployment is
complete; transient `Applied` runs after the new state and event. Removed hooks run after the
transient is absent, except for unit-cleanup removals, which suppress hooks.

Placed objects have stable IDs, copied creator side, duration, trigger counts, and a board-cell
index. They can block movement and line of sight. The five trigger points are committed leave,
enter, end-of-move, activation-start-on-cell, and activation-end-on-cell. Trigger counts increment
and emit before authored effects; an exhausted configured trigger removes its object afterwards.

Timeline durations join scheduler boundaries. At an exact boundary statuses, shields, and objects
expire before ready selection; owner-activation durations are captured at activation start and
decrement only from that capture at activation end. A stun-tagged active status pauses bar fill
without clearing the accumulated fill; its timeline expiry allows the scheduler to resume.

## Spatial operations and reaction seams

Voluntary movement, displacement, and teleport use the same committed single-unit transition:
occupancy/unit placement updates, then `UnitMovementStep`, then leave and enter seams. Displacement
follows the exact traversal-graph cardinal neighbor selected from the source/target axis (X wins
ties); it never scans a column for a convenient replacement. `UnitDisplaced` preserves original
and final cells, the locked cardinal vector, requested/applied distance, and records `(0, 0)` plus
`NoDirectionalAxis` when source and target share x/z. Ended-move runs once only after a non-zero
completed spatial action.

Swap commits both occupancy transitions atomically, appends `UnitsSwapped`, then appends movement
steps, all leave seams, all enter seams, and all ended-move seams in ascending unit ID order.
Teleport uses only the captured anchor, performs one committed step, then leave, enter,
`UnitTeleported`, and ended-move. Failed or same-cell operations dispatch no spatial seam.

The eight resolver reaction points (source `afterDamageDealt`, target `afterDamageTaken`, source
`afterHealingDone`, target `afterHealingReceived`, `afterAbilityCast`, leave, enter, and ended-move)
dispatch the status/object service at those exact insertion points. Source hooks precede target
hooks even for self effects; dispatch stays inside the original staged batch and cannot call the
public command gateway.

## Cast end and scope of this step

After a non-terminal cast, the session automatically ends the current activation with
`NoLegalCommands` when no supported cast or move remains, or `CommandCap` when the session limit
is reached. A source defeated by its own cast ends with `ActiveUnitDefeated`; simultaneous defeat
is evaluated as a draw after the complete cast boundary.

The resolver supports the whole closed payload catalog: damage, healing, shields, statuses,
cleanse/removal, resources/penalties, turn-bar adjustment, displacement/swap/teleport, and placed
objects. All resulting facts remain in the enclosing atomic batch.
