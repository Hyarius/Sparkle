# PROMPT FOR FABLE — Plan the full implementation documentation for "Erelia" in the Sparkle engine

You are **Fable** (`claude-fable-5`), acting as a **lead software architect + technical writer**.

Your job in this task is **NOT to write game code**. Your job is to produce a **complete,
unambiguous, implementation-ready documentation set** for building the game **Erelia** inside
the **Sparkle** C++ engine, so that a *later* agent — **Claude Opus 4.8** — can implement the
whole game step by step with as few open design decisions as possible.

Think of yourself as the person who turns a game design + an engine into a build manual:
architecture, UML, data schemas, a dependency-ordered step plan, and how-to guides. Opus 4.8
will execute your plan; the quality of your documentation determines whether that succeeds.

---

## 0. How to work (process — follow in order)

1. **Read first, plan second.** Before writing any documentation, read the sources listed in
   §2. Do not rely on memory or assumptions about the engine — **read the actual code**.
2. **Verify the engine.** Every Sparkle (`spk::`) API you reference in your docs must be
   checked against the real source. **Never invent an API.** Where the engine lacks something
   the game needs, say so explicitly and decide whether it should be new `pg::` (Playground)
   code or a deliberate promotion into the `spk::` library.
3. **Produce an "Understanding & Gaps" report and ASK questions.** After reading, write a
   concise summary of what you understood and a **numbered list of clarifying questions** for
   the user about anything missing, ambiguous, or under-specified (see §6). **Then stop and
   wait for the user's answers.** Do not guess on game-design or scope decisions.
4. **Only after the user answers**, produce the full documentation set (§4), in the layout of
   §5, at the quality bar of §3.
5. Keep a **decision log** and an **index** so the documentation is navigable and its
   rationale is traceable.

You are explicitly allowed — encouraged, even — to **delete or rewrite anything currently in
the `Playground/` folder** (prototype code and existing planning docs alike). Treat the
current Playground contents as disposable scaffolding. Keep only what you deliberately decide
is worth keeping, and say what you removed and why.

---

## 1. The game — Erelia (concept)

Erelia is a **single-player, offline creature-collection tactical RPG** set in a
**procedurally generated voxel world**. It blends:
- Pokémon-style **collection + gym progression**,
- **FF12 / PoE-style board-based progression** (a per-species **Feat Board**),
- **tactical, grid-based, turn-based battles** driven by a **stamina / turn-bar** system.

**Win condition:** defeat **8 gyms in a randomized order**, then the **Elite Four**.

**Signature differences from Pokémon:**
- **No experience levels.** Creatures grow via **Feat Progress** (this concept was historically
  called "Quest Validation" in older text — the correct term everywhere is **Feat**; use
  `Feat` / `FeatProgress` / `FeatReward` / `FeatRequirement` / `FeatBoard` / `FeatNode`, and
  **never "Quest"**). Progress depends on *what a creature did in battle*, not merely on
  winning. Losing a battle still grants progression.
- **Stamina turn-bar combat.** Each creature has a Stamina/Recovery stat; a turn bar fills over
  time; lower stamina ⇒ acts more often. On a unit's turn its **AP** (ability points) and
  **MP** (movement points) refill to max. Movement and abilities are **separate** resources.
- **Battles are tactical grid battles derived from the surrounding voxel world.** Special
  battles (gyms/bosses) can use handcrafted boards.
- **Taming instead of capture items.** Each wild species has a **Taming Profile**: a set of
  feat-like in-battle conditions. Fulfil them all during the fight and the creature is
  "impressed", leaves the board, and joins the team if you win. Defeat it first and taming
  fails.
- **Procedural world per run** from a persistent seed: ~8 biome-anchor cities, satellite towns,
  roads (Voronoi/MST graph), bridges/ports/tunnels; randomized gym/biome order.

**Core loop:** explore → trigger an encounter (wild in bushes / interior / trainer line-of-sight
/ gym) → tactical board battle → win or lose → creatures earn **Feat Progress** → unlock
feats / evolutions → progress through gyms.

The **authoritative game spec is `GDD.md`** in the Unity reference project (§2). When code,
mockups, and the GDD disagree, resolve in this precedence order:
**user clarification → `GDD.md` → latest UI wireframe → existing code.**

---

## 2. Sources you must read (in priority order)

You are running inside the **Sparkle repository**: `c:\Users\User\source\repos\Sparkle`.

### 2a. The game design + reference implementation (a Unity/C# project — READ-ONLY reference)
Located at `C:\Users\User\Documents\UnityProject\Erelia`. We are **not shipping** the Unity
game; it is the design source and a worked example of the systems. Read, in priority:

| File | Why |
|------|-----|
| `GDD.md` | **The design bible.** Full game design. |
| `AGENTS.md` | Terminology map, non-negotiable design rules, current Unity implementation state, ASCII UI wireframes, open questions. |
| `folderArchitecture.md` | The full `Assets/Scripts/` tree (~199 C# files). (UTF-16 encoded.) |
| `Architecture/proposition.md` | Battle phase-system architecture rationale. |
| `Architecture/modelAnimationProposition.md` | "Fake animation" rig design (logical-part offset animation; no skeletal rig). |
| `Architecture/Archive/WorldGenerationIdeas.md` | Two-layer world generation (macro plan → voxel realization). |
| `Architecture/diagrams/*.puml` (+ rendered `*.png`) | 26 UML class/sequence diagrams of every system — the single best fast reference for each system's shape, and a model for the diagrams you will produce. |
| `Backlog/*.md` + `Backlog/global.md` | Priority-scored epics (200 = mandatory for a playable prototype) — signal for what matters. |
| `UI_Design/*.png` | Wireframes (monochrome layout specs, not final art): main menu, exploration HUD, battle HUD, creature/ability/passive cards, editor windows. |
| `Assets/Scripts/**/*.cs` | The reference implementation of every system. Especially the `Voxel/`, `World/`, `Board/`, `Battle/`, `Creatures/`, `Feats/`, `Encounters/`, `AI/` folders. |

### 2b. Existing planning for the Sparkle port (in this repo)
- `Playground/ERELIA.md` — the current master port document: goal, decisions already made,
  Sparkle capability gap analysis, runtime-structure decision, phased roadmap. **Read it fully.**
- `Playground/steps/README.md`, `Playground/steps/step1.md`, `step2.md`, `step3.md` — the
  current (partial) step breakdown. You may supersede these.

### 2c. The engine itself (Sparkle) — verify capabilities by reading code
- `includes/structures/**` and `srcs/structures/**` — the `spk::` library (namespace `spk::`).
- `Playground/srcs/**` — the current Playground app (namespace `pg::`), including a **prototype
  3D layer** you may keep, rework, or delete.
- `README.md`, `CMakePresets.json`, `Playground/CMakeLists.txt`, `vcpkg.json` — build system.
- `tests/**` — the test suite and its conventions.

---

## 3. Decisions already locked (do not re-litigate; build on them)

These were decided with the user. Treat them as constraints unless your reading reveals a hard
technical problem, in which case raise it as a question (§6) rather than silently diverging.

1. **Full 3D from the start.** The world is 3D voxel; there is no 2D fallback. (A previous 2D
   tilemap experiment was removed from the library.)
2. **Data-driven via JSON.** Creatures, abilities, statuses, voxels, biomes, feat boards,
   encounters, etc. are authored as JSON and loaded into registries. The user intends to build
   **tools** that generate/edit this JSON.
3. **Runtime structure = "Plan Gamma" (fully seamless).** The **battle board is an overlay on
   the live world voxels** — no separate battle scene, no snapshot/reload. One persistent 3D
   voxel world is always loaded. On an encounter: freeze free-roam, overlay a logical grid on
   the terrain around the player, reframe the camera, and run the turn system in place; on
   battle end, drop the overlay and restore free-roam. Consequently, **"modes" (Exploration,
   Battle, later Menu) are control-states over one persistent world/engine**, not separate
   scenes. Keep a **logical `BoardData` seam** (cells + navigation graph + runtime registry)
   so battle *logic* stays headless/unit-testable, populated live from world voxels at runtime
   and from synthetic grids in tests.
4. **Multi-camera aware rendering.** The camera's view-projection is uploaded **as part of the
   render process** (a render command in the stream), so multiple cameras (e.g. a minimap) can
   each drive their own pass. Do **not** assume a single global camera.
5. **Milestone 1 = no main menu.** Boot directly into voxel-world exploration and be able to
   enter fights from there. The main menu is deferred to later polish.
6. **Everything the game needs lives under `Playground/`** in namespace **`pg::`**. The `spk::`
   library is only modified for **deliberate promotions** (a `pg::` feature proven in the game,
   lifted into `spk::`), which must be explicitly called out. The user does authorize such
   promotions when justified.
7. **Terminology:** `Feat` not `Quest` (see §1); keep names aligned with GDD/code
   (`CreatureSpecies`, `CreatureUnit`, `FeatBoard`, `FeatNode`, `EncounterTable`,
   `EncounterTier`, `Ability`, `AP`, `MP`).

---

## 4. What to produce (deliverables)

A cohesive documentation set (Markdown + PlantUML) that fully specifies the build. It must
include all of the following:

1. **Overview / vision doc** — what Erelia is, the pillars, the target Milestone-1 slice, and
   how the docs are organized (an index).
2. **Architecture document** — the runtime architecture in Sparkle terms: the persistent
   world/engine, the mode/control-state manager, the event bus, the ECS usage
   (`spk::Component` data + `spk::ComponentLogic<T>`), the rendering pipeline (render commands,
   cameras), the data/registry layer, and how systems communicate. Map every major Unity
   concept to its Sparkle counterpart, and flag every capability the engine is missing.
3. **Data model + JSON schemas** — concrete JSON schemas (with examples) for every authored
   definition type: voxel definitions & shapes, creature species & forms, abilities & effects,
   statuses, biomes, feat boards (nodes/requirements/rewards), encounter tables/tiers, taming
   profiles, save data. Define how polymorphic types (e.g. voxel shapes, effects, feat
   requirements) are represented and deserialized.
4. **Per-system design docs** — one document per system, each with: responsibilities, key
   types, data flow, dependencies, open questions, and the relevant JSON. Systems at minimum:
   Voxel core & mesher, World (chunks/streaming + procedural generation), Board & pathfinding,
   Battle (turn-bar FSM, actions, effects, statuses, targeting/LoS/range, AI), Creatures &
   Feats, Encounters & Taming, Rendering/3D layer & cameras, UI/HUD, Save/Load & meta
   progression, Fake-animation, Audio, and the Tooling suite.
5. **UML diagrams (PlantUML `.puml`)** — for each major system: a **class diagram** and, where
   behavior matters, a **sequence diagram** (e.g. bootstrap, exploration movement, encounter →
   battle overlay, battle action resolution, feat progression, save). Mirror the granularity of
   the Unity `Architecture/diagrams/` set. Keep diagrams in one folder with an index.
6. **Implementation plan for Opus 4.8** — the centerpiece. A dependency-ordered breakdown of
   **phases → steps → tasks**. See §3 quality bar below. Include the flagship 3D/voxel risk
   early, the tool-building track, and the promotion track. Define **Milestone 1** precisely.
7. **How-to guides** — short, concrete recipes for the recurring operations Opus 4.8 will
   perform, e.g.: "add a data `Component` + its `ComponentLogic`", "add a new `RenderCommand`
   and shader", "author and load a new JSON definition type", "build & run the Playground",
   "add a `tests/` unit test and what to test vs. hand-validate visually", "build a new
   editor/tool as a widget app", "promote a `pg::` feature into `spk::`". Ground each in the
   real Sparkle APIs.
8. **Glossary** — every game and engine term, its meaning, and its canonical spelling.
9. **Decision log** — the decisions in §3 plus any you make or the user makes, with rationale.

---

## 5. Output location, structure, and format

Create the documentation under **`Playground/docs/`** (create it). Suggested layout — adapt if
you have a better structure, but keep it indexed:

```
Playground/docs/
  README.md                     # index + how to navigate; links everything
  00-overview.md
  01-architecture.md
  02-data-model.md              # JSON schemas + examples (or split under schemas/)
  03-systems/
    voxel.md  world.md  board.md  battle.md  creatures-feats.md
    encounters-taming.md  rendering-cameras.md  ui-hud.md  save-meta.md
    animation.md  audio.md  tooling.md
  diagrams/
    README.md                   # diagram index
    *.puml                      # class + sequence diagrams
  plan/
    implementation-plan.md      # phases → steps overview
    steps/step-NN-*.md          # one file per step (self-contained)
  howto/
    *.md
  glossary.md
  decision-log.md
```

- Prose in **Markdown**; diagrams in **PlantUML** (`.puml`) with a one-line note on how to
  render them.
- Use clickable relative links between docs.
- Do not leave TODOs that hide unresolved design decisions — either resolve them, or turn them
  into explicit questions for the user (§6) / entries in the decision log.

---

## 3-bis. Quality bar for the implementation plan (critical)

The plan is the product Opus 4.8 will actually run against. It must be executable by a **fresh
Opus 4.8 agent that has only your docs** (no memory of this conversation). Therefore each
**step** must be:

- **Self-contained**: state its goal, preconditions/dependencies (which prior steps), the exact
  files to create/modify, the `pg::` types and their responsibilities, and the JSON involved.
- **Sized for a single focused implementation session** — not too large to finish, not so tiny
  it's noise.
- **Verifiable**: a clear **Definition of Done**, the **tests** to add (and what must instead be
  **hand-validated visually**, per the project's visual-test convention), and **how to build &
  run** to confirm it.
- **Dependency-ordered**: earlier steps unblock later ones. Front-load the highest-risk,
  highest-leverage work (the minimal **3D render layer** and the **voxel core + mesher**),
  because the whole game renders on it.
- **Faithful to §3 decisions** and to `GDD.md`.
- Cross-referenced to the relevant system doc, diagrams, and how-to guides.

Also include, in the plan overview: the **critical path**, what **Milestone 1** contains
(exploration in a hand-authored voxel world → scripted in-place battle → resolve → return, no
menu), and where the **tool-building** and **`spk::` promotion** work slots in.

---

## 6. Ask the user (clarification phase — do this before the big write-up)

After reading, ask the user about everything you need and don't have. Be specific and batch
your questions. Likely areas (not exhaustive — add your own):
- **Scope & priorities:** exact contents of Milestone 1; which systems are in vs. out for a
  first playable; how far to plan beyond Milestone 1.
- **Existing Playground code:** confirm you may delete/rewrite the current prototype 3D layer
  and the current `steps/` docs, or whether to preserve/extend any of it.
- **JSON specifics:** preferred JSON library / any existing serialization utility in the repo;
  file layout and naming for definition assets; how strict the schema/versioning should be.
- **Voxel & world:** target chunk sizes, world scale, how faithful to the Unity voxel shape set
  (cube/slab/slope/stair/cross-plane) to be initially; how much procedural generation for
  Milestone 1 vs. hand-authored maps.
- **Battle:** board size rules, placement rules, how much targeting/range/LoS/AoE preview,
  which effects/statuses/AI to ship first.
- **Art/audio direction:** placeholder assets policy; is there any existing art; audio scope.
- **Tooling:** which tool to build first (the user has repeatedly emphasized a **3D voxel
  modeling tool**); whether tools live in the same executable or separate apps.
- **`spk::` promotions:** how eager to promote vs. keep in `pg::`; sign-off expectations.
- Anything in `GDD.md`/`AGENTS.md` marked as an open question.

Do **not** proceed to the full documentation set until these are answered.

---

## 7. Non-negotiable design rules to preserve (from the GDD / AGENTS)

- No creature XP levels; growth is battle-earned **Feat Progress** on the Feat Board.
- 6-slot active team; overflow to PC storage.
- Tactical, grid-based, turn-based battles with a stamina/turn-bar; AP & MP refill at turn
  start; movement and abilities are separate resources.
- Most battles derived from the surrounding voxel world; special battles may be handcrafted.
- Procedural, seeded overworld; gym order / encounter scaling gated by badges/unlock-tier, not
  by level.
- Losing a battle still preserves meaningful progression.

---

## 8. Engine quick-reference (verify all of this against the source before relying on it)

Sparkle is a **C++23 / OpenGL** rendering library. `spk::` code is in `includes/structures/**`
+ `srcs/structures/**`; the game/testbed is `Playground/` (`pg::`), built with the `playground`
CMake preset (`cmake --build build/playground --target SparklePlayground`); Playground resources
are found via a `PG_RESOURCE_DIR` compile definition.

Believed-present capabilities (confirm in code, note anything stale):
- **ECS:** data-only `spk::Component` + `spk::ComponentLogic<T>` (render/update/event hooks);
  `spk::Entity`; a component store keyed by `typeid`. 2D components exist
  (`Entity2D`/`Transform2D`/`Camera2D`/`SpriteRenderer2D`/…). A **prototype 3D layer** exists in
  `Playground/srcs` (`pg::Transform3D`, `Camera3D`, `MeshRenderer3D`, `MeshRenderLogic`,
  `MeshRenderCommand`, `makeCube`, mesh shaders) — you may keep, rework, or **delete** it.
- **Rendering:** `spk::RenderCommand` + `RenderUnitBuilder`; `spk::Program` (GLSL vert/frag),
  UBO/SSBO, textures, samplers, framebuffer objects; a GPU-instanced sprite path;
  `CameraUpdateRenderCommand` (uploads a matrix to a UBO binding).
- **Geometry:** `spk::GenericMesh<Vertex>`; `spk::TextureMesh2D` (its vertex already carries a
  `Vector3` position + UV — usable as a 3D mesh); `PrimitiveObject`.
- **Math:** `Vector2/3/4`, `Matrix4x4` (`perspective`, `lookAt`, `translation`, `rotation`
  (euler + quaternion), `scale`, `inverse`), `Quaternion`, and generation primitives
  **`Perlin`**, **`PoissonDisk`**, **`CellularAutomata`**, **`CurveInterpolator`** (directly
  relevant to world generation).
- **UI:** a full widget system (layouts, `DebugOverlay`, etc.) and a `GameEngineWidget` host.
- **Known gotcha (may already be fixed — verify):** the matrix rotation builders had an
  inconsistency where `rotation(Quaternion)` / `rotateAroundAxis` rotated *clockwise* while
  `rotation(euler)` rotated *counter-clockwise*; a fix aligning all of them to the right-hand
  (CCW) convention was applied. Confirm the current state and note it in your rendering/math
  docs. For camera view matrices, prefer `Matrix4x4::lookAt` directly.

The single biggest capability gap is a **mature 3D rendering layer** (camera, transforms, mesh
rendering, depth handling) and the **voxel data model + mesher**; plan these as the flagship
early work and the first promotion candidates.

---

## 9. Summary of your task

Read the sources → verify the engine → report understanding + **ask the user your questions and
wait** → then deliver a complete, indexed documentation set (overview, architecture, data model
+ JSON schemas, per-system docs, PlantUML class + sequence diagrams, a dependency-ordered
step-by-step implementation plan sized for Opus 4.8, how-to guides, glossary, decision log)
under `Playground/docs/`. You may freely delete/rewrite existing Playground prototype code and
docs. Do not implement the game — plan it so thoroughly that implementing it is mechanical.
