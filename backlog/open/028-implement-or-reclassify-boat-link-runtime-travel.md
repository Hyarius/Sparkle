# Implement or reclassify boat-link runtime travel

- **Status:** Ready
- **Priority:** P2 — Medium
- Area: Playground / World-generation feature gap

## Problem

World generation creates `boatLinks` between disconnected road components. Those links are drawn on the generated map and the report uses their count to label the world logically connected. Runtime gameplay, however, installs only interior `PlanPortal` entries; it does not create a boat interaction, traversal edge, teleport, or other way for the actor to use a boat link.

This may be an unfinished feature rather than a low-level defect, but the current report presents planned connectivity as if it were playable connectivity.

## Evidence

- `playground/srcs/world/generator/world_plan.hpp:283` stores generated boat-link endpoint pairs.
- `playground/srcs/world/generator/world_plan_generator.cpp:1660-1761` creates links between road components via port cities.
- `playground/srcs/world/generator/world_plan_generator.cpp:2928` reports the number of generated links.
- `playground/srcs/world/generator/world_plan_generator.cpp:3032-3036` derives the `logically connected` claim from road components and boat-link count.
- `playground/srcs/world/generator/world_map_image.cpp:421` renders boat links in the preview map.
- `playground/srcs/game_scene_widget.cpp:170-180` installs only interior portals into the runtime teleport table.

## Failure scenario

Generate a world with two disconnected road components and one boat link between their port cities. The report says the plan is logically connected and the preview shows a connection. The player reaches the first port, but there is no interaction or traversable route to the linked port, so the second component remains unreachable in gameplay.

## Proposed direction

Make an explicit product decision and implement it consistently:

- **Runtime feature:** materialize each boat link as an interaction, portal, transport route, or navigation edge with clear endpoint cells and player feedback; or
- **Planning-only annotation:** rename/reword the report and preview so they do not claim playable connectivity, and track runtime boat travel as a separate feature.

If implemented, endpoints must resolve to standable, loaded arrival cells and integrate with camera/streaming/navigation teleport behavior.

## Acceptance criteria

- [ ] The intended meaning of `boatLinks` is documented as runtime travel or planning-only metadata.
- [ ] Reports distinguish road connectivity, planned transport connectivity, and playable connectivity.
- [ ] If runtime travel is intended, every emitted link has a discoverable player action and a valid bidirectional or explicitly one-way transition.
- [ ] Arrival uses the normal streaming, navigation-bound, actor-position, and camera update paths.
- [ ] Invalid or duplicate endpoints are rejected or reported rather than silently installed.

## Tests

- [ ] Generate a two-component plan with one boat link and verify the chosen reporting semantics.
- [ ] If implemented, exercise travel in both intended directions and assert the destination cell is standable.
- [ ] Test streaming to an initially unloaded destination component.
- [ ] Test malformed, missing, and duplicate port endpoints.
- [ ] Verify worlds with no boat links retain existing behavior.

## Dependencies / notes

This ticket requires a gameplay/design decision before implementation. If boat travel becomes portal-like, reuse the safe teleport path and coordinate with ticket 029's portal validation rather than creating a second unchecked transition mechanism.
