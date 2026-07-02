# 01 — Architecture

The runtime architecture of the Erelia port, in Sparkle terms. Read
[00-overview.md](00-overview.md) first; decisions referenced as `Dxx` live in the
[decision log](decision-log.md).

---

## 1. Big picture

**One persistent world, control-state modes, headless game core, thin presentation.**

```
┌────────────────────────────────────────────────────────────────────────────┐
│ spk::Application / spk::Window                                             │
│  └─ pg::GameSceneWidget  (spk::GameEngineWidget)                           │
│      ├─ spk::GameEngine ── entities + components + component logics        │
│      │    ├─ world entities: VoxelWorld, chunks, player, actors, cameras   │
│      │    └─ logics: update logics, MeshRenderLogic, overlay logic, …      │
│      ├─ pg::GameContext  (root state: WorldContext + PlayerData)           │
│      ├─ pg::ModeManager  (ExplorationMode ⇄ BattleMode ⇄ MainMenuMode)     │
│      └─ HUD widgets (children of the widget tree, observable-bound)        │
│                                                                            │
│  pg::EventCenter (typed bus) — the only cross-system channel               │
│  pg::Registries  (immutable definitions loaded from resources/data/*.json) │
└────────────────────────────────────────────────────────────────────────────┘
```

- The **voxel world entity and its renderer persist across every mode switch** (D03).
  A `Mode` swaps *input handling + HUD + camera behavior + overlays*, never the scene.
- **Battle logic is headless**: it operates on `pg::BoardData` (cells + navigation graph +
  runtime registry) and never touches rendering, widgets, or the `GameEngine`. At runtime the
  board is derived live from world voxels; in tests it is built from synthetic grids.
- **Definitions are immutable after load** (from JSON, D02/D10); all mutable state lives in
  `GameContext` (world + player) or per-battle in `BattleContext`.

## 2. Process composition & lifetime

`main()` (SparklePlayground):

1. Create `spk::GraphicalApplication` + `spk::Window`.
2. Load all **registries** from `PG_RESOURCE_DIR "/data"` (fail fast on any error, D10).
3. Create `pg::GameContext` (seed → `WorldContext`, `PlayerData`).
4. Create `pg::GameSceneWidget` (hosts the `spk::GameEngine`), build the world scene
   (world entity, player entity, camera entity), register logics.
5. Create `pg::ModeManager`, enter `ExplorationMode`.
6. `application.run()`.

Ownership: `GameSceneWidget` owns the engine, the scene entities and the `ModeManager`;
`GameContext` and registries are owned at `main` scope and passed by reference. **Registries
must outlive every entity** (component logics may hold definition pointers).

Update flow per frame: widget `_onUpdate(tick)` → mode update (input state → intents) →
engine update (logics `onUpdate`) → widget `_buildRenderUnit()` → engine render (logics
`onRender` emit RenderCommands) → commands execute on the GL context.

## 3. ECS usage rules

Sparkle's model ([spk_component_logic.hpp](../../includes/structures/game_engine/spk_component_logic.hpp)):

- **`spk::Component`** = data only (e.g. `pg::Transform3D`, `pg::MeshRenderer3D`,
  `pg::CreatureView`). No virtual update methods; plain accessors.
- **`spk::ComponentLogic<TComponent>`** = the system that processes *all* components of that
  type. Register once per engine: `engine.add<pg::MeshRenderLogic>()`. Hooks:
  - update: `_onUpdateStarted(tick)` → `_parseComponentForUpdate(tick, comp)` per component →
    `_executeUpdate(tick)`;
  - render: `_onRenderStarted(count)` → `_parseComponentForRender(comp)` →
    `_executeRender(builder)` — emit RenderCommands only in `_executeRender`;
  - input: `_parseComponentForKeyPressedEvent(...)`, mouse variants, etc.
- **`spk::Entity`** = hierarchy node; `pg::Entity3D` adds an owned `Transform3D`.
  Add entities to the engine with `engine.addEntity(&entity)`; entities are owned by the
  scene (the widget), not the engine.
- Ordering between logics uses `spk::PriorizableTrait` (lower priority runs first). Fixed
  plan: input/mode logics → simulation logics → view-sync logics → render logics.
- Components are found via `entity->component<T>()` (dynamic_cast walk) — cache pointers in
  logics when per-frame cost matters (the pattern `Entity3D` uses).

**What is NOT ECS:** battle rules, feat evaluation, world generation, pathfinding — these are
plain headless classes/functions in `PlaygroundCore`, called by modes/logics. Components only
carry *view/world* state (positions, meshes, animation), mirroring the Unity split between
MonoBehaviours (presenters/views) and plain C# classes (rules/contexts).

## 4. Rendering pipeline (multi-camera by construction, D04)

A frame's render is an ordered list of `spk::RenderCommand`s. The 3D path (Step 1, verified):

```
MeshRenderLogic::_executeRender(builder):
    builder.emplace<spk::CameraUpdateRenderCommand>(binding=1, camera.viewProjection())
    for each MeshRenderer3D: builder.add(MeshRenderCommand{mesh, texture, modelMatrix})
```

- **UBO binding 1** = camera view-projection (`CameraData`), **binding 2** = per-draw model
  matrix (`ModelData`). Shaders: `resources/shaders/mesh/mesh.vert|frag`.
- The camera matrix is uploaded **in-stream every pass**; a second camera (minimap, tool
  viewport) is simply another `CameraUpdateRenderCommand` (+ viewport command) followed by
  its draws. Never cache a "current camera" on the GPU across passes.
- **Depth**: `spk::OpenGLClearCommand` already clears depth; the 3D mesh commands enable
  `GL_DEPTH_TEST` for their draw and restore state after (same pattern as their existing
  back-face-culling toggle). Hardened in step 03.
- Voxel terrain renders as **one mesh per chunk** (rebuilt on edit), emitted by a
  `ChunkRenderLogic`; overlays (battle grid, range/path/AoE highlights) render as translucent
  meshes drawn after terrain (see [03-systems/rendering-cameras.md](03-systems/rendering-cameras.md)).

## 5. Events & observables

- **`pg::EventCenter`** — typed pub-sub built from `spk::ContractProvider`: one
  `ContractProvider<Payload…>` member per event (`playerMoved(Vector3Int)`,
  `encounterTriggered(EncounterSpawn)`, `battleStarted(BattleContext&)`,
  `battleResolved(BattleContext&, BattleSide)`, `battleEventOccurred(const BattleEvent&)`,
  `featProgressUpdated(...)`, …). Subscribing returns a `Contract` (RAII — store it; dropping
  it unsubscribes). Cross-system communication goes **only** through the bus: Exploration and
  Battle never hold references to each other (mirrors Unity's `EventCenter`).
- **`spk::ObservableValue<T>`** exists in the library; **`pg::ObservableResource`**
  (int current/max, `set/setCurrent/change/reset`, `ratio()`) and
  **`pg::ObservableFloatResource`** are built in step 02, mirroring Unity's. All HUD widgets
  bind to observables and never poll.

## 6. Modes (control-states over one world)

```
pg::Mode           — enter() / exit() / update(tick); owns nothing persistent
pg::ModeManager    — holds current mode; switches on EventCenter battle events
pg::ExplorationMode— free-roam input intents, follow camera, encounter emission enabled
pg::BattleMode     — owns one BattleContext + orchestrator; battle input; tactical camera;
                     board overlay visible; exploration triggers frozen
pg::MainMenuMode   — (polish phase) menu HUD over a live vista
```

`ModeManager` subscribes: `battleStarted` → enter battle; `battleResolved` → back to
exploration. A mode's `enter/exit` toggles: which input logic is active, which HUD widget
tree is activated, which camera controller drives `Camera3D`, and whether the board overlay
component is active. **The world entity, chunks, and renderer are never torn down** (the one
exception: entering an *interior* loads a different voxel map — still ExplorationMode,
pointed at another `VoxelWorld`).

## 7. Data & registry layer

- `pg::JsonLoader` (nlohmann-json): read file → parse → validate `version` → strict field
  extraction helpers that throw `pg::JsonError{file, path, message}` on missing/unknown keys
  (D10).
- `pg::Registry<TDefinition>`: `load(directory)`, `get(id) -> const TDefinition&`,
  `tryGet`, `ids()`. Ids are filename stems. Numeric voxel ids for `VoxelCell` are assigned
  at load in sorted-id order and stable within a run; **persisted data always stores string
  ids** (never runtime indices).
- Polymorphic definitions (`VoxelShape`, `Effect`, `FeatRequirement`, `FeatReward`,
  `AICondition`, `AIDecision`, animation `Step`) deserialize via a per-hierarchy
  **factory map**: `"type"` string → parse function. See [02-data-model.md](02-data-model.md).
- Cross-references between definitions are **string ids resolved at load time** into
  pointers (abilities referenced by species, statuses by effects, tables by biomes) —
  load order: voxels → statuses → abilities → feat boards → species → biomes/encounters.

## 8. Planned source layout

```
Playground/
  CMakeLists.txt          # PlaygroundCore (static lib) + SparklePlayground (exe)
                          # + PlaygroundTests (gtest) + EreliaTools (exe)      (D17/D15)
  srcs/
    main.cpp              # game entry
    core/                 # event_center, observables, json_loader, registry,
                          # game_context, mode, mode_manager, modes
    components/           # Transform3D, Camera3D, MeshRenderer3D, CreatureView, …
    logics/               # MeshRenderLogic, ChunkRenderLogic, PlayerControlLogic, …
    rendering/            # MeshRenderCommand + 3D render helpers
    geometry/             # primitive builders (makeCube, …), Mesh3D vertex
    voxel/                # definition, data, shapes/, cell, grid, mesher, registry
    world/                # chunk, voxel_world, streaming, map stamping, raycast, generator/
    board/                # board_data, terrain/navigation/runtime layers, pathfinder, los
    battle/               # context, unit, actions, events, rules/, phases/, orchestrator
    creatures/            # attributes, species, form, unit, storage
    abilities/            # ability, effects/
    statuses/
    feats/                # board, node, requirement/, reward/, progress, service
    encounters/           # table, tier, resolver, emitter
    taming/
    ai/
    animation/            # logical parts, rig, recipe, steps, animator
    audio/
    ui/                   # HUD widgets (battle hud, exploration hud, cards…)
    save/
  tools/                  # EreliaTools entry + editor pages
  tests/                  # PlaygroundTests sources (mirror srcs/ structure)
  resources/
    shaders/  data/  textures/
```

Header-only where practical (components, small data types), `.cpp` for logic-heavy classes —
mirror the existing Sparkle idiom file by file.

## 9. Unity → Sparkle concept map

| Unity concept | Port equivalent |
|---|---|
| `ScriptableObject` asset (`Ability`, `CreatureSpecies`, `Status`, `VoxelDefinition`, `BiomeDefinition`) | JSON file in `resources/data/<domain>/` loaded into a `pg::Registry<T>` (D02) |
| `[SerializeReference]` polymorphic field | `"type"`-discriminated JSON + factory map ([02-data-model.md](02-data-model.md)) |
| `MonoBehaviour` presenter/view | `spk::Component` (data) + `spk::ComponentLogic<T>` (behavior) |
| `GameBootstrapper` | `main.cpp` bootstrap sequence (§2) |
| `ModeManager` / `Mode.Enter/Exit` | `pg::ModeManager` / `pg::Mode` — control-states, not scenes (§6) |
| `GameContext` (`WorldContext` + `PlayerData`) | `pg::GameContext` — same shape |
| static `EventCenter` (C# events) | `pg::EventCenter` over `spk::ContractProvider` (RAII contracts instead of manual `-=`) |
| `ObservableValue<T>` / `ObservableResource` / `ObservableList<T>` | `spk::ObservableValue<T>` (exists) / `pg::ObservableResource` / `pg::ObservableList<T>` |
| static rule classes (`BattleTurnRules`, `BattleActionValidator`, …) | same: stateless functions in `pg::battle` (namespace-level or static-member) |
| `BattleOrchestrator` + phases + controllers | same 7-phase FSM ([03-systems/battle.md](03-systems/battle.md)) |
| `VoxelMesher` → `UnityEngine.Mesh` | `pg::VoxelMesher` → `pg::Mesh3D` (a `spk::GenericMesh` with position/normal/uv vertex) |
| `Physics.Raycast` (mouse pick, LoS) | `pg::VoxelRaycaster` — grid DDA over voxel data (no physics engine) |
| Unity editor windows (`VoxelDefinitionEditor`, `FeatBoardEditorWindow`, …) | pages of the `EreliaTools` widget app (D15) |
| `Newtonsoft.Json` save serialization | nlohmann-json (`saves/<slot>.json`) |
| Unity `Camera` | `pg::Camera3D` component + camera-controller logics; VP uploaded in-stream (D04) |
| Coroutines/tweens | tick-driven tween helpers updated by logics (no coroutines) |

## 10. Engine capability gaps → where they are closed

| Gap (verified absent from `spk::`) | Closed by |
|---|---|
| Depth-tested, lit 3D mesh path | plan step 03 (then promoted, step 13) |
| Voxel data model + occlusion mesher | steps 04–05 |
| 3D chunk streaming | step 06 |
| JSON + registries | step 01 (nlohmann-json via vcpkg) |
| Typed event bus + resource observables | step 02 |
| A* / navigation graph / voxel raycast | steps 07, 09 |
| Turn/phase FSM, battle rules | step 10 |
| Screen→world ray picking | step 07 (exploration), used by battle in step 11 |
| Fake-animation rig | step 34 |
| Audio (nothing in `spk::` at all) | step 35 (miniaudio, D16) |
| Tool apps | steps 25–28 |

## 11. Threading & determinism

Everything runs on the render/update thread (Sparkle widgets are single-threaded); the voxel
mesher is synchronous per chunk — acceptable at 16³ with occlusion culling; if profiling ever
disagrees, chunk meshing is the first candidate for a worker thread, behind the existing
mesh/command seam. World generation must draw **only** from seeded RNG owned by the
generator (`std::mt19937` seeded from the run seed + purpose-specific stream ids); never
`rand()` or time-based seeds — save/load reproducibility depends on it.
