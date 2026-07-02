# Step 16 — Statuses: hooks, durations, stacks, shields, stun

**Phase F · needs step 15**

## Goal

The status runtime: stackable statuses with durations firing effects at hook points,
shields absorbing damage, stun pausing the turn bar — completing the battle rule set.

## Reading

[battle.md §5](../../03-systems/battle.md) (hook semantics: order, non-reentrancy) ·
[02-data-model.md §4](../../02-data-model.md).

## Files

`srcs/statuses/status.hpp` + parser (displayName/icon/tags/hookPoint/effects) + registry;
`srcs/battle/battle_statuses.hpp/.cpp` (entries {status*, stacks, Duration,
isSourcePassive}; add/remove/removeByTags/contains(includeSourcePassives)/
advanceTurnDurations); `rules/battle_status_rules.hpp/.cpp` (applyHook: matching-hook
statuses fire their effects with hook contexts, non-reentrant flag, list order);
`battle_attributes` shields go live (absorb per kind before HP, break events, turn
durations); `rules/battle_resource_rules.*` — fire OnHPLoss/Gain/OnAPLoss/… hooks on real
changes; `rules/battle_turn_rules.*` — TurnStart/TurnEnd hooks + stun tag honored (already
coded in step 10 — now actually reachable), duration advancement at endTurn;
step 15's deferred effect bodies (applyStatus/removeStatus/consumeStatus/cleanse/
applyShield) get real implementations; permanent passives: battle start seeds
`isSourcePassive` infinite entries from `unit->permanentPassives`.
Data: `statuses/poison.json` (turnStart magical DoT), `statuses/burn.json`,
`statuses/stun.json` (tag `stun`, no effects), `statuses/regrowth.json` (HoT); ember
applies burn; new `abilities/daze.json` applies stun (1 turn).

## Tests (`[test]`)

Stack/duration bookkeeping (add stacks refreshes per Duration.Clone semantics — new entry
per apply, as spec'd: entries are independent instances); hook firing order (two statuses
same hook = list order); non-reentrancy (a DoT whose damage would trigger OnHPLoss hooks
does not cascade); stun pauses turn-bar fill and resumes from current; shield absorb tables
(kind matching, partial absorb, break events, infinite durations); cleanse by tags skips
source passives; endTurn duration expiry emits StatusRemoved. Golden event-log updated.

## Definition of Done

`[build]`/`[test]` green; `[run]`: burn ticks damage at the burned unit's turn start
(visible HP drop + log), daze visibly delays the enemy's turn, bulwark absorbs one hit.
