# Implementation Plan — Erelia in Sparkle

The dependency-ordered build. Each step links to a **self-contained file** in
[steps/](steps/) with goals, files, types, JSON, tests, and a Definition of Done. Execute
steps **in order** unless a step file says it's parallel-safe.

**If you are the implementing agent (Opus 4.8), your loop is:**

1. Open the next step file. Read it fully, plus the system docs it links.
2. Read the *current* code the step touches (files move; the step file's file lists are the
   plan, reality wins — if they diverge, update the step file first, then implement).
3. Implement. Follow [howto/](../howto/) recipes for recurring operations, the
   [glossary](../glossary.md) for names, the [decision log](../decision-log.md) for settled
   choices — do not re-litigate them; raise genuine blockers to the user instead.
4. Verify the Definition of Done: build, run tests, do the listed visual check (screenshot
   for the user when the step says so — visual results are **hand-validated by the user**,
   never self-declared).
5. Record progress: check the step's box below, note deviations in the step file, add any
   new decision to the decision log. Commit (one step = one or few commits).

**Global rules:** all game code in `pg::` under `Playground/` (D06); `spk::` touched only in
promotion steps; never invent an `spk::` API — read the header first; keep `tests/` (spk)
green; PlaygroundTests green after every step; JSON per
[02-data-model.md](../02-data-model.md); no "Quest"/"Recovery"/"Obstacle-Walkable"/"capture"
naming (glossary). The Unity project is a **worked example, not a spec** (D28): steps may
point you at a Unity file for a behavior reference — read it for the *what*, design the
*how* per these docs.

---

## Phases, critical path, milestones

```
Phase A  Foundations            steps 01–02        (build split, JSON, bus, modes)
Phase B  3D + Voxel flagship    steps 03–06        (depth/lighting, voxel core, mesher, world)   ◄ highest risk, front-loaded
Phase C  Exploration slice      steps 07–08        (click-to-move, encounter trigger)
Phase D  Battle core            steps 09–12        (board seam, backend, overlay, M1 wiring)
         ──────────────────────► MILESTONE 1 after step 12
Phase E  First promotion        step 13            (3D layer → spk::)
Phase F  Creatures & battle depth  steps 14–18     (species/team, abilities/effects, statuses, feats)
Phase G  Taming & encounters    steps 19–20
Phase H  HUD & AI               steps 21–24        (battle HUD, placement UI, AI, exploration HUD, interiors)
         ──────────────────────► MILESTONE 2 after step 24 (full wild-encounter loop with real data)
Phase I  Tools                  steps 25–28        (suite + voxel modeler first — the anchor tool)
Phase J  World generation       steps 29–31        (macro plan, realization, interiors/tunnels)
Phase K  Meta & persistence     steps 32–33        (save/load, gyms/badges/Elite Four)
         ──────────────────────► MILESTONE 3 after step 33 (a full run is playable)
Phase L  Presentation polish    steps 34–36        (fake animation, audio, menu + minimap + result screens)
Phase M  Promotions II          step 37            (viewport/graph-canvas/property-editor → spk::)
```

**Critical path:** 01 → 03 → 04 → 05 → 06 → 07 → 09 → 10 → 11 → 12. Everything else hangs
off it. The riskiest unknowns (depth-tested voxel rendering on Sparkle, live board
derivation) are all resolved by step 12 — that's why M1 is shaped this way.

**Milestone 1 (D05):** boot → hardcoded voxel world → click-to-move exploration → bush
encounter → in-place overlay battle (Move / one Ability / End Turn, mouse targeting, trivial
enemy) → win/lose banner → free-roam. No menu/HUD/feats/taming/statuses.

**Tool track:** the tool suite deliberately waits until the systems it edits exist and are
data-proven (post-M2), but its **anchor** (voxel modeler) comes first in Phase I because
content authoring gates everything after (D15). If hand-editing JSON becomes painful earlier,
step 25 may be pulled forward — it only depends on step 06.

**Promotion track (D18):** step 13 (3D layer) right after M1; step 37 (tool widgets) at the
end; each requires user sign-off before touching `spk::`.

**Parallel-safe notes:** steps 14–18 are sequential among themselves; 21–22 (HUD) can
interleave with 19–20; 25–28 (tools) and 29–31 (worldgen) are independent tracks after 24;
34/35 are independent of each other.

## Step index

| ✔ | Step | Title | Phase |
|---|------|-------|-------|
| ☐ | [01](steps/step-01-build-restructure-json.md) | Build split (PlaygroundCore/exe/tests) + nlohmann-json + JSON loader & registries | A |
| ☐ | [02](steps/step-02-foundations-events-modes.md) | EventCenter, observables, GameContext, Mode/ModeManager | A |
| ☐ | [03](steps/step-03-render3d-hardening.md) | 3D hardening: Mesh3D (normals), depth test, lambert light, translucent pass | B |
| ☐ | [04](steps/step-04-voxel-core.md) | Voxel core: definitions, 5 shapes, heights, cells/grids, JSON + registry | B |
| ☐ | [05](steps/step-05-voxel-mesher.md) | VoxelMesher with neighbour occlusion → showcase chunk renders | B |
| ☐ | [06](steps/step-06-world-chunks.md) | Chunks, VoxelWorld, map loading, M1 testground, chunk rendering | B |
| ☐ | [07](steps/step-07-player-exploration.md) | Traversal graph + A*, mouse picking, click-to-move player, orbit camera | C |
| ☐ | [08](steps/step-08-encounter-trigger.md) | Biome/encounter JSON (minimal), EncounterEmitter/Resolver, bush trigger | C |
| ☐ | [09](steps/step-09-board-seam.md) | BoardData seam, live world derivation, deployment zones, board fixture | D |
| ☐ | [10](steps/step-10-battle-backend.md) | Battle backend: units, turn bar, phases FSM, Move/Ability/EndTurn, M1 enemy stand-in | D |
| ☐ | [11](steps/step-11-battle-overlay.md) | Overlay rendering, tactical camera blend, mouse targeting + previews | D |
| ☐ | [12](steps/step-12-milestone1-wiring.md) | **Milestone 1**: end-to-end wiring, unit views, move tween, result banner | D |
| ☐ | [13](steps/step-13-promotion-3d.md) | Promotion: 3D layer → `spk::` (sign-off required) | E |
| ☐ | [14](steps/step-14-creatures-team.md) | Attributes, species/forms/units JSON, PlayerData team, creature views | F |
| ☐ | [15](steps/step-15-abilities-effects.md) | Ability definitions, full effect hierarchy, targeting/AoE/LoS rules | F |
| ☐ | [16](steps/step-16-statuses.md) | Statuses: hooks, durations, stacks, shields, stun; battle events completed | F |
| ☐ | [17](steps/step-17-feat-model.md) | Feat board model: nodes, requirements (scope windows), rewards, JSON | F |
| ☐ | [18](steps/step-18-feat-progression.md) | Battle log → FeatBoardService evaluation → applyProgress pipeline | F |
| ☐ | [19](steps/step-19-taming.md) | Taming: profiles, live evaluation, impressed/recruit flow | G |
| ☐ | [20](steps/step-20-encounters-full.md) | Full encounters: tables/tiers JSON, weighted rolls, trainer LoS | G |
| ☐ | [21](steps/step-21-battle-hud-1.md) | Battle HUD I: team columns, active-unit panel, ability bar, turn strip | H |
| ☐ | [22](steps/step-22-battle-hud-2.md) | Battle HUD II: cards/hover, placement UI, result + feat summary, info window | H |
| ☐ | [23](steps/step-23-ai.md) | AI: behaviours JSON, conditions/decisions, enemy turn evaluation | H |
| ☐ | [24](steps/step-24-exploration-hud-interiors.md) | Exploration HUD, heal points, interiors (maps + teleport) | H |
| ☐ | [25](steps/step-25-tools-suite-voxel-editor.md) | EreliaTools shell + Viewport3D/PropertyPanel + **Voxel Modeler** | I |
| ☐ | [26](steps/step-26-tools-world-editor.md) | World/Structure editor | I |
| ☐ | [27](steps/step-27-tools-featboard-editor.md) | Feat Board editor (GraphCanvas) | I |
| ☐ | [28](steps/step-28-tools-encounter-ability-editors.md) | Encounter/Team + Ability/Status editors | I |
| ☐ | [29](steps/step-29-worldgen-macro.md) | Macro world plan: landmass, cities, biomes, transport graph (+debug image) | J |
| ☐ | [30](steps/step-30-worldgen-realization.md) | Chunk realization: terrain, roads, flora, structure stamping | J |
| ☐ | [31](steps/step-31-worldgen-interiors.md) | Tunnels/caves (CellularAutomata), ports/sea links, interior generation | J |
| ☐ | [32](steps/step-32-save-load.md) | Save/Load: GameSaveData, SaveService, respawn/heal points, autosave | K |
| ☐ | [33](steps/step-33-meta-progression.md) | Gyms (handcrafted boards), badges/tier scaling, Elite Four, PC storage UI | K |
| ☐ | [34](steps/step-34-animation.md) | Fake animation: rig/recipes/animator + battle hooks | L |
| ☐ | [35](steps/step-35-audio.md) | Audio: miniaudio service, battle/UI/step hooks | L |
| ☐ | [36](steps/step-36-menu-polish.md) | Main menu, minimap (2nd camera), floating text, settings stub | L |
| ☐ | [37](steps/step-37-promotion-2.md) | Promotion: Viewport3D, GraphCanvas, PropertyPanel → `spk::` (sign-off) | M |

## Sizing & verification bar

Each step ≈ one focused implementation session. Every step file specifies: **Goal /
Dependencies / Files / Work items / JSON / Tests / Definition of Done** (build command, test
command, and what to show the user visually). Build/run/test commands are centralized in
[howto/build-and-run.md](../howto/build-and-run.md) — steps reference them as `[build]`,
`[test]`, `[run]`, `[tools]`.
