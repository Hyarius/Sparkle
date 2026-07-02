# Step 3 — Exploration Slice (outline)

**Phase:** 3 (ERELIA.md §9) · **Namespace:** `pg::` · **Depends on:** Steps 1–2

> Outline only — will be detailed once Steps 1–2 land and we know the real ergonomics of the
> 3D + voxel layer.

## Goal

Walk around a **hand-authored voxel world** in 3D: chunked terrain, a player entity moving on
it, a **follow `Camera3D`**, and **encounter triggers** that fire events (the hook the battle
step will later consume). No procedural generation yet, no battle yet.

## Likely pieces

- **`pg::VoxelChunk`** — a `VoxelGrid<CX,CY,CZ>` component + its meshed `MeshRenderer3D`
  (rebuild mesh on edit). Echoes the old 2D chunk pattern (entity owns chunk sub-entities).
- **`pg::VoxelWorld`** — an entity owning chunk sub-entities keyed by chunk coord; load/unload
  around the player; global `voxelAt(cell)` / `setVoxel(cell)`. Start with a small fixed set
  of hand-authored chunks (no streaming from noise yet).
- **`pg::PlayerController3D` + logic** — ZQSD/arrow movement over the voxel terrain; snap/step
  height from the top walkable voxel + its `CardinalHeightSet` (reuse Step-2 data). Collision
  can start crude (block on obstacle voxels).
- **Follow `Camera3D`** — parent the camera to the player (like the 2D demo parents Camera2D),
  or a smoothed chase cam; feed viewport/aspect on resize.
- **Encounter triggers** — `pg::EncounterTriggerComponent` on bush/interior/trainer volumes;
  on enter, emit an event. This needs a minimal **event bus** — the first slice of Phase 0's
  `pg::EventCenter` (build just enough here). Battle consumes it in a later step.

## Foundations pulled in here (Phase 0, just-in-time)

- Minimal `pg::EventCenter` (pub/sub) for encounter/mode events.
- Minimal `pg::GameContext` (world + player refs) if scene wiring needs a shared root.
- (Full `ModeManager` / battle control-states come with the battle step; under Plan Gamma
  they are control-states over this one persistent world — see ERELIA.md §8b.)

## Done means

Player walks over a hand-built voxel map, camera follows, stepping onto a marked cell prints/
emits an encounter event. Tests cover movement/height resolution and the event bus.
