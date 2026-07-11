# Make navigation rebuilds incremental and bounded

- **Status:** Ready
- **Priority:** P1 — High
- Area: Playground / Navigation performance

## Problem

Navigation refresh synchronously rebuilds the complete graph for the complete traversal bounds. The builder first scans every X/Y/Z cell. It then visits every standable node, every cardinal direction, and scans the full Y range of the neighboring column to select a connection.

The resulting cost is approximately `O(bounds volume + node count * 4 * bounds height)`, with repeated graph lookups and walk-height queries. A single recognized voxel revision can make the next path or standability query perform all of this work on the update/input path.

## Evidence

- `playground/srcs/world/world_navigation.cpp:24-29` synchronously replaces the entire graph when the revision changes.
- `playground/srcs/board/traversal_graph_builder.cpp:35-52` scans every cell in the configured volume to create nodes.
- `playground/srcs/board/traversal_graph_builder.cpp:54-69` iterates every node and direction, then starts a complete vertical scan.
- `playground/srcs/board/traversal_graph_builder.cpp:69-88` searches every Y candidate in the neighboring column.
- `playground/srcs/game_scene_widget.cpp:212-221` creates an initial region roughly 97 by 97 columns and up to the provider height.
- `playground/srcs/game_scene_widget.cpp:368-372` invalidates and recenters those bounds when the streaming focus changes.

## Failure scenario

Edit one loaded voxel, then request a path or hover a destination that queries navigation. `WorldNavigation::refresh()` rebuilds hundreds of thousands of cells and repeatedly scans vertical columns before returning. Repeated edits can turn this into a recurring frame hitch even when only one column changed.

## Proposed direction

Introduce a navigation update strategy with bounded work:

- index standable nodes by X/Z column so neighbor selection does not rescan the full height for every node;
- track dirty cells/columns/chunks and rebuild only affected nodes plus adjacent columns;
- separate a bounds shift into retained overlap and newly entered strips;
- consider building from an immutable world snapshot off the interactive thread, then publish atomically;
- expose timings, dirty-region size, and node counts to verify the budget.

At minimum, remove the per-node full-height search before attempting broader asynchronous work.

## Acceptance criteria

- [ ] Connecting a node to a neighboring column does not linearly scan the complete world height for every direction.
- [ ] A one-cell edit rebuilds only a bounded local region, not the complete navigation volume.
- [ ] Re-centering bounds reuses the overlapping region or otherwise runs within a documented budget.
- [ ] Callers never observe a partially updated graph.
- [ ] Navigation results remain equivalent for unchanged world snapshots.
- [ ] A repeatable performance budget and measurement method are documented.

## Tests

- [ ] Add correctness tests comparing incremental output with a clean full build on representative flat, stepped, sloped, and disconnected maps.
- [ ] Add a benchmark or instrumented test showing that one-cell invalidation work is independent of total bounds volume, within the chosen local radius.
- [ ] Add a tall-column benchmark proving neighbor connection no longer scales as `node count * height`.
- [ ] Test bounds movement with overlap and verify stale nodes are removed.
- [ ] Run pathfinding and navigation suites across multiple generated seeds.

## Dependencies / notes

Ticket 024 should provide reliable, granular invalidation signals. This ticket should not hide invalidation defects by caching stale graph data.
