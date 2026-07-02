# Step 15 — Ability definitions, full effect hierarchy, targeting rules complete

**Phase F · needs step 14**

## Goal

The complete data-driven ability system: the polymorphic `Effect` hierarchy with its
factory, execution contexts, full range/AoE/LoS/profile targeting, and the generated
rules-text `describe()` foundation. (Statuses/shields effects parse but their runtime lands
in step 16.)

## Reading

[battle.md §4](../../03-systems/battle.md) ·
[02-data-model.md §5/§5.1](../../02-data-model.md) (the effect catalog is the spec) ·
[howto/add-json-definition.md](../../howto/add-json-definition.md).

## Files

`srcs/abilities/`: `ability.hpp` + parser (cost/range/AoE/targetProfile/casterAnimation/
targetAnimation/travelVfx refs — animation & vfx ids stored as strings, resolved when those
registries exist (steps 34); validate non-empty), `effect.hpp`
(`Effect` base + `BattleAbilityExecutionContext{context, ability?, sourceObject,
targetObject, anchorCell, affectedCell}`), `effects/` one file per effect type from the
catalog (damage, heal, applyStatus, removeStatus, consumeStatus, cleanse, revive,
applyShield, resourceChange, stealResource, displacement, swapPosition,
swapPositionWithCaster, teleportSelf, adjustTurnBarTime, adjustTurnBarDuration, placeObject,
removeObject), `effect_factory.cpp` (registers all + parse tests hook),
`effect_describe.cpp` (`describe(const Effect&) -> std::string` per type — one sentence from
fields; used by HUD step 22 and the tools step 28).
`srcs/battle/rules/battle_action_resolver.*` — ability resolution becomes: costs →
AbilityCast → per-AoE-cell execution contexts → `effect->apply(ctx)` loop → pending
defeats (replaces step 10's hardcoded damage; `tackle.json` gains a real `effects` array).
`rules/battle_geometry.*` — complete: all range shapes × AoE shapes (step 11 started).
Status-dependent effects (applyStatus/…): apply through a `BattleStatuses` API that step 16
fills — this step lands them behind interfaces with TODO(step-16) no-op bodies **plus
parse + describe coverage**, so data authoring is never blocked.
Data: flesh out `abilities/` — `ember.json` (magical, range 1–6 circle LoS, AoE cross 1,
burn applyStatus), `gust.json` (line range, displacement), `mend.json` (heal, ally profile),
`bulwark.json` (applyShield self).

## Tests (`[test]`)

Factory: every type parses its sample + unknown-type error + per-type field errors.
Resolver with real effects: damage+heal math (ratios, bonusHealing), lifesteal/
omnivampirism, displacement paths (blocked mid-push), swap/teleportSelf board sync, steal
amounts = actual removed, AoE multi-target ordering (anchor order), targetProfile
enforcement, min-range ring. `describe()` golden strings per type. Event-log golden test
extended (Displacement, ResourceStolen, Teleported…).

## Definition of Done

`[build]`/`[test]` green; `[run]`: creatures cast their JSON abilities in battle — ember's
cross AoE hits multiple targets, gust pushes, mend heals (bind keys 1–4 to the unit's
ability list — the input already supports selection by index). User validates previews
match effects.
