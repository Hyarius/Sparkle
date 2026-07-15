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

## Spatial operations and reserved seams

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

The eight reaction points (source `afterDamageDealt`, target `afterDamageTaken`, source
`afterHealingDone`, target `afterHealingReceived`, `afterAbilityCast`, leave, enter, and ended-move)
are explicit resolver no-ops in this step. Source hooks precede target hooks even for self effects.
They stage no events and cannot call the public command gateway. Step 10 supplies status/object
hook bodies at these exact insertion points, preserving non-reentrant batch resolution.

## Cast end and scope of this step

After a non-terminal cast, the session automatically ends the current activation with
`NoLegalCommands` when no supported cast or move remains, or `CommandCap` when the session limit
is reached. A source defeated by its own cast ends with `ActiveUnitDefeated`; simultaneous defeat
is evaluated as a draw after the complete cast boundary.

Step 09 supports damage, healing, shields, current-resource changes, next-activation penalties,
turn-bar adjustment, displacement, swap, and source teleport. Status and battle-object payloads
remain atomically unavailable until Step 10; that step also implements duration expiration and
the reaction bodies above.
