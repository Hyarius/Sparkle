# Prevent dangling voxel-world state when scene construction fails

- **Status:** Ready
- **Priority:** P0 — High, memory safety
- **Area:** Playground / scene lifetime / voxel map ownership

## Problem

`GameSceneWidget` publishes a newly created `VoxelWorld` into the externally owned `GameContext` before `_buildScene()` has completed. Later initialization performs resource loading, world generation, chunk registration, navigation construction, and other operations that can throw.

The cleanup that resets navigation and destroys the world exists only in `GameSceneWidget::~GameSceneWidget()`. In C++, the destructor of the most-derived object is not run when its constructor throws. The base `GameEngineWidget` and its engine are still destroyed during unwinding, while `GameContext` can retain the partially initialized `VoxelWorld`.

`VoxelMap` subsequently calls back into its stored `GameEngine*` from its destructor to unregister chunks. If the external context destroys that world after the failed widget construction has already destroyed the engine, this is a use-after-free. Navigation and provider references retained in the context can be stale for the same reason.

## Evidence

- Construction calls `_buildScene()` before the widget is fully constructed: [`playground/srcs/game_scene_widget.cpp:108`](../../playground/srcs/game_scene_widget.cpp#L108).
- The world is published into the external context early in scene setup: [`playground/srcs/game_scene_widget.cpp:156`](../../playground/srcs/game_scene_widget.cpp#L156).
- Setup then loads many chunks and can explicitly throw if no standable spawn exists: [`playground/srcs/game_scene_widget.cpp:194`](../../playground/srcs/game_scene_widget.cpp#L194), [`playground/srcs/game_scene_widget.cpp:221`](../../playground/srcs/game_scene_widget.cpp#L221).
- Context cleanup currently happens only in the derived destructor: [`playground/srcs/game_scene_widget.cpp:126`](../../playground/srcs/game_scene_widget.cpp#L126).
- `VoxelMap::~VoxelMap()` calls `clear()`, which dereferences the stored engine while unregistering chunks: [`srcs/structures/voxel/spk_voxel_map.cpp:35`](../../srcs/structures/voxel/spk_voxel_map.cpp#L35), [`srcs/structures/voxel/spk_voxel_map.cpp:133`](../../srcs/structures/voxel/spk_voxel_map.cpp#L133).

## Failure scenario

1. Construct a `GameSceneWidget` with an externally owned `GameContext`.
2. `_buildScene()` assigns `context.world.world` and loads at least one chunk into the widget's engine.
3. A later operation throws, for example because the generated spawn column has no standable cell or a resource/allocation operation fails.
4. Constructor unwinding destroys the engine, but does not call `GameSceneWidget::~GameSceneWidget()`.
5. The context still owns the world. Resetting or destroying the context later invokes `VoxelMap::clear()` through a dangling engine pointer, producing undefined behavior.

## Proposed direction

Give scene construction transactional ownership semantics. Viable approaches include keeping the world/navigation in local or widget-owned staging objects until setup succeeds, or installing an exception guard that restores the external context before engine destruction begins. The chosen design should also restore `explorationActive` and destroy navigation before the world.

Avoid relying on callers to clear the context after catching the exception: by then the engine may already be gone. It may also be worth making the engine/map lifetime contract explicit or mechanically safer, but that broader ownership change is not required to close this immediate exceptional path.

## Acceptance criteria

- [ ] Any exception after world creation leaves `GameContext::world.world` and `GameContext::world.navigation` in a valid, documented state.
- [ ] A partially constructed scene never leaves an object that can call a destroyed `GameEngine`.
- [ ] `explorationActive` is restored consistently on both successful destruction and failed construction.
- [ ] Normal scene construction and destruction continue to unregister loaded chunk entities while the engine is alive.
- [ ] Cleanup order is explicit: navigation/provider users are released before the world and its map.

## Tests

- [ ] Add a deterministic failure-injection test after the world has loaded at least one chunk.
- [ ] Destroy/reset the external context after catching the constructor exception; verify no crash or sanitizer error.
- [ ] Verify context pointers/flags after failure.
- [ ] Retain a success-path lifetime test covering ordinary widget destruction with loaded chunks.
- [ ] Run the test under AddressSanitizer where supported.

## Dependencies / notes

This ticket is independent of normal-path engine/map ownership cleanup. A later ownership-hardening ticket may replace raw borrowed engine pointers, but this failure path should not wait for that larger redesign.
