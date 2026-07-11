# Ticket 040 — Harden Playground runtime edge cases

- **Status:** Ready
- **Priority:** P2 — Medium/low hardening
- **Area:** Playground / input / component logic / runtime validation

## Problem

Several smaller runtime paths assume the current one-player, writable, always-visible happy path. Individually they are modest, but together they make future NPCs, minimized windows, rapid input, portals, and malformed world data fragile.

## Evidence

- `playground/srcs/logics/exploration_input_logic.cpp:134-155,175-181` raycasts and rebuilds/allocates a hover mesh on every mouse move, even when the hovered cell is unchanged.
- `playground/srcs/rendering/mouse_picker.cpp:17-20` throws when a minimized/zero-sized viewport receives a mouse event.
- `playground/srcs/logics/camera_controller_logic.cpp:31-36` resets shared `_wasActive` for every non-player actor, making state depend on component iteration order once an NPC exists.
- `playground/srcs/logics/actor_path_logic.cpp:103-124` continues movement after a portal event is queued; `game_scene_widget.cpp:175-180` stores only the last pending target.
- `playground/srcs/world/biome_definition.cpp:27-36,76-90` validates voxel existence but not the traversal/shape semantics required by surface, stair, or deep-terrain slots.
- `playground/srcs/world/generator/plan_chunk_provider.cpp:119-126` logs and skips unknown planned prefabs, producing a partial world.

## Failure scenario

Adding an NPC can repeatedly reset camera activation state; moving the mouse over an unchanged cell creates avoidable allocations; input during minimization can throw; a large frame can cross multiple portals and make the last one win; semantically invalid biome voxels or missing prefabs load into a broken partial world.

## Proposed direction

Treat these as one hardening pass: make state explicitly player-scoped, make hover rebuilding change-driven, make zero viewport a no-op, stop/resolve movement deterministically when a portal triggers, and strengthen semantic validation so invalid world data fails before scene construction.

## Acceptance criteria

- [ ] An unchanged hovered cell does not rebuild its mesh.
- [ ] Mouse input with a zero-sized viewport is ignored safely.
- [ ] Non-player actors cannot modify player-camera activation state.
- [ ] Crossing a portal stops or resolves remaining movement according to a documented deterministic rule.
- [ ] Biome palette roles validate required traversal and shape capabilities.
- [ ] Unknown planned prefabs reject the plan or are represented by an explicit, tested optional-placement policy.

## Tests

- [ ] Add a two-actor camera update test with both component orders.
- [ ] Count hover-mesh rebuilds across repeated mouse positions/cells.
- [ ] Exercise mouse input while the viewport is zero-sized.
- [ ] Exercise multiple portal crossings in a large simulated frame.
- [ ] Reject semantically invalid biome pools and required missing prefabs during loading.

## Dependencies / notes

If this ticket proves too broad during implementation, preserve this file as the parent ticket and create linked subtasks before starting work; do not mark it complete until every acceptance item is satisfied.
