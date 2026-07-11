# Ticket 035 — Prevent generated voxel identifiers from colliding with authored IDs

- **Status:** Ready
- **Priority:** P2 — Medium
- **Area:** Playground / voxel registry / data loading

## Problem

Fluid stages receive generated names such as `water#1`, but authored file IDs are allowed to use the same syntax. Registry insertion ignores whether the string-to-numeric map accepted the name. A collision leaves the numeric definition tables and name lookup map inconsistent: both numeric entries exist, while string lookup resolves only one of them.

## Evidence

- `playground/srcs/voxel/voxel_registry.cpp:42-50` ignores the result of `stringToNumeric.emplace()`.
- `playground/srcs/voxel/voxel_registry.cpp:75-104` generates `id + "#" + stage` names.
- Generic registry IDs come from JSON filenames, so an authored `water#1.json` is otherwise valid.

## Failure scenario

Add `water#1.json` next to `water.json`. Loading produces a generated stage and an authored definition with the same public name. `ids()`, `stringId()`, and `numericId()` cease to form a bijection, and the authored voxel becomes unreachable by name.

## Proposed direction

Reserve a namespace/syntax for generated IDs or precompute every authored and generated name before registration and reject collisions transactionally. Check every insertion result and maintain an explicit registry invariant that each numeric ID has exactly one unique string ID.

## Acceptance criteria

- [ ] Authored/generated name collisions fail loading with the conflicting IDs and source file in the error.
- [ ] Registry state remains unchanged after a collision failure.
- [ ] `numericId(stringId(n)) == n` for every registered numeric ID.
- [ ] Generated-name rules are documented for content authors.
- [ ] Total numeric capacity is checked after generated stages are included.

## Tests

- [ ] Load an authored `water#1` alongside generated water stages and expect a contextual error.
- [ ] Verify name/numeric round trips for normal and generated definitions.
- [ ] Verify a failed reload preserves the previous valid registry.

## Dependencies / notes

If the identifier syntax is part of a public data contract, document the migration before renaming existing generated IDs.
