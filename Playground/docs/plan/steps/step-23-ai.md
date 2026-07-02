# Step 23 — AI: behaviours JSON, conditions/decisions, enemy turn evaluation

**Phase H · needs step 15 (abilities); replaces step 10's stand-in**

## Goal

Data-driven enemy turns: per-creature rule lists (AND-conditions → decision, top-down,
first legal wins), replacing `simple_enemy_controller`.

## Reading

[battle.md §6](../../03-systems/battle.md) ·
[02-data-model.md §10](../../02-data-model.md) ·
[10-class-ai.puml](../../diagrams/10-class-ai.puml).

## Files

`srcs/ai/`: `ai_behaviour.hpp` + parser (activeMode + rulesByMode), `ai_condition.hpp` +
`conditions/` (enemyWithinRange, allyHpBelow, selfHpBelow, hasStatus, canCast) + factory,
`ai_decision.hpp` + `decisions/` (castAbility{target: nearestEnemy|lowestHpEnemy|self|
lowestHpAlly; walks into cast range first when MP allows — path toward nearest valid
casting cell via flood ∩ range geometry}, moveTowardNearestEnemy, moveAwayFromEnemies,
endTurn) + factory, `ai_evaluator.hpp/.cpp` (rule loop → legal `BattleAction` or EndTurn;
decisions can yield two-part turns: the evaluator submits move, then re-evaluates for the
cast — EnemyTurnPhase already loops via canContinueTurn),
`phases/enemy_turn_phase` — uses the evaluator with the unit's behaviour (from
EncounterUnit data); delete `simple_enemy_controller`.
Registries: `ai` domain. Data: `ai/aggressive-melee.json`, `ai/skirmisher.json` (kite:
attack then move away), `ai/healer.json` (heal ally <50% else advance); wire into encounter
tables + trainer team.

## Tests (`[test]`)

Per-condition truth tables on fixtures; per-decision action legality (cast with move-in,
blocked path fallback to next rule); evaluator ordering (first-match wins; illegal decision
falls through; all-fail → EndTurn); scripted battle vs the skirmisher behaviour (golden
action sequence over 3 turns); parser + factory errors.

## Definition of Done

`[build]`/`[test]` green; `[run]` (user validates): the healer-backed trainer team behaves
visibly differently from wild melee (healing when hurt, kiting skirmisher); no stalls —
every enemy turn ends.
