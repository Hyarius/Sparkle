# 00 — Overview & Vision

## What Erelia is

Erelia is a **single-player, offline creature-collection tactical RPG** in a **procedurally
generated voxel world**, reimplemented from a Unity/C# prototype into the **Sparkle C++23 /
OpenGL engine**, entirely under `Playground/` in namespace `pg::`.

It blends:

- **Pokémon**-style collection and gym progression — beat **8 gyms in randomized order**,
  then the **Elite Four**;
- **FF12/PoE**-style progression — creatures have **no XP levels**; they earn **Feat
  Progress** by *doing things in battle*, unlocking nodes (abilities, stat bonuses,
  evolutions) on a per-species **Feat Board**;
- **tactical, grid-based, turn-based battles** driven by a **stamina turn-bar** — lower
  Stamina acts more often; AP (abilities) and MP (movement) are separate per-turn resources
  refilled at turn start;
- **battles fought where you stand** — the board is an overlay on the live voxel world
  around the player (special battles use handcrafted boards);
- **taming instead of capture** — impress a wild creature by fulfilling its feat-like
  Taming Conditions during the fight; it leaves the board and joins you if you win;
- a **seeded overworld** — 8 biome-anchor cities, satellite towns, road/bridge/port/tunnel
  network, randomized gym/biome order per run.

**Core loop:** explore → trigger an encounter (bush / interior / trainer LoS / gym) →
tactical board battle → win or lose → creatures earn Feat Progress → unlock feats /
evolutions → progress through gyms.

The **authoritative game spec** is `GDD.md` in the Unity reference project
(`C:\Users\User\Documents\UnityProject\Erelia`). Precedence when sources disagree:
**user clarification → GDD.md → latest UI wireframe → existing code.**
The Unity *implementation* is only a worked example — one previous attempt at the game.
Consult it for behaviors; never treat its class design as something to duplicate
([D28](decision-log.md)). These docs are the design authority for the port.

## Why this project exists (double goal)

The game is a **testbed for Sparkle**: build the game in `pg::`, grow the engine along the
way, and **promote** proven generic pieces (3D layer, viewport widget, graph canvas, property
editor…) into `spk::` — each promotion deliberate and signed-off
([decision D06/D18](decision-log.md)).

## Non-negotiable design rules

From the GDD/AGENTS — preserve these in every design and implementation choice:

1. No creature XP levels; growth is battle-earned Feat Progress on the Feat Board.
2. Unlocks depend on *what the creature did*, not merely on winning; **losing still grants
   progression**.
3. 6-slot active team; overflow to PC storage.
4. Tactical grid battles with the stamina turn-bar; AP & MP refill at turn start; movement
   and abilities are separate resources.
5. Most battles derive from the surrounding voxel world; special battles may be handcrafted.
6. Procedural seeded overworld; gym order / encounter scaling gated by badges (unlock tier),
   never by level.
7. Eight-ability action bar; desktop-first, panel-based UI, world kept visible.

## Milestone 1 — the target slice ([D05](decision-log.md))

Boot **directly into exploration** of a hardcoded deterministic voxel world (flat ground,
elevation bumps, a small wall, slopes, stairs, bush patches). Move by **clicking a world
cell** (mouse raycast → world A* → the player walks the path, D27); ZQSD/mouse orbit the
camera. Stepping into a **bush** rolls a wild encounter → the battle grid overlays the world in place,
camera blends to a tactical angle → auto-placement → stamina turn-bar loop with **Move, one
damage Ability, End Turn** (mouse targeting + previews) against a trivially scripted enemy →
win/lose banner → overlay drops, free-roam resumes. No menu, no HUD beyond the debug overlay,
no taming/feats/statuses yet.

Everything after M1 (creatures/feats, taming, encounters, HUD, AI, tools, world generation,
save, meta, animation, audio, menu) is planned to full step detail in
[plan/implementation-plan.md](plan/implementation-plan.md).

## How this documentation is organized

| Doc | Contents |
|-----|----------|
| [README.md](README.md) | Index and navigation. |
| [01-architecture.md](01-architecture.md) | Runtime architecture in Sparkle terms; Unity → Sparkle concept map; engine gaps. |
| [02-data-model.md](02-data-model.md) | JSON schemas + examples for every authored definition type; polymorphism and loading rules. |
| [03-systems/](03-systems/) | One design doc per system (voxel, world, board, battle, creatures-feats, encounters-taming, rendering, UI, save, animation, audio, tooling). |
| [diagrams/](diagrams/README.md) | PlantUML class + sequence diagrams. |
| [plan/](plan/implementation-plan.md) | The dependency-ordered implementation plan: phases → steps, one self-contained file per step. **This is what Opus 4.8 executes.** |
| [howto/](howto/) | Recipes for recurring operations (build & run, add a component+logic, add a render command, add a JSON definition type, add tests, add a tool tab, promote to spk::). |
| [glossary.md](glossary.md) | Canonical terms and naming rules. |
| [decision-log.md](decision-log.md) | Every locked decision with rationale. Read before proposing changes. |

**If you are an implementing agent, start with
[plan/implementation-plan.md](plan/implementation-plan.md)**; each step file tells you what
to read, build, and verify. The system docs are your reference for *why* and *how* a system
is shaped; the Unity reference project is the worked example when a behavior is ambiguous.
