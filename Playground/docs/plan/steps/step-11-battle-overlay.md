# Step 11 — Overlay rendering (mask system), tactical camera blend, mouse targeting

**Phase D · critical path · needs steps 05 (masks), 08, 10**

## Goal

The battle becomes visible and playable in place (Gamma, D03): the board overlay rendered
through the **voxel mask system** (D31), the tactical camera blend, and mouse-driven
Move/Ability targeting with previews feeding `PlayerTurnPhase::submitAction`.

## Reading

[rendering-cameras.md](../../03-systems/rendering-cameras.md) §3 (overlay) / §4 (cameras) ·
[battle.md](../../03-systems/battle.md) §targeting ·
[voxel.md](../../03-systems/voxel.md) §Meshing (buildMaskMesh) ·
[22-seq-encounter-to-battle.puml](../../diagrams/22-seq-encounter-to-battle.puml).

## Files

`srcs/battle/board_overlay_state.hpp` — plain struct: cell sets per category (deployment,
moveRange, path, abilityRange, areaOfEffect, losBlocked, hovered, invalid) + a change
counter; written by phases/input, read by the view.
`srcs/components/board_overlay_view.hpp` — component: `const BoardOverlayState*`, board
ref, baked `Mesh3D` per category-layer, last-seen change counter.
`srcs/logics/board_overlay_logic.hpp/.cpp` — update: when the change counter moved, rebuild
via `VoxelMesher::buildMaskMesh` (cells of each category, `maskOf` from game-rules
`overlayMasks`, layer index = category order → stacked Y offsets); render: emit translucent
draws with the mask sprite sheet after terrain.
`srcs/battle/battle_input.hpp/.cpp` — battle mouse/keyboard handler (active only during
PlayerTurnPhase): mode Move (default: hover → path preview + moveRange; click → MoveAction)
/ mode Ability (key 1 selects `tackle`: abilityRange + hovered AoE + losBlocked shading;
click valid anchor → AbilityAction; Esc → Move mode); all queries through
`BattleActionValidator` (extend range shapes: line/diagonal/self + AoE square/cross/circle/
line — move their geometry into `rules/battle_geometry.*` shared with previews).
`srcs/logics/tactical_camera_logic.hpp/.cpp` — frames the board (anchor+size → distance,
~50° pitch, yaw from player deployment side), 0.6 s ease blend in/out from/to the
exploration camera pose.
`srcs/core/battle_mode.*` — enter: build overlay entity, activate battle input + tactical
camera, freeze exploration input/emitter; exit: reverse. `EncounterService` (first slice,
`srcs/encounters/encounter_service.*`): on `encounterTriggered` → instantiate M1 enemy
stand-ins → `BoardBuilder::fromWorld` → `BattleContext` → `battleStarted`.

## Work items

1. Deployment strips shown during Placement phase (auto-placement still — the zones flash
   briefly before Idle; keeps the visual pipeline honest for step 22's manual placement).
2. LoS shading: cells in ability range without LoS get the `losBlocked` mask **instead of**
   the range mask.
3. Camera blend must not cut (interpolate position+target; exploration controller resumes
   from the blended-back pose).

## Tests (`[test]`)

Battle geometry: range shapes × AoE shapes truth tables (the shared `battle_geometry`
functions). Overlay state diffing (change counter → single rebuild). Input state machine
headless (mode transitions on key/click/Esc given a scripted validator). Camera blend math
(pose at t=0/0.5/1).

## Definition of Done

- `[build]`/`[test]` green.
- `[run]` visual (user validates): trigger a bush encounter → camera glides to tactical
  framing over the live terrain, deployment strips flash, the player unit's move range
  drapes over slopes/stairs with the mask texture (not flat color), hovering shows the
  path; pressing 1 shows tackle's range with LoS-blocked cells behind the wall visibly
  different; clicking resolves the action (units teleport cell-to-cell for now — tween in
  step 12); Esc cancels. Battle is fully playable to win/lose via mouse (banner in step 12).
