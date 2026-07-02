# Erelia — Implementation Documentation

The complete, authoritative documentation set for building the **Erelia** creature-RPG
inside the Sparkle engine, entirely under `Playground/` (`pg::`). Produced by the Fable
planning pass (2026-07-02); executed step-by-step by the implementing agent.

**Authority chain:** user decisions → the GDD (`GDD.md` in the Unity reference project) →
**these docs**. The Unity project itself is a worked example, never a spec
([D28](decision-log.md)).

## Start here

| You are… | Read |
|---|---|
| **Implementing the game** (Opus 4.8) | [plan/implementation-plan.md](plan/implementation-plan.md) — the loop, the rules, the next unchecked step |
| New to the project | [00-overview.md](00-overview.md) |
| Checking why something is the way it is | [decision-log.md](decision-log.md) |
| Looking up a name/term | [glossary.md](glossary.md) |

## Contents

- [00-overview.md](00-overview.md) — vision, pillars, non-negotiables, Milestone 1.
- [01-architecture.md](01-architecture.md) — runtime architecture, ECS usage, rendering
  pipeline, event bus, data layer, source layout, Unity→Sparkle concept map, gap table.
- [02-data-model.md](02-data-model.md) — JSON conventions + schemas for every domain:
  game rules, voxels, models, vfx, statuses, abilities/effects, feat boards +
  battle conditions, animations, AI, creatures, encounter tables, biomes, prefabs, maps,
  worldgen, saves.
- [03-systems/](03-systems/) — per-system designs:
  [voxel](03-systems/voxel.md) · [world](03-systems/world.md) ·
  [board](03-systems/board.md) · [battle](03-systems/battle.md) ·
  [creatures-feats](03-systems/creatures-feats.md) ·
  [encounters-taming](03-systems/encounters-taming.md) ·
  [rendering-cameras](03-systems/rendering-cameras.md) ·
  [ui-hud](03-systems/ui-hud.md) · [save-meta](03-systems/save-meta.md) ·
  [animation](03-systems/animation.md) · [audio](03-systems/audio.md) ·
  [tooling](03-systems/tooling.md)
- [diagrams/](diagrams/README.md) — 12 class + 6 sequence PlantUML diagrams.
- [plan/](plan/implementation-plan.md) — phases → 37 self-contained step files
  ([plan/steps/](plan/steps/)); milestones 1–3; critical path; promotion track.
- [howto/](howto/) — [build & run](howto/build-and-run.md) ·
  [component+logic](howto/add-component-logic.md) ·
  [render command](howto/add-render-command.md) ·
  [JSON definition](howto/add-json-definition.md) · [tests](howto/add-tests.md) ·
  [tool tab](howto/add-tool-tab.md) · [promote to spk::](howto/promote-to-spk.md)
- [glossary.md](glossary.md) — canonical terms + naming rules.
- [decision-log.md](decision-log.md) — D01–D38, every settled choice with rationale.
- [archive/](archive/) — historical documents (the original Fable planning prompt).

## Keeping these docs honest

They are living documents: when implementation reveals a better shape, **update the doc in
the same commit as the code** and log the decision. A doc that contradicts reality is worse
than no doc. The old `Playground/ERELIA.md` and `Playground/steps/` are superseded by this
set (preserved in git history, commit `b84451e`).
