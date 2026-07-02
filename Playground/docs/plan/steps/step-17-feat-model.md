# Step 17 — Feat board model: nodes, BattleCondition core, rewards, JSON

**Phase F · needs step 16**

## Goal

The feat data model and the shared condition-evaluation engine (D33/D34): boards of
UUID'd nodes, the `BattleCondition` core with its four scope windows, the FeatRequirement
catalog/factory, rewards, and UUID-keyed progress types. (Battle-log evaluation wires up
in step 18.)

## Reading

[creatures-feats.md §2–§4](../../03-systems/creatures-feats.md) (the spec) ·
[02-data-model.md §6/§6.1/§6.2](../../02-data-model.md) ·
`includes/structures/container/spk_uuid.hpp`.

## Files

`srcs/feats/`: `battle_condition.hpp/.cpp` — the shared core: Scope enum
(Ability/Turn/Fight/Game), requiredRepeatCount, `Advancement{progress, repeats}`,
`evaluateEvents(events, adv)` implementing the four window semantics exactly per
[creatures-feats.md §3](../../03-systems/creatures-feats.md), `BattleConditionTemplated<TEvent>`,
`computeLinearProgress`; `feat_requirement.hpp` (= BattleCondition + the feat factory
declaration), `conditions/` — one file per catalog type from
[02-data-model.md §6.1](../../02-data-model.md) (dealDamage, takeDamage, surviveHit,
healHealth, healTarget, applyShield, applyShieldCount, absorbDamageWithShield,
maxDamageAbsorbedInOneHit, shieldBroken, applyStatusCount, removeStatusCount, killCount,
lastHit, winBattleCount, consumeResources, totalDistanceTravelled, displacementDealt,
displacementReceived, teleportCount, teleportDistance, turnStartPosition, turnEndPosition,
castAbilityCount, and, or), `feat_requirement_factory.cpp` (registers all; the
TamingCondition factory reuses these registrations in step 19),
`feat_reward.hpp` + `rewards/` (bonusStats, ability, removeAbility, passive, changeForm) +
factory, `feat_node.hpp` (uuid, displayName, position, kind, neighbourUuids, repeatLimit,
requirement entries {uuid, condition}, rewards), `feat_board.hpp/.cpp` + parser
(rootNode uuid, node lookup, neighbour resolution, validation: connected graph, unique
uuids, dangling neighbours, form nodes tier-ascending in file order — D35),
`feat_progress.hpp/.cpp` (FeatRequirementProgress/FeatNodeProgress/FeatBoardProgress,
UUID-keyed, toJson/fromJson with unknown-uuid drop+warn — replaces step 14's stub),
`feat_registry` additions: boards registry + global `uuid → (board, node)` lookup.
`creatures/creature_species` — resolve `featBoard` ref (step 14 left it unresolved);
`apply_progress` — full reward replay (base + defaultAbilities + defaultFormId, then per
completed node × completionCount in board order → derived attributes/abilities/passives/
**form**).
Data: `featboards/sprout-board.json` (root + ~8 nodes: stat bonuses, ember unlock,
a passive, a `blaze` form node with tier gate), `featboards/ember-fox-board.json` (smaller);
species reference them.

## Tests (`[test]`) — highest-value suite of Phase F

Window semantics × 4 scopes: single-event 100% (ability), per-turn grouping (turn),
cross-battle carryover reset (fight), persistence (game) — including repeat counts and
progress remainders (golden tables). and/or combinators. Reachability (root completed,
neighbour unlock, exhaustion, form tier gate + branch lockout after evolving). Reward
replay: applyProgress determinism + idempotence, **form derivation** (complete the form
node → currentFormId flips; save/load-shaped round trip via progress JSON). Board
validation errors. Progress JSON round-trip with an unknown uuid (dropped + warning
counted).

## Definition of Done

`[build]`/`[test]` green; `[run]` unchanged visually; boards load at boot (log node
counts). A debug assertion path: `completeNodeDirect(starter, emberNodeUuid)` at newGame
(temporary, removed in step 18) proves sprout gains Ember in its battle ability list.
