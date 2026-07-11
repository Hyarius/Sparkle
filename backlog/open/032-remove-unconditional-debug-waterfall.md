# Ticket 032 — Remove the unconditional debug waterfall from normal gameplay

- **Status:** Ready
- **Priority:** P2 — Medium
- **Area:** Playground / scene setup

## Problem

Every normal game scene injects a water source six blocks above the player spawn. The code identifies it as a visual probe, but it is not protected by a debug option. It changes the generated world, creates a floating solid/raycastable node, starts fluid work around spawn, and can spread water across the intended starting area.

## Evidence

- `playground/srcs/game_scene_widget.cpp:275-282` unconditionally calls `setCell()` to create the probe.
- `playground/resources/data/voxels/water.json:1` makes the inserted source solid.

## Failure scenario

Start any normal interactive game. Regardless of seed or data, the generated terrain is immediately modified by a developer visualization artifact and the fluid simulator begins propagating it.

## Proposed direction

Remove the probe from normal scene construction. If it remains useful, move it into a dedicated test scene or gate it behind an explicit developer/debug configuration that defaults to off.

## Acceptance criteria

- [ ] Normal game startup does not add any voxel that was not produced by world realization or gameplay.
- [ ] A developer can still enable a fluid visualization probe through an explicit debug-only path, if retained.
- [ ] Debug probe state is visible in logging or the overlay when enabled.
- [ ] Spawn navigation and terrain are identical with fluid simulation enabled but the probe disabled.

## Tests

- [ ] Add a scene/world setup test proving the cell at `spawn + {0,6,0}` is not modified in normal mode.
- [ ] If a debug flag is retained, test both its default-off and explicit-on behavior.

## Dependencies / notes

Coordinate with Ticket 031 if the retained debug mode needs explicit fluid traversal semantics.
