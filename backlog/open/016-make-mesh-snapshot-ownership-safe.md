# Make voxel mesh snapshot ownership safe

- **Status:** Ready
- **Priority:** P2 — Medium
- Area: Voxel renderer / Lifetime / Synchronization API

## Problem

VoxelChunkRenderer internally publishes meshes by replacing a shared_ptr, but parts of its public API bypass the lifetime and immutability guarantees implied by a snapshot.

meshes() returns a const reference into the currently owned allocation. A caller that retains the reference across publishMesh() can observe a destroyed object. MeshHandle is a shared_ptr to mutable VoxelRenderMeshes, so code can modify a mesh after publication while render commands consume it. The public begin/build/publish/fail sequence also relies on callers pairing state transitions correctly; consuming the synchronization request without publication or failure can leave a stale renderer that no longer reports itself dirty.

## Evidence

- includes/structures/voxel/spk_voxel_chunk_renderer.hpp:19 — MeshHandle owns mutable VoxelRenderMeshes.
- includes/structures/voxel/spk_voxel_chunk_renderer.hpp:26 — comments promise new immutable-style snapshots, but the type does not enforce immutability.
- includes/structures/voxel/spk_voxel_chunk_renderer.hpp:43 — the synchronization protocol is exposed as independent public calls.
- includes/structures/voxel/spk_voxel_chunk_renderer.hpp:50 — meshes() exposes an unowned reference.
- srcs/structures/voxel/spk_voxel_chunk_renderer.cpp:38 — publishMesh replaces the owning pointer.
- srcs/structures/voxel/spk_voxel_chunk_renderer.cpp:58 — meshes() dereferences the replaceable pointer.
- srcs/structures/voxel/spk_voxel_chunk_renderer.cpp:63 — sharedMeshes() demonstrates the safer lifetime-preserving access pattern.

## Failure scenario

Store const VoxelRenderMeshes& from meshes(), publish a replacement, and then inspect the stored reference. If no other owner retained the old allocation, the reference dangles. Separately, keep the mutable MeshHandle passed to publishMesh and modify its buffers while another thread renders the published snapshot.

## Proposed direction

Make published mesh snapshots immutable and lifetime-carrying. Prefer shared_ptr<const VoxelRenderMeshes> for published state, remove or narrowly document the reference accessor, and ensure all consumers retain a snapshot handle for their full use. Encapsulate begin/build/publish/fail in an RAII transaction or a single operation so every begun synchronization is either committed or marked for retry.

## Acceptance criteria

- [ ] No public accessor encourages retaining an unowned reference across mesh publication.
- [ ] A published mesh cannot be mutated through any handle retained by the producer.
- [ ] Render commands keep the exact snapshot they were built from alive.
- [ ] Every begun synchronization ends in successful publication or leaves the renderer dirty for retry.
- [ ] Mesh revision advances only when a complete new snapshot is committed.
- [ ] Existing update-thread/render-thread snapshot behavior remains lock-free or has a documented replacement.

## Tests

- [ ] Add a lifetime test retaining the old snapshot across multiple publications.
- [ ] Add compile-time or API tests proving the published handle exposes const mesh data.
- [ ] Add failure-injection tests for build and publication paths and assert synchronization remains requested.
- [ ] Add a revision test covering success, failure, and retry.
- [ ] Add a render-command test proving it continues using its captured old snapshot after a new one is published.

## Dependencies / notes

Current built-in render commands already use aliasing shared_ptr handles and are the model to preserve. This ticket is API hardening; it should not assert a current built-in render-thread race where none has been demonstrated.
