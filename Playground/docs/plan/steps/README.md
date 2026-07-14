# Erelia battle implementation plan

This directory contains the ordered implementation prompts for building Erelia's battle
mode in `Playground`. Each numbered file is intended to be handed to an implementation
agent on its own. The agent must finish, verify, and report one step before receiving the
next one.

This is a fresh plan for the repository at `6ae3151` (2026-07-13). It is not a restoration
of the battle code deleted before the current world/exploration rewrite, and it must not be
implemented by cherry-picking that removed code.

---

# 1. Authority and reading order

When sources disagree, use this order:

1. The user's latest instruction.
2. `Playground/docs/plan/GDD.md`, except for the explicitly resolved GDD clarifications in
   section 5 of this index.
3. The locked decisions in this index and the current step files.
4. Current source and tests.
5. Current `Playground/docs/decision-log.md` decisions that do not conflict with this plan.
6. The Unity project at `C:/Users/User/Documents/UnityProject/Erelia` as a worked example.
7. Older aspirational Playground documentation and deleted code as historical evidence.

The Unity draft is not a porting target. Reuse its proven rules and test cases where they
fit; do not reproduce MonoBehaviour, ScriptableObject, service-locator, or editor-only
architecture in C++.

Before implementing a step, read this index, the complete step file, every prerequisite
step's handoff report, and the live files named by the step. Treat paths and APIs in a step
as a map, not as permission to ignore repository changes made by earlier steps.

---

# 2. Verified starting point

The current executable is an exploration and procedural-world prototype:

* `PlaygroundCore`, `SparklePlayground`, and `PlaygroundTests` already exist.
* `pg::JsonReader`, `pg::Registry<T>`, and `spk::loadJsonDirectory` already provide strict,
  filename-ID, sorted, transactional JSON loading.
* `pg::Registries::loadAll` constructs local registries and publishes them only after a
  successful full load.
* `pg::VoxelWorld`, `pg::WorldNavigation`, `pg::ICellSource`, `pg::TraversalGraph`,
  `pg::TraversalGraphBuilder`, and `pg::Pathfinder` exist.
* Exploration picking already uses `Camera3D::rayFromViewport` and `spk::VoxelRayCast`.
* The world, player, camera, sun, hover renderer, streaming, fluid simulation, and
  exploration logics are owned by `pg::GameSceneWidget`.
* `pg::GameContext` currently contains only `EventCenter` and `WorldContext`.
* There is no implemented mode manager, battle session, BoardData, species, persistent
  creature, ability, status, effect, AI, encounter, feat, taming, battle HUD, or save code.
* The battle-oriented sections of `docs/01-architecture.md`, `docs/02-data-model.md`,
  `docs/03-systems/board.md`, and `docs/03-systems/ui-hud.md` are design history, not an
  implementation inventory.

Do not spend a step rebuilding generic facilities that already exist. Extend them in the
same style and preserve their strict diagnostics and transactional publication.

---

# 3. Final outcome

After step 20, a development build must support this complete loop with placeholder art:

```text
explore the generated voxel world
    -> enter a Bush cell or launch a named debug encounter
    -> atomically switch from Exploration to Battle control
    -> derive a logical board over live terrain for normal battles, or load the authored
       isolated arena required by a Gym/Special encounter
    -> manually deploy every non-empty roster creature (up to six)
    -> run deterministic stamina-bar turns
    -> move, preview, target, and cast JSON-authored abilities
    -> resolve physical/magical damage, healing, shields, statuses, traps, buffs,
       debuffs, displacement, resource changes, passives, and defeat
    -> let JSON-authored ordered-rule AI use the same command gateway
    -> track typed events for taming and Feat Progress
    -> win, lose, or impress wild creatures
    -> commit progression and recruits transactionally
    -> show a result summary and return safely to exploration
```

The final content pack is deliberately small. It exists to exercise every important rule,
not to establish production balance or final creature lore.

---

# 4. Architecture rules shared by every step

## 4.1 Playground owns game behavior

All new game-specific code belongs under `Playground/srcs` in namespace `pg`. Do not edit
`includes/` or `srcs/` to add battle behavior. Promotion into `spk::` requires a separate,
explicitly approved task.

## 4.2 Definitions are immutable; sessions are mutable

JSON registries own immutable authored definitions. Persistent player state stores string
definition IDs plus mutable instance state. A `BattleUnit` is an encounter-local projection
of a persistent `CreatureUnit` or an encounter spawn. Never put current HP, board position,
turn-bar fill, status instances, AI scratch state, or taming progress in a definition.

## 4.3 The simulation is headless

Rules under `battle/`, `abilities/`, `statuses/`, `ai/`, `feats/`, and `taming/` must run in
`PlaygroundTests` without a window, OpenGL context, widget, entity, or `GameEngine`.
Presentation maps stable battle IDs to entities/widgets and observes immutable snapshots or
events. Widgets and component logics never become the authoritative battle state.

## 4.4 One command path

Player input, AI, tests, and future scripting submit the same value-based commands through
one validator/resolver. Invalid commands perform no mutation, consume no resource, advance
no RNG, and emit no gameplay event. Do not add an AI-only damage/movement path or let the UI
mutate `BattleContext` directly.

## 4.5 One typed event vocabulary

Every successful gameplay state change appends value-owned typed events to the battle log in a
defined order. Presentation notifications, Feat Progress, taming, result summaries, and
tests consume that vocabulary. Events use stable IDs and copied scalar data; they do not
retain component pointers or references to temporary actions.

Event-delivery cursors and AI pump guard counters are authoritative orchestration scratch,
not gameplay mutations. They may change without a gameplay event, are excluded from batch
snapshots/material-progress digests, and are included only where the detailed steps require
full replay state.

## 4.6 Live-world normal battles and authored special arenas

Normal Wild/Trainer battles are an overlay on the persistent voxel world, per decision D03.
Board terrain is read through `WorldCellSource`; presentation keeps rendering the same
chunks. At entry, build traversal data once, pin the region, stop player movement, and pause
terrain-mutating fluid simulation. Do not copy the world merely to restore it later.

Gym/Special battles instead use a strict JSON-authored handcrafted board whose geometry is
materialized from a registered voxel prefab into an isolated immutable arena. The arena is
rendered by the same `GameSceneWidget` and persistent camera—not a second game scene—and
consumes no world seed, chunk stream, or fluid state for its geometry. Exploration remains
alive but hidden and is restored exactly after teardown. `GridCellSource` is the generic
handcrafted/synthetic headless fixture seam. Tests that claim `LiveWorld` provenance instead
own a frozen minimal `VoxelWorld` and use the real `WorldCellSource`; they never relabel an
owned grid as live terrain.

## 4.7 Determinism is a feature

Use a battle-local RNG derived from the world seed and stable encounter identity. Never use
wall-clock time, `random_device`, pointer ordering, unordered-container iteration, or render
frame delta in a rule. Sort externally visible cell/unit choices explicitly. Identical
definitions, seed, deployment, and commands must yield the same event log and state digest.

## 4.8 JSON contains data, not executable scripts

Species, forms, abilities, effect specifications, statuses, passives, battle objects,
Feat Boards, conditions, rewards, taming profiles, AI rules, encounter teams, tier choices,
board policies, and handcrafted battle-board geometry references are JSON. Safe algorithms,
enum semantics, validation, formulas, and effect implementations remain C++. Polymorphism
uses a closed `type` discriminator and a factory/variant parser; do not embed expressions or
a general scripting language.

## 4.9 Every step is shippable

Each step must keep `SparklePlayground` and all existing tests green. Use final ownership
and identifiers from the start; do not create a temporary parallel architecture that a
later step must replace. Later features may extend an API, but must not invalidate the
earlier contract.

## 4.10 Placeholder presentation is intentional

This series may use colored cube creatures, simple movement interpolation, text feedback,
and the current mask atlas. Final creature models, animation recipes, VFX, audio, icons,
and authoring tools are outside scope. Gameplay resolution must never wait for cosmetic
presentation.

---

# 5. Locked high-level mechanical decisions

The GDD leaves several implementation-critical details open and contains two phrases whose
literal readings conflict with its surrounding mechanics. The numbered prompts resolve them
fully; the victory and baseline-stat bullets below are authoritative clarifications of the
GDD for this implementation, not optional deviations. These summaries prevent cross-step
drift:

* One activation can contain any legal sequence of moves and ability casts until explicit
  `EndTurn`, defeat, or no legal command remains. Time does not advance while commands are
  selected or resolved.
* JSON `stamina` is seconds to fill a bar; lower is faster. Runtime time is fixed-point.
  Bars start empty, keep their fill while stunned, reset after an activation, and refill
  AP/MP at the next activation start.
* A timeline-duration stun can expire while its unit is unable to act. An owner-turn stun
  is invalid content.
* Turn ties resolve player side first, then encounter roster order, then stable battle-unit
  ID. No pointer or container accident may affect the result.
* Direct AP/MP changes distinguish the current pool from an explicitly authored
  next-activation penalty. Status modifiers change the effective maximum.
* Movement is four-directional. Entering the destination cell pays its positive integer
  terrain MP cost. Units block both destination and traversal; no ally pass-through in v1.
* Ability range ignores elevation and uses x/z geometry. Line of sight remains a 3D voxel
  query. `Range` extends maximum anchor range only, never minimum range or AoE size.
* Anchor legality and affected-target legality are separate authored profiles, so AoE
  friendly fire is never an accidental side effect.
* All targets/cells captured for one cast resolve in canonical order. Pending defeats are
  finalized after each effect application, but the already captured cast continues unless
  its next effect requires a living source.
* Victory means the opposing side has no active combatants. `RemovalReason::Impressed` is
  not defeat and grants no defeat/kill credit.
* "Same baseline" means no level or random stat scaling: a fresh creature starts at its
  species-authored base definition. It does not force unrelated species to have identical
  numbers.
* The magical offense stat is named `magicPower` in C++ and JSON to avoid collision with
  `AbilityDefinition`.
* V1 has no reserve/unplace selection: every non-empty player roster creature must be
  manually deployed and receives that battle's Feat Progress even if defeated. Storage
  creatures receive none. Losing still commits progress. Nodes that were inactive at battle
  start cannot retroactively consume that battle's earlier events.
* Feat rewards become available only after the battle result transaction, never mid-turn.
* AI re-evaluates its ordered list after each successful command and has both a command cap
  and a no-state-change guard.
* Mid-battle saving and resuming is not supported. Save/commit boundaries are immediately
  before battle construction and after a terminal result.
* Encounter repeatability is kind-locked: Wild and Debug are repeatable; Trainer, Gym, and
  Special are permanently cleared on victory. The explicit JSON boolean is validated against
  that rule rather than enabling a contradictory content override.
* Wild/Trainer encounters use frozen live-world boards. Gym/Special encounters must reference
  strict handcrafted board JSON and cannot derive arena geometry from the world seed. Debug
  encounters may deliberately select either source for reproduction.

---

# 6. Ordered steps

| Step | Prompt | Primary deliverable | Visible checkpoint |
|---:|---|---|---|
| 01 | [Battle foundations and locked contracts](step-01-battle-foundations.md) | Accurate modules, fixed-point battle time, IDs, expanded rules config, test workflow | None; foundation tests |
| 02 | [Combat definition registries](step-02-combat-definition-registries.md) | Status, ability, effect, battle-object JSON and two-phase validation | Registry load report |
| 03 | [Progression definition registries](step-03-progression-definition-registries.md) | Shared condition specs, Feat Boards, rewards, taming profiles | Registry load report |
| 04 | [Species, AI, encounters, and roster state](step-04-species-ai-encounters-and-roster.md) | Species/AI/encounter/handcrafted-board JSON, `CreatureUnit`, team, storage | Starter roster log |
| 05 | [Tactical board seam](step-05-tactical-board.md) | Source-tagged BoardData, live snapshots, authored arenas, movement, LoS, deployment | Headless board demo/tests |
| 06 | [Battle session, units, placement, and event log](step-06-battle-session-and-events.md) | BattleContext, BattleUnit projection, placement commands, typed log | Headless placed battle |
| 07 | [Stamina scheduler and phase machine](step-07-stamina-scheduler-and-phases.md) | Deterministic simulated time, multi-command activations, outcome | Headless turn trace |
| 08 | [Movement and ability targeting commands](step-08-movement-and-targeting.md) | Atomic command gateway, paths, range/AoE/LoS previews | Headless command demo |
| 09 | [Core effect resolution](step-09-core-effect-resolution.md) | Damage/heal/shield/resource/position effect semantics | Scripted combat tests |
| 10 | [Statuses, passives, and traps](step-10-statuses-passives-and-traps.md) | Durations, hooks, modifiers, DOT/HOT, stun, cleanse, objects | Full headless battle |
| 11 | [Rule-driven enemy AI](step-11-rule-driven-ai.md) | Ordered JSON rules using the common command path | AI-vs-fixture trace |
| 12 | [Modes, encounters, and lifecycle](step-12-modes-encounters-and-lifecycle.md) | Transactional live/authored board entry, deterministic triggers, teardown | Debug battle round trip |
| 13 | [Overlay, camera, and player input](step-13-overlay-camera-and-input.md) | Source-agnostic masks/picking, tactical camera, click/keyboard targeting | First playable battle |
| 14 | [Unit views and battle HUD](step-14-unit-views-and-hud.md) | Cube views, deployment UI, team/turn/resource/ability HUD | Readable manual battle |
| 15 | [Shared battle-condition engine](step-15-battle-condition-engine.md) | Ability/turn/fight/game windows and GDD-capable aggregators | Condition golden tests |
| 16 | [Wild taming runtime](step-16-wild-taming.md) | Live impressed/failed state and provisional recruits | Visible tame-and-leave flow |
| 17 | [Feat Progress and evolution](step-17-feat-progress-and-evolution.md) | Active-node progress, rewards, branching form derivation | New ability next battle |
| 18 | [Results, persistence boundary, and loss flow](step-18-results-persistence-and-loss.md) | Atomic outcome commit, result summary, save DTO, heal return | Win/loss/reload loop |
| 19 | [Playable JSON content pack](step-19-playable-content-pack.md) | Three species plus live and authored-arena encounters exercising all systems | Wild + trainer + Gym scenarios |
| 20 | [End-to-end hardening](step-20-end-to-end-hardening.md) | Replay, soak, both board sources, lifecycle, docs, performance, validation | Battle feature complete |

Steps are deliberately dependency-ordered. Do not move presentation before the headless
rules, AI before the shared validator, or Feat/taming evaluation before the event vocabulary
is stable.

---

# 7. Verification commands

Run commands from the repository root. Step 01 corrects the current preset/documentation
disagreement; until then, use the already configured test tree:

```powershell
cmake --build build/test --target PlaygroundTests SparklePlayground
./build/test/Playground/tests/PlaygroundTests.exe
./build/test/Playground/SparklePlayground.exe --resource-path Playground/resources
```

After step 01, every step uses the documented commands:

```powershell
cmake --preset test
cmake --build build/test --target PlaygroundTests SparklePlayground
ctest --test-dir build/test --output-on-failure -L playground
./build/test/Playground/SparklePlayground.exe --resource-path Playground/resources
```

Pure data/rule assertions belong in tests. Rendered masks, camera motion, layout, and visual
readability require an honest controlled run and user validation. Never report a build,
test, soak, or visual check that was not actually performed.

---

# 8. Required implementation-agent handoff

At the end of each step, the implementation agent must report:

1. The behavior now available, led by the outcome rather than an activity log.
2. Files created, materially modified, and deliberately not touched.
3. Any repository drift from the step and the exact adaptation made.
4. Build/test commands actually run and their results.
5. Visual scenarios actually checked, with skipped checks and reasons.
6. New JSON IDs/schema versions and any compatibility consequence.
7. Remaining known limitations that belong to a later numbered step.

If a locked contract must change, stop and update this index plus all affected future step
files before implementing the alternative. Do not silently make a local choice that leaves
later prompts contradictory.

---

# 9. Series-wide non-goals

Do not add resource authoring tools, editors, hot reload, final models, final textures,
animation systems, VFX systems, audio, a general scripting language, multiplayer, rollback,
items, capture balls/actions, elemental type charts, accuracy/evasion, critical hits,
multi-cell units, summons, reserves/swapping during battle, fleeing, mid-battle saves,
production gym/story scripting, a full runtime Feat Board editor, or balance for a complete
creature catalog.

The data and event seams should make those features possible later, but speculative
implementations would obscure the goal of seeing the actual game loop work.
