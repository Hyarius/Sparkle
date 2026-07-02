# Diagram Index

PlantUML sources. Render any of them with `plantuml <file>.puml` (or the VS Code PlantUML
extension / https://www.plantuml.com/plantuml). Class diagrams show the *planned* `pg::`
shape (mirroring the verified Unity reference where noted); sequence diagrams show the
canonical runtime flows the plan steps implement.

## Class diagrams

| File | System | Doc |
|---|---|---|
| [01-class-engine-core.puml](01-class-engine-core.puml) | GameContext, ModeManager, EventCenter, registries | [01-architecture.md](../01-architecture.md) |
| [02-class-rendering-3d.puml](02-class-rendering-3d.puml) | 3D layer, cameras, picking, overlay | [rendering-cameras.md](../03-systems/rendering-cameras.md) |
| [03-class-voxel.puml](03-class-voxel.puml) | Voxel definitions, shapes, mesher | [voxel.md](../03-systems/voxel.md) |
| [04-class-world.puml](04-class-world.puml) | Chunks, streaming, actors, generation | [world.md](../03-systems/world.md) |
| [05-class-board.puml](05-class-board.puml) | BoardData seam, graph, A*, LoS | [board.md](../03-systems/board.md) |
| [06-class-battle-phases.puml](06-class-battle-phases.puml) | Orchestrator/coordinator/phases FSM | [battle.md](../03-systems/battle.md) |
| [07-class-battle-domain.puml](07-class-battle-domain.puml) | Units, actions, abilities, effects, statuses, events | [battle.md](../03-systems/battle.md) |
| [08-class-creatures-feats.puml](08-class-creatures-feats.puml) | Species/units, feat board, requirements, progression | [creatures-feats.md](../03-systems/creatures-feats.md) |
| [09-class-encounters-taming.puml](09-class-encounters-taming.puml) | Encounter pipeline, taming | [encounters-taming.md](../03-systems/encounters-taming.md) |
| [10-class-ai.puml](10-class-ai.puml) | Rule-list AI | [battle.md](../03-systems/battle.md) §AI |
| [11-class-animation.puml](11-class-animation.puml) | Rig/recipes/animator | [animation.md](../03-systems/animation.md) |
| [12-class-tooling.puml](12-class-tooling.puml) | EreliaTools shell + shared widgets | [tooling.md](../03-systems/tooling.md) |

## Sequence diagrams

| File | Flow |
|---|---|
| [20-seq-bootstrap.puml](20-seq-bootstrap.puml) | Process start → registries → world → exploration |
| [21-seq-exploration-movement.puml](21-seq-exploration-movement.puml) | Click-to-move (D27): pick → A* → path drive → playerMoved |
| [22-seq-encounter-to-battle.puml](22-seq-encounter-to-battle.puml) | Bush roll → spawn → live board derivation → battle start (Gamma) |
| [23-seq-battle-action-resolution.puml](23-seq-battle-action-resolution.puml) | Player ability: target → validate → resolve → effects → events |
| [24-seq-feat-progression.puml](24-seq-feat-progression.puml) | Battle end → log evaluation → node completion → applyProgress |
| [25-seq-save-load.puml](25-seq-save-load.puml) | Save gather/write; load → recompute pipeline |
