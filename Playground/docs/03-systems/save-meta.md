# System — Save/Load & Meta Progression

Persistence (seed, team, feat progress, cleared content) and the badge/tier meta loop
(gyms, Elite Four, respawn).

Diagrams: [25-seq-save-load.puml](../diagrams/25-seq-save-load.puml).
Plan: steps [32](../plan/steps/step-32-save-load.md),
[33](../plan/steps/step-33-meta-progression.md).
JSON: [02-data-model.md §15](../02-data-model.md).

---

## Save system (`Playground/srcs/save/`)

```
pg::GameSaveData     // mirrors saves/<slot>.json exactly:
    int worldSeed; Vector3Int respawnPoint;
    PlayerSave { Vector3 position; team[6] (nullable CreatureSave); storage[] }
    CreatureSave { speciesId; FeatBoardProgress json }   // form is derived (D35)
    std::vector<std::string> clearedGyms, clearedTrainers;
pg::SaveService
    save(const GameContext&, slot)   // gather → serialize → atomic write
                                     // (write tmp, rename over saves/<slot>.json)
    load(slot) -> GameSaveData       // strict parse (same JsonReader machinery)
    newGame(seed) -> GameSaveData    // fresh save with starter team
```

Principles (locked):
- **Seed is fixed for the run** and saved; the macro world plan re-derives from it
  deterministically ([world.md](world.md)) — the plan itself is *not* saved.
- Creature stats/abilities/passives **and current form** are **not** saved — recomputed
  from `FeatBoardProgress` via `applyProgress` at load (D35;
  [creatures-feats.md](creatures-feats.md) §1).
- Requirement progress is **UUID-keyed** (D34): node uuid → requirement uuid →
  {progress, repeats}. Board definitions can be reordered/edited freely; a save entry whose
  uuid no longer exists in the board is dropped with a logged warning at load. The feat
  registry maintains the uuid → node lookup.
- Save points: explicit save (HUD button / menu), plus autosave after battle resolution and
  gym clears. Loading happens only at boot (Milestone scope; in-run load = restart flow).
- Slot files under `Playground/saves/`; `"version"` gate like all JSON.

## Meta progression (`Playground/srcs/save/` + services)

- **Badges** = `clearedGyms.size()`. Drives `EncounterTable::tierForBadges` everywhere
  (wild, trainer, gym teams) — the GDD's tier scaling without levels.
- **Gyms** (step 33): 8 gym markers placed by world gen (one per major city); each gym has
  an authored `encounter-tables/gym-<biome>.json` whose 8+1 tiers are its "8 different teams
  depending on gyms already defeated" (GDD §10). Clearing adds its id to `clearedGyms`
  (permanent — won battles never repeat). Elite Four: post-8-badges scripted gauntlet of 4
  battles at a world landmark; clearing = win condition (credits banner v1).
- **Trainers**: `clearedTrainers` gates their LoS trigger permanently.
- **Loss flow**: on defeat, feat progress still applies (D25), then the player teleports to
  `respawnPoint` (last visited **heal point**; heal points are map/world markers that set
  `respawnPoint` and heal the team on touch), team fully healed. No money penalty (none
  exists).
- **PC storage**: team/storage swap UI is part of the team-management screen (step 22's
  creature window grows a storage tab in step 33 — deferred-screen list in Unity AGENTS
  keeps this minimal for the prototype).

## Testing

Headless: save→load→save round-trip equality; recompute-on-load correctness (attributes
*and form* identical to pre-save); unknown-uuid tolerance (board def changed → entry
dropped with warning, no crash); tier selection per badge count incl. fallback; respawn
flow state (position, healed team, progress kept); cleared gyms/trainers gating.
Atomic-write behavior (no partial file on simulated failure).
