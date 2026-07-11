# Bound the retained voxel chunk cache

- **Status:** Ready
- **Priority:** P1 — High
- Area: Voxel streaming / Playground runtime

## Problem

Retaining inactive chunks is intentional behavior, not itself a defect: it makes revisiting an area cheap and avoids regenerating chunks. The fragile part is that retention is enabled by default, has no size or memory budget, and is used unchanged by the Playground.

As a player explores, every visited chunk keeps its voxel grid and baked meshes for the rest of the session. Systems that enumerate loaded chunks also pay for the entire exploration history rather than the current streaming window. Memory usage and periodic simulation work therefore grow without a bound.

## Evidence

- includes/structures/voxel/spk_voxel_chunk_streamer_logic.hpp:25 — retained inactive chunks are the stored default.
- includes/structures/voxel/spk_voxel_chunk_streamer_logic.hpp:31 — the public constructor also defaults retention to true.
- srcs/structures/voxel/spk_voxel_chunk_streamer_logic.cpp:91 — unwanted chunks are merely deactivated when retention is enabled.
- playground/srcs/game_scene_widget.cpp:145 — Playground constructs the streamer logic with the unbounded default.
- playground/srcs/logics/fluid_simulation_logic.cpp:62 — fluid synchronization obtains all loaded chunks, including retained inactive chunks.

## Failure scenario

Move continuously through the generated world so that each streaming window contains previously unseen chunks. The active chunk count stays approximately constant, but loaded chunk count, retained meshes, fluid source bookkeeping, and the cost of loaded-chunk scans keep increasing until the process suffers large frame spikes or exhausts memory.

## Proposed direction

Keep retention as an explicit supported policy, but make it bounded and observable. Introduce a configurable inactive-chunk budget, such as an LRU count or memory budget, with deterministic eviction that never removes currently wanted chunks. Alternatively, configure Playground to unload inactive chunks if caching is not required there.

Document the trade-off and expose loaded/active/inactive counts so the chosen policy can be monitored. Eviction must use the normal map unload path so renderer, neighbor, fluid, and entity bookkeeping remain consistent.

## Acceptance criteria

- [ ] The engine clearly distinguishes active chunks, retained inactive chunks, and unloaded chunks in its documented policy.
- [ ] Playground cannot retain an unlimited number of inactive chunks during ordinary exploration.
- [ ] Currently wanted chunks are never selected for cache eviction.
- [ ] Eviction uses normal chunk teardown and neighbor synchronization paths.
- [ ] The configured limit and current retained count are inspectable for diagnostics.
- [ ] Re-entering a retained area still avoids regeneration while it remains within the budget.

## Tests

- [ ] Add a traversal test that visits more streaming windows than the cache limit and asserts a bounded loaded-chunk count.
- [ ] Add a test proving active/wanted chunks survive eviction pressure.
- [ ] Add a test proving the least-recently-used or otherwise documented victim is selected deterministically.
- [ ] Add a revisit test covering both cache hit and post-eviction regeneration.
- [ ] Add a fluid bookkeeping test proving evicted chunks and their sources are forgotten.

## Dependencies / notes

This ticket deliberately does not remove the retain-inactive feature. Coordinate the implementation with streamer lifecycle cleanup and map-iteration safety tickets so eviction does not introduce stale map pointers or callback invalidation.
