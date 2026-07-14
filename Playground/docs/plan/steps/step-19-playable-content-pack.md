# Step 19 - Ship a small playable JSON content pack

Implement this step only after steps 01 through 18 are complete and their handoff reports
are available. This is a content-integration step, not an excuse to add a second rules
engine or an authoring tool. The result must be three deliberately small playable battle
scenarios—Wild, Trainer, and an authored-arena Gym—plus a repeatable debug reproduction
encounter, all exercising the systems already built through strict JSON definitions.

---

# 1. Current state at the start of this step

The preceding steps should have produced all of the following:

* transactional registries for abilities, statuses, battle objects, Feat Boards, species
  (including optional inline taming profiles), AI profiles, handcrafted battle boards, and
  encounters;
* a persistent six-slot team and storage plus encounter-local `BattleUnit` projections;
* live-world, handcrafted, and fixture tactical boards, deployment, movement, targeting, effects,
  statuses, passives, traps, displacement, and a deterministic stamina scheduler;
* ordered-rule AI submitting the same commands as a player;
* exploration-to-battle lifecycle, playable overlay and HUD;
* shared condition evaluation, taming, Feat Progress, result commits, and save/reload;
* minimal fixture definitions added by earlier steps for isolated tests.

Those fixture definitions proved contracts. They are not yet a coherent game slice. There
is no guarantee that a player can discover every important mechanic, that AI profiles use
all of their rules, or that a complete wild and trainer battle remain winnable.

Before changing data, inspect the schemas and fixture IDs actually delivered by steps
02-04 and 15. The field names below are the canonical target established by those prompts.
If an earlier implementation report documents a harmless naming adaptation, use the live
schema consistently and record the mapping; do not create an alternate content-only parser.

---

# 2. Outcome of this step

Ship a compact vertical-slice catalog with three creature families:

| Species | Battle role | Systems it demonstrates |
|---|---|---|
| `cinderkit` | fast physical/magical skirmisher | movement, direct damage, burn, push, range reward |
| `mossback` | slow guardian and trap controller | armor, shields, taunt-like AI priority, trap, fortify |
| `lumenwing` | mobile support/controller | healing, cleanse, regeneration, poison, taming |

The catalog must support these visible loops without hard-coded species checks:

1. Start with a `cinderkit` and `mossback` in the player team.
2. Enter a meadow/forest Bush encounter against a tameable `lumenwing`.
3. Damage it with two authored abilities, apply and deliberately cleanse `burning`, satisfy
   its taming conditions, and see it leave as `Impressed` rather than defeated.
4. Win, recruit it, place it in the first empty team slot, and save.
5. Reload and use the recruited creature in a deterministic three-versus-three trainer
   battle that exercises healing, AI, traps, displacement, shields, DOT/HOT, cleanse, and
   defeat.
6. Complete at least one Feat node on a win and one progress contribution on a loss; see
   rewards only in the following battle.
7. Launch a non-repeatable Gym whose strict JSON selects an isolated handcrafted arena;
   verify its geometry is independent of world seed and its ordinary battle/result rules are
   identical to a live-world encounter.

Use placeholder names, tints, and shapes. The point is executable rule coverage and data
quality, not final lore, balance, models, icons, animations, VFX, or audio.

---

# 3. Scope and constraints

## 3.1 Data is the deliverable

Create or replace JSON files in the registry directories established in steps 02-04. Use
one definition per file and let the filename be the registry ID. Every cross-reference is a
string registry ID. Do not add C++ branches such as `if (speciesId == "cinderkit")`.

Keep the parser strict. Content must not rely on ignored fields, JSON comments, duplicate
keys, implicit enum aliases, or load order. Registry loads must remain transactional.

## 3.2 This is not production balance

Numbers below form a deterministic reference balance. Preserve them unless a previously
locked formula makes a scenario impossible. If adjustment is necessary, calculate and
document the relevant damage/healing sequence rather than tuning by intuition.

Do not add levels, random stat rolls, equipment, elemental weaknesses, critical hits,
accuracy, capture items, reserve swapping, or experience points.

## 3.3 No resource-creation tooling

Hand-author the small JSON set. Do not build an editor, exporter, spreadsheet importer,
schema generator, hot reload system, or Unity converter. Tooling is a later project phase.

---

# 4. Catalog manifest

The final resource tree must contain these stable IDs. Exact parent directories come from
steps 02-04; the intended layout is shown here:

```text
Playground/resources/data/
  abilities/
    basic-strike.json
    ember-dart.json
    driving-pounce.json
    brace.json
    thorn-snare.json
    shell-ram.json
    soothing-light.json
    cleansing-gust.json
    toxic-dust.json
    radiant-pulse.json
  statuses/
    burning.json
    poisoned.json
    regenerating.json
    fortified.json
    dazed.json
    moss-carapace.json
    luminous-memory.json
  battle-objects/
    thorn-snare.json
  featboards/
    cinderkit-growth.json
    mossback-growth.json
    lumenwing-growth.json
  ai/
    cinderkit-skirmisher.json
    mossback-guardian.json
    lumenwing-support.json
    debug-autoplay-basic.json
  creatures/
    cinderkit.json
    mossback.json
    lumenwing.json
  battle-boards/
    verdant-trial-arena.json
  encounter-tables/
    meadow-lumenwing-wild.json
    trainer-verdant-trio.json
    gym-verdant-trial.json
    debug-content-showcase.json
  prefabs/
    verdant-trial-arena-geometry.json
```

If step 04 intentionally used singular directory names, retain those names. Do not rename
an established registry just to match this tree.

Add all IDs to a human-readable content manifest test. The test is allowed to enumerate
the intended vertical-slice IDs; runtime code is not.

## 4.1 Retire bootstrap resources from the production root

The `training-*` files from steps 02-04 were temporary integration seeds. Once their tests
have been migrated to temporary directories or test-owned fixture JSON, remove these files
from `Playground/resources/data`:

```text
abilities/training-strike.json
abilities/training-guard.json
abilities/training-snare.json
statuses/training-guarded.json
battle-objects/training-snare-object.json
featboards/training-board.json
creatures/training-sprout.json
ai/training-aggressive.json
encounter-tables/training-wild.json
encounter-tables/debug-battle.json
```

Replace `config/new-game.json` rather than deleting it. Repoint every generated-world biome
to a surviving wild encounter. A parser/unit test may still construct a definition whose
ID begins `training-`, but no bootstrap creature, encounter, or dependent definition may
remain in the production resource transaction. This makes the manifest above exact: three
species families and four encounters, not three showcase families plus a hidden training
family. Existing world prefabs/voxels remain in their established registries; the manifest
is exact only for battle-feature additions and replacements named above.

---

# 5. Reference combat numbers

The canonical fresh-form base statistics are:

| ID | max HP | AP | MP | strength | magicPower | armor | resistance | stamina seconds | range |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| `cinderkit` | 30 | 6 | 4 | 8 | 7 | 3 | 3 | 3.50 | 0 |
| `mossback` | 42 | 6 | 3 | 6 | 4 | 8 | 6 | 5.00 | 0 |
| `lumenwing` | 32 | 7 | 4 | 4 | 9 | 3 | 6 | 4.00 | 0 |

Stamina values are positive seconds-to-fill values; lower means faster. Store them using
the decimal-to-fixed-point parsing contract from step 01. There is no random or tier-based
stat scaling. Encounter tier selects authored forms/teams only.

Use the checked integer formulas and `mitigationScale` locked in steps 01 and 09. Encode
expected exact results in tests by running the real resolver; do not duplicate or subtly
change the formula in content code, AI scoring, HUD prediction, or a test helper.

## 5.1 Ability balance table

| Ability | AP | MP | range | target/AoE | Authored behavior |
|---|---:|---:|---:|---|---|
| `basic-strike` | 2 | 0 | 1 | enemy unit, single | physical `4 + 0.65 * strength` |
| `ember-dart` | 3 | 0 | 3 | enemy unit, single, LoS | magical `3 + 0.70 * magicPower`; apply `burning` for 2 owner activations |
| `driving-pounce` | 3 | 1 | 2 | enemy unit, single | physical `2 + 0.55 * strength`; push target 1 cell away from caster |
| `brace` | 2 | 0 | 0 | self | add separate 6-point physical and magical shields; apply `fortified` for 2 owner activations |
| `thorn-snare` | 3 | 0 | 2 | empty standable cell | create `thorn-snare` battle object |
| `shell-ram` | 3 | 1 | 1 | enemy unit, single | physical `3 + 0.70 * strength`; apply `dazed` for 1.50 timeline seconds |
| `soothing-light` | 3 | 0 | 3 | self or ally, single, LoS | heal `5 + 0.75 * magicPower`; apply `regenerating` for 2 owner activations |
| `cleansing-gust` | 2 | 0 | 2 | any living unit, radius 1 | cleanse every status tagged `harmful` from each affected unit |
| `toxic-dust` | 3 | 0 | 3 | cell anchor, radius 1, LoS | apply `poisoned` to affected enemies for 3 owner activations |
| `radiant-pulse` | 4 | 0 | 2 | ally/self anchor, radius 1 | heal affected allies `2 + 0.55 * magicPower`, once per captured target |

The decimal ratios in this table are explanatory. JSON uses the integer permille fields
from step 02 (`650`, `700`, `550`, and so on); it never stores runtime floating ratios.

All effects must declare the exact `sourceUnit`, `primaryUnit`, `affectedUnits`,
`anchorCell`, or `affectedCells` execution scope established in step 02. Resource payment
occurs once per cast, never once per affected target. `radiant-pulse` demonstrates an AoE
whose anchor and affected profiles both allow self/allies; `toxic-dust` separately
demonstrates enemy-only AoE. Do not infer either profile from target count.

`driving-pounce` does not move the caster in this slice. Its MP cost demonstrates that an
ability can consume MP. A blocked push still deals damage, appends the canonical
`UnitDisplaced{requestedDistance=1, appliedDistance=0}`, and, where step 09 emits the
diagnostic skip, uses `EffectApplicationSkipped{reason=DestinationBlocked}`. It does not
invent a parallel `DisplacementBlocked` payload or move the target.

### Canonical ability JSON mapping

Author the table through these exact step-02 values. All unspecified filter booleans are
`false`; every listed unit filter also sets `allowDefeated:false`. Preserve effect order.

* `basic-strike`: diamond range 1..1 with LoS, single area, enemy anchor and enemy affected
  filters. Effect `impact` is `damage/affectedUnits`, living source required, physical,
  base 4, strength ratio 650, magic-power ratio 0.
* `ember-dart`: diamond range 1..3 with LoS, single area, enemy/enemy filters. Effect
  `ember-hit` is magical damage to `affectedUnits` with base 3 and magic-power ratio 700;
  then `ignite` applies `burning`, one stack, for two owner activations. Both require a
  living source.
* `driving-pounce`: diamond range 1..2 with LoS, single area, enemy/enemy filters. Effect
  `pounce-hit` is physical damage to `affectedUnits`, base 2, strength ratio 550; then
  `drive-back` displaces `affectedUnits` one cell `awayFromSource`.
* `brace`: self range 0..0, single area, self/self filters. Apply `physical-guard` and
  `magical-guard` to `affectedUnits` in that order, each amount 6 for two owner activations;
  then `fortify` applies one `fortified` stack for two owner activations.
* `thorn-snare`: diamond range 1..2 with LoS, single area; anchor and affected filters allow
  only `allowEmptyCell:true`. `place-snare` is `placeObject/anchorCell`, references object
  `thorn-snare`, requires a living source, and gives the instance a 12-second timeline
  duration. Runtime placement still rejects occupied/nonstandable cells.
* `shell-ram`: diamond range 1..1 with LoS, single area, enemy/enemy filters. `ram-hit` is
  physical damage to `affectedUnits`, base 3, strength ratio 700; then `daze` applies one
  `dazed` stack for exactly 1.50 timeline seconds.
* `soothing-light`: diamond range 0..3 with LoS, single area; anchor and affected filters
  allow self/allies. `direct-comfort` heals `affectedUnits`, base 5, magic-power ratio 750;
  then `lasting-comfort` applies one `regenerating` stack for two owner activations.
* `cleansing-gust`: diamond range 0..2 without LoS, diamond area radius 1; anchor and
  affected filters allow self, allies, and enemies, but not empty cells. `cleanse-harmful`
  is `cleanse/affectedUnits`, tag list `["harmful"]`, living source required.
* `toxic-dust`: diamond range 1..3 with LoS, diamond area radius 1. The anchor filter allows
  enemies and empty cells; the affected filter allows enemies only. `spread-poison` applies
  one `poisoned` stack to `affectedUnits` for three owner activations.
* `radiant-pulse`: diamond range 0..2 with LoS, diamond area radius 1; anchor and affected
  filters allow self/allies. `shared-radiance` heals `affectedUnits`, base 2,
  magic-power ratio 550.

Every ordinary ability effect in this list authors `requiresLivingSource:true`; the only
`false` values in the pack are the delayed status-hook and trap-trigger payloads explicitly
called out below. All unmentioned strength/magic-power ratios are zero. All effect IDs
follow the content-ID grammar and are unique inside their ability. Every ability root supplies version 1,
non-empty display/description strings, a valid non-negative placeholder icon cell, the AP/
MP cost in the table, and no unknown field.

## 5.2 Status semantics

| Status | Classification | Stack policy | Hooks/modifiers |
|---|---|---|---|
| `burning` | harmful, removable | refresh duration, max 1 | owner activation end: 3 magical HP damage, source credited |
| `poisoned` | harmful, removable | replace stacks and refresh duration, max 1 | owner activation start: 2 magical HP damage |
| `regenerating` | beneficial, removable | refresh duration, max 1 | owner activation start: heal 3 |
| `fortified` | beneficial, removable | refresh duration, max 1 | `armor +3`, `resistance +3` while present |
| `dazed` | harmful, removable | refresh duration, max 1 | stunned; timeline duration only |
| `moss-carapace` | beneficial default passive | permanent, max 1 | `armor +1`; after damage is taken, grant a 1-point physical shield for 1 owner activation if bearer remains alive |
| `luminous-memory` | beneficial unlockable passive | permanent, max 1 | `magicPower +1` while present |

Encode classification through the step-02 status tags: harmful statuses include
`harmful` and `removable`, beneficial statuses include `beneficial` and `removable`, and
`dazed` additionally includes the reserved `stun` tag. The two permanent definitions use
`beneficial` and `passive`, but omit `removable`. `cleansing-gust` uses the generic
`cleanse` payload with tag `harmful`, so it removes every matching status in stable status-
instance order. Do not add content-specific cleanse behavior.

The `burning` or `poisoned` source may already be defeated when delayed damage resolves;
credit uses the copied stable source ID and does not dereference a dead object. Both use
the magical channel and the mitigation/shield rules from step 09. Do not invent a hidden
true-damage channel for this content pack.

`dazed` must never use an owner-turn duration because a stunned unit cannot reliably spend
such a duration. Its 1.50 seconds advance on the deterministic stamina timeline.

### Canonical status JSON mapping

Every status root is version 1 and has the exact nested object
`"stacking":{"maxStacks":1,"reapply":"replaceStacks","durationRefresh":"replace"}`.
Those three fields are not legal at the status root. Preserve the following tags, modifier
IDs, hook IDs, and nested effect IDs so event/replay diagnostics stay stable:

* `burning`: tags `harmful`, `removable`, `fire`; no modifiers; hook `burn-at-end` on
  `activationEnd`; nested `burn-damage` is magical damage to `affectedUnits`, base 3, both
  ratios zero, and `requiresLivingSource:false`.
* `poisoned`: tags `harmful`, `removable`, `poison`; no modifiers; hook `poison-at-start`
  on `activationStart`; nested `poison-damage` has the same scope/liveness/ratios as burn,
  with magical base 2.
* `regenerating`: tags `beneficial`, `removable`, `regeneration`; hook `regenerate-at-start`
  on `activationStart`; nested `regeneration-heal` heals `affectedUnits`, base 3, both
  ratios zero, with `requiresLivingSource:false`.
* `fortified`: tags `beneficial`, `removable`, `guard`; modifiers `fortified-armor` and
  `fortified-resistance` add 3 to the corresponding stat, not per stack; no hooks.
* `dazed`: tags `harmful`, `removable`, `stun`; no modifiers or hooks. Its legality comes
  from the timeline duration on `shell-ram`'s apply effect.
* `moss-carapace`: tags `beneficial`, `passive`, `carapace`; modifier `carapace-armor` adds
  1 armor; hook `carapace-after-hit` on `afterDamageTaken`; nested `carapace-shield` applies
  a physical shield of 1 to `sourceUnit` for one owner activation and requires a living
  source.
* `luminous-memory`: tags `beneficial`, `passive`, `memory`; modifier
  `memory-magic-power` adds 1 magicPower; no hooks.

Passive definitions do not encode `infinite` in their status root. Species projection
applies entries from `defaultPassives`/derived unlocked passives as infinite statuses, per
steps 04 and 10. No ability may apply either passive in this pack.

## 5.3 Battle object semantics

`thorn-snare` occupies one cell but does not block movement or line of sight. It has one
charge and belongs to its creator's side. When an opposing unit successfully enters its
cell:

1. append a `BattleObjectTriggered` event;
2. apply a current-pool MP change of `-1`, clamped at zero;
3. deal 4 physical damage credited to the creator;
4. consume the charge and append `BattleObjectRemoved`;
5. continue the already validated movement command only according to step 10's locked
   enter-hook ordering.

After the trap's MP loss, step 08 rechecks affordability before the next planned cell. If
current MP is below that cell's positive cost, the move ends successfully on the snare cell
with an aggregate containing only the applied prefix; it never underflows or walks the
stale remainder for free.

Allies may cross without triggering it. Re-entering after removal does nothing. A unit
displaced onto the cell uses the same enter hook as a normal move. The object must not
trigger on placement during initial deployment.

Author the object as version 1 with tags `trap` and `snare`, both blocking flags false, and
one trigger `snare-on-enter` for `unitEnteredCell`. Its filter allows enemies only,
`maxTriggers` is 1, and `removeWhenExhausted` is true. Trigger effects, in order, are
`snare-movement-loss` (`changeResource/primaryUnit`, movementPoints delta -1,
`requiresLivingSource:false`) and `snare-hit` (`damage/primaryUnit`, physical base 4, zero
ratios, `requiresLivingSource:false`). This order lets the resource event remain meaningful
even when the subsequent damage defeats the mover. The placement effect, not this object
root, owns the 12-second instance duration.

---

# 6. Species definitions

Each species definition must contain display metadata, base form, derived form choices,
base stats, initial abilities, passive references if any, a Feat Board, optional inline
taming profile, and placeholder rendering metadata. AI is intentionally not a species
field: each encounter member selects an AI behaviour so the same species can fill different
encounter roles. Do not put mutable Feat completion or current battle state in species JSON.

## 6.1 `cinderkit`

Use display name `Cinderkit`, a warm orange tint, and a compact placeholder scale. Initial
abilities are `basic-strike`, `ember-dart`, and `driving-pounce`. Use
`cinderkit-growth` for progression; encounter members in this pack assign
`cinderkit-skirmisher` for non-player control.

Its `base` form is the only tier-0 form. The two evolved forms below are tier 1; every form
provides a valid RGBA tint and scale-permille placeholder block.

Its two authored evolved forms are cosmetic/stateless identities derived from mutually
exclusive Feat rewards:

* `flaretail`: gold presentation/identity selected by the branch whose separate reward
  unlocks `radiant-pulse`;
* `ashprowler`: dark-red presentation/identity selected by the branch whose separate reward
  grants `range +1`.

Do not copy base stats into save data. The active form plus completed one-time rewards
derive the current definition snapshot.

## 6.2 `mossback`

Use display name `Mossback`, a deep green tint, and a broad placeholder scale. Initial
abilities are `basic-strike`, `brace`, `thorn-snare`, `shell-ram`, and
`cleansing-gust`. The cleanse is deliberately available to the starter roster so the
wild-taming sequence below is reachable without a guest unit or hidden encounter action. Use
`mossback-growth`; encounter members assign `mossback-guardian`. It is not tameable in this
slice. Its default passive list contains `moss-carapace`.

Its `base` form is tier 0 and both evolved forms below are tier 1.

Forms:

* `ironbloom`: presentation/identity selected by the same branch whose adjacent reward
  grants the permanent authored `armor +2` derived modifier;
* `briarfort`: presentation/identity selected by the same branch whose adjacent reward
  unlocks `soothing-light` to demonstrate a cross-role branch.

## 6.3 `lumenwing`

Use display name `Lumenwing`, a cyan-white tint, and a tall/light placeholder scale.
Initial abilities are `basic-strike`, `soothing-light`, `cleansing-gust`, and
`toxic-dust`. Use `lumenwing-growth` and the inline taming profile documented in section 9;
encounter members assign `lumenwing-support`.

Its `base` form is tier 0 and both evolved forms below are tier 1.

Forms:

* `dawnwing`: presentation/identity selected by the branch whose separate reward unlocks
  `radiant-pulse`;
* `mirewing`: presentation/identity selected by the branch whose separate reward unlocks
  `thorn-snare`.

Only wild encounter spawns run the taming profile. A player-owned Lumenwing does not retain
an active taming tracker.

---

# 7. AI profiles

AI rules are ordered data interpreted by the generic rule engine from step 11. Each
profile needs an explicit fallback that ends the activation. It may use generic selectors
from step 04 such as `self`, `nearestEnemy`, `lowestHealthAlly`, and `bestArea`; it may not
name a particular battle-unit instance or invent a selector/score expression outside that
closed schema.

## 7.1 `cinderkit-skirmisher`

Use this order:

1. `burn-lowest`: when `ember-dart` is affordable and `lowestHealthEnemy` does not have `burning`, try to
   cast it with a `unit/lowestHealthEnemy` anchor.
2. `pounce-nearest`: when `driving-pounce` is affordable and `nearestEnemy` is at distance at most 2, try to
   cast it with a `unit/nearestEnemy` anchor.
3. `strike-adjacent`: when `basic-strike` is affordable and `nearestEnemy` is at distance at most 1, try to
   cast it with a `unit/nearestEnemy` anchor.
4. `approach`: unconditionally try `moveToward nearestEnemy` using at most 2 MP.
5. `finish`: unconditionally `endTurn`.

## 7.2 `mossback-guardian`

Use this order:

1. `brace-low`: when self HP ratio is at most 0.60 and `brace` is affordable, cast it at `selfCell`.
2. `cleanse-self`: when self has tag `harmful` and `cleansing-gust` is affordable, cast at `selfCell`.
3. `cleanse-ally`: when `lowestHealthAlly` has tag `harmful` and `cleansing-gust` is affordable, try to cast
   it with a `unit/lowestHealthAlly` anchor.
4. `ram-nearest`: when `shell-ram` is affordable and `nearestEnemy` is at distance at most 1, try to cast
   it with a `unit/nearestEnemy` anchor.
5. `set-snare`: when `thorn-snare` is affordable, try to cast it using
   `nearestLegalCellTo/nearestEnemy`.
6. `strike-adjacent`: when `basic-strike` is affordable and `nearestEnemy` is at distance at most 1, try to
   cast it with a `unit/nearestEnemy` anchor.
7. `approach`: unconditionally try `moveToward nearestEnemy` using all available MP.
8. `finish`: unconditionally `endTurn`.

## 7.3 `lumenwing-support`

Use this order:

1. `cleanse-ally`: when `lowestHealthAlly` has tag `harmful` and `cleansing-gust` is affordable, try to cast
   it with a `unit/lowestHealthAlly` anchor.
2. `heal-ally`: when `lowestHealthAlly` HP ratio is at most 0.65 and `soothing-light` is affordable, try
   to cast it with a `unit/lowestHealthAlly` anchor.
3. `poison-area`: when `toxic-dust` is affordable, try to cast it with `bestArea/enemies`.
4. `strike-adjacent`: when `basic-strike` is affordable and `nearestEnemy` is at distance at most 1, try to
   cast it with a `unit/nearestEnemy` anchor.
5. `retreat`: unconditionally try `moveAway nearestEnemy` using at most 2 MP.
6. `finish`: unconditionally `endTurn`.

Deliberately omit a self-cleanse rule from this profile. The wild Lumenwing acts before the
starter Mossback on the first timeline cycle; self-cleansing `burning` would make the
authored player-source cleanse taming condition unreachable. The ability itself still
permits self targeting for the recruited player's manual use, and trainer Lumenwing can
cleanse an ally.

## 7.4 `debug-autoplay-basic`

This is the explicit player-side profile used only when the developer opts into step 12's
debug autoplay switch. It is data, not a hard-coded default. Every shipped species owns
`basic-strike`, so it remains valid for the complete mandatory roster. Author this exact
file:

```json
{
  "version": 1,
  "displayName": "Debug Autoplay Basic",
  "rules": [
    {
      "id": "strike-nearest",
      "conditions": [
        { "type": "abilityAffordable", "ability": "basic-strike" }
      ],
      "decision": {
        "type": "castAbility",
        "ability": "basic-strike",
        "anchor": { "type": "unit", "selector": "nearestEnemy" }
      }
    },
    {
      "id": "approach",
      "conditions": [],
      "decision": {
        "type": "moveToward",
        "selector": "nearestEnemy",
        "maximumMovementPoints": 0
      }
    },
    {
      "id": "finish",
      "conditions": [],
      "decision": { "type": "endTurn" }
    }
  ]
}
```

Do not assign this profile to an encounter member. With `--battle-autoplay
debug-autoplay-basic`, the development driver first performs ordinary command-driven
canonical deployment through `PlaceUnit`/`ConfirmDeployment`, then places the player side
under this profile; without the option, deployment and player activations remain manual. The
enemy still uses each member's own encounter-authored profile. Validate the requested profile
before consuming an encounter ordinal, deriving seeds, or queuing a mode transition.

Express decimal health ratios as integer permille (`600`, `650`) in JSON. Empty condition
arrays on the movement/end rules are the schema's unconditional AND identity. Every cast
rule still validates ability ownership, AP/MP, target profiles, range, LoS, and effect
legality through the shared command gateway. If a matching decision has no legal command,
continue to the next authored rule. After a successful command, re-evaluate from rule 1.
Enforce the command cap and state-digest no-progress guard from step 11.

---

# 8. Feat Boards

Each board has a free root that is complete on a fresh creature. Node IDs are local stable
IDs, saved as `(boardId, nodeId)`. All progress is event-derived through the shared
condition engine. Counts below are intentionally reachable in one to three showcase
battles.

## 8.1 `cinderkit-growth`

```text
root
  -> warm-up: deal at least 18 total damage across fights
      -> flare-path: apply burning 3 times [exclusive group: cinder-path]
      -> ash-path: move at least 8 cells and cast Driving Pounce 2 times [exclusive group: cinder-path]
```

Rewards:

* `warm-up`: unlock `brace`;
* `flare-path`: separate rewards select form `flaretail` and unlock `radiant-pulse`;
* `ash-path`: separate rewards select form `ashprowler` and add `range +1`.

If both exclusive nodes become eligible from the same result, choose the first eligible
node in authored board order and record the rejected alternative as locked. Do not rely on
object/map ordering.

## 8.2 `mossback-growth`

```text
root
  -> shelter: absorb at least 12 shield damage
      -> iron-path: finish 2 fights still active [exclusive group: moss-path]
      -> briar-path: cast Thorn Snare 3 times [exclusive group: moss-path]
```

Rewards:

* `shelter`: permanent derived `maxHealth +4`;
* `iron-path`: separate rewards select form `ironbloom` and add derived `armor +2`;
* `briar-path`: separate rewards select form `briarfort` and unlock `soothing-light`.

## 8.3 `lumenwing-growth`

```text
root
  -> caretaker: restore at least 18 effective HP
      -> dawn-path: cleanse 3 harmful statuses [exclusive group: lumen-path]
      -> mire-path: successfully apply poison 4 times [exclusive group: lumen-path]
```

Rewards:

* `caretaker`: unlock passive `luminous-memory`, whose modifier derives `magicPower +1`;
* `dawn-path`: separate rewards select form `dawnwing` and unlock `radiant-pulse`;
* `mire-path`: separate rewards select form `mirewing` and unlock `thorn-snare`.

Only effective healing, shield absorption, successful status applications/cleanses,
actual movement, and actual displacement count. Requested amounts and invalid commands do
not count. Progress belongs only to deployed player creatures, including defeated ones,
and commits on both victory and loss as established in steps 17-18.

## 8.4 Canonical board graph metadata

All three roots use ID `root`, position `[0,0]`, kind `root`, no requirements/rewards, and
one symmetric edge to the first node. Use these exact remaining node fields:

| Board | Node | Position | Kind | Symmetric neighbours | min completed | from form | exclusive group |
|---|---|---|---|---|---:|---|---|
| `cinderkit-growth` | `warm-up` | `[1,0]` | `ability` | `root`, `flare-path`, `ash-path` | 0 | omitted | omitted |
| `cinderkit-growth` | `flare-path` | `[2,-1]` | `evolution` | `warm-up` | 1 | `base` | `cinder-path` |
| `cinderkit-growth` | `ash-path` | `[2,1]` | `evolution` | `warm-up` | 1 | `base` | `cinder-path` |
| `mossback-growth` | `shelter` | `[1,0]` | `stats` | `root`, `iron-path`, `briar-path` | 0 | omitted | omitted |
| `mossback-growth` | `iron-path` | `[2,-1]` | `evolution` | `shelter` | 1 | `base` | `moss-path` |
| `mossback-growth` | `briar-path` | `[2,1]` | `evolution` | `shelter` | 1 | `base` | `moss-path` |
| `lumenwing-growth` | `caretaker` | `[1,0]` | `passive` | `root`, `dawn-path`, `mire-path` | 0 | omitted | omitted |
| `lumenwing-growth` | `dawn-path` | `[2,-1]` | `evolution` | `caretaker` | 1 | `base` | `lumen-path` |
| `lumenwing-growth` | `mire-path` | `[2,1]` | `evolution` | `caretaker` | 1 | `base` | `lumen-path` |

The root's `neighbours` array contains its first node, and every listed reverse edge must
appear in JSON. Use `minimumCompletedNodes` exactly as shown. Omit—not null-fill—optional
`fromForm`/`exclusiveGroup` fields where the table says omitted.

## 8.5 Canonical condition and reward objects

Every board root is version 1 with `rootNode:"root"`. Supply non-empty display text for
boards/nodes and use these exact condition/reward payloads. The compact JSON below is a list
of each node's `requirements` followed by `rewards`; it uses every mandatory field from the
patched step-03 schema, including `windowActor`.

### Cinderkit nodes

```json
{
  "warm-up": {
    "requirements": [
      {"id":"deal-eighteen","type":"damage","description":"Deal 18 effective HP damage over the game.","window":"game","windowActor":"any","requiredWindowCount":1,"windowMode":"cumulative","actor":"subject","role":"source","counterpart":"opponentTeam","kind":"any","component":"health","targetHadShield":"any","sourceAbilities":[],"aggregation":"sum","comparison":"atLeast","threshold":18}
    ],
    "rewards": [
      {"id":"learn-brace","type":"unlockAbility","ability":"brace"}
    ]
  },
  "flare-path": {
    "requirements": [
      {"id":"ignite-three","type":"status","description":"Apply Burning three times.","window":"game","windowActor":"any","requiredWindowCount":1,"windowMode":"cumulative","actor":"subject","role":"source","counterpart":"opponentTeam","action":"applied","statuses":["burning"],"tags":[],"aggregation":"count","comparison":"atLeast","threshold":3}
    ],
    "rewards": [
      {"id":"become-flaretail","type":"changeForm","form":"flaretail"},
      {"id":"learn-radiant-pulse","type":"unlockAbility","ability":"radiant-pulse"}
    ]
  },
  "ash-path": {
    "requirements": [
      {"id":"travel-eight","type":"movement","description":"Move eight cells voluntarily in one fight.","window":"fight","windowActor":"any","requiredWindowCount":1,"windowMode":"cumulative","actor":"subject","movement":"voluntary","direction":"any","aggregation":"sum","comparison":"atLeast","threshold":8},
      {"id":"pounce-twice","type":"abilityCast","description":"Affect enemies with Driving Pounce twice in one fight.","window":"fight","windowActor":"any","requiredWindowCount":1,"windowMode":"cumulative","actor":"subject","abilities":["driving-pounce"],"sameAbility":false,"affected":"opponentTeam","minimumAffectedUnits":1,"minimumTargetDistance":0,"maximumTargetDistance":64,"comparison":"atLeast","threshold":2}
    ],
    "rewards": [
      {"id":"become-ashprowler","type":"changeForm","form":"ashprowler"},
      {"id":"extend-range","type":"bonusStat","stat":"range","amount":1}
    ]
  }
}
```

### Mossback nodes

```json
{
  "shelter": {
    "requirements": [
      {"id":"absorb-twelve","type":"shield","description":"Absorb 12 damage with shields.","window":"game","windowActor":"any","requiredWindowCount":1,"windowMode":"cumulative","actor":"subject","role":"target","counterpart":"any","action":"absorbed","kind":"any","aggregation":"sum","comparison":"atLeast","threshold":12}
    ],
    "rewards": [
      {"id":"grow-health","type":"bonusStat","stat":"maxHealth","amount":4}
    ]
  },
  "iron-path": {
    "requirements": [
      {"id":"survive-two-fights","type":"battleOutcome","description":"Finish two fights while active.","window":"fight","windowActor":"any","requiredWindowCount":2,"windowMode":"cumulative","outcome":"completed","requireSubjectActive":true}
    ],
    "rewards": [
      {"id":"become-ironbloom","type":"changeForm","form":"ironbloom"},
      {"id":"harden-armor","type":"bonusStat","stat":"armor","amount":2}
    ]
  },
  "briar-path": {
    "requirements": [
      {"id":"place-three-snares","type":"abilityCast","description":"Cast Thorn Snare three times over the game.","window":"game","windowActor":"any","requiredWindowCount":1,"windowMode":"cumulative","actor":"subject","abilities":["thorn-snare"],"sameAbility":false,"affected":"any","minimumAffectedUnits":0,"minimumTargetDistance":0,"maximumTargetDistance":64,"comparison":"atLeast","threshold":3}
    ],
    "rewards": [
      {"id":"become-briarfort","type":"changeForm","form":"briarfort"},
      {"id":"learn-soothing-light","type":"unlockAbility","ability":"soothing-light"}
    ]
  }
}
```

### Lumenwing nodes

```json
{
  "caretaker": {
    "requirements": [
      {"id":"restore-eighteen","type":"healing","description":"Restore 18 effective HP to allies.","window":"game","windowActor":"any","requiredWindowCount":1,"windowMode":"cumulative","actor":"subject","role":"source","counterpart":"subjectAllies","sourceAbilities":[],"aggregation":"sum","comparison":"atLeast","threshold":18}
    ],
    "rewards": [
      {"id":"gain-luminous-memory","type":"unlockPassive","status":"luminous-memory"}
    ]
  },
  "dawn-path": {
    "requirements": [
      {"id":"cleanse-three","type":"status","description":"Remove three harmful statuses from the team.","window":"game","windowActor":"any","requiredWindowCount":1,"windowMode":"cumulative","actor":"subject","role":"source","counterpart":"subjectTeam","action":"removed","statuses":[],"tags":["harmful"],"aggregation":"count","comparison":"atLeast","threshold":3}
    ],
    "rewards": [
      {"id":"become-dawnwing","type":"changeForm","form":"dawnwing"},
      {"id":"learn-radiant-pulse","type":"unlockAbility","ability":"radiant-pulse"}
    ]
  },
  "mire-path": {
    "requirements": [
      {"id":"poison-four","type":"status","description":"Apply Poisoned four times.","window":"game","windowActor":"any","requiredWindowCount":1,"windowMode":"cumulative","actor":"subject","role":"source","counterpart":"opponentTeam","action":"applied","statuses":["poisoned"],"tags":[],"aggregation":"count","comparison":"atLeast","threshold":4}
    ],
    "rewards": [
      {"id":"become-mirewing","type":"changeForm","form":"mirewing"},
      {"id":"learn-thorn-snare","type":"unlockAbility","ability":"thorn-snare"}
    ]
  }
}
```

These compact objects are documentation mappings, not an alternate dynamic-key schema:
each key becomes the ordinary node object in the ordered `nodes` array using graph metadata
from section 8.4. Preserve node order `root`, first node, negative-Y branch, positive-Y
branch so exclusive simultaneous completion uses the documented first branch.

---

# 9. Taming profile

`lumenwing-curiosity` is the documentation name for the inline `tamingProfile` embedded in
`lumenwing.json`; step 03 deliberately does not create a separate taming-profile registry.
It has three visible top-level conditions. All must be satisfied during the current fight
while the wild Lumenwing is still active:

1. receive at least 1 effective HP damage from player-owned `basic-strike`;
2. receive at least 1 effective magical HP damage from player-owned `ember-dart`;
3. complete an `allOf` whose children observe player-applied `burning` on the subject and
   a later player-sourced removal of `burning` from the subject.

This deliberately requires helping the opponent. The exact starter roster can satisfy it:
Cinderkit uses `basic-strike` and `ember-dart`, then Mossback uses its initial
`cleansing-gust` to remove `burning` before Lumenwing is defeated. All three conditions use
actual typed events and stable source/ability/status IDs. Invalid casts, zero applied
damage, a cleanse that removes nothing, or an enemy-side cleanse do not count. Do not add a
guest unit, encounter modifier, or hidden command.

Embed this exact profile object in `lumenwing.json`:

```json
{
  "conditions": [
    {
      "id": "feel-basic-strike",
      "type": "damage",
      "description": "Take effective HP damage from Basic Strike.",
      "window": "ability",
      "windowActor": "opponentTeam",
      "requiredWindowCount": 1,
      "windowMode": "cumulative",
      "actor": "opponentTeam",
      "role": "source",
      "counterpart": "subject",
      "kind": "physical",
      "component": "health",
      "targetHadShield": "any",
      "sourceAbilities": ["basic-strike"],
      "aggregation": "sum",
      "comparison": "atLeast",
      "threshold": 1
    },
    {
      "id": "feel-ember-dart",
      "type": "damage",
      "description": "Take effective magical HP damage from Ember Dart.",
      "window": "ability",
      "windowActor": "opponentTeam",
      "requiredWindowCount": 1,
      "windowMode": "cumulative",
      "actor": "opponentTeam",
      "role": "source",
      "counterpart": "subject",
      "kind": "magical",
      "component": "health",
      "targetHadShield": "any",
      "sourceAbilities": ["ember-dart"],
      "aggregation": "sum",
      "comparison": "atLeast",
      "threshold": 1
    },
    {
      "id": "accept-mercy",
      "type": "allOf",
      "description": "Be burned and then cleansed by the opposing team.",
      "children": [
        {
          "id": "burning-received",
          "type": "status",
          "description": "Receive Burning from the opposing team.",
          "window": "ability",
          "windowActor": "opponentTeam",
          "requiredWindowCount": 1,
          "windowMode": "cumulative",
          "actor": "opponentTeam",
          "role": "source",
          "counterpart": "subject",
          "action": "applied",
          "statuses": ["burning"],
          "tags": [],
          "aggregation": "count",
          "comparison": "atLeast",
          "threshold": 1
        },
        {
          "id": "burning-cleansed",
          "type": "status",
          "description": "Have Burning removed by the opposing team.",
          "window": "ability",
          "windowActor": "opponentTeam",
          "requiredWindowCount": 1,
          "windowMode": "cumulative",
          "actor": "opponentTeam",
          "role": "source",
          "counterpart": "subject",
          "action": "removed",
          "statuses": ["burning"],
          "tags": [],
          "aggregation": "count",
          "comparison": "atLeast",
          "threshold": 1
        }
      ]
    }
  ]
}
```

Because battle units start without `burning`, the removed event cannot precede the applied
event in this exact encounter. If future content permits pre-battle statuses, add an
explicit ordered-event condition type before relying on the same personality pattern; do
not make `allOf` itself order-sensitive.

When the final condition becomes true, the Taming system batch emits the changed
`TamingConditionStateChanged` entry with its copied previous/new material state (including
the final advancement change), followed by `UnitImpressed` and
`UnitRemoved{reason=Impressed}` in the step-16 order. The wild creature grants no defeat
credit and becomes a provisional recruit. It joins the team only after victory and the
result transaction. If the wild creature is defeated first, taming fails for that fight.

Because this reachability issue is easy to miss, add a test that builds the exact starter
roster and exact encounter JSON and proves at least one legal command sequence can satisfy
all taming conditions.

---

# 10. Encounter definitions

## 10.1 `meadow-lumenwing-wild`

Purpose: first recruit and taming tutorial without bespoke tutorial code.

* kind `wild`, `allowsTaming:true`, `repeatable:true`, and
  `triggerChancePermille:1000` for a guaranteed vertical-slice encounter;
* tier 0 contains team `curious-lumenwing`, weight 1, with member `lumenwing-a`: species
  `lumenwing`, root-only progress (`completedFeatNodes:[]`), and explicit
  `ai:"lumenwing-support"`;
* board policy: `liveWorld`, size `[11, 11]`, deployment depth 2, and deterministic `byLine`
  opponent placement from the normal encounter schema;
* player deployment: ordinary manual deployment from the current team (the new-game team
  has two members at this point);
* enemy deployment: the `byLine` policy uses `rowsFromEnemyEdge:0` and `order:"centerOut"`;
* result: normal victory/loss plus provisional recruit on impressed removal;
* identity: derived from world seed, Bush world cell, encounter ID, and the persisted
  encounter serial/ordinal;
* repeatability: wild encounters do not create cleared IDs. The step-12 arrival/cell guard
  prevents an immediate retrigger without movement; a later intentional arrival may roll a
  new encounter. Debug launch remains repeatable and does not mutate cleared-world state.

If the board cannot provide enough legal cells, battle construction fails atomically and
returns to exploration with a diagnostic; it must not silently relocate to unrelated
terrain.

## 10.2 `trainer-verdant-trio`

Purpose: complete tactical showcase after recruiting Lumenwing.

Enemy roster order is exactly:

1. member `guardian-a`: `mossback`, `ai:"mossback-guardian"`, root-only/base form;
2. member `skirmisher-a`: `cinderkit`, `ai:"cinderkit-skirmisher"`, root-only/base form;
3. member `support-a`: `lumenwing`, `ai:"lumenwing-support"`, root-only/base form.

Tier 0 contains only team `verdant-trio`, weight 1. The encounter is `trainer`,
`allowsTaming:false`, `repeatable:false`, and uses `triggerChancePermille:1000` as the
strict schema value even though named trainer launch does not roll it. Use the `liveWorld`
policy with size `[13, 13]`, deployment depth 2, and `byLine` placement using
`rowsFromEnemyEdge:0`/`order:"centerOut"`. The explicit roster order participates in scheduler tie
breaking after side priority. The player manually places every non-empty current team
member, as required by step 06; the expected showcase team has three after recruitment.
Victory requires defeating all three;
no wild recruit is created.

## 10.3 `gym-verdant-trial` and `verdant-trial-arena`

Purpose: prove the GDD's special-battle path with a real authored arena, without bespoke Gym
logic. Add the geometry prefab first, using the existing strict prefab schema and registered
`meadow-block` voxel:

```json
{
  "version": 1,
  "palette": { "g": "meadow-block" },
  "fill": [
    { "min": [0, 0, 0], "max": [12, 0, 12], "voxel": "g" }
  ],
  "anchors": [
    { "name": "center", "at": [6, 0, 6] }
  ]
}
```

Save it as `prefabs/verdant-trial-arena-geometry.json`. Then author the complete board file
`battle-boards/verdant-trial-arena.json`:

```json
{
  "version": 1,
  "displayName": "Verdant Trial Arena",
  "size": [13, 3, 13],
  "geometryPrefab": "verdant-trial-arena-geometry",
  "playerApproach": "positiveZ",
  "deploymentDepth": 2
}
```

Finally author this encounter shape, retaining the exact member fields and strict keys from
step 04:

```json
{
  "version": 1,
  "displayName": "Verdant Trial",
  "kind": "gym",
  "allowsTaming": false,
  "repeatable": false,
  "triggerChancePermille": 1000,
  "board": {
    "type": "handcrafted",
    "definition": "verdant-trial-arena",
    "opponentPlacement": {
      "type": "byLine",
      "rowsFromEnemyEdge": 0,
      "order": "centerOut"
    }
  },
  "tiers": [
    {
      "tier": 0,
      "teams": [
        {
          "id": "verdant-trial-team",
          "displayName": "Verdant Trial Team",
          "weight": 1,
          "members": [
            {
              "id": "trial-guardian",
              "species": "mossback",
              "ai": "mossback-guardian",
              "completedFeatNodes": []
            },
            {
              "id": "trial-skirmisher",
              "species": "cinderkit",
              "ai": "cinderkit-skirmisher",
              "completedFeatNodes": []
            },
            {
              "id": "trial-support",
              "species": "lumenwing",
              "ai": "lumenwing-support",
              "completedFeatNodes": []
            }
          ]
        }
      ]
    }
  ]
}
```

This Gym reuses the trio to keep the catalog small; its distinct responsibility is board
source and lifecycle coverage. Launch it through `--debug-battle gym-verdant-trial`, but
preserve `kind:gym`: named development launch is only a trigger route and must not rewrite
encounter semantics. The first victory clears the stable encounter ID. A second launch is
suppressed before ordinal/RNG consumption and before any transition. World cell remains the
exploration return origin, but `BoardData` source is Handcrafted, presentation uses the
isolated arena, and changing world seed cannot change arena cells or deployment zones.

## 10.4 `debug-content-showcase`

Purpose: repeatable manual reproduction through the named-debug seam.

Author it as the step-04 v1 `debug` encounter with a normal `liveWorld` `[13, 13]` board,
`byLine` placement, `allowsTaming:false`, `repeatable:true`, and
`triggerChancePermille:1000`. Team `debug-verdant-trio` has weight 1 and uses member IDs
prefixed `debug-` in the same Mossback/Cinderkit/Lumenwing order, with explicit AI fields.
Launch it through the existing F8/`--debug-battle debug-content-showcase` seam. Direct debug
launch skips the chance roll. The stable debug encounter identity plus run encounter serial
supplies its battle/team seeds; its live-world board is still derived by that policy.

The debug encounter cannot tame and never creates a cleared trainer/gym/special ID. Its
terminal player participants otherwise use the same progression/result transaction as any
completed battle; tests that need isolation operate on a copied `PlayerData`. Do not make
debug battles a separate simulation mode or add an unplanned commit bypass. Automated LoS/
elevation/high-cost/non-rectangular goldens continue to use the programmatic `BoardFixture`
from step 05 with the same headless session rules. The shipped handcrafted Gym is production
data for a flat authored arena, not a replacement for pathological test fixtures.

---

# 11. Starter roster and discovery wiring

Replace the bootstrap entries in `resources/data/config/new-game.json` through the
new-game seam established in step 04, not in `main.cpp` or the battle widget:

| Team slot | Persistent species | Initial form | Initial completed nodes |
|---:|---|---|---|
| 0 | `cinderkit` | base | root only |
| 1 | `mossback` | base | root only |
| 2-5 | empty | - | - |

Use deterministic persistent IDs generated by the new-game factory. The IDs must remain
stable after save/reload but must not be fixed global constants shared by every creature.

Set the biome-v2 `wildEncounterTable` field for meadow and forest to
`meadow-lumenwing-wild`. The encounter's tier-0 `teams` array contains its deterministic
positive-weight Lumenwing team. Preserve the generic biome/encounter selection seam from
steps 04 and 12. Point every other generated overworld biome at the same placeholder wild
table for this small slice; `training-wild` is removed with the other bootstrap content.
Interior/cave definitions keep the explicit no-wild policy from step 04. Do not make
`VoxelId == Bush` imply a specific species in C++.

Expose `trainer-verdant-trio` through the existing named debug encounter launcher. A world
trainer NPC is outside this slice; the debug route is the visible checkpoint.
Expose `gym-verdant-trial` through the same named launcher. Its non-repeatable Gym semantics,
authored board, result clear, and relaunch suppression remain active despite that development
entry route. Document optional `--battle-autoplay debug-autoplay-basic` separately; it never
changes the selected encounter or enemy AI data.

---

# 12. Content validation and reachability

Add a `PlayableContentPackTests` group that loads the real resource directory through
`Registries::loadAll`, then validates more than syntax.

## 12.1 Referential validation

Assert:

* every manifest ID exists exactly once;
* all ability/status/object/Feat-Board/taming/AI/encounter references resolve;
* the handcrafted board resolves its existing voxel/prefab references, every prefab cell is
  inside the declared board volume, and materialization provides enough standable cells for
  both mandatory deployment sides;
* every species base form and reward-selected form exists;
* every initial and unlockable ability resolves and is not duplicated in one loadout;
* every encounter-member AI cast rule names an ability that member's species can own in the
  tested form/reward state, and every debug-autoplay cast ability is owned by every shipped
  species that may appear in the mandatory player roster;
* every Feat neighbour is known, non-self, and symmetric; each graph is connected from its
  root; root/evolution/exclusive-group rules are well formed. Connected undirected cycles
  are legal and must not be rejected;
* every condition/event/reward/effect discriminator is supported by its closed factory;
* all status durations are valid for their hooks, especially timeline-only `dazed`;
* no encounter exceeds the six-creature team maximum or references a tameable trainer;
* every display name is non-empty and every placeholder tint/scale is valid.

## 12.2 Mechanical coverage test

Build a coverage table from parsed definitions and fail if the pack no longer includes at
least one reachable example of:

* physical and magical damage, including delayed status-hook damage;
* direct healing and healing over time;
* shield gain and absorption;
* beneficial and harmful status, refresh, expiry, and cleanse;
* stat modifier and stun;
* default and Feat-unlocked permanent passives, including one safe reactive hook;
* current AP/MP/resource change;
* push/displacement and blocked displacement;
* create, enter-trigger, consume, and remove battle object;
* self, ally, enemy, empty-cell, single-target, and area targeting;
* Feat count, sum, fight-end, exclusive branch, modifier, form, and ability rewards;
* taming completion and impressed removal;
* aggressive, guardian, support, and explicit debug-autoplay AI selection;
* a frozen live-world source and the handcrafted Gym source, both reaching the common
  BoardData/session rules without source-specific combat branches.

This is test metadata/inspection, not runtime behavior keyed to this catalog.

## 12.3 Solvability scripts

Use shared commands to run deterministic golden command scripts against a programmatic
step-05 `BoardFixture` populated with the showcase encounter's resolved teams. Cover at
least:

1. player win with one defeated participant;
2. player loss with Feat progress still committed in an opted-in result test;
3. Lumenwing impressed before defeat and recruited after victory;
4. trap entered by movement and by displacement;
5. shield absorbs before HP, followed by effective-heal accounting;
6. burn is cleansed before its next hook;
7. dazed expires because timeline advances while the owner cannot activate;
8. AI completes an activation without a no-progress loop;
9. a Feat reward is absent in the completing battle and present after result/reload;
10. both exclusive branches becoming eligible resolves in authored order.
11. the production `gym-verdant-trial` resolves and materializes
    `verdant-trial-arena`; two different world seeds produce identical board cells/source
    digest and zero board-construction RNG draws.

Store commands as test values or a small JSON replay fixture only if step 20's replay DTO
already exists. Do not add a second command parser just for golden tests.

## 12.4 Balance smoke simulations

Run at least 100 headless battles for each of these fixed matchup families while varying
only the deterministic battle seed:

* two-creature starter team versus wild Lumenwing;
* three-player-species mirror versus trainer trio;
* three-player-species mirror versus authored-arena Gym trio;
* AI-controlled trio versus AI-controlled trio.

The assertions are safety bounds, not desired win rates:

* every battle reaches a terminal result within scheduler/activation safety rules and the
  harness's explicit `SoakLimits`;
* no invalid numeric state, negative pool, duplicate unit ID, or occupied-cell collision;
* identical seed repeats exactly;
* AI choices remain identical for identical state because v1 has no random tie selector;
* event count stays below the test-only `SoakLimits.maxEvents`; exceeding it fails and
  reports the still-valid session without fabricating a production abort.

Do not fail on a narrow win-rate target. This slice is not production-balanced.

---

# 13. Manual verification scenarios

Run these in a development executable with the real resource root:

## Scenario A - first wild recruit

1. Start a new game and confirm exactly Cinderkit and Mossback are available.
2. Enter a meadow/forest Bush and verify the battle overlays the same terrain.
3. Deploy both creatures manually.
4. Inspect the wild Lumenwing's taming requirements in the HUD.
5. Satisfy each condition and verify progress changes only on actual events.
6. Confirm Lumenwing visibly leaves as impressed, not with defeat feedback.
7. Win, accept the result, confirm Lumenwing occupies team slot 2, save, and reload.

## Scenario B - trainer showcase

1. Launch `trainer-verdant-trio` with the three-creature team.
2. Observe each enemy profile perform its intended role.
3. Trigger a Thorn Snare, cleanse poison/burning, absorb shield damage, heal, push a unit,
   and see a daze expire on the timeline.
4. End in victory once and loss once; confirm the world and controls recover both times.
5. Confirm all mandatory deployed roster creatures gained only their relevant Feat progress
   and a storage creature gained none.

## Scenario C - progression boundary

1. Complete a node whose explicit rewards unlock an ability and/or select a form identity.
2. Confirm the reward is not available before the current battle result.
3. Commit the result and start another battle.
4. Confirm the derived form/tint/stat/loadout and save/reload stability.

## Scenario D - authored Gym arena

1. Launch `gym-verdant-trial` and verify exploration rendering is hidden while the isolated
   13x13 arena is visible in the same scene/camera.
2. Deploy the complete roster, pick cells at the arena edges, and complete a normal battle.
3. Confirm victory returns to the encounter world cell, restores exploration rendering/
   streaming/fluid/input state exactly once, and records the Gym as cleared.
4. Relaunch the same ID and confirm it is suppressed before ordinal/RNG/transition mutation.
5. In a fresh copied player state, change only world seed and confirm arena geometry,
   deployment cells, and picking remain identical.

Record which visual checks were actually performed. Automated tests cannot prove that
mask colors, text wrapping, cursor feedback, or cube readability are acceptable.

---

# 14. Expected files

Primarily add or modify:

* `Playground/resources/data/abilities/*.json`
* `Playground/resources/data/statuses/*.json`
* `Playground/resources/data/battle-objects/*.json`
* `Playground/resources/data/featboards/*.json`
* `Playground/resources/data/ai/*.json`
* `Playground/resources/data/creatures/*.json`
* `Playground/resources/data/battle-boards/*.json`
* `Playground/resources/data/encounter-tables/*.json`
* `Playground/resources/data/prefabs/verdant-trial-arena-geometry.json`
* the established biome encounter/default-roster data files;
* removal of the listed production `training-*` resources and migration of any tests that
  depended on them to test-owned fixtures;
* `Playground/tests/.../playable_content_pack_tests.cpp` and CMake registration;
* a short content catalog section in `Playground/docs/02-data-model.md` or the live
  replacement document chosen by earlier steps.

Modify C++ only to fix a demonstrated generic bug or missing validation in an already
planned contract. Any such change requires a regression test and an explanation in the
handoff. A content-specific C++ special case fails this step.

---

# 15. Documentation update

Document:

* every shipped ID and its category;
* where filename-derived IDs and cross-references are validated;
* the three species roles and the Wild, Trainer, and authored-Gym playable loops;
* how to launch the named debug encounters;
* final unattended smoke commands using `--battle-autoplay debug-autoplay-basic`, replacing
  any earlier example that named the now-removed `training-aggressive` production resource;
* the fact that numbers and lore are reference content, not final balance;
* how to add data manually without claiming that an authoring tool exists.

Keep examples synchronized with real JSON. Copy at least one complete, validated ability,
species, Feat Board, AI profile, and encounter file into documentation snippets only after
the resources pass their loader tests.

---

# 16. Non-goals

Do not add:

* a fourth species or production creature catalog;
* final lore, localization infrastructure, models, animation, VFX, sound, or icons;
* an encounter editor or any other content-generation tool;
* levels, XP, items, elements, critical hits, accuracy, equipment, capture actions, or
  reserve swapping;
* procedural ability/Feat generation;
* bespoke tutorial scripting or a trainer NPC;
* content hot reload or schema migration beyond the save policy already established;
* runtime code that switches on any ID from this content pack.

---

# 17. Acceptance checklist

This step is complete only when all statements are true:

* [ ] The real registry loader transactionally loads all manifest files with no warnings.
* [ ] Exactly three coherent species families and their documented forms exist.
* [ ] No bootstrap `training-*` definition remains in the production resource transaction;
      earlier parser tests use isolated fixtures instead.
* [ ] The ten abilities, seven statuses, one battle object, three Feat Boards, one
      handcrafted battle board plus geometry prefab, one inline taming profile, four AI
      profiles, and four encounters resolve all references.
* [ ] The exact starter roster can legally complete the exact Lumenwing taming profile.
* [ ] Wild impressed removal creates a recruit only after victory and never defeat credit.
* [ ] The trainer scenario exercises AI, traps, displacement, shields, status hooks,
      cleanse, healing, damage, defeat, and result flow through generic systems.
* [ ] The Gym uses its registered handcrafted arena, consumes no board RNG/world geometry,
      remains ordinary battle simulation, clears only on victory, and restores exploration.
* [ ] Feat rewards are derived after the result boundary and survive save/reload.
* [ ] Every production JSON category and every discriminator used by the shipped mechanics
      in section 12.2 has a real reachable example; valid but unshipped alternatives such as
      swap/teleport/line shapes remain covered by isolated strict-parser/runtime fixtures.
* [ ] Golden command scripts produce exact expected events and state digests.
* [ ] At least 100 deterministic seeds per smoke matchup terminate within safety caps.
* [ ] No runtime C++ branch names a content-pack ID.
* [ ] All earlier and new Playground tests pass.
* [ ] The wild, trainer, progression, and authored-Gym manual scenarios were run or honestly
      reported as not visually verified.

---

# 18. Handoff to step 20

Report the complete ID manifest, exact schema versions, final taming condition chosen for
starter-roster reachability, any balance adjustment and its calculated reason, smoke seed
counts/results, and visual scenarios performed. Step 20 will treat these files as the
canonical vertical slice and will harden full-system determinism, lifecycle, performance,
diagnostics, and documentation without expanding the content catalog.
