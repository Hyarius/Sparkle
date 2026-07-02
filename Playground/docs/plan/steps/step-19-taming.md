# Step 19 — Taming: profiles, live evaluation, impressed/recruit flow

**Phase G · needs step 18**

## Goal

The signature acquisition mechanic (D24/D33): wild creatures with TamingProfiles evaluated
live per battle event; impressed creatures leave the fight and join the team on victory.

## Reading

[encounters-taming.md §2](../../03-systems/encounters-taming.md) (the spec) ·
[02-data-model.md §8 tamingProfile](../../02-data-model.md).

## Files

`srcs/taming/`: `taming_condition.hpp` + `taming_condition_factory.cpp` (own factory over
the shared `BattleCondition` core, D33 — registers the shared catalog from step 17;
taming-only types get added here as design demands), `taming_profile.hpp` + parser
(species parsing now consumes the deferred `tamingProfile` field from step 14),
`taming_progress.hpp/.cpp` (per-condition Advancements, isImpressed/hasFailed,
evaluateEvents, markFailed, reset), `wild_battle_unit.hpp` (BattleUnit + TamingProgress),
`taming_service.hpp/.cpp` — battle-scoped: on `battleEventOccurred` (allowsTaming only)
evaluate live wild units; impressed → `context.removeUnit` + `creatureImpressed`; on
`battleResolved`: player win → recruit per impressed unit (fresh CreatureUnit
**inheriting the spawn's completedNodes**, D35 corollary, then applyProgress) →
`player.addCreatureToTeamOrStorage` + `creatureRecruited`; loss → forfeit.
`battle_context` — wild enemy instantiation creates `WildBattleUnit` when the species has
conditions and `allowsTaming`; `defeatUnit` marks untamable (already spec'd step 10 —
activate).
Data: sprout + ember-fox gain tamingProfiles (2 conditions each, achievable: e.g. deal ≥8
magical in one hit + end 2 turns adjacent).

## Tests (`[test]`)

Live evaluation on scripted event streams: impressed exactly when the last condition's
window completes; defeat-before-completion → failed permanently (later events ignored);
impressed unit leaves board (registry cleared) and victory triggers when it was the last
enemy; recruit inherits completedNodes (form!) but not partial progress; team full →
storage; loss forfeits. Parser: taming-only factory separation (a type registered only in
the feat factory errors in a taming profile — craft one to prove the split).

## Definition of Done

`[build]`/`[test]` green; `[run]`: fulfill sprout's conditions in a wild fight → it visibly
leaves the board mid-fight, banner shows "Sprout was impressed!", after victory it appears
in the team (7th recruit lands in storage — force by JSON-tweaking team size scenario or
recruit twice). User validates the moment-to-moment feel.
