# Battle presentation and tactical input

Step 13 keeps battle rules headless while making the persistent `GameSceneWidget` playable.
`ModeManager::BattleModeRuntime` owns a `BattlePresentationRuntime` for exactly the lifetime of
its `BattleSession`; it detaches the presentation entity and callbacks before releasing the
session or restoring exploration.

```text
GameSceneWidget (one Camera3D)
  -> ModeManager / BattleModeRuntime
       -> BattleSession              authoritative commands and read-only plans
       -> BoardPresentationCellSource
       -> BattlePresentationRuntime
            -> TacticalCameraController
            -> BattleBoardPicker
            -> BattleInteractionController
            -> BattleOverlayPresenter (one translucent mesh entity)
```

`BoardData::toPresentationCell` and `fromPresentationCell` are the only mapping used by the
picker, overlays, and camera. A battle coordinate is always the solid support voxel beneath a
unit. This matters for both sources: a live board is translated into world presentation space;
an authored board starts at its isolated arena origin. The existing Bush interaction deliberately
uses `Actor.cell + (0, 1, 0)`; neither the UI nor the picker changes that exploration contract.

## Input

- During deployment, `1`–`6` select player roster slots, left-click submits a `PlaceUnitCommand`,
  and Enter submits `ConfirmDeploymentCommand`.
- During a player activation, `M` selects/cancels movement; `1`–`8` select an ability; left-click
  submits the previewed move or cast; Space submits the explicit `EndTurnCommand`.
- Escape or a short right-click cancels targeting. Right-drag orbits the tactical camera; the
  wheel zooms. A drag past three pixels is never also treated as cancel.

All of these actions call `BattleInteractionController`, which only submits value commands through
`BattleSession::submit`. It never edits occupancy, unit resources, scheduler state, effects, or
the world directly. A self-only ability with exactly one legal source-cell anchor is submitted as
part of ability selection.

## Overlay and picking

The overlay model resolves exactly one mask per canonical board cell. The mask atlas keys are
`deployment`, `movement`, `path`, `abilityRange`, `areaOfEffect`, `losBlocked`, `hovered`, and
`invalid`; priority is invalid, hovered, LoS-blocked, AoE, path, ability range, deployment, then
movement. Its revision changes only when this effective sorted set changes, so the presenter swaps
one immutable mesh only when necessary.

Deployment cells are masked as soon as deployment begins, before a creature is selected. Once a
player selects a roster creature, the same mask is narrowed to that creature's exact
`planPlacement` result. The battle renderer uses the same walk-surface mask texture and mesh
builder as exploration hover, but draws it in the opaque pass at full alpha so deployment cells
remain unmistakable over the board materials.

`WalkSurfaceMaskMeshBuilder` is shared with exploration hover. It masks every upward polygon of a
cube, slope, or stair and lifts it 0.02 units. `BattleBoardPicker` ray-casts the presentation
source’s solid cells and accepts only the nearest hit if it is an exact frozen board support cell;
it never searches down a column or clicks through an obstacle.

The tactical camera frames the presentation AABB of those same upward support surfaces. Exploration
camera logic is inactive in battle and resumes by smoothing from the current camera transform after
tactical teardown. Fluid remains paused for the whole battle; live board streaming remains pinned,
while a handcrafted arena remains an isolated immutable presentation source.

## Unit and object views

Step 14 adds source-agnostic `BattleUnitPresenter` and `BattleObjectPresenter`. Both consume
committed event spans and reconcile to the copied batch-after snapshot; neither queries a
`VoxelWorld`, maps a live-world cell, or holds mutable battle state. The presenter only uses
`BoardData::toPresentationCell` through the shared board position helpers. Unit feet sample the
board-local walk surface, receive the presentation translation once, then add half the rendered
cube height and a small clearance. `Actor.cell` and battle positions are solid support cells.

Unit cube tint and scale come from the current form's JSON `PlaceholderVisual`; missing legacy
visuals use a deterministic semantic-id fallback. Movement animation follows the committed
`UnitMovementStep` chain, never a recomputed path. Object markers are generic and keyed by
`BattleObjectId`; their tint only distinguishes creator side. Cosmetic queues cannot mutate or
delay rules, scheduler progression, event publication, or RNG.

On teardown, the HUD unbinds and unit/object entities are removed from the game engine before the
board binding, handcrafted arena, or `BattleSession` are destroyed.
