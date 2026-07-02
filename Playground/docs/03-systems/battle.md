# System — Battle

The tactical battle backend: 7-phase FSM, stamina turn bar, actions, targeting, effects,
statuses, events, outcome, and rule-list AI. **Entirely headless** — operates on
`BoardData` + `BattleContext`; presentation subscribes to observables/events
([ui-hud.md](ui-hud.md), [rendering-cameras.md](rendering-cameras.md) for the overlay).

The design below is the authority; the Unity battle (`Assets/Scripts/Battle/**`,
`proposition.md`) is a worked example to consult when a *behavior* is ambiguous (D28).
Turn semantics locked in [D23](../decision-log.md).

Diagrams: [06-class-battle-phases.puml](../diagrams/06-class-battle-phases.puml),
[07-class-battle-domain.puml](../diagrams/07-class-battle-domain.puml),
[23-seq-battle-action-resolution.puml](../diagrams/23-seq-battle-action-resolution.puml).
Plan: steps [10](../plan/steps/step-10-battle-backend.md) (core),
[15](../plan/steps/step-15-abilities-effects.md), [16](../plan/steps/step-16-statuses.md),
[23](../plan/steps/step-23-ai.md).

---

## 1. Domain types (`Playground/srcs/battle/`)

```
pg::BattleSide   enum { Neutral, Player, Enemy }
pg::BattleObject { BattleSide side; std::vector<std::string> tags; }   // base of units + objects
pg::BattleUnit : BattleObject
    CreatureUnit* sourceUnit;            // the persistent creature (never mutated mid-battle
                                         //  except via feat service post-battle)
    BattleAttributes attributes;         // observable battle stats (below)
    BattleStatuses statuses;             // stacks + durations + source passives
    optional<Vector3Int> boardPosition;  // board-local
    bool hasLeftBattle;
    isDefeated() = hp<=0; isActiveInBattle() = !defeated && !left
    isTurnReady() = turnBar.current >= turnBar.max
pg::WildBattleUnit : BattleUnit { TamingProgress tamingProgress; }     // see encounters-taming.md
pg::BattleAttributes                     // built from CreatureUnit.attributes at battle start
    ObservableResource hp, ap, mp;
    ObservableFloatResource turnBar;     // max = stamina (seconds), current = fill
    ObservableValue<float> staminaRate;
    ObservableValue<int> attack, armor, armorPen, magic, resistance, resistPen, bonusRange;
    ObservableValue<float> lifeSteal, omnivampirism;
    ObservableList<BattleShield> shields;
    setup(attrs); addShield(); absorbDamage(kind, amount) -> AbsorptionResult;
    advanceShieldDurations(); clearShields();
pg::BattleShield { ShieldKind kind; int amount; int remainingTurns; /* -1 = infinite */ }
pg::BattleContext
    std::vector<BattleUnit*> playerUnits, enemyUnits;   // owned (unique_ptr storage)
    BoardData board;  PlacementStyle placementStyle;
    optional<Vector3Int> returnWorldCell;
    TurnContext currentTurn;             // active unit + per-turn counters (moved, cast…)
    bool allowsTaming;                   // wild encounters only
    getUnits(side); getOpponents(unit); hasLivingUnits(side);
    tryPlaceUnit/tryMoveUnit/trySwapUnits/removeUnit/defeatUnit  (position + registry sync,
        defeatUnit marks wild units untamable and fires unitDefeated)
pg::BattleAction (abstract) { BattleUnit& source; apCost(); mpCost(); }
    ├─ MoveAction    { Vector3Int destination; int distance; mpCost = distance }
    ├─ AbilityAction { const Ability& ability; std::vector<Vector3Int> targetCells;
    │                  apCost = ability.cost.ap; mpCost = ability.cost.mp }
    └─ EndTurnAction { }
pg::BattleEvent (+ ~20 typed subclasses)   // the combat log vocabulary:
    Damage, Heal, AbilityCast{targetDistance}, ShieldApplied, DamageAbsorbed, ShieldBroken,
    StatusApplied, StatusRemoved, UnitDefeated, BattleWon{unitSurvived}, ResourceConsumed,
    ResourceChanged, ResourceStolen, DistanceTravelled, TurnStarted/TurnEnded{closestAlly/
    EnemyDistance}, HitSurvived, Teleported{distance}, Displacement{distance,orientation},
    SwapPosition, TurnBarTimeAdjusted, TurnBarDurationAdjusted, Revive
    common: { int turnIndex; const Ability* sourceAbility; BattleUnit* caster; BattleUnit* target; }
```

`pg::BattleLog` collects every emitted event for the battle (feats + taming + result screen
consume it). Emission goes through `battle::reportEvent(event)` →
`EventCenter.battleEventOccurred` + append to the log.

## 2. Phase FSM (`battle/phases/`, port of `proposition.md`)

```
Setup → Placement → Idle → PlayerTurn | EnemyTurn → Resolution → Idle … → End
```

- **`BattleOrchestrator`** — a dumb state machine: holds current `IBattlePhase`, drives
  `enter/tick/exit`. Never decides what comes next.
- **`BattleCoordinator`** — owns the rules of succession: creates phases up front with their
  dependencies, subscribes to phase-completion signals, reads `BattleContext` to pick the
  next phase (victory check → End; next ready unit's side → Player/EnemyTurn; action chosen
  → Resolution with the pending action injected).
- **Phases** are self-contained states; each exposes `ContractProvider` signals
  (`onPlacementFinished`, `onActionChosen(BattleAction)`, `onResolved`, …). Input is bound
  per-phase: `PlayerTurnPhase` activates the battle input handler in `enter`, deactivates in
  `exit` — the orchestrator never touches input.

| Phase | Does | Completes when |
|---|---|---|
| Setup | build BattleContext runtime state, reset units, compute deployment zones | immediately |
| Placement | M1: auto-place both sides (`BattlePlacementRules`: player strip order, enemy policy random/fixed/byLine); later: manual placement UI drives it | all units placed |
| Idle | `advanceTurnBarsToNextReady` (simulation jump, D23); select ready unit (tie: player side, then list order); `beginTurn` | a unit is ready |
| PlayerTurn | wait for a legal `BattleAction` from input | action chosen |
| EnemyTurn | evaluate AI rules → legal action (fallback EndTurn) | action chosen |
| Resolution | validate + resolve the pending action; then if action ≠ EndTurn and turn can continue (`canContinueTurn`), back to the same side's turn phase; else `endTurn` → Idle | resolved |
| End | winner known: emit `BattleWon` per surviving player unit into the log; fire `EventCenter.battleResolved(context, winner)` | terminal |

Victory (`BattleOutcomeRules`): a side loses when it has no `isActiveInBattle()` units.
Wild units that left via taming count as neither alive nor defeated (they simply left).

## 3. Turn rules (`battle/rules/battle_turn_rules`, D23)

- `beginTurn`: set turnBar to max; emit `TurnStarted` (with closest ally/enemy distances);
  apply `turnStart` status hooks; resolve pending defeats.
- `endTurn`: `turnEnd` hooks; advance status turn-durations; advance shield durations;
  advance board-object durations; **refill AP/MP to max**; turnBar := 0; emit `TurnEnded` +
  `EventCenter.battleTurnEnded`.
- `advanceTurnBars(Δt)`: for every living, non-stunned unit:
  `turnBar += Δt · staminaRate`. **Stun check:** unit has a status tagged `stun` ⇒ skip
  (fill pauses, resumes at current value — GDD).
- `timeUntilNextReady()` = min over units of `(max−current)/staminaRate` (stunned units
  excluded); Idle advances all bars by exactly that (no real-time drift while UI waits).
- `canContinueTurn`: unit alive ∧ (reachable cells with current MP ≠ ∅ ∨ any ability is
  castable with a valid target).

## 4. Validation & resolution

**`BattleActionValidator`** (all pure queries — also power the UI previews):
- `canAfford(unit, action)`; `getReachableCells(ctx, turn)` (MP flood, board.md);
- `canUseAbility(ctx, turn, ability)` (AP/MP affordable);
- `getValidTargets/TargetCells(ctx, turn, ability)`:
  range shape from caster cell (min..max+bonusRange) ∩ LoS (if required) ∩ targetProfile
  (ally/enemy/empty/everything, checked against occupancy).

**Range shapes** (x,z plane, caster at origin): `circle` = Manhattan ring min≤d≤max;
`line` = 4 straight rays; `diagonal` = 4 diagonal rays; `self` = caster cell only.
**AoE shapes** around the anchor cell: `square` (Chebyshev ≤ v), `cross` (Manhattan ≤ v on
axes), `circle` (Manhattan ≤ v), `line` (v cells along anchor−caster direction). Elevation
ignored; LoS from caster to anchor only.

**`BattleActionResolver`**:
- Move: validate path (revalidate at resolution), `tryMoveUnit` per cell… v1 resolves the
  whole move atomically then emits `DistanceTravelled` + `ResourceConsumed(mp)`; the view
  layer tweens through the path.
- Ability: spend costs (`ResourceConsumed` events); emit `AbilityCast{targetDistance}`; for
  each cell of the AoE (anchor-ordered), build a `BattleAbilityExecutionContext{ctx, ability,
  sourceObject, targetObject(unit at cell), anchorCell, affectedCell}` and apply each effect
  in authored order. Effects and their exact semantics: [02-data-model.md §5.1](../02-data-model.md).
- EndTurn: nothing (the coordinator calls `endTurn`).
- After any resolution: `BattleUnitRules::resolvePendingDefeats` — units at hp≤0 are defeated
  (`UnitDefeated` event, removed from board; wild ⇒ untamable). Revive-able corpses: v1
  keeps defeated units off-board (revive targets a defeated *unit*, not a cell — as Unity).

**`BattleResourceRules`** centralizes hp/ap/mp changes: clamps, records
`BattleResourceChangeResult{gain/loss}`, fires the `OnHPLoss/OnHPGain/…` status hooks, feeds
loss amounts back to effects (steal amounts are *actual* removed amounts).

## 5. Statuses (`battle/rules/battle_status_rules`, step 16)

- A unit's `BattleStatuses` holds `BattleStatus{const Status& def; int stacks; Duration
  remaining; bool isSourcePassive}` — passives (from the creature) are infinite-duration
  entries added at battle start.
- `applyHook(hookContext)`: for each status on the hook's owner whose `def.hookPoint`
  matches, execute its effects with source/target = hook context. **Non-reentrant** (a
  global in-progress flag): effects applied during a hook never trigger further hooks; hooks
  fire in status-list order (D23).
- Turn durations decrement at `endTurn`; expired stacks removed (StatusRemoved events).
- `stun` is only special via its tag (turn-bar pause). No other hardcoded statuses.

## 6. AI (`battle/ai/`, step 23)

Port of the Unity framework, JSON-authored ([02-data-model.md §10](../02-data-model.md)):
`AIBehaviour{activeMode, rulesByMode}`; per rule: AND-conditions + one decision. EnemyTurn
evaluates top-down; first rule whose conditions pass **and** whose decision yields a legal
action wins; nothing legal ⇒ `EndTurnAction`.

Initial condition catalog: `enemyWithinRange{range}`, `allyHpBelow{percent}`,
`selfHpBelow{percent}`, `hasStatus{status, on:"self"|"enemy"}`, `canCast{ability}`.
Initial decisions: `castAbility{ability, target:"nearestEnemy"|"lowestHpEnemy"|"self"|
"lowestHpAlly"}` (moves into range first if MP allows: path toward nearest valid cast cell),
`moveTowardNearestEnemy`, `moveAwayFromEnemies`, `endTurn`.
M1 stand-in (step 10, before the framework exists): hardcoded
"move toward nearest enemy, attack if adjacent" enemy controller — replaced by step 23.

## 7. Battle entry/exit dataflow

```
encounterTriggered(spawn)                            [encounters-taming.md]
  → BattleMode.enter: BoardBuilder.fromWorld(...) → BattleContext{playerTeam, enemyTeam,
      board, placementStyle, allowsTaming, returnWorldCell=player cell}
  → EventCenter.battleStarted(ctx) → coordinator starts Setup
  … End phase → EventCenter.battleResolved(ctx, winner)
  → FeatProgressionService processes the BattleLog        [creatures-feats.md]
  → TamingService finalizes impressed creatures           [encounters-taming.md]
  → ModeManager returns to ExplorationMode (loss ⇒ respawn at heal point, save-meta.md)
```

## 8. Testing

Everything above is headless. Priority suites (PlaygroundTests, using `BoardFixture`):
turn-bar ordering & tie-breaks & stun; AP/MP refill; validator truth tables (range shapes ×
LoS × profiles); resolver damage/heal math against hand-computed values (mitigation, ratios,
shields, lifesteal); status hooks (order, non-reentrancy, durations); outcome rules incl.
tamed-left units; full scripted battles (sequence of actions ⇒ expected end state + expected
event log). The **event log assertions** are the highest-value tests — feats and taming
depend on exact event emission.
