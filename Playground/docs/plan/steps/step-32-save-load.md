# Step 32 — Save/Load: GameSaveData, SaveService, autosave

**Phase K · needs steps 18, 24 (30 for world-seed relevance; runs fine on the testground too)**

## Goal

Persistence per [save-meta.md](../../03-systems/save-meta.md): versioned save slots,
UUID-keyed feat progress, derived-state recompute on load, autosave hooks, and the real
Save button.

## Reading

[save-meta.md](../../03-systems/save-meta.md) (the spec) ·
[02-data-model.md §15](../../02-data-model.md) ·
[25-seq-save-load.puml](../../diagrams/25-seq-save-load.puml).

## Files

`srcs/save/`: `game_save_data.hpp` (mirror of the schema), `save_service.hpp/.cpp`
(`save(ctx, slot)` — gather → serialize (creatures: species id + FeatBoardProgress only,
D34/D35) → **atomic write** (tmp + rename) to `Playground/saves/<slot>.json`;
`load(slot)` — strict parse, unknown-uuid drop+warn; `newGame(seed)` — starter team
assembly moves here from GameContext), `save_binder.hpp/.cpp` (autosave on
`battleResolved` + gym clears; HUD Save button wiring — replaces step 24's stub).
Boot flow: `main` → if `saves/auto.json` exists → load, else newGame with a random seed
(menu chooses later, step 36); `--new-game[=seed]` CLI override for testing.
`GameContext` — `clearedGyms`/`clearedTrainers` round-trip; respawnPoint persisted.

## Tests (`[test]`)

Round-trip equality (save→load→save byte-stable); recompute-on-load: a creature with 3
completed nodes loads with identical derived attributes/abilities/passives/**form** as
pre-save; unknown node/requirement uuid dropped with warning (count asserted); Game-scope
requirement progress survives the trip; cleared sets gate trainers after load; atomic
write leaves no partial file when interrupted (simulate via injected failing stream);
version mismatch → clean error.

## Definition of Done

`[build]`/`[test]` green; `[run]` (user validates): play (progress feats, tame, clear the
trainer, move respawn), Save, quit, relaunch → identical state (same world from the seed,
team, progress, cleared trainer stays cleared); defeat → respawn at the saved heal point.
