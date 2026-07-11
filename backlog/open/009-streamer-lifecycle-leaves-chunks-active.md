# Reconcile chunk state when streamers disappear or deactivate

- **Status:** Ready
- **Priority:** P1 — Medium/high, lifecycle correctness
- **Area:** Engine / voxel chunk streaming / entity lifecycle

## Problem

`VoxelChunkStreamerLogic` clears its per-frame map aggregation at the start of every update and reconciles only maps that received at least one active streamer component during that frame. Historical wanted sets are stored separately.

If the last streamer for a map is removed, deactivated, or stops being parsed, that map is absent from `_mapStreamings`; `_executeUpdate()` never visits it. Chunks wanted on the previous frame therefore remain active—or remain loaded when retention is disabled—even though no streamer wants them. Historical state also retains raw `VoxelMap*` keys indefinitely, which risks stale-address bookkeeping if maps are destroyed and later allocated at the same address.

Manual chunk activation/deactivation is also not reconciled when the desired coordinate set is unchanged, because a clean streamer causes the map pass to be skipped. Existing behavior may be intentional for manual chunks, but the ownership rule is surprising and needs an explicit contract.

## Evidence

- Current-frame map aggregation is discarded every update: [`srcs/structures/voxel/spk_voxel_chunk_streamer_logic.cpp:32`](../../srcs/structures/voxel/spk_voxel_chunk_streamer_logic.cpp#L32).
- Only parsed streamers recreate a `MapStreaming` entry: [`srcs/structures/voxel/spk_voxel_chunk_streamer_logic.cpp:38`](../../srcs/structures/voxel/spk_voxel_chunk_streamer_logic.cpp#L38).
- Execution iterates only the maps seen during the current update: [`srcs/structures/voxel/spk_voxel_chunk_streamer_logic.cpp:61`](../../srcs/structures/voxel/spk_voxel_chunk_streamer_logic.cpp#L61).
- A map is skipped when its wanted set/configuration appears unchanged: [`srcs/structures/voxel/spk_voxel_chunk_streamer_logic.cpp:66`](../../srcs/structures/voxel/spk_voxel_chunk_streamer_logic.cpp#L66).
- Previous wanted state is keyed by raw `VoxelMap*` and has no pruning path: [`includes/structures/voxel/spk_voxel_chunk_streamer_logic.hpp:28`](../../includes/structures/voxel/spk_voxel_chunk_streamer_logic.hpp#L28), [`srcs/structures/voxel/spk_voxel_chunk_streamer_logic.cpp:103`](../../srcs/structures/voxel/spk_voxel_chunk_streamer_logic.cpp#L103).

## Failure scenario

1. One active streamer requests a set of chunks from a map.
2. The logic loads/activates those chunks and records the wanted set.
3. Remove or deactivate that streamer before the next update.
4. No component contributes the map to `_mapStreamings`.
5. The reconciliation loop never visits the map, so the old chunk set remains active/loaded permanently.

A destroyed map can additionally leave a raw key whose address may later alias a different map.

## Proposed direction

Define lifecycle semantics for the transition from one-or-more streamers to zero streamers, then make it observable safely. Alternatives include explicit component/map registration hooks, owner-safe map handles with teardown notification, or a reconciliation pass over tracked maps that can prove each map is still alive. Do not dereference historical raw pointers merely because they were present last frame.

Also define whether the streamer owns all active state in its maps or only chunks it previously activated. That decision determines how manual chunk state should interact with an unchanged wanted set.

## Acceptance criteria

- [ ] Removing or deactivating the last streamer produces the documented zero-streamer state on the next update.
- [ ] Behavior is correct for both `retainInactiveChunks == true` and `false`.
- [ ] Removing one of several streamers does not deactivate chunks still wanted by another streamer on the same map.
- [ ] Destroyed maps leave no stale pointer state and cannot contaminate a new map at a reused address.
- [ ] Manual chunk ownership/reconciliation semantics are documented and tested.
- [ ] Historical wanted-state storage is pruned when it can no longer be used.

## Tests

- [ ] Load chunks with one streamer, then remove it and verify the zero-streamer result.
- [ ] Repeat by deactivating the component/entity rather than destroying it.
- [ ] Repeat with retention enabled and disabled.
- [ ] Test two streamers on one map, removing them one at a time.
- [ ] Destroy a map after streaming, create another map, and verify no historical state is reused.
- [ ] Cover manual activation/deactivation while the wanted coordinate set is unchanged.

## Dependencies / notes

The right cleanup behavior depends on whether streamed chunks and manually managed chunks share ownership. Resolve and document that policy as part of this ticket rather than embedding another implicit convention.
