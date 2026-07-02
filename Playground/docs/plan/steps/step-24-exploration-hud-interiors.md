# Step 24 — Exploration HUD, heal points, interiors via portals

**Phase H · needs steps 20, 21 · closes Milestone 2**

## Goal

Exploration reaches its wireframe HUD, heal points work, and portals connect maps
(house + tunnel on the testground) — the full out-of-battle loop.

## Reading

[ui-hud.md §Exploration](../../03-systems/ui-hud.md) ·
[world.md §Interiors & portals](../../03-systems/world.md) (D37) ·
[02-data-model.md §13/§13b](../../02-data-model.md).

## Files

`srcs/ui/exploration_hud.hpp/.cpp` — top-left time widget (static placeholder), top-center
zone label (active map/biome display name), top-right icon buttons (Save — stub until
step 32, logs; Quit), left party strip (6 compact creature cards bound to the team),
interaction prompt label (shows near portals/heal points); activated by ExplorationMode.
`srcs/world/portal_service.hpp/.cpp` — watches `playerMoved`; on portal cell: load target
map into `WorldContext` (cache loaded maps), move player to target portal cell, rebuild
navigation focus, update zone label (D37).
Heal points: marker-driven; stepping on one heals the team to full and sets
`GameContext.respawnPoint`; loss flow now teleports to respawnPoint + full heal (replaces
step 12's log line).
Data: `maps/small-house-interior.json` (tiny room, portal `entrance` back to the
testground), `maps/test-tunnel.json` (corridor, portals `west`/`east` to two testground
doors), testground gains the house stamp + two tunnel doors + portals; second bush biome
inside the tunnel (`caveFloor` tag rule) proving interior encounters.

## Tests (`[test]`)

Portal transitions (world swap, player placement at target, return through either tunnel
end lands at the right door); loaded-map caching; heal point sets respawn + heals; defeat
teleports to respawn healed with feat progress intact; interior encounter rule fires on
caveFloor tags; HUD binding cycle safety (mode switches).

## Definition of Done — **Milestone 2 (user validates)**

`[build]`/`[test]` green; `[run]`: the complete loop with real data — explore with HUD,
enter the house and tunnel (both exits correct), heal at the heal point, wild + trainer +
interior encounters, manual placement, full HUD battles with AI, feats progress, taming
recruits, defeat respawns at the heal point. Tag `milestone-2` after sign-off.
