# Step 02 — EventCenter, observables, GameContext, Mode/ModeManager

**Phase A · critical path · needs step 01**

## Goal

The cross-system plumbing: typed event bus, resource observables, root game state, and
control-state modes (D03 — modes over one persistent world).

## Reading

[01-architecture.md §5/§6](../../01-architecture.md) ·
[01-class-engine-core.puml](../../diagrams/01-class-engine-core.puml) ·
`includes/structures/design_pattern/spk_contract_provider.hpp` ·
`includes/structures/container/spk_observable_value.hpp` ·
Unity `Mode/ModeManager.cs`, `Observables/ObservableResource.cs` (semantics reference).

## Files

- **Create** `srcs/core/event_center.hpp` — `pg::EventCenter`: one
  `spk::ContractProvider<...>` member per event. Initial set: `actorMoveRequested(Actor&,
  Vector3Int)`, `playerMoved(Vector3Int)`, `encounterTriggered(const EncounterSpawn&)`
  *(forward-declared struct, defined step 08)*, `battleStarted(BattleContext&)`,
  `battleResolved(BattleContext&, BattleSide)`, `battleTurnEnded(BattleContext&,
  BattleUnit&)`, `battleEventOccurred(const BattleEvent&)`, `featProgressUpdated(
  CreatureUnit&, int)`, `creatureImpressed(BattleUnit&)`, `creatureRecruited(CreatureUnit&)`.
  Events whose payload types don't exist yet: declare the provider only when its payload
  lands (leave `// step NN` comments) — **do not** invent placeholder payloads. Owned by
  `GameContext`; passed by reference (no singleton).
- **Create** `srcs/core/observable_resource.hpp` — `pg::ObservableResource`
  (int current/max; `set(cur,max,force)`, `setCurrent`, `setCurrentAllowOverflow`, `setMax`,
  `change/increase/decrease`, `reset`, `ratio()`; notifies via
  `spk::ContractProvider<const ObservableResource&>` on real change — mirror Unity clamping
  semantics exactly), `pg::ObservableFloatResource` (float twin),
  `pg::ObservableList<T>` (add/removeAt/clear/notifyItemChangedAt + change signal).
- **Create** `srcs/core/game_context.hpp/.cpp` — `pg::GameContext{ EventCenter events;
  WorldContext world; PlayerData player; }`. v1 `WorldContext{int seed;}` and
  `PlayerData{Vector3 position;}` stubs (grown in steps 06/14).
- **Create** `srcs/core/mode.hpp` — `pg::Mode` (enter/exit/update(tick); protected
  `GameContext&`), `srcs/core/mode_manager.hpp/.cpp` — holds modes, `switchTo`, subscribes
  `battleStarted`/`battleResolved` (contracts stored as members).
- **Create** `srcs/core/exploration_mode.hpp/.cpp`, `srcs/core/battle_mode.hpp/.cpp` —
  skeletons: enter/exit log + activate/deactivate hooks (filled by steps 07/10/11).
- **Modify** `srcs/main.cpp` + `game_scene_widget.*` — construct GameContext + ModeManager;
  enter ExplorationMode at boot (cube scene still the visible content until step 05/06).

## Tests (`[test]`)

ObservableResource: clamping table (set beyond max, negatives, overflow variant), change
notification fires only on real change, force-notify. ObservableList notifications.
EventCenter: subscribe/trigger/RAII-resign (drop contract, trigger, assert no call).
ModeManager: enter/exit ordering on switch, battleStarted→BattleMode / battleResolved→
ExplorationMode transitions (fire the providers directly with dummy payloads… BattleContext
doesn't exist yet — parameterize ModeManager's subscriptions behind small `enterBattle()`
methods so tests call them directly; wire real payloads in step 10 — note this in code).

## Definition of Done

`[build]`/`[test]`/`[run]` green; boot logs `ExplorationMode::enter`; no behavior change on
screen. No `spk::` changes.
