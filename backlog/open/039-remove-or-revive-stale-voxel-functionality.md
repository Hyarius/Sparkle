# Ticket 039 — Remove or revive stale voxel and world-generation functionality

- **Status:** Ready
- **Priority:** P3 — Low
- **Area:** Playground / engine voxel maintenance

## Problem

Several compiled or required concepts no longer participate in the active system, while comments describe behavior that changed. This increases maintenance cost and can cause future work to be based on false assumptions.

## Evidence

- `playground/srcs/world/generator/perlin_chunk_provider.cpp` is compiled by the recursive CMake glob but is never instantiated; it also folds 64-bit seeds to 32 bits at `:18-20`.
- `playground/srcs/world/biome_definition.cpp:174-178` requires `palette.flora`, but no Playground source consumes it.
- `includes/structures/voxel/spk_slab_voxel_shape.hpp:7-9` says slab sides are inner faces, while `srcs/structures/voxel/spk_slab_voxel_shape.cpp:43-53` places them in the outer shell.
- `playground/srcs/voxel/voxel_registry.cpp:81-84` retains a full-fluid-stage workaround/comment based on the obsolete inner-side behavior.

## Failure scenario

A maintainer changes fluid meshing based on the stale slab comment, assumes `flora` controls placement when it does not, or tries to use the legacy provider and receives colliding terrain for distinct 64-bit seeds.

## Proposed direction

For each stale item, explicitly choose to remove it or restore it as a supported, tested feature. Update comments and documentation in the same change. Avoid retaining compiled dead paths “for later” without an owner or test.

## Acceptance criteria

- [ ] `PerlinChunkProvider` is either removed from the target or has an active entry point, full-seed behavior, and tests.
- [ ] `palette.flora` is either consumed as documented or removed/made optional through a data migration.
- [ ] Slab header documentation matches the actual outer-shell implementation.
- [ ] Fluid-stage comments and implementation no longer reference obsolete slab behavior.
- [ ] Repository search finds no superseded names/comments associated with these paths.

## Tests

- [ ] Add tests for any functionality retained.
- [ ] Build both engine and Playground after removing any dead source/data fields.
- [ ] Load all shipped biome and voxel resources after migration.

## Dependencies / notes

This is a cleanup ticket. If any item is revived into a substantial feature, split that work into a new numbered ticket and link it here.
