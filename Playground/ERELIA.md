
# Erelia → Sparkle Playground — Master Port Document

> **Read this whole file before doing any work on the game port.** It is the single
> centralized knowledge base + roadmap for reimplementing the *Erelia* game inside
> Sparkle. It is deliberately long; it replaces re-reading the whole Unity project
> each session. Verify code claims against current source before asserting them as
> fact (both repos move).

> **⚠️ Handoff in progress (2026-07-01).** A **Fable**-led planning pass is being launched via
> [`FABLE_PROMPT.md`](FABLE_PROMPT.md) to produce the full implementation-documentation set —
> architecture, UML, JSON schemas, a dependency-ordered step plan for **Opus 4.8**, and how-to
> guides — under **`Playground/docs/`**. Once that exists it becomes the **authoritative
> implementation plan** and supersedes the roadmap in §9 and the files in `Playground/steps/`.
> Until then, **this file is the concept + locked-decisions + engine-state reference** that
> Fable should read. Fable is explicitly authorized to rewrite or delete the current Playground
> prototype code (§4) and these planning docs.

---

## 0. The Goal (North Star)

Reimplement the **Erelia** game — currently a Unity/C# prototype — **inside the Sparkle
C++ engine, entirely under `Playground/`**, using Sparkle's currently implemented
features as the starting point.

The game is **not the end product**; it is a **subject / testbed**:
- Use it to stress-test and grow Sparkle's architecture.
- Build new tools and engine features *while* implementing the game.
- **Promote** the good ones back into the `spk::` library once proven in the game.
  (Same pattern already used for the 2D engine step and the 2D tilemap/autotile:
  prototyped in Playground, then lifted into `includes/structures/…`.)

Hard constraints from the user:
- **Everything stays in `Playground/`.** Do not touch `includes/`/`srcs/` (the `spk::`
  library) unless we are deliberately *promoting* a proven feature, and only then with
  the user's go-ahead.
- Start from **current Sparkle capabilities**; add new ones incrementally.
- The **design documents are the spec** — the GDD above all. When code, mockups, and
  GDD disagree, follow the Unity project's own precedence: user clarification → GDD →
  latest wireframe → existing code.

Workflow the user wants: *I write/update this doc → user validates understanding →
then we implement.* Keep this doc current as the plan evolves.

---

## 1. Reference Project (Unity "Erelia")

- **Location:** `C:\Users\User\Documents\UnityProject\Erelia`  (a *read-only reference*;
  we are not shipping the Unity game).
- **It is the "other project"** the user refers to. When a system is ambiguous, open the
  Unity source/docs rather than guessing.

Key docs to consult there (in rough priority):

| File | What it gives you |
|------|-------------------|
| `GDD.md` | **The design bible.** Full game design (see §2). |
| `AGENTS.md` | Terminology map, non-negotiable design rules, current Unity impl state, UI wireframes as ASCII, open questions. |
| `folderArchitecture.md` | Full `Assets/Scripts/` tree (~199 C# files, no namespaces). UTF-16 encoded. |
| `Architecture/proposition.md` | Battle phase-system architecture rationale (orchestrator vs coordinator vs phases). |
| `Architecture/modelAnimationProposition.md` | "Fake animation" rig design (logical-part offset animation; no skeletal rig). |
| `Architecture/FeatRequirementList.md` | Enumerated feat-requirement types (content). |
| `Architecture/Archive/WorldGenerationIdeas.md` | **Two-layer world gen** (macro plan → voxel realization). Directly maps to Sparkle's Perlin/Poisson/Voronoi tools. |
| `Architecture/diagrams/*.puml` (+ rendered `*.png`) | 26 UML class/sequence diagrams of every system. Best fast reference for a system's shape. |
| `Backlog/*.md` + `Backlog/global.md` | Priority-scored epics (200 = mandatory for playable prototype). Good signal for *what mattered to the user*. |
| `UI_Design/*.png` | Wireframes (monochrome, layout-truth not final art): main menu, exploration HUD, battle HUD, creature/ability/passive cards, editor windows. |

Unity project is a **prototype**, scoped to 3 runtime surfaces: **Main Menu**,
**Exploration Screen**, **Battle Screen**. Everything else is deferred there — and that
scoping is a good guide for our Playground port too.

---

## 2. What Erelia Is (game design digest)

**Genre:** single-player, offline **creature-collection RPG** — Pokémon-like collection &
gym progression, FF12/PoE-style board progression, tactical stamina-turn board battles,
in a **procedurally generated voxel overworld**.

**Win condition:** beat **8 gyms in randomized order**, then the **Elite Four**.

**Signature differences vs Pokémon:**
- **No XP levels.** Creatures grow via **Feat progress** (the GDD's old term "Quest
  Validation" — renamed to **Feat**) → unlock nodes on a per-species **Feat Board**
  (abilities, stat bonuses, evolutions). Progress depends on *what you did in battle*, not
  just winning. Losing still grants progression.
- **Stamina-turn combat:** each creature has Stamina/Recovery; a Turn Bar fills over time;
  lower stamina ⇒ acts more often. On a unit's turn, **AP** (ability points) and **MP**
  (movement points) refill to max. Movement and abilities are separate resources.
- **Tactical grid board battles.** Most battles are on a board **derived from the
  surrounding voxel overworld**; special battles (gyms/bosses) use handcrafted boards.
- **Taming instead of capture-by-item:** each wild species has a **Taming Profile** (a list
  of feat-like in-battle conditions). Fulfil them all during the fight → the creature is
  "impressed", leaves the board, and joins you if you win. Defeating it before conditions
  are met = taming fails.
- **Procedural world per run** from a persistent seed: 8 biome-anchor cities, satellite
  towns, roads (Voronoi/MST graph), bridges/ports/tunnels; randomized gym/biome order.

**Core loop:** explore → trigger encounter (wild in bushes / interior / trainer LoS / gym) →
board battle → win/lose → creatures earn **Feat progress** → unlock feats/evolutions →
progress through gyms.

**Creature stats:** Health, Strength (phys dmg), Ability (magic dmg), Armor (phys mit),
Resistance (magic mit), AP, MP, Stamina (turn frequency), Range, Passives.

**Team:** 6 active creatures; overflow to PC storage.

**Battle building blocks:** Abilities cost AP (+maybe MP), have range/LoS/AoE, are
physical or magical; effects include damage, DoT, HoT, heal, shield, buff/debuff, cleanse,
traps, status apply/remove, revive. Enemy AI is a rule list (ordered condition→decision,
first match wins).

*(For anything finer-grained — exact feat requirement types, evolution branching rules,
encounter tiering by badges — go read `GDD.md` §6–§12 and `FeatRequirementList.md`.)*

---

## 3. Unity Reference Architecture (what we're porting *from*)

C# code lives in `Assets/Scripts/`, **no namespaces** (prototype simplification),
organized into domain folders. Patterns worth preserving conceptually:

- **Mode state machine:** `GameBootstrapper → ModeManager → { ExplorationMode, BattleMode }`;
  `Mode` has `Enter()/Exit()`; `GameContext` = root state (`WorldContext` + `PlayerData`).
- **Static rule classes** for battle logic (`BattleTurnRules`, `BattleActionValidator`,
  `BattleActionResolver`, `BattleStatusRules`, `BattleLineOfSightRules`, …) — stateless
  utilities; new rules go *there*, not inline in controllers.
- **Polymorphic ScriptableObjects** as data assets: `Ability`, `CreatureSpecies`, `Status`,
  `BiomeDefinition`, `VoxelDefinition`; abstract bases (`Effect`, `FeatRequirement`,
  `FeatReward`, `VoxelShape`, `AICondition`, `AIDecision`) with many concrete subclasses.
- **Observables** (`ObservableValue<T>`, `ObservableResource`, `ObservableList<T>`) drive UI;
  UI subscribes to `.Changed`, never polls.
- **EventCenter**: static pub-sub for cross-system events (`PlayerMoved`,
  `BattleStartRequested`, `BattleEnded`, …). Systems never hold direct refs across modes.

**Battle** = 7-phase FSM in `BattleOrchestrator`:
`Setup → Placement → Idle → PlayerTurn/EnemyTurn → Resolution → End`. Coordinator decides
transitions; phases are dumb states; input handled per-phase. Feat events recorded during
resolution, evaluated post-battle by `FeatProgressionService`.

### 3a. The Voxel system (highest-value reference — read this closely)

This is the heart of the world and the battle board, and the target of the "3D modeling
tool". Data model (`Assets/Scripts/Voxel/`):

- **`VoxelDefinition`** (SO) = `VoxelData` (traversal + string tags) + a polymorphic
  **`VoxelShape`** (`[SerializeReference]`).
- **`VoxelShape`** (abstract) builds, on `Initialize()`:
  - a **render `FaceSet`** and a **collision `FaceSet`** — each = `InnerFaces` (always drawn)
    + an **`OuterShell`** of 6 axis-plane faces (PosX/NegX/PosY/NegY/PosZ/NegZ) that can be
    **occluded by neighbors**.
  - a **`MaskSet`** (top/bottom mask faces for decal/autotile-style sprite overlays).
  - **`CardinalHeightSet`** per Y-flip: per-edge heights (PosX/NegX/PosZ/NegZ/Stationary)
    used by pathfinding/traversal to know walk heights across slopes/stairs.
  - Faces → `Polygon`s → `Vertex{ Position, UV }`.
  - Concrete shapes: **Cube, Slab, Slope, Stair, CrossPlane** (`Voxel/Shape/*.cs`).
- **`VoxelCell`** = a placed instance: `Id` (index into `VoxelRegistry`),
  `Orientation` (4 rots about Y), `FlipOrientation` (Y up/down). `IsEmpty` when `Id < 0`.
- **`VoxelGrid`** = `VoxelCell[x,y,z]`.
- **`VoxelMesher`** (static) = builds render/collider/mask meshes with **neighbor face
  occlusion culling** (`MapWorldPlaneToLocal` applies cell orientation/flip); mask layers
  stacked with tiny Y offset; sprite-sheet UV rects for masks.
- **Authoring today** = `VoxelDefinitionEditor` (Unity custom inspector): dropdown picks
  the concrete `VoxelShape` subtype via managed-reference, then edits its fields. **This is
  exactly the "3D modeling tool" we'll rebuild in Sparkle** (see §7).

**Board** (`Assets/Scripts/Board/`): `BoardData` wraps `BoardTerrainLayer` (voxels) +
`BoardNavigationLayer` (walkable graph, A* via `BoardPathfinder`) + `BoardRuntimeRegistry`
(unit positions, traps). `BoardDataBuilder` is meant to slice world voxels → a battle board
(pipeline not fully wired in Unity yet).

**World** (`Assets/Scripts/World/`): chunk load/unload around player (`WorldPresenter`),
seeded `MetaWorldGenerator` (biome distribution still stubbed to `defaultBiome`), pathfinding
graph cache. `WorldGenerationIdeas.md` describes the intended **two-layer** approach:
1. **Macro world plan** (once per seed): landmass mask (radial falloff + noise), height &
   rivers, 8 major cities (Poisson-disk placement), biome assignment (shuffle → weighted
   Voronoi), satellite towns, transport graph (MST + extra edges), classify edges
   (road/bridge/port+sea/tunnel), pathfind roads on a terrain cost map.
2. **Voxel realization**: chunks query the macro plan (biome? road? bridge? settlement?
   tunnel? river?) → deterministic local voxel generation.

### 3b. Fake-animation system (for creatures/VFX)

`modelAnimationProposition.md`: no skeletal rig. A **`Rig`** maps **logical parts**
(root/body/head/limbs/weapon/jaw/tail/whole-rig) → transforms + captured rest pose. An
**`Animator`** plays **`Recipe`s** (named, e.g. `Idle`/`TakeDamage`/`Death`) composed of
sequential **`Phase`s** of parallel **`Step`s** (MoveLocal/RotateLocal/Scale/Shake/Flash/
Wait/SpawnVfx/PlaySound). One main channel + one additive overlay. Board tile-to-tile
movement is a *separate* tween, not part of a recipe.

---

## 4. Sparkle — Current Capabilities (what we build *on*)

Repo: `c:\Users\User\source\repos\Sparkle`. C++23, OpenGL. Library under
`includes/structures/…` (`spk::`); game/testbed under `Playground/` (`pg::`).

**Verified present (as of this doc):**

- **Rendering (OpenGL):** programs, buffers (VBO/IBO/UBO/**SSBO**), VAOs, textures,
  samplers, **framebuffer objects**, sprite sheets, fonts, render commands incl.
  **instanced** sprite rendering, viewport, nine-slice, text. `graphics/geometry` has a
  **generic mesh** (`spk_generic_mesh.hpp`, `spk_mesh.hpp`) plus 2D meshes
  (`spk_mesh_2d`, `spk_texture_mesh_2d`, `spk_color_mesh_2d`) and `spk_primitive_object`.
- **Game engine (ECS):** data-`spk::Component` + `spk::ComponentLogic<T>` model. `spk::` ships
  2D components: `Entity2D`, `Transform2D`, `Camera2D`, `SpriteRenderer2D`, `Animation2D`,
  `sprite_render_logic`, `animation_logic`, component store/registry/container. **No 3D
  components in `spk::` yet**, but a **prototype 3D layer now lives in `Playground/` (`pg::`)** —
  `Transform3D`, `Entity3D`, `Camera3D`, `MeshRenderer3D`, `MeshRenderLogic`, `MeshRenderCommand`,
  `makeCube` + mesh shaders (Step 1; see §11). Promotion candidate.
- **(REMOVED) 2D tilemap + autotile** — a `TileMap2D`/`Chunk2D`/`Autotile2D`/
  `ChunkRenderLogic` stack existed but the user **deleted it from the library** (2026-07-01,
  going full-3D). Not present anymore; the autotile *corner-resolver* algorithm knowledge is
  still archived in memory [[tilemap-autotile-2d]] in case 3D face-masking needs it.
- **Math:** `Vector2/3/4`, `Matrix4x4` (`ortho`, `perspective`, `lookAt`, `translation`,
  `rotation` (euler + quaternion), `scale`, `inverse`), `Quaternion`, `Rect2D`, `ApproxValue`,
  and generation primitives: **`Perlin`**, **`PoissonDisk`**, **`CellularAutomata`**,
  **`CurveInterpolator`**. (Rotation builders were fixed 2026-07-01 to a consistent right-hand
  **CCW** convention — see §11; for camera views prefer `Matrix4x4::lookAt` directly.)
- **Widgets / UI:** full widget system ported from V0 (layouts, debug overlay, etc.) — see
  memory [[sparkle-v0-widget-port]], [[widget-resize-by-ratio]], [[ui-showcase-scaffold]].
- **Existing Playground testbed (`pg::`):** a **3D scene** — a rotating **textured cube** viewed
  through a perspective `Camera3D`, with a `DebugOverlay`. Files under `Playground/srcs/`
  (`components/`, `logics/`, `rendering/`, `geometry/`, `game_scene_widget`, `main.cpp`) + mesh
  shaders under `resources/shaders/mesh/`. Build:
  `cmake --build build/playground --target SparklePlayground`; resources via `PG_RESOURCE_DIR`.
  **This is a disposable Step-1 prototype** — Fable may keep, rework, or delete it.

**Notable synergies:** Perlin (terrain/biome noise), PoissonDisk (city placement),
CellularAutomata (caves/tunnels), Voronoi-style partition (biome regions/roads) — the world
generation from `WorldGenerationIdeas.md` maps almost 1:1 onto tools Sparkle already has.

---

## 5. Capability Gap Analysis (Sparkle now → Erelia needs)

| Need | Status in Sparkle | Gap / plan |
|------|-------------------|-----------|
| **3D rendering** (voxel meshes, lit/shaded) | Generic mesh + 3D math in `spk::`; a **prototype 3D layer now exists in `pg::`** (Transform3D/Camera3D/MeshRenderer3D/MeshRenderLogic/MeshRenderCommand/makeCube + mesh shaders — currently **unlit, back-face-cull only, no depth test**). Not yet in `spk::`. | Harden it (real depth buffer, lighting/normals) and promote to `spk::`. First promotion candidate. |
| **Voxel data model + mesher** | None | Port `VoxelDefinition`/`VoxelShape`/`VoxelCell`/`VoxelGrid` + neighbor-occlusion mesher into `pg::`. Reuse generic mesh. |
| **Voxel chunk streaming** | None (2D tilemap was removed) | New 3D `VoxelChunk`/`VoxelWorld`; the old 2D chunk *pattern* (entity-owns-chunk-subentities + templated render logic) is a decent shape to echo. |
| **Grid/tactical board + A\*** | Pathfinding exists only implicitly; no A* utility found yet | Build board model + A* (mirror Unity `BoardPathfinder`). |
| **Data assets / SO-equivalent** | No ScriptableObject analog | **DECIDED: JSON files + loaders + registries** (user wants tools that author creature/ability/voxel JSON). Needed for creatures/abilities/voxels/biomes/feat boards. |
| **Observable/event plumbing** | Sparkle has signals/design-patterns (verify) | Provide an `EventCenter`-like bus + observable values for UI binding. |
| **Turn/phase FSM** | None game-specific | Port battle phase machine in `pg::`. |
| **Fake-animation rig** | `Animation2D` exists (sprite frames) | New logical-part transform-offset animator (2D first, 3D later). |
| **World-gen orchestration** | Primitives present (Perlin/Poisson/CA) | Build macro-plan generator using them. |
| **Save/Load** | Verify IO utils in `includes/utils` | Serialize `GameSaveData` equivalent (seed, team, feats, cleared gyms). |
| **Tools/editors** (voxel modeler, feat board, encounter, team) | Widget UI exists; no tools yet | Build as Playground apps/widgets; promote reusable tool framework later. |

---

## 6. Concept Mapping (Unity → Sparkle Playground)

- `ScriptableObject` data asset → a `pg::` definition type loaded from a **JSON** file
  into a registry (decided, §8.2).
- `MonoBehaviour` → `spk::Component` (data) + `spk::ComponentLogic<T>` (behavior).
- `[SerializeReference]` polymorphic base (`VoxelShape`, `Effect`, `FeatRequirement`) →
  C++ polymorphic class hierarchy + a type-tagged (de)serializer / factory.
- `EventCenter` (static pub-sub) → a `pg::EventCenter` (verify if `spk::` already has a
  signal/observer to reuse under design_pattern).
- `ObservableValue<T>`/`ObservableResource` → same idea; back UI widgets.
- Unity Editor windows (`VoxelDefinitionEditor`, `FeatBoardEditorWindow`,
  `EncounterTeamEditorWindow`, `PlayerTeamEditorWindow`) → **Sparkle widget-based tool
  apps** (like `UIShowcase`), the "tools we'll promote to Sparkle".
- Unity `VoxelMesher` → `pg::VoxelMesher` emitting into Sparkle's generic mesh + a 3D
  instanced/batched draw path.

---

## 7. Tools to Build (and later promote to `spk::`)

The user explicitly wants tools built along the way. Anchor tool = **the 3D voxel modeling
tool** (the reason the Unity project was cited). Likely tool set:

1. **Voxel Definition Editor / 3D modeler** — author `VoxelShape`s (cube/slab/slope/stair/
   cross-plane + custom), preview mesh, edit faces/UVs/cardinal heights/masks. Sparkle
   analog of `VoxelDefinitionEditor`. Reusable pieces (3D viewport widget, mesh preview,
   managed-reference-style polymorphic property editor) → promote to `spk::`.
2. **World / structure editor** — place voxels & prefab "structures" (dungeon rooms, ponds,
   buildings) into chunks; author reusable cells.
3. **Feat Board editor** — adjacency-locked node graph with requirements/rewards (Unity
   `FeatBoardEditorWindow` is the reference; a graph-canvas widget is promotable).
4. **Encounter / Team editors** — 6-slot team, encounter tiers, biome tables.
5. **Ability / Status authoring** — polymorphic effect composition + generated rules text.

General rule: build the tool in Playground; when a *widget/framework* piece proves itself,
lift it into `spk::` (graph canvas, 3D preview viewport, property editor, gizmos, …).

---

## 8. Decisions

**Resolved (2026-07-01):**
1. **Full 3D from the start.** Not 2D-first. The user removed the 2D tilemap from the
   library because it is no longer needed. The minimal 3D layer + voxel world is built early
   (see roadmap). This is the flagship new-feature stream and the first promotion candidate.
2. **Data format = JSON files + loaders + registries.** The user intends to build tools that
   author creature/ability/voxel JSON, so definitions are data-driven from day one.
3. **Doc location** — user is indifferent; keeping it in the memory folder + `MEMORY.md`
   pointer for now. May later also drop a copy at `Playground/ERELIA.md` for repo tracking.

**Resolved (2026-07-01, cont.):**
4. **Runtime structure = Plan Gamma (fully seamless).** The user's mental model: the **battle
   board is just an overlay on top of the world voxels** — no separate battle scene, no
   snapshot, no suspend/restore. One persistent live 3D voxel world; a battle is an in-place
   grid overlay + turn control-state on that same world. See §8b (Gamma marked CHOSEN).
5. **Milestone 1 = no entry menu.** Boot **directly into world exploration**; be able to
   **enter fights from there**. Main menu is deferred to later polish. See §9 "Milestone 1".

---

## 8b. Runtime Structure — Plans (Main Menu / Exploration / Battle)

**DECIDED: Plan Gamma (see below).** This section keeps all three options for context/
rationale, but Gamma is the chosen structure. The GDD constraints that drove it:
- "Most battles use a board **derived from the world around the player**."
- "Battles occur **where the player is standing** for immersion."
- Buildings/gyms/caves **teleport to interior spaces** ⇒ separate loaded "places" exist
  regardless of the battle choice.
- Special battles (gyms/bosses) use **handcrafted** boards.

Two independent questions: (a) how the **menu** relates to the world, and (b) how
**exploration ↔ battle** transition. Three plans were considered:

### Plan Alpha — "Classic modes" (simplest, most decoupled)
- **Menu:** its own screen (static art or a simple 3D vista); world loads only on New/Load.
- **Exploration:** streamed voxel world; free movement.
- **Battle:** on trigger, **cut** to a battle scene built from a *handcrafted or generic*
  board; win/lose → return to exploration where you left.
- **Interiors:** separate loaded voxel maps.
- **Pros:** fastest, each mode testable in isolation, mirrors Unity `ModeManager`, low risk.
- **Cons:** battle can feel visually disconnected from the exact spot in the world.

### Plan Beta — "In-place snapshot battle" (considered; NOT chosen — fallback option)
- **Menu:** overlaid on a live 3D scene (shows the engine off; answers AGENTS.md's open menu
  question toward "rendered 3D scene").
- **Exploration:** streamed voxel world.
- **Battle:** on trigger, **snapshot the voxel region around the player** into a
  self-contained `BoardData` (terrain slice + navigation graph + runtime registry); **suspend**
  exploration (don't destroy it); build a battle instance **visually reconstructed from that
  slice** so it reads as the same place; **blend** the camera from exploration framing to a
  battle framing; run the turn FSM; on end, restore exploration exactly.
- **Interiors & battle boards share one mechanism:** "load/enter a discrete voxel space",
  so `BoardDataBuilder` and interior loading reuse the same code.
- **Pros:** matches the GDD's "battle where you stand"; reuses the voxel renderer; still
  isolated and unit-testable via the `BoardData` boundary; makes the world's 3D value pay off.
- **Cons:** more work than Alpha; deriving a good board from arbitrary streamed terrain
  (flattening, edges, LoS on real geometry, camera framing on slopes) needs care.
- **De-risking:** build `BattleMode` against a `BoardData` *interface* and feed it a
  **hardcoded board first** (Alpha-style), then later swap in the world snapshot. Battle
  systems get built before world-gen/`BoardDataBuilder` is ready. This is why the roadmap
  can build battle early even though full snapshotting comes later.

### Plan Gamma — "Fully seamless" ✅ CHOSEN (2026-07-01)
The user's model: **the battle board is an overlay on the world voxels.** No scene swap, no
snapshot, no suspend/restore. Concretely:
- One **persistent live 3D voxel world**, always loaded and rendered.
- On encounter: freeze free-roam movement, switch to a **battle control-state**, **overlay a
  logical grid** on the world voxels around the player (highlights / grid lines / deployment
  zones drawn on top of the existing terrain), reframe the camera to a tactical angle
  (no cut), and run the turn FSM directly on that region. World entities coexist.
- On battle end: drop the overlay, restore free-roam control + camera. Nothing to reload.

**Implications / architecture under Gamma:**
- "Modes" are **control-states over one shared world/engine**, not separate scenes:
  `ExplorationMode` and `BattleMode` swap input handling + HUD + camera behavior + overlay,
  but the voxel world entity and its renderer persist across both.
- The **board is derived live** from the world: pick an origin (player's cell) and extent
  (from encounter data / GDD board size); for each grid cell compute walkability + walk height
  from the topmost walkable voxel + its `CardinalHeightSet`; LoS via voxel raycasting; range
  on the grid. This derivation replaces Beta's `BoardDataBuilder` snapshot.
- **Keep a logical `BoardData`/grid seam anyway** (a plain data structure: cells, nav graph,
  runtime registry of unit positions/traps/turn-bars) so battle *logic* stays headless-
  unit-testable — feed it a synthetic grid in tests, and the live-world-derived grid at
  runtime. The difference from Beta is purely: no separate rendered scene, world stays live.

**Challenges to watch (flagged, not blockers):** bounding a clean grid over uneven streamed
terrain; camera framing on slopes; freezing/deferring other world actors + exploration
triggers during battle; making sure battle runtime state references world cells without
mutating world data. All manageable; just build the `BoardData` seam early.

### Plans Alpha / Beta (NOT chosen — kept for context)
Alpha = cut to a generic/handcrafted battle scene; Beta = snapshot the region into a separate
reconstructed battle instance. Both were rejected in favor of the in-place overlay (Gamma).
If Gamma's live-derivation ever proves too costly, Beta (snapshot) is the natural fallback and
shares the same `BoardData` seam, so a pivot would not throw away the battle backend.

### Sparkle architecture to realize Gamma
- `pg::GameContext` = root state (`WorldContext` + `PlayerData`), + a `pg::EventCenter` bus.
- **One persistent world/engine + renderer.** `pg::ModeManager` swaps a `pg::Mode`
  (`Enter/Exit/Update`) = `ExplorationMode` / `BattleMode` (later `MainMenuMode`), but under
  Gamma a mode is a **control-state**, not a scene: it swaps input handling + HUD widget +
  camera behavior + battle overlay, while the voxel world entity and its `Camera3D`-driven
  renderer **persist across the switch**. (Interiors = `ExplorationMode` pointed at another
  loaded voxel map — the one place a genuine world reload happens.)
- The **battle grid overlay** is drawn by the same voxel-world renderer path (cell highlights /
  grid / deployment zones / range / LoS / AoE rendered on top of the live terrain).
- `BoardData` is a **logical seam** (cells + nav graph + runtime registry), not a scene:
  populated **live** from the world voxels at runtime, and from a **synthetic grid** in tests.
  Build the battle backend against it so it stays headless-testable.

---

## 9. Phased Roadmap (3D-first)

Reflects the decisions in §8: full 3D from start + JSON + Plan Gamma. **Milestone 1** is the
thinnest end-to-end slice.

> **Concrete execution steps** (class/file-level designs, definition-of-done, tests) live in
> [`Playground/steps/`](steps/README.md). Start with [`steps/step1.md`](steps/step1.md) — the
> 3D engine layer. Steps map 1:1 onto the phases below.

- **Phase 0 — Foundations.** `pg::EventCenter`, observable value/resource types,
  `pg::GameContext`, `pg::ModeManager`/`Mode` (Enter/Exit/Update). Under **Gamma**, modes are
  **control-states over ONE persistent world/engine** (swap input + HUD + camera + overlay;
  the voxel world + renderer persist across modes) — not separate scenes. JSON loading utility
  + a definition-registry pattern (verify a JSON lib in `vcpkg.json`/`includes/utils` first).
- **Phase 1 — 3D engine layer (flagship, promotion candidate).** `Transform3D`, `Camera3D`
  (perspective + orbit/follow), a mesh-render `ComponentLogic`, and a basic lit/textured
  shader. Render a spinning textured cube in Playground to prove the path. This is the piece
  most likely lifted into `spk::` once solid.
- **Phase 2 — Voxel core + first tool.** Port `VoxelDefinition`/`VoxelShape`(cube/slab/slope/
  stair/cross-plane)/`VoxelCell`/`VoxelGrid` + neighbor-occlusion `VoxelMesher` (→ generic
  mesh). Load voxel defs from JSON. Render a hand-authored voxel chunk. Begin the **Voxel
  Definition / 3D modeling tool** (author shapes, preview mesh).
- **Phase 3 — Exploration slice.** `VoxelChunk`/`VoxelWorld` streaming; a player entity moving
  on the voxel terrain with `Camera3D` follow; encounter triggers (bush/trainer-LoS/interior)
  firing events on the bus. World is hand-authored (no procedural gen yet).
- **Phase 4 — Battle core (backend-first, against a logical board).** A **logical `BoardData`
  seam** (cells + nav graph + runtime registry) — fed a **synthetic grid** in tests, and the
  **live world-derived grid** at runtime (Gamma). Board model + A* (`BoardPathfinder`);
  turn-bar FSM `Setup→Placement→Idle→PlayerTurn/EnemyTurn→Resolution→End` (orchestrator = dumb
  state machine, coordinator = rules, per-phase input — per Unity `proposition.md`);
  `Ability`/`Effect`/`Status` polymorphic types (JSON-authored); `BattleUnit`, AP/MP refill,
  damage/heal/shield/DoT/HoT, LoS (voxel raycast)/range/targeting rules. Headless-testable in
  `tests/` (mirror Unity `BattleBackendTestRunner`).
- **Phase 5 — Battle presentation + HUD (in-place overlay).** Derive the battle grid live from
  the world voxels around the player; **render it as an overlay** on the existing voxel world
  (cell highlights, grid, deployment zones, path/range/LoS/AoE preview); **reframe the camera**
  to a tactical angle (blend, no cut); place unit views on world cells with a tile-to-tile move
  tween. HUD: team columns (6 slots), ability bar (8 slots, AP/MP), active-unit HUD, end turn.
  Widget-based, observable-bound. On end, drop overlay + restore free-roam.
  **→ Milestone 1 target (no menu): boot directly into a hand-authored voxel world → walk
  around → trigger a scripted fight (in-place grid overlay) → resolve → return to free-roam.**
- **Phase 6 — Creatures & Feats.** `CreatureSpecies`/`CreatureUnit`, `FeatBoard`/`FeatNode`
  (adjacency-locked), `FeatRequirement`/`FeatReward`, `FeatProgressionService`; record feat
  events in battle, evaluate post-battle. Author ≥1 real creature in JSON. **Feat Board tool.**
- **Phase 7 — Taming + encounters + full live board derivation (Gamma).** Taming profiles/
  conditions; `EncounterTable`/`EncounterTier` (biome-weighted rolls); generalize the Phase-5
  live grid derivation to arbitrary streamed terrain + encounter-defined board size;
  **Encounter/Team tools.**
- **Phase 8 — World generation.** Two-layer generator: macro plan (landmass falloff+Perlin,
  Poisson-disk cities, shuffled→Voronoi biomes, MST+extra transport graph, road pathfinding,
  bridge/port/tunnel classification via `CellularAutomata` for caves) → deterministic voxel
  realization per chunk. Seeded & reproducible.

<!-- status: see §11 for build progress. -->

- **Phase 9 — Meta & polish.** Save/Load (seed, team, feats, cleared gyms, PC storage), gym
  progression & tier scaling, **main menu (deferred from Milestone 1)**, fake-animation rig
  (logical-part offsets), audio, VFX, battle result & feat-summary screens.
- **Continuous:** promote proven widgets/engine features into `spk::` (3D layer, viewport
  widget, graph canvas, property editor, gizmos); keep `tests/` green; keep this doc updated.

---

## 10. Working Conventions (for this port)

- Namespace **`pg::`** for all Playground game code (not `spk::`) until a piece is promoted.
- **Everything under `Playground/`.** Touch `spk::` library only for deliberate promotions,
  with user sign-off.
- Mirror Sparkle's existing idioms: data `Component` + `ComponentLogic<T>`; GPU-instanced
  rendering; resources via `PG_RESOURCE_DIR`; header-only components/logics where practical.
- Preserve Erelia's **non-negotiable design rules** (from `AGENTS.md`): no XP levels;
  battle-earned feat progression; 6-slot team; tactical grid battles with stamina turn bar;
  AP/MP refill at turn start; movement ≠ ability resource; procedural seeded overworld;
  badge/tier gating not level gating; losing still gives progression.
- Keep terminology aligned with the GDD/code: `CreatureSpecies`, `CreatureUnit`,
  `FeatBoard`, `FeatNode`, `EncounterTable`, `EncounterTier`, `Ability`(=Action UI),
  `AP`(=PA), `MP`(=PM).
- **"Quest" is renamed to "Feat"** (user decision, 2026-07-01). Use `Feat` /
  **`FeatProgress`** / **`FeatReward`** / `FeatRequirement` / `FeatBoard` / `FeatNode`. The
  GDD's "Quest Validation" == a creature earning **Feat progress**. **Never use "Quest"** in
  our port's names, types, or JSON. (The Unity code already uses `Feat*`; only the GDD prose
  still says "Quest Validation" — see the note below about sweeping the source docs.)
- Tests: match the project's habit — add `tests/` coverage for backend/logic, hand-validate
  visuals (see memory [[sparkle-visual-test-workflow]]).

---

## 11. Status Log (update me each session)

- **2026-07-01** — Doc created. Read the full Unity Erelia project (GDD, AGENTS,
  architecture, voxel/world/board source, backlog) and surveyed Sparkle's current
  capabilities. No game code written yet in Playground.
- **2026-07-01 (update)** — User decisions: **full 3D from start** (2D tilemap removed from
  the library — confirmed gone; engine is 2D-base only, no 3D components yet), **JSON** data
  format. Doc updated: §8 decisions, new §8b runtime-structure plans, §9 rewritten 3D-first.
- **2026-07-01 (update 2)** — User chose **Plan Gamma (fully seamless)**: battle board is an
  **overlay on the world voxels**, no scene swap/snapshot. **Milestone 1 = no menu**, boot
  straight into voxel-world exploration and enter fights in place. Doc updated: §8.4/§8.5
  resolved, §8b Gamma marked CHOSEN + implications/challenges + kept logical `BoardData` seam
  for testability, §9 Phases 0/4/5/7/9 + Milestone-1 rewritten.
- **2026-07-01 (update 3)** — Renamed **"Quest" → "Feat"** everywhere (concept = **Feat
  Progress**): swept GDD.md + AGENTS.md in the Unity reference and this doc; recorded the rule
  in §10. Skipped `Request*`/"Questions" false positives.
- **2026-07-01 (update 4)** — Grounded the plan in Sparkle's real APIs and wrote concrete
  execution steps in **`Playground/steps/`** (`README.md` index + detailed `step1.md`,
  moderate `step2.md`, outline `step3.md`). Key findings: `Matrix4x4` already has
  `perspective`/`lookAt`; `spk::TextureMesh2D` already stores `Vector3` position+UV (reusable
  as the Step-1 3D mesh); `SpriteRenderLogic` + `CameraUpdateRenderCommand` +
  `InstancedSpriteRenderCommand` are exact templates → **Step 1 (3D layer) needs no `spk::`
  change.** Flagged the **depth-buffer question** as the main Step-1/2 risk (convex cube works
  with back-face culling alone; real depth needed by Step 2). **Next: on go-ahead, implement
  `steps/step1.md`. Still nothing built in Playground.**
- **2026-07-01 (update 5)** — **Step 1 implemented** (the 3D engine layer). Added `pg::`
  `Transform3D`, `Entity3D`, `Camera3D`, `MeshRenderer3D`, `MeshRenderLogic`,
  `MeshRenderCommand`, `makeCube`, mesh shaders; rewired `game_scene_widget` + `main` to a
  rotating textured cube through a perspective `Camera3D`. **No `spk::` change** (reused
  `TextureMesh2D`, `CameraUpdateRenderCommand`). Builds clean via the `playground` preset; a
  5 s smoke run initialized + rendered with no crash/stderr. Depth handled by back-face
  culling only (convex cube); real depth test deferred to Step 2. Tests deferred until the 3D
  layer is promoted to `spk::` (pg:: isn't linked into `tests/`). **Awaiting user's visual
  confirmation of the cube; then Step 2 (voxel core + mesher). Changes uncommitted.**
- **2026-07-01 (update 6)** — Step 1 **visually verified** (screenshot: textured cube renders in
  perspective; 1 mesh / 12 tris). Final Step-1 design after user review: `MeshRenderCommand`
  takes the model matrix in its **constructor** (write-once, no setter); the **camera
  view-projection is emitted in the render process** each frame (multi-camera-ready, e.g. a
  minimap); `Camera3D` uses `Matrix4x4::lookAt` directly. **`spk::` library fix (user-authorized):**
  `Matrix4x4::rotation(Quaternion)` and `rotateAroundAxis` were transposed (clockwise) versus
  `rotation(euler)` (CCW); all rotation builders now use the right-hand **CCW** convention, with
  new `tests/.../matrix_test.cpp` cases locking cross-representation agreement. **Handoff:** wrote
  [`FABLE_PROMPT.md`](FABLE_PROMPT.md) so **Fable** produces the full implementation-documentation
  set under `Playground/docs/` (which will supersede §9 and `steps/`). Everything still
  uncommitted.
