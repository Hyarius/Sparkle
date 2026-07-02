# Step 33 — Gyms, badges/tier scaling, Elite Four, team management

**Phase K · needs steps 30, 32 · closes Milestone 3**

## Goal

The win condition: 8 gyms on handcrafted boards scaled by badges, the Elite Four gauntlet,
and the team/PC management screen — a full run becomes playable.

## Reading

[save-meta.md §Meta progression](../../03-systems/save-meta.md) · GDD §10/§11 ·
[encounters-taming.md](../../03-systems/encounters-taming.md) (handcrafted-board path).

## Files

Gyms: `srcs/world/gym.hpp` + gym prefab interaction (portal into the gym interior map;
inside, a gym-leader trainer on a **handcrafted board** — the battle uses
`BoardBuilder::fromGrid` over the arena map region instead of live derivation: extend
`EncounterSpawn` with an optional `arenaMap` id; `EncounterService` builds from the map
when present — the only non-Gamma path, per GDD special battles), gym clear →
`clearedGyms` + badge, re-challenge blocked.
Data: 8 gym interior maps (world-editor authored, one per biome; simple arenas with
terrain personality: elevation, chokepoints), `encounter-tables/gym-<biome>.json` × 8 —
each authored across tiers so gym difficulty follows badges (GDD's "8 teams per gym";
author 2–3 real tiers each, fallback covers the rest for now — flagged as content debt in
the table files' displayNames), Elite Four: a landmark map gated at 8 badges; 4 consecutive
battles (EncounterService gains a scripted gauntlet: next battle chains on victory,
healing between per GDD silence → decide: full heal between E4 battles [decision-log it]);
victory → credits banner + `postGame` tier unlock.
Team management: `srcs/ui/team_screen.hpp/.cpp` — exploration-HUD button opens: team 6
slots + PC storage grid, swap/move, creature detail reuse (info window content); blocked
during battle.
Wild/trainer scaling: tables everywhere now select tiers by real badge count (already
wired step 20 — author higher-tier entries for the forest + one more biome as proof).

## Tests (`[test]`)

Arena-map battle building (fromGrid path, spawn zones from map markers); gym gating
(cleared → blocked, badge count feeds tiers); E4 gauntlet chaining (loss exits, win
chains, heal-between); team/storage swap rules; postGame tier reachable.

## Definition of Done — **Milestone 3 (user validates)**

`[build]`/`[test]` green; `[run]`: a compressed full run — beat 2 gyms (badges visibly
scale wild encounters), swap a tamed creature in from the PC, reach the Elite Four with a
badge-count debug bump, clear it, see credits, postGame world persists via save. Tag
`milestone-3`.
