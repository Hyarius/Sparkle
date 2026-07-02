# Step 20 — Full encounters: tiers, board-size overrides, trainer line-of-sight

**Phase G · needs step 19**

## Goal

Encounter completeness: badge-tiered tables everywhere, encounter-defined board sizes on
arbitrary terrain, and trainer actors that spot the player and force a no-taming battle.

## Reading

[encounters-taming.md §1](../../03-systems/encounters-taming.md) ·
[board.md §derivation](../../03-systems/board.md) · GDD §5/§10 (trainer/tier intent).

## Files

`encounters/encounter_table` — full 10-tier parsing + fallback (authored: two more tiers
for forest-basic); `EncounterSpawn.boardSize` honored end-to-end (add a 13×9 wild variant
to prove non-square); `board_builder` — generalize the nudge/shrink validation for rough
generated-like terrain (fixture-driven hardening, no behavior change on flat maps).
Badges: `GameContext` gains `clearedGyms` (empty set until step 33) → tier =
`clearedGyms.size()`.
Trainers: `srcs/world/trainer.hpp` (component: authored team ref, facing, sightRange,
clearedFlag id), `trainer_sight_logic.hpp/.cpp` (on `playerMoved`: DDA trainer→player ≤
range along facing cone (±1 cell lateral per distance); hit → scripted approach (path to
adjacent) → `encounterTriggered{allowsTaming=false, trainer team}`), map schema gains a
`trainers` array (extend `maps/*.json` + parser + one trainer on the testground),
cleared-trainer suppression via `clearedTrainers` (set in `GameContext`; persisted step 32);
victory over a trainer adds the flag.
Data: `encounter-tables/trainer-timmy.json` (fixed team, weight 1).

## Tests (`[test]`)

Tier fallback matrix (0–9+ badges × sparse tables); non-square board derivation +
deployment on it; trainer sight cone truth table (range, cone edges, wall blocks via DDA);
cleared flag suppresses; taming disabled in trainer battles (no WildBattleUnit built).

## Definition of Done

`[build]`/`[test]` green; `[run]`: crossing the testground trainer's gaze starts a battle
(no taming triggers even if conditions met), beating him is permanent for the session;
the 13×9 wild encounter derives and plays on the plateau edge. User validates sight feel.
