# Step 34 — Fake animation & VFX: rig/recipes/animator, ability presentation

**Phase L · needs step 14 (models); best after 22 (HUD) · parallel-safe with 35**

## Goal

Creatures come alive (D26/D36): the logical-part animator playing JSON recipes, and the
ability presentation sequence (caster anim → travel VFX → target anim with TakeDamage
fallback).

## Reading

[animation.md](../../03-systems/animation.md) (the spec) ·
[02-data-model.md §7/§5.2](../../02-data-model.md) ·
[11-class-animation.puml](../../diagrams/11-class-animation.puml).

## Files

`srcs/animation/`: `logical_part.hpp`, `easing.hpp` (linear/sineInOut/quadIn/quadOut),
`animation_step.hpp` + `steps/` (moveLocal, rotateLocal, scale, shake, flash, wait,
spawnVfx, playSound) + factory, `animation_recipe.hpp`/`animation_set.hpp` + parser
(mandatory-recipe validation: Idle/TakeDamage/Death), `animation_rig.hpp/.cpp` (part→
entity map from model parts, rest-pose capture, missing-part skip),
`animator.hpp` (component: set/rig/channels) + `animator_logic.hpp/.cpp` (advance, pose
accumulate rest+main+overlay, write Transform3Ds, idle return; flash → view tint),
`vfx_definition.hpp` + parser (`vfx/*.json`), `vfx_instance.hpp` + `vfx_logic.hpp/.cpp`
(billboard quads toward camera in the translucent pass; staticAt/projectile/beam kinds).
`srcs/battle/ability_presentation.hpp/.cpp` — sequences on `AbilityCast`+resolution
events: caster recipe → travelVfx (projectile speed / beam duration) → per-target
`targetAnimation` with **fallback to TakeDamage when the target's set lacks it (D36)**;
plain damage outside a sequence → overlay TakeDamage; `UnitDefeated` → Death then hide.
Presentation is cosmetic: the input-lock `viewBusy` flag (step 12) now also covers
sequences (capped at ~1.2 s).
Registries: `animations` + `vfx`; species forms resolve `animationSet` (step 14 stored the
string); rigs bind at creature-view spawn.
Data: `animations/basic-creature.json` (Idle bob, TakeDamage shake+flash, Death topple,
AttackMelee lunge, AttackRanged recoil), `vfx/fireball.json`, `vfx/magic-beam.json`;
ember gains `travelVfx` projectile + `targetAnimation: "Scorched"` (deliberately missing
from the basic set → demonstrates the fallback), tackle gains `casterAnimation:
AttackMelee`.

## Tests (`[test]`)

Playback math: phase sequencing/looping, easing curves, additive composition, rest-pose
restoration after non-loop recipes, missing-part skip, overlay-over-main compositing;
fallback selection logic (named-recipe present/absent); projectile arrival timing; parser
+ mandatory-recipe validation.

## Definition of Done

`[build]`/`[test]` green; `[run]` (user validates): idle creatures breathe; tackle lunges;
ember shoots a fireball that flies to the target which shakes red (fallback working);
deaths topple. Battles read dramatically better; no gameplay timing changed (tests prove
resolution unchanged).
