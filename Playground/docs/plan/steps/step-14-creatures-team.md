# Step 14 — Creatures: species/forms/units JSON, PlayerData team, creature views

**Phase F · needs step 12 (13 can land before or after)**

## Goal

Real creatures behind the battle: `CreatureSpecies`/`CreatureForm`/`CreatureUnit` with the
derived-state pipeline stub, the 6-slot team, model-driven creature views, and battle units
sourced from creatures.

## Reading

[creatures-feats.md §1](../../03-systems/creatures-feats.md) ·
[02-data-model.md §8/§9](../../02-data-model.md) ·
[08-class-creatures-feats.puml](../../diagrams/08-class-creatures-feats.puml).

## Files

`srcs/creatures/`: `creature_species.hpp` + parser (attributes, defaultAbilities refs,
featBoard **string id stored unresolved until step 17**, defaultFormId, forms, tamingProfile
parsed-but-unused until step 19 — parse to raw JSON snippets held for later, or gate fields
by presence; prefer: parse fully but conditions factory arrives step 19 ⇒ **defer
tamingProfile parsing to step 19** and `forbidUnknown` allows the key), `creature_form.hpp`,
`creature_unit.hpp/.cpp` (species*, derived fields, featBoardProgress **stub type** until
step 17: empty struct + toJson/fromJson passthrough), `creature_storage.hpp`,
`player_data.hpp/.cpp` (team[6] nullable + storage + addCreatureToTeamOrStorage; replaces
the step-02 stub).
`srcs/creatures/apply_progress.hpp/.cpp` — v1 pipeline: reset to species base +
defaultAbilities + defaultFormId (reward replay joins in step 18; keep the function the
single entry point already).
Model loading: `srcs/animation/model_definition.hpp` + parser (`models/*.json`: parts →
boxes; `placeholder-cube` built-in), creature view building
(`battle_unit_view_logic` grows: spawn part entities per model with per-part
`MeshRenderer3D` — rig binding arrives step 34).
`srcs/battle/battle_unit.*` — swap `BattleUnitSource` to `CreatureUnit*` (step 10 left the
seam); battle attributes setup from `unit->attributes`.
`encounters/encounter_service.*` — instantiate enemies as `CreatureUnit`s from species
registry (completedNodes ignored until step 18 — log if non-empty).
Data: `creatures/sprout.json`, `creatures/ember-fox.json` (two species, distinct stats/
abilities), `models/placeholder-cube.json`, player starter team = one of each (hardcoded
newGame assembly in `GameContext` until saves).

## Tests (`[test]`)

Species/model parsers (+errors); applyProgress v1 (derived fields reset correctly, list
identity vs registry pointers); PlayerData team/storage overflow; battle-unit-from-creature
attribute mapping; encounter instantiation from species ids.

## Definition of Done

`[build]`/`[test]` green; `[run]`: battles now field "Sprout" vs wild species from the
encounter table, stats from JSON (tweak sprout stamina in JSON → visible turn-order change
without recompiling — demonstrate). Two-species team fights a wild encounter.
