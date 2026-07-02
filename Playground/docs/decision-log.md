# Decision Log

Every decision that shapes the Erelia port, with its rationale. Newest entries at the bottom.
Entries marked **[user]** were decided by the project owner; **[arch]** are architectural
decisions made during planning (overridable by the user, but treat them as settled unless a
hard technical problem surfaces — then raise it, don't silently diverge).

Conventions used across all docs: `pg::` = Playground game code, `spk::` = the Sparkle
library, "Unity reference" = `C:\Users\User\Documents\UnityProject\Erelia` (read-only).

---

## D01 — Full 3D from the start **[user, 2026-07-01]**
The world is a 3D voxel world; there is no 2D fallback. The 2D tilemap/autotile stack was
deleted from `spk::`. The minimal 3D render layer + voxel core is the flagship early work.

## D02 — Data-driven via JSON, using nlohmann-json **[user, 2026-07-01 + 2026-07-02]**
All authored definitions (voxels, creatures, abilities, statuses, biomes, feat boards,
encounters, taming, animation recipes, save data) are JSON files loaded into registries.
The JSON library is **`nlohmann-json` added via vcpkg** (not a hand-written `spk::JSON`
module). It stays a Playground-side dependency; `spk::` does not link it.

## D03 — Runtime structure = Plan Gamma (fully seamless) **[user, 2026-07-01]**
The battle board is an **overlay on the live world voxels**. One persistent 3D voxel world is
always loaded; no separate battle scene, no snapshot/reload. On encounter: freeze free-roam,
overlay a logical grid on the terrain, reframe the camera (blend, no cut), run the turn system
in place; on battle end drop the overlay and restore free-roam. "Modes" (Exploration, Battle,
later MainMenu) are **control-states over one persistent world/engine**, not scenes.
Battle *logic* still goes through a headless **`pg::BoardData` seam** (cells + navigation +
runtime registry) populated live from world voxels at runtime and from synthetic grids in
tests. Fallback if live derivation ever proves too costly: Plan Beta (snapshot), which shares
the same seam.

## D04 — Multi-camera-aware rendering **[user, 2026-07-01]**
The camera view-projection is uploaded **inside the render process** (a
`spk::CameraUpdateRenderCommand` emitted each frame by the render logic), never cached
globally on the GPU across frames. A later pass (e.g. a minimap) emits its own camera command
+ viewport before its meshes. Do not assume a single global camera. (Implemented this way in
Step 1; verified.)

## D05 — Milestone 1 = no main menu, bush-triggered wild battle **[user, 2026-07-01 + 2026-07-02]**
Boot directly into voxel-world exploration. The M1 encounter is a **wild encounter triggered
by walking into a "bush"** (cross-plane voxel tagged `bush`). Main menu deferred to polish.
M1 battle slice: bush trigger → in-place overlay board → auto-placement (no placement UI) →
turn-bar loop with Move / one damage Ability / End Turn → win/lose detection → return to
free-roam. Exploration movement is click-to-move (D27). **Excluded from M1:** taming, feats,
statuses, real AI (trivial "move toward + attack nearest" stand-in), battle result screen
(debug-text banner suffices), exploration HUD (DebugOverlay only).

## D06 — Everything lives in `pg::` under `Playground/`; `spk::` only via promotion **[user, standing]**
Game code never touches `includes/`/`srcs/` except a deliberate **promotion** of a proven
feature, each promotion being its own plan step requiring explicit user sign-off.

## D07 — Terminology: Feat, never Quest **[user, 2026-07-01]**
The GDD's historical "Quest Validation" concept is named **Feat Progress**. Use `Feat`,
`FeatProgress`, `FeatRequirement`, `FeatReward`, `FeatBoard`, `FeatNode`. The word "Quest"
must not appear in types, files, or JSON.

## D08 — Full step-by-step plan for the entire game **[user, 2026-07-02]**
Fable (this documentation) is the architect; **Opus 4.8 implements** with as few open
decisions as possible, minimizing expensive-model usage. Therefore *every* phase has fully
detailed, self-contained step files — not just the near-term ones. Later steps encode intent
precisely; if reality diverges by then, update the step file first, then implement.

## D09 — Playground cleanup **[user, 2026-07-02]**
Keep the Step-1 3D layer (verified; seed of the rendering stream). Deleted the stale 2D
`player_controller.hpp` / `player_control_logic.hpp`. `Playground/steps/` is superseded by
`docs/plan/` and removed. `ERELIA.md` shrinks to a pointer here. `FABLE_PROMPT.md` archived
at [archive/FABLE_PROMPT.md](archive/FABLE_PROMPT.md). Root `diag.txt`/`diagerr.txt` deleted.
All prior content preserved in git history (commit `b84451e` and earlier).

## D10 — Definition-asset layout & strictness **[user, 2026-07-02]**
`Playground/resources/data/<domain>/<id>.json` — one definition per file, `id` = filename
stem, directory scanned into its registry at boot. Polymorphic types use a `"type"`
discriminator string. Every file carries a `"version"` integer. Unknown or missing fields are
**hard load errors** with file + field context (fail fast; tools author these files).
See [02-data-model.md](02-data-model.md).

## D11 — Voxel shape set from the voxel-core step; slopes and stairs walkable from M1 **[user, 2026-07-02]**
All five shapes (**Cube, Slab, Slope, Stair, CrossPlane**) plus `CardinalHeightSet` traversal
land in the voxel-core step, because M1 requires walkable slopes/stairs and cross-plane
bushes. Chunks are **16×16×16**, mirroring the Unity reference.

## D12 — M1 world is a hardcoded deterministic composition **[user, 2026-07-02]**
Flat ground + hardcoded elevation bumps, a small wall, slopes/stairs, and bush patches —
predetermined obstacles for testing. The JSON chunk/world format is defined and used
underneath anyway. Hand-authored worlds arrive with the world/structure editor; procedural
generation arrives in the world-gen phase.

## D13 — Battle board defaults (M1, refine later) **[user, 2026-07-02]**
Default wild-encounter board: **11×11 cells** centered near the player; deployment zones are
**two opposing 2-cell-deep strips**; the side facing the player's approach direction is the
player zone. Encounter data can override size later, per GDD.

## D14 — Mouse-driven battle targeting; camera→mouse ray via voxel raycast **[user, 2026-07-02]**
Hover = preview, click = confirm, keyboard shortcuts 1–8 for abilities, Esc cancels.
Requires a **screen-point → world ray** utility (unproject through `Camera3D`) plus a **voxel
DDA raycast** to find the hovered cell. Preview overlays at prototype level: move range +
path, ability range, AoE shape, LoS-blocked shading.

## D15 — Tools = one suite executable with tabs **[user, 2026-07-02]**
A single `EreliaTools` executable hosting each editor as a page/tab (Voxel Modeler first,
then World/Structure, Feat Board, Encounter/Team, Ability/Status). Tools read/write the same
`resources/data/` JSON the game loads.

## D16 — Audio via miniaudio **[user, 2026-07-02]**
Sparkle has no audio support. Add **miniaudio via vcpkg** and build a small `pg::` audio
service (SFX + BGM) late in the roadmap. Not a `spk::` module initially; promotable later.

## D17 — Build restructure: PlaygroundCore + exe + tests **[user, 2026-07-02]**
Split `Playground/` into a **static library `PlaygroundCore`** (all `pg::` code), a thin
**`SparklePlayground`** executable, and a **`PlaygroundTests`** gtest executable linking the
library, so battle/board/feat logic is headless-testable. `spk::` tests remain untouched.
This also eases later promotions of generic classes into Sparkle.

## D18 — Promotion cadence **[user, 2026-07-02]**
First promotion = the 3D layer (Transform3D/Camera3D/mesh render path), **right after
Milestone 1**. Later candidates: voxel mesher primitives, 3D viewport widget, graph-canvas
widget, property editor. Each promotion is its own step and needs sign-off.

## D19 — Stat naming: Stamina + StaminaRate **[arch, 2026-07-02]**
GDD calls the turn-frequency stat **Stamina** ("seconds to fill the Turn Bar", lower = acts
more often). Unity code splits it into `Recovery` (base seconds to fill) and `Stamina`
(a fill-rate multiplier, default 1). The port follows the GDD name for the authored value:
- `stamina` (float, seconds to fill the turn bar; maps Unity `Recovery`; default 4.0)
- `staminaRate` (float multiplier on fill speed; maps Unity `Stamina`/`StaminaRatio`;
  default 1.0; mainly moved by effects/buffs)

Never use "Recovery" in `pg::` names or JSON.

## D20 — Voxel traversal naming: Solid / Passable **[arch, 2026-07-02]**
Unity's `VoxelTraversal.Obstacle` actually means "solid ground you stand **on**" and
`Walkable` means "pass-through (bushes, cross-planes) you walk **through**" — a naming trap.
The port renames the enum to **`Solid`** and **`Passable`** (JSON: `"traversal": "solid" |
"passable"`). A standable cell = a `Solid` voxel with 2 cells of `Passable`-or-empty
head-clearance above it (see [03-systems/board.md](03-systems/board.md)).

## D21 — Feat requirement scope naming: Ability / Turn / Fight / Game **[arch, 2026-07-02]**
Unity's enum is `Action/Turn/Fight/Game`; the authored catalog
(`Architecture/FeatRequirementList.md`) says `Ability`. The port uses **`Ability`** (window =
one ability resolution), matching the catalog and the game's own vocabulary.

## D22 — Space conventions **[arch, 2026-07-02]**
1 voxel = 1.0 world unit; **+Y up**; right-handed; rotations CCW about their axis (the
`spk::Matrix4x4` convention, locked by tests since commit `8a638ba`). Grid coordinates are
`spk::Vector3Int` (cell = integer corner; a unit "stands at" cell center `x+0.5, top, z+0.5`).
Voxel cell orientation = 4 rotations about Y (`PositiveX, PositiveZ, NegativeX, NegativeZ`)
plus a Y-flip (`PositiveY, NegativeY`), mirroring the Unity model exactly.

## D23 — Battle turn semantics (locked from Unity reference, GDD-compatible) **[arch, 2026-07-02]**
- Turn bar: `max = stamina` seconds; fills by `elapsed * staminaRate`; unit is ready at full.
- Ties resolve player-side first, then list order.
- Out of battle-pause, time only advances in simulation jumps: the loop asks "time until next
  ready unit" and advances all bars by exactly that (no real-time drift while UI waits).
- AP/MP refill to max as part of ending a turn (observable effect: every turn starts at full
  pools, per the GDD's non-negotiable rule).
- Stun pauses turn-bar fill; resumes from current value (GDD §6).
- Status hooks fire in status-list order, non-reentrantly; effects applied during a hook do
  not trigger further hooks (mirrors `BattleStatusRules`).

## D24 — Taming reuses FeatRequirement **[arch, 2026-07-02]**
A `TamingProfile` is a list of `FeatRequirement`s evaluated against the same typed
battle-event stream (mirrors Unity). No separate condition hierarchy. Taming replaces capture
everywhere; there is **no CaptureAction** (the "capture mechanic" item in the Unity
`AGENTS.md` critical path is stale — the GDD's Taming section supersedes it).

## D25 — Feat evaluation timing **[arch, 2026-07-02]**
Typed `BattleEvent`s are appended to a per-battle log during resolution. Taming is evaluated
**live** (per event) so an impressed creature leaves mid-fight; feat-board progress is
evaluated **post-battle** from the log. Per the GDD, losing still grants progression — the
port applies feat progress on *both* outcomes (deviation from the current Unity
implementation, which only processes wins; GDD outranks code).

## D26 — M1 creature visuals **[user, 2026-07-02]**
Placeholder cube/voxel models with the fake-animation `Rig` attached from the first render,
so animation recipes work from day one. No existing art to reuse; real models come from the
voxel modeling tool later.

## D27 — Exploration movement is click-to-move **[user, 2026-07-02]**
Since the mouse raycast exists anyway (D14), exploration movement uses it too: **click a
world cell → raycast picks the standable cell → world A* path → a path driver moves the
player cell-to-cell** (emitting `playerMoved` per cell reached, which drives encounter
checks). Keyboard (ZQSD) and mouse-drag/wheel control only the **camera** (orbit/zoom
around the player), not the character. Consequence: the world traversal graph + A* are
built in the exploration step (07), before the battle board step, and the board reuses
them. (The Unity reference happens to use the same click-to-move pattern.)

## D28 — The Unity project is a worked example, not a spec **[user, 2026-07-02]**
The Unity implementation is **one attempt** at the game — consult it when a *behavior* is
ambiguous, but never treat its class design as mandatory. The design authority chain is:
user → GDD → these docs. Where these docs say "reference: Unity X", read that as "a worked
example exists there", not "duplicate it". Design fresh; deviate freely when the fresh
design is better, and record notable deviations here.

## D29 — Chunk dimensions are compile-time constants, not config **[user, 2026-07-02]**
`chunkSize` / world height do **not** live in `game-rules.json`. Chunk extent is a
compile-time constant of the chunk type (`pg::Chunk::Size = {16,16,16}` as
`static constexpr spk::Vector3Int`, or a template parameter if a second size ever appears).
Config JSON is for gameplay tunables only.

## D30 — No collision FaceSet / collision mesh **[user, 2026-07-02]**
All spatial queries are analytic: DDA raycasts over voxel data (mouse picking, LoS,
trainer sight) and grid walk-heights (movement). `VoxelShape` therefore has **no collision
FaceSet** and no `_constructCollisionFaces`; there is no `buildCollisionMesh`.
Reintroduce a collision mesh **only** if a future feature needs sub-voxel surface
precision (e.g. physics debris, light/shadow geometry distinct from render mesh) — at that
point add it as a separate shape product with its own doc section stating the consumer.

## D31 — Voxel masks are the battle-overlay mechanism (M1 scope) **[user, 2026-07-02]**
Every `VoxelShape` defines **mask faces** describing how an overlay decal drapes over it
(flat on cubes, slanted on slopes, stepped on stairs). The battle overlay renders by
composing a **mask mesh** over the highlighted board cells, textured from a **mask sprite
sheet** (`resources/textures/mask.png`; category → atlas cell, e.g. movement, path, ability
range, AoE, deployment, LoS-blocked; Unity's `MaskTexture.png` is an example of the look).
Not flat color tints. Mask meshing ships in the mesher step and the overlay consumes it —
both are **Milestone-1 scope**. The same mask system later serves world decals
(paths/roads).

## D32 — Chunks are synchronizable components **[user, 2026-07-02]**
`pg::Chunk` is itself the `spk::Component` (data package: coordinates + `VoxelGrid` +
baked products: render mesh, mask mesh, traversal-graph slice) and a
`spk::SynchronizableTrait`. `setCell(...)` → `requestSynchronization()`; a
`ChunkSynchronizationLogic` (the "baker") scans for `needsSynchronization()` components and
calls `synchronize()`, whose `_synchronize()` rebuilds the baked products. No separate
"ChunkView" component; edits that touch a chunk border also request synchronization on the
neighbour chunk (occlusion + graph cross borders).

## D33 — Taming conditions and feat requirements share the engine, not the catalog **[user+arch, 2026-07-02]**
One evaluation core — `pg::BattleCondition` (scope window: Ability/Turn/Fight/Game,
`requiredRepeatCount`, typed-event scoring, Advancement) — but **two separate polymorphic
catalogs/factories** built on it: `FeatRequirement` (feat boards) and `TamingCondition`
(taming profiles). Shared behaviors register a parser in both factories; taming-only or
feat-only condition types register in just theirs. This keeps the (valuable) shared
machinery while letting the two vocabularies diverge freely.

## D34 — Feat progress is UUID-keyed, never positional **[user, 2026-07-02]**
`FeatNode`s and each of their requirement entries carry a **UUID** (`spk::UUID` string,
generated by the tools; any unique string accepted in hand-authored seed files). Save data
stores progress keyed by node UUID → requirement UUID — robust against board reordering
and requirement edits (unknown UUIDs in a save are dropped with a warning at load).

## D35 — Form and encounter power are derived from completed feat nodes **[user, 2026-07-02]**
A creature's current form is **derived** by replaying `changeForm` rewards during
`applyProgress` — it is saved nowhere and authored nowhere per-unit. Encounter team members
specify only `species`, `ai`, `completedNodes` (no form field). Authoring constraint
enforced by the feat-board editor: reward replay runs in board node order, so form nodes
must be ordered tier-ascending within the board definition. Corollary **[arch]**: a tamed
recruit inherits the wild spawn's `completedNodes` (completions only, not partial
progress) — otherwise its form would reset to base, which contradicts the Pokémon-style
expectation that a tamed evolved creature stays evolved.

## D36 — Abilities carry target animation + travel VFX **[user, 2026-07-02]**
An ability may name, in addition to `casterAnimation`:
- `targetAnimation` — played on each affected unit; defaults to `TakeDamage`; if the
  target's animation set lacks the named recipe it **falls back to `TakeDamage`**;
- `travelVfx` — an effect in the space between caster and target:
  `kind: "projectile"` (spawned at caster, moves to anchor cell at `speed` cells/s, e.g.
  Ember's fireball) or `kind: "beam"` (stretched between caster and anchor for `duration`
  seconds, e.g. Magic Beam), referencing a `vfx/*.json` definition.
Gameplay resolution stays immediate and headless; animations/VFX are presentation sequenced
on top (cosmetic, never gameplay-blocking).

## D37 — Interiors connect through named portals **[user, 2026-07-02]**
Maps (and the overworld) expose named **portals**; each portal links to
`{targetMap, targetPortal}`. Entering a portal places the player at the target portal's
cell. A tunnel is a map with two portals ("west"/"east") each linked back to a different
overworld door — no "return to entry position" assumption. Interiors are either
**hand-authored maps** (houses, gyms) or **generated maps** (caves via CellularAutomata,
step 31); world generation instantiates generated interiors and binds portal pairs.

## D38 — Elite Four: full heal between gauntlet battles **[arch, 2026-07-02]**
The GDD is silent on healing inside the Elite Four gauntlet. Decision: the team is fully
healed (HP; statuses cleared) between the four consecutive battles — the challenge is four
strong teams in a row, not attrition micromanagement, and turn-bar combat lacks the
consumable economy that makes attrition runs interesting. Losing any battle exits the
gauntlet (respawn flow); re-entry restarts at battle 1.

