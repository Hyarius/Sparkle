# System — Fake Animation & VFX

Logical-part offset animation — no skeletal rig. The Unity design doc
`Architecture/modelAnimationProposition.md` seeded this design (nothing was implemented in
Unity); this doc + the JSON schema are the spec. Also covers ability **VFX** (projectiles,
beams, spawned effects — D36).

Diagrams: [11-class-animation.puml](../diagrams/11-class-animation.puml).
Plan: step [34](../plan/steps/step-34-animation.md) (movement tween arrives earlier, step 12).
JSON: [02-data-model.md §7](../02-data-model.md).

---

## Concept

A creature view is a small hierarchy of part entities (from `models/<id>.json` — each part
= one `Entity3D` with meshes). A **Rig** maps `LogicalPart` names → part entities and
captures each part's **rest pose**. An **Animator** plays named **Recipes** (sequential
**Phases** of parallel **Steps**) by applying offsets *relative to the rest pose*, restoring
cleanly when done. One main channel + one additive overlay channel (hit flashes, recoil).
Recipes authored against logical parts are reusable across creatures; a rig lacking a part
skips steps targeting it.

**Board movement is NOT a recipe**: tile-to-tile motion is the path-driver tween
([world.md](world.md) actors / battle move views); the animator only adds bob/lunge/recoil
on top.

## Types (`Playground/srcs/animation/`)

```
pg::LogicalPart enum { Root, Body, Head, Front, Rear, DominantLimb, OffLimb,
                       Weapon, Jaw, Tail, WholeRig }
pg::AnimationRig (spk::Component on the creature-view root)
    std::unordered_map<LogicalPart, Entity3D*> parts;   // built from the model's parts
    rest pose per part {position, rotation, scale};     // captured at bind
    bind(modelRootEntity);  resolve(part) -> Entity3D*  // nullptr = skip
pg::AnimationStep (abstract, JSON-polymorphic)
    LogicalPart part; EasingKind easing; bool additive;
    virtual void apply(Rig&, float phaseT, PoseAccumulator&) const = 0;
    concrete: MoveLocalStep{offset}, RotateLocalStep{eulerOffset}, ScaleStep{multiplier},
              ShakeStep{amplitude, frequency}, FlashStep{color, intensity},
              WaitStep{}, SpawnVfxStep{vfxId, offset}, PlaySoundStep{soundId}
pg::AnimationPhase  { float duration; std::vector<unique_ptr<AnimationStep>> steps; }
pg::AnimationRecipe { name; bool loop; std::vector<AnimationPhase> phases; }
pg::AnimationSet    { idleRecipeName; map<string, AnimationRecipe> recipes; }  // registry item
pg::Animator (spk::Component)
    const AnimationSet* set; AnimationRig* rig;
    ChannelState main, overlay;         // recipe*, phase index, phase t
    play(name), playOverlay(name), stopOverlay()
pg::AnimatorLogic (spk::ComponentLogic<Animator>)
    per tick: advance channels; PoseAccumulator starts from rest pose, main channel applies,
    overlay adds; write final pose to part Transform3Ds; on recipe end → idle recipe (main)
    / clear (overlay). Flash writes a per-view tint uniform (render command already takes a
    color modulation — add it in this step). SpawnVfx/PlaySound fire once at phase start.
```

Easings: `linear, sineInOut, quadIn, quadOut` (a tiny `pg::easing` header; extend as
authored content needs).

## VFX (`vfx/*.json`, D36)

```
pg::VfxDefinition   // billboard (camera-facing quad, atlas frames, fps, loop) or model ref
pg::VfxInstance (spk::Component)  // position/target, kind (staticAt | projectile | beam),
                                  // lifetime, frame state
pg::VfxLogic (spk::ComponentLogic<VfxInstance>)
    // projectile: move caster→anchor at speed, despawn on arrival
    // beam: stretch a quad strip caster→anchor for duration
    // staticAt: play at a cell/part anchor (SpawnVfxStep)
    // renders in the translucent pass, billboarded toward the camera
```

## Integration points

- Mandatory recipe names per set: `Idle`, `TakeDamage`, `Death` (loader validates).
- **Ability presentation sequence** (cosmetic — resolution already happened headlessly):
  1. caster plays `casterAnimation` (if named);
  2. `travelVfx` runs caster→anchor (projectile at `speed` / beam for `duration`);
  3. each affected unit plays the ability's `targetAnimation` — **fallback rule (D36):**
     if the target's animation set lacks that recipe, play `TakeDamage` instead;
  4. damage floating text / status icons (HUD).
- Battle view hooks (step 34 wires them): `DamageEvent` on a unit → overlay `TakeDamage`
  (when no ability sequence already covers it); `UnitDefeated` → main `Death` then hide
  view; `SpawnVfxStep`/`PlaySoundStep` fire at phase start.
- Exploration: walking plays `Idle` (a `Walk` recipe when authored) — driven by actor state.

## Testing

Headless: recipe playback math (phase sequencing, loop, easing curves, additive
composition, rest-pose restoration after non-looping recipes, missing-part skip). Visual:
the placeholder cube breathing (Idle), flashing red (TakeDamage), toppling (Death).
