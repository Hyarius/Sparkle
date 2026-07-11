# Render the last valid chunk mesh while rebaking

- **Status:** Ready
- **Priority:** P2 — Medium
- Area: Voxel rendering / Frame continuity

## Problem

Both opaque and transparent render passes discard a chunk whenever its renderer needs synchronization. A synchronization request can coexist with a complete, previously published mesh, but that valid snapshot is not drawn.

If an edit happens after the frame's mesh-bake phase, the chunk disappears for at least one frame. A repeated or failed bake can keep it invisible longer. Correct logic priority currently masks the problem for some Playground systems, but that is an implicit scheduling dependency rather than a renderer guarantee.

## Evidence

- srcs/structures/voxel/spk_voxel_chunk_render_logic.cpp:202 — opaque render parsing handles each renderer.
- srcs/structures/voxel/spk_voxel_chunk_render_logic.cpp:204 — any pending synchronization causes an immediate return.
- srcs/structures/voxel/spk_voxel_chunk_transparent_render_logic.cpp:65 — transparent render parsing follows the same pattern.
- srcs/structures/voxel/spk_voxel_chunk_transparent_render_logic.cpp:67 — dirty renderers are skipped even when a published transparent mesh exists.
- srcs/structures/voxel/spk_voxel_chunk_renderer.cpp:38 — publication is already an atomic replacement at the ownership level, so an old snapshot remains available until commit.

## Failure scenario

Start with a visible baked chunk. Request synchronization from logic that runs after the bake phase but before render command collection. The opaque and transparent passes both omit the chunk during that frame even though renderer.sharedMeshes() still contains its last complete mesh.

## Proposed direction

Continue rendering the last successfully published snapshot while a replacement is pending or building. Skip only a renderer that has never produced a drawable snapshot, is deactivated, or is otherwise explicitly invalid. On successful publication the next render command should naturally use the new revision. On failure, retain the last valid snapshot and keep synchronization scheduled for retry.

## Acceptance criteria

- [ ] A dirty renderer with a previously published mesh remains visible.
- [ ] Opaque and transparent passes use the same pending-rebuild policy.
- [ ] A successful publication replaces the visible revision without an empty intermediate frame.
- [ ] A failed build preserves the last valid visible snapshot and remains retryable.
- [ ] A newly created chunk with no published geometry follows an explicit, tested first-load policy.
- [ ] Deactivation and unloading still remove chunks immediately as intended.

## Tests

- [ ] Add an opaque test that requests synchronization after initial publication and still observes a render command before rebake.
- [ ] Add the equivalent transparent test.
- [ ] Add a failed-rebuild test proving old geometry stays visible and a retry is requested.
- [ ] Add a successful-rebuild test proving the revision changes exactly once with no blank frame.
- [ ] Add a first-load test for a renderer with no prior drawable snapshot.

## Dependencies / notes

The mesh-snapshot ownership ticket should establish a safe way to retain the old published revision. Rendering stale geometry briefly is preferable to a whole-chunk blink, but the behavior should be documented for gameplay systems that require immediate visual feedback.
