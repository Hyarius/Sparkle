# Step 08 — Biome/encounter JSON (minimal), EncounterEmitter/Resolver, bush trigger

**Phase C · critical path · needs step 07**

## Goal

Walking into a bush rolls a wild encounter and fires `encounterTriggered` (D05). No battle
yet — the event logs to console/overlay. Minimal data slice of the full encounter system.

## Reading

[encounters-taming.md §1](../../03-systems/encounters-taming.md) ·
[02-data-model.md §11/§12](../../02-data-model.md) ·
[09-class-encounters-taming.puml](../../diagrams/09-class-encounters-taming.puml).

## Files

`srcs/encounters/`: `encounter_types.hpp` (`EncounterSpawn{enemyTeam, allowsTaming,
boardSize, placementStyle}`, `EncounterTeamMember{speciesId, aiId, completedNodeUuids}` —
species stays a **string id** until step 14 gives it a registry),
`encounter_table.hpp` + parser (tiers/weightedTeams per schema; only `noBadge` authored
now), `encounter_resolver.hpp/.cpp` (same-cell suppression, seeded `std::mt19937` roll vs
`chancePerStep`, tier fallback, weighted pick), `encounter_emitter.hpp/.cpp` (subscribes
`playerMoved`; reads voxel tags at the player's cell — the *passable* voxel occupying it;
matches active biome's `encounterRules`; exploration-mode gate).
`srcs/encounters/biome.hpp` + parser (palette ids + encounterRules).
Registries grow `biomes` + `encounter-tables`; `WorldContext` resolves the active map's
biome id at load.
Data: `resources/data/biomes/forest.json` (rule: trigger `bush`, table `forest-basic`,
chance 0.15), `resources/data/encounter-tables/forest-basic.json` (one team: species
`"sprout"` — a string for now, one `null`-padded slot list).
`srcs/core/exploration_mode.*` — enables the emitter on enter, disables on exit.

## Work items

1. Tag lookup at cell: the cell the player *occupies* (bush = passable cross-plane at the
   standing cell + 1? No — the bush occupies the standing space itself: the player stands
   *in* the bush cell, on the solid below it. Check the cell **above the solid** the player
   stands on, i.e. the player's occupancy cell).
2. Determinism: resolver RNG seeded from `GameContext.world.seed` + a fixed stream id.
3. On trigger: log spawn summary + a debug overlay line; freeze further rolls for 2 s
   (visual sanity until battle consumes the event in step 12).

## Tests (`[test]`)

Resolver: no re-roll on same cell; chance 0 never fires, 1 always; tier fallback (empty
`oneBadge` → `noBadge`); weighted pick distribution pinned for a fixed seed sequence.
Emitter: fires only in exploration mode; only on rule-matching tags; biome without rules
never rolls. Parsers: biome + table (+ error cases).

## Definition of Done

`[build]`/`[test]` green; `[run]`: walking through a bush patch eventually prints
"Encounter: lone sprout (sprout) — board 11×11" on the overlay; standing still in the bush
never re-rolls; the far bush patches also work. User validates the feel of the trigger rate.
