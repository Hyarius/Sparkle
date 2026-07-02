# Step 10 — Battle backend: units, turn bar, phase FSM, actions, M1 enemy stand-in

**Phase D · critical path · needs step 09**

## Goal

A complete headless battle: two teams on a `BoardData`, stamina turn-bar scheduling, the
7-phase FSM, Move / one damage Ability / EndTurn actions, victory detection, event log.
Runnable entirely from tests. (Abilities/effects here are the *minimal* slice — one hard
damage effect; the full polymorphic system replaces its internals in step 15 without
changing this step's interfaces.)

## Reading

[battle.md](../../03-systems/battle.md) §1–§4/§7 (the spec; D23 semantics) ·
[06-class-battle-phases.puml](../../diagrams/06-class-battle-phases.puml) ·
[07-class-battle-domain.puml](../../diagrams/07-class-battle-domain.puml) ·
[23-seq-battle-action-resolution.puml](../../diagrams/23-seq-battle-action-resolution.puml).

## Files

`srcs/creatures/attributes.hpp` (the stat block struct + JSON parse — pulled forward from
step 14, minimal), `srcs/battle/`:
`battle_side.hpp`, `battle_object.hpp` (moves from step 09 stub),
`battle_attributes.hpp/.cpp` (observable resources per [battle.md](../../03-systems/battle.md);
shields list present but only used from step 15/16),
`battle_unit.hpp/.cpp` (M1: holds `Attributes` directly + a display name — the
`CreatureUnit*` source arrives step 14; keep the field behind a tiny
`BattleUnitSource` indirection so step 14 swaps it in cleanly),
`battle_event.hpp` (full typed event set — cheap to define now, most emitted later),
`battle_log.hpp`, `battle_action.hpp` (Move/Ability/EndTurn),
`turn_context.hpp` (active unit + per-turn counters + turnIndex),
`battle_context.hpp/.cpp` (unit lists, board, placement style, taming flag,
tryPlace/tryMove/trySwap/defeat/remove per spec),
`rules/battle_turn_rules.*` (beginTurn/endTurn/advanceTurnBars/timeUntilNextReady/
selectReady + stun-tag check + canContinueTurn),
`rules/battle_action_validator.*` (afford/reachable(flood)/valid targets — range shapes
minimal: `circle` only this step),
`rules/battle_action_resolver.*` (move along validated path + events; ability = spend costs
+ `AbilityCast` + fixed damage roll via `MathFormulas::computeDamage`; endTurn),
`rules/battle_resource_rules.*` (clamped hp/ap/mp changes + change-result; hook points
no-op until step 16), `rules/battle_outcome_rules.*`,
`rules/battle_placement_rules.*` (auto-place: player strip order, enemy random with seeded
RNG), `math_formulas.hpp/.cpp` (damage/heal/mitigation/turn-bar formulas from
[battle.md](../../03-systems/battle.md) + game-rules constants),
`phases/` (`i_battle_phase.hpp`, setup/placement/idle/player_turn/enemy_turn/resolution/
end phases + `battle_orchestrator.*` + `battle_coordinator.*` — signals via
`spk::ContractProvider`),
`ai/simple_enemy_controller.hpp/.cpp` (M1 stand-in: move toward nearest enemy via A*,
attack when in range, else end turn — replaced in step 23).
`srcs/core/battle_mode.*` — owns context + coordinator; `enter(BattleContext)` starts
Setup; ticks the orchestrator; **no rendering yet** (step 11).

Data: `resources/data/abilities/tackle.json` — parsed into a minimal `pg::Ability` struct
(cost/range/one damage entry); registry `abilities` starts here.

## Work items

1. Turn scheduling exactly per D23 (simulation jumps, tie-breaks, stun tag, AP/MP refill in
   endTurn).
2. Phase wiring per the diagram: coordinator owns transitions, phases own signals, input
   binding stubbed (PlayerTurnPhase exposes `submitAction(BattleAction)` — the UI calls it
   in step 11; tests call it directly).
3. Event emission: ResourceConsumed, AbilityCast, Damage, HitSurvived, UnitDefeated,
   DistanceTravelled, TurnStarted/Ended, BattleWon — asserted by tests.

## Tests (`[test]`) — the core suite of the project

Scripted battles on fixtures: 1v1 walkthrough (advance → attack → defeat → BattleWon);
turn order for stamina 2 vs 4 (unit A acts twice before B); tie-break player-first;
canContinueTurn matrix; validator: reachable flood with MP, circle range min/max, occupied
exclusion; resolver: damage math vs hand values (armor mitigation table), path revalidation
failure; outcome on simultaneous last-unit cases; full event-log golden test for a scripted
2v2 (the exact ordered event sequence — the highest-value regression net in the project).

## Definition of Done

`[build]`/`[test]` green. A `BattleMode::runHeadlessDemo()` debug hook (temporary, removed
in step 12) plays the scripted 1v1 to completion at boot and prints the winner + event
count — proving the FSM ticks outside tests too. No visual change otherwise.
