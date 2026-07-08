# 02 — Data Model & JSON Schemas

Every authored definition type: file layout, schemas, polymorphism, loading rules.
Decisions: D02 (nlohmann-json), D10 (layout & strictness), D19–D22 (naming).

---

## 1. Conventions (apply to every file)

- Location: `Playground/resources/data/<domain>/<id>.json`. The **id is the filename stem**,
  kebab-case (`grass-block.json` → `"grass-block"`). One definition per file.
- Every file starts with `"version": <int>`. Each domain has a current version (starts at 1);
  a mismatch is a **hard load error** (migrations can be added later, per-domain).
- **Strict parsing**: a missing required field, an unknown field, or a wrong type throws
  `pg::JsonError` with `file`, `json path`, and message. No silent defaults except where a
  field is explicitly documented `optional (default: X)` below.
- **Polymorphic objects** carry `"type": "<TypeName>"` (lower-camel). Each hierarchy has a
  factory map `type → parse function`; an unknown type is a hard error listing known types.
- **Cross-references are string ids** (`"ability": "ember"`), resolved to pointers at load
  time. Load order: `config → voxels → models → vfx → statuses → abilities → featboards →
  animations → ai → creatures → encounter-tables → biomes → prefabs → maps`. A dangling
  reference is a hard error at load, not at use.
- Enums are lower-camel strings: `"physical"`, `"turnStart"`, `"crossPlane"`.
- Numbers: JSON numbers; ints where the game uses ints. Vectors: arrays `[x, y]` / `[x, y, z]`.

Domains:

```
resources/data/
  config/game-rules.json          # global gameplay tunables (single file)
  voxels/*.json                   # VoxelDefinition
  models/*.json                   # creature/prop voxel models (modeling-tool output)
  vfx/*.json                      # visual effects (projectiles, beams, spawned VFX)
  statuses/*.json                 # Status
  abilities/*.json                # Ability
  featboards/*.json               # FeatBoard
  animations/*.json               # animation Sets (recipes)
  ai/*.json                       # AIBehaviour
  creatures/*.json                # CreatureSpecies
  encounter-tables/*.json         # EncounterTable
  biomes/*.json                   # BiomeDefinition
  prefabs/*.json                  # reusable voxel structures (pure content + anchors)
  maps/*.json                     # full playable spaces (markers, portals, biome)
  worldgen/default.json           # macro world-plan generation parameters
```

Save files are **not** definitions; they live in `Playground/saves/<slot>.json` (writable).

---

## 2. `config/game-rules.json`

Global constants formerly scattered as magic numbers. Loaded first, into `pg::GameRules`.

```jsonc
{
  "version": 1,
  "teamMemberCount": 6,
  "abilityBarSlots": 8,
  "maxVerticalTraversalGap": 0.5,     // world units, walkable edge height difference
  "defaultBoardSize": [11, 11],       // D13
  "deploymentStripDepth": 2,          // D13
  "minimumTurnBarDuration": 0.1,      // seconds; floor for stamina
  "mitigationScaling": 10.0,          // armor/(armor+scaling) damage reduction
  "timeEffectResistanceScaling": 10.0,
  "overlayMasks": {                   // battle-overlay category → mask atlas cell (D31)
    "deployment": [0, 0],
    "movement":   [1, 0],
    "path":       [2, 0],
    "abilityRange": [3, 0],
    "areaOfEffect": [0, 1],
    "losBlocked":   [1, 1],
    "hovered":      [2, 1],
    "invalid":      [3, 1]
  }
}
```

Structural constants are **not** config: chunk extent is a compile-time constant of the
chunk type (`pg::Chunk::Size = {16,16,16}`, D29); world height is a property of each
map/generator, not a global.

---

## 3. `voxels/<id>.json` — VoxelDefinition

```jsonc
// voxels/grass-block.json
{
  "version": 1,
  "traversal": "solid",              // "solid" (stand on) | "passable" (walk through) — D20
  "tags": ["ground", "grass"],       // free strings; "bush" marks encounter triggers
  "shape": {
    "type": "cube",                  // cube | cuboid | slab | slope | stair | crossPlane | cross
    "textures": {                    // shape-specific slots → atlas cell [col, row]
      "top": [0, 0],
      "side": [1, 0],
      "bottom": [2, 0]
    }
  }
}
```

The atlas is a single `resources/textures/voxels.png` sprite sheet with a uniform grid
(declared in `config/game-rules.json` later if it needs parameters; initially the loader
derives cell UVs from the sheet's grid — same mechanism as `spk::SpriteSheet`).

**Shape types and their texture slots** (geometry, heights and occlusion behavior per shape
are specified in [03-systems/voxel.md](03-systems/voxel.md)):

| `type` | Texture slots | Extra fields |
|---|---|---|
| `cube` | `top, bottom, side` (or `posX,negX,posZ,negZ` instead of `side`) | — |
| `cuboid` | same as `cube` | `"min"` and `"max"`: float `[x,y,z]` bounds inside `[0,1]³`; every min component must be strictly below max |
| `slab` | `top, bottom, side` | `"height"`: float 0–1 (default 0.5) |
| `slope` | `slope, back, bottom, sideLeft, sideRight` | — (rises toward local +Z) |
| `stair` | `top, riser, back, bottom, sideLeft, sideRight` | `"stepCount"`: int (default 2) |
| `crossPlane` | `plane` | — (two crossed quads, no outer shell, never occludes) |
| `cross` | `plane` | — (two centered axis-aligned quads, no outer shell, never occludes) |

Orientation/flip are **per-cell** (in maps), not per-definition.

---

## 4. `statuses/<id>.json` — Status

```jsonc
// statuses/poison.json
{
  "version": 1,
  "displayName": "Poison",
  "icon": [3, 0],                       // HUD icon atlas cell
  "tags": ["poison", "damageOverTime"],
  "hookPoint": "turnStart",             // see glossary: 10 hook points
  "effects": [
    { "type": "damage",
      "baseDamage": 3, "damageKind": "magical",
      "attackRatio": 0.0, "magicRatio": 0.25 }
  ]
}
```

A status instance on a unit additionally has a stack count and a Duration — those come from
the `applyStatus` **effect** that applied it, not from the status definition.
`stun` is a status with tag `"stun"`; the turn system checks that tag to pause turn-bar fill
(D23). Statuses granted as **passives** are applied with `duration: {"type": "infinite"}`.

**Duration object** (used by statuses, shields, board objects):

```jsonc
{ "type": "turnBased", "turns": 3 }   // or {"type":"seconds","seconds":5} or {"type":"infinite"}
```

---

## 5. `abilities/<id>.json` — Ability

```jsonc
// abilities/ember.json
{
  "version": 1,
  "displayName": "Ember",
  "icon": [0, 1],
  "cost": { "ap": 3, "mp": 0 },
  "range": {
    "shape": "circle",                 // circle | line | diagonal | self
    "min": 1,
    "max": 6,
    "requiresLineOfSight": true
  },
  "areaOfEffect": {
    "shape": "cross",                  // square | cross | circle | line
    "value": 1                         // radius / half-extent in cells; 0 = single cell
  },
  "targetProfile": "enemy",            // everything | ally | enemy | empty
  "casterAnimation": "AttackRanged",   // optional (default: none) — recipe name
  "targetAnimation": "Scorched",       // optional (default: "TakeDamage"); if the target's
                                       // animation set lacks it, falls back to TakeDamage (D36)
  "travelVfx": {                       // optional (default: none) — caster→anchor space (D36)
    "kind": "projectile",              // projectile (moves at speed) | beam (stretched for duration)
    "vfx": "fireball",                 // vfx/<id>.json
    "speed": 12.0                      // cells/s (projectile) — beams use "duration": seconds
  },
  "effects": [
    { "type": "damage",
      "baseDamage": 5, "damageKind": "magical",
      "attackRatio": 0.0, "magicRatio": 1.0 },
    { "type": "applyStatus", "status": "burn", "stackCount": 1,
      "duration": { "type": "turnBased", "turns": 2 } }
  ]
}
```

Rules text shown on ability cards is **generated from the effects**, not authored
(see [03-systems/ui-hud.md](03-systems/ui-hud.md)).

### 5.1 Effect types (polymorphic, embedded in abilities and statuses)

All effects apply per affected cell/unit within the resolved AoE
(see [03-systems/battle.md](03-systems/battle.md) for resolution order and events emitted).

| `type` | Fields (beyond `type`) | Semantics (mirrors Unity) |
|---|---|---|
| `damage` | `baseDamage:int, damageKind:"physical"\|"magical", attackRatio:float, magicRatio:float` | dmg = (base + atk·attackRatio + magic·magicRatio) · (1 − mitigation); shields of matching kind absorb first; lifesteal (physical) / omnivampirism (magical) heal caster from HP damage |
| `heal` | `baseHealing:int, attackRatio:float, magicRatio:float` | healing = (base + contributions) · (1 + bonusHealing) |
| `applyStatus` | `status:id, stackCount:int (1), duration:Duration` | adds stacks + duration to target |
| `removeStatus` | `status:id, stackCount:int (1)` | removes stacks |
| `consumeStatus` | `status:id, stackCount:int (1)` | like remove, but semantically "spent" (emits no StatusRemoved feat event distinction in v1; kept separate for authoring intent) |
| `cleanse` | `tags:[string]` | removes all statuses carrying any tag |
| `revive` | `healthRestored:int (1)` | only affects defeated units; restores computed healing (min 1) |
| `applyShield` | `kind:"physical"\|"magical", amount:int, durationTurns:int (-1 = infinite)` | adds a shield to target |
| `resourceChange` | `resource:"ap"\|"mp"\|"range", value:int (±)` | changes target's resource |
| `stealResource` | `resource:"health"\|"ap"\|"mp"\|"range"\|"turnBarTime", value:int` | removes from target, grants what was actually removed to caster |
| `displacement` | `orientation:"towardCaster"\|"awayFromCaster", force:int` | pushes/pulls cell by cell along dominant axis; stops at blockers |
| `swapPosition` | — | swaps target with the unit at the anchor cell |
| `swapPositionWithCaster` | — | swaps target and caster |
| `teleportSelf` | — | caster teleports to the anchor cell (once per cast) |
| `adjustTurnBarTime` | `delta:float (±s)` | moves target's turn-bar fill; reduced by timeEffectResistance |
| `adjustTurnBarDuration` | `delta:float (±s)` | changes target's turn-bar max (floored at minimumTurnBarDuration) |
| `placeObject` | `object:id, duration:Duration` | places an interactive object (trap…) at the affected cell |
| `removeObject` | `tags:[string]` | removes tagged objects at the affected cell |

(Unity's absolute-destination `TeleportEffect` is dropped — only `teleportSelf` is authorable;
absolute teleports were unused editor-era scaffolding.)

### 5.2 `vfx/<id>.json` — visual effects (used by `travelVfx`, `spawnVfx` steps)

```jsonc
// vfx/fireball.json
{
  "version": 1,
  "render": { "type": "billboard",     // billboard (camera-facing quad) | model (models/<id>)
              "texture": [0, 3],       // vfx atlas cell (resources/textures/vfx.png)
              "size": 0.6 },
  "animation": { "frames": [[0,3],[1,3],[2,3]], "fps": 12, "loop": true },  // optional
  "light": null                        // reserved (future glow); must be null/absent v1
}
```

A `travelVfx` of kind `projectile` spawns the vfx at the caster's cell center (+0.5 Y) and
moves it to the anchor cell at `speed`; kind `beam` stretches a quad strip textured with the
vfx between the two points for `duration` seconds. Cosmetic only (D36) — resolution never
waits on VFX.

---

## 6. `featboards/<id>.json` — FeatBoard

```jsonc
// featboards/sprout-board.json
{
  "version": 1,
  "rootNode": "5b1f4c2e-8d3a-4f6b-9c0d-2e7a1b3c4d5e",
  "nodes": [
    {
      "uuid": "5b1f4c2e-8d3a-4f6b-9c0d-2e7a1b3c4d5e",   // stable identity (D34) —
      "displayName": "Awakening",                        // generated by the tools
      "position": [0, 0],              // editor canvas coordinates
      "kind": "statsBonus",            // statsBonus | ability | passive | form
      "neighbours": ["a3e8…", "c9f1…"],                  // node uuids
      "repeatLimit": 1,                // 0 = infinitely repeatable, n = max completions
      "requirements": [],              // root: none (starts completed)
      "rewards": []
    },
    {
      "uuid": "c9f10d7b-2a45-4e8c-b1f3-6d0e9a8b7c6d",
      "displayName": "First Spark",
      "position": [1, 0],
      "kind": "ability",
      "neighbours": ["5b1f…", "e2d4…"],
      "repeatLimit": 1,
      "requirements": [
        { "uuid": "f7a2c4e6-1b3d-4a5f-8e9c-0d1b2a3c4d5e",   // per-requirement identity (D34)
          "type": "dealDamage", "scope": "fight", "requiredRepeatCount": 2,
          "requiredAmount": 30, "damageKind": "any", "sourceAbilities": [] }
      ],
      "rewards": [ { "type": "ability", "ability": "ember" } ]
    }
  ]
}
```

Unlock rule: the root node starts completed; a node is **active** (can progress) when any
neighbour is completed; requirements of a node all must complete; then rewards apply and the
node's neighbours activate. `repeatLimit`: 0 = unlimited, n = max completions. Species
reference boards by id (`"featBoard": "sprout-board"`). **UUIDs** identify nodes and
requirement entries stably (save data keys on them, D34); tools generate them
(`spk::UUID::generate()`); hand-authored seed files may use any unique string. Reward
replay runs in board node order — the editor validates that `form` nodes appear
tier-ascending (D35).

### 6.1 Battle conditions — shared core, two catalogs (D33)

Feat requirements and taming conditions are both **battle conditions**: a `type`, a `scope`
(`"ability" | "turn" | "fight" | "game"` — D21; some types lock their scope and reject the
field), `requiredRepeatCount:int (1)`, plus type-specific fields. Each type scores typed
battle events into 0–100% progress per scope window; a window reaching 100% counts one
repeat (see [03-systems/creatures-feats.md](03-systems/creatures-feats.md)).
The evaluation core (`pg::BattleCondition`) is shared; **`FeatRequirement`** (feat boards)
and **`TamingCondition`** (taming profiles) are separate factories over it — a condition
type is available where its parser is registered (shared types register in both; e.g. a
taming-only "witness" condition never pollutes feat authoring, and vice versa).

Initial catalog (registered in **both** factories unless noted; grow it freely following
the same pattern — the Unity reference's `FeatRequirementList.md` is a large idea list):

| `type` | Event consumed | Fields |
|---|---|---|
| `dealDamage` | DamageEvent (as caster) | `requiredAmount:int, damageKind:"any"\|"physical"\|"magical", sourceAbilities:[id]` |
| `takeDamage` | DamageEvent (as target) | `requiredAmount:int, damageKind` |
| `surviveHit` | HitSurvivedEvent | `requiredAmount:int` (scope-locked: ability) |
| `healHealth` | HealEvent (as caster) | `requiredAmount:int` |
| `healTarget` | HealEvent | `requiredAmount:int, target:"self"\|"ally"\|"any"` |
| `applyShield` | ShieldAppliedEvent | `requiredAmount:int, kind:"any"\|"physical"\|"magical"` |
| `applyShieldCount` | ShieldAppliedEvent | `requiredCount:int` |
| `absorbDamageWithShield` | DamageAbsorbedEvent (as target) | `requiredAmount:int` |
| `maxDamageAbsorbedInOneHit` | DamageAbsorbedEvent | `requiredAmount:int` (scope-locked: ability) |
| `shieldBroken` | ShieldBrokenEvent | `requiredCount:int, kind` |
| `applyStatusCount` | StatusAppliedEvent (as caster) | `requiredCount:int, status:id?, sourceAbilities:[id]` |
| `removeStatusCount` | StatusRemovedEvent (as caster) | `requiredCount:int, status:id?` |
| `killCount` | UnitDefeatedEvent (as killer) | `requiredCount:int, sourceAbilities:[id]` |
| `lastHit` | UnitDefeatedEvent | `requiredCount:int` |
| `winBattleCount` | BattleWonEvent | `requireUnitSurvival:bool` (scope-locked: game) |
| `consumeResources` | ResourceConsumedEvent | `resource:"ap"\|"mp", requiredAmount:int` |
| `totalDistanceTravelled` | DistanceTravelledEvent | `requiredDistance:int` |
| `displacementDealt` | DisplacementEvent (as caster) | `requiredDistance:int, orientation?` |
| `displacementReceived` | DisplacementEvent (as target) | `requiredCount:int, orientation?` |
| `teleportCount` | TeleportedEvent | `requiredCount:int` |
| `teleportDistance` | TeleportedEvent | `requiredDistance:int` |
| `turnStartPosition` | TurnStartedEvent | `target:"ally"\|"enemy"\|"anyUnit", condition:"within"\|"atLeast"\|"between", distance:int, maximumDistance:int` (scope-locked: ability) |
| `turnEndPosition` | TurnEndedEvent | same as turnStartPosition |
| `castAbilityCount` | AbilityCastEvent | `abilities:[id], requiredCount:int, targetRangeCondition:"either"\|"atLeast"\|"within", range:int` |
| `and` | children | `children:[requirement]` — completes when all children complete in the same evaluation |
| `or` | children | `children:[requirement]` — completes when any child completes |

### 6.2 FeatReward (polymorphic)

| `type` | Fields | Applies |
|---|---|---|
| `bonusStats` | `attribute:<attribute name>, value:float` | adds to the creature's Attributes (see §8 attribute names) |
| `ability` | `ability:id` | adds to ability pool |
| `removeAbility` | `ability:id` | removes from pool (used by form changes) |
| `passive` | `status:id` | adds a permanent passive status |
| `changeForm` | `form:string` | sets current form (must exist on the species) |

---

## 7. `animations/<id>.json` — Animation Set

```jsonc
// animations/basic-creature.json
{
  "version": 1,
  "idleRecipe": "Idle",
  "recipes": {
    "Idle":       { "loop": true,  "phases": [
      { "duration": 1.0, "steps": [
        { "type": "moveLocal", "part": "body", "offset": [0, 0.05, 0], "easing": "sineInOut", "additive": true } ] },
      { "duration": 1.0, "steps": [
        { "type": "moveLocal", "part": "body", "offset": [0, -0.05, 0], "easing": "sineInOut", "additive": true } ] } ] },
    "TakeDamage": { "loop": false, "phases": [
      { "duration": 0.15, "steps": [
        { "type": "shake", "part": "wholeRig", "amplitude": 0.08, "frequency": 30 },
        { "type": "flash", "part": "wholeRig", "color": [1, 0.3, 0.3, 1], "intensity": 0.8 } ] } ] },
    "Death":      { "loop": false, "phases": [
      { "duration": 0.4, "steps": [
        { "type": "rotateLocal", "part": "wholeRig", "eulerOffset": [0, 0, 90], "easing": "quadIn" },
        { "type": "scale", "part": "wholeRig", "multiplier": [1, 0.6, 1], "easing": "quadIn" } ] } ] }
  }
}
```

Mandatory recipe names per set: `Idle`, `TakeDamage`, `Death`. Abilities may reference any
recipe by name (`casterAnimation`). Step types: `moveLocal`, `rotateLocal`, `scale`, `shake`,
`flash`, `wait`, `spawnVfx`, `playSound` — fields in
[03-systems/animation.md](03-systems/animation.md). `part` = a LogicalPart
(glossary); a rig missing that part skips the step.

---

## 8. `creatures/<id>.json` — CreatureSpecies

```jsonc
// creatures/sprout.json
{
  "version": 1,
  "displayName": "Sprout",
  "attributes": {
    "health": 12, "ap": 6, "mp": 3,
    "attack": 2, "armor": 1, "armorPenetration": 0,
    "magic": 3, "resistance": 1, "resistancePenetration": 0,
    "bonusRange": 0,
    "stamina": 4.0,          // seconds to fill turn bar (lower = faster) — D19
    "staminaRate": 1.0,
    "bonusHealing": 0.0, "lifeSteal": 0.0, "omnivampirism": 0.0,
    "timeEffectResistance": 0.0
  },
  "defaultAbilities": ["tackle"],
  "featBoard": "sprout-board",
  "defaultFormId": "base",
  "forms": {
    "base":  { "displayName": "Sprout",  "tier": 0, "model": "placeholder-cube",
               "animationSet": "basic-creature", "avatar": [0, 2] },
    "blaze": { "displayName": "Blazeling", "tier": 1, "model": "placeholder-cube",
               "animationSet": "basic-creature", "avatar": [1, 2] }
  },
  "tamingProfile": {
    "conditions": [
      { "type": "dealDamage", "scope": "ability", "requiredRepeatCount": 1,
        "requiredAmount": 8, "damageKind": "magical", "sourceAbilities": [] },
      { "type": "turnEndPosition", "requiredRepeatCount": 2,
        "target": "enemy", "condition": "within", "distance": 1, "maximumDistance": 1 }
    ]
  }
}
```

- Attribute names are exactly the keys above (also the values `bonusStats.attribute` takes,
  minus `staminaRate` runtime-only modifiers being allowed too).
- `tamingProfile.conditions` use the battle-condition schema (§6.1) via the
  **TamingCondition factory** (D33) — no `uuid` needed here (taming progress is
  battle-scoped, never saved). An empty/absent `conditions` array means the species cannot
  be tamed.
- `forms.*.model` references `models/<id>.json`; `"placeholder-cube"` is a built-in model id
  that resolves to a unit cube with a single-part rig (D26).

## 9. `models/<id>.json` — voxel models (modeling-tool output; format seeded early)

```jsonc
// models/placeholder-cube.json (hand-written seed; the tool authors richer ones)
{
  "version": 1,
  "parts": [
    { "logicalPart": "wholeRig", "origin": [0, 0, 0],
      "voxels": [ { "min": [0, 0, 0], "max": [1, 1, 1], "textures": { "all": [4, 0] } } ] }
  ]
}
```

A model = named logical parts, each a set of textured boxes (voxel-style volumes) with an
origin (pivot). The rig maps `logicalPart → part entity` automatically. The voxel modeling
tool (step 25) grows this format (per-face textures, sub-voxel grids); the loader version
gates it.

## 10. `ai/<id>.json` — AIBehaviour

```jsonc
// ai/aggressive-melee.json
{
  "version": 1,
  "activeMode": "default",
  "rulesByMode": {
    "default": [
      { "conditions": [ { "type": "enemyWithinRange", "range": 1 } ],
        "decision":   { "type": "castAbility", "ability": "tackle", "target": "nearestEnemy" } },
      { "conditions": [],
        "decision":   { "type": "moveTowardNearestEnemy" } }
    ]
  }
}
```

Evaluation: top-down, first rule whose conditions are all met (AND) wins; empty conditions =
always true; if the decision produces no legal action, evaluation continues to the next rule;
if nothing is legal, end turn. Condition/decision catalogs in
[03-systems/battle.md](03-systems/battle.md) §AI.

## 11. `encounter-tables/<id>.json` — EncounterTable

```jsonc
// encounter-tables/forest-basic.json
{
  "version": 1,
  "tiers": {
    "noBadge": {
      "weightedTeams": [
        { "displayName": "lone sprout", "weight": 3,
          "team": [
            { "species": "sprout", "ai": "aggressive-melee",
              "completedNodes": [] },                // node uuids; form & stats derive from
            null, null, null, null, null             // replaying these rewards (D35)
          ] }
      ]
    },
    "oneBadge":   { "weightedTeams": [] },
    "twoBadges":  { "weightedTeams": [] },
    "threeBadges":{ "weightedTeams": [] },
    "fourBadges": { "weightedTeams": [] },
    "fiveBadges": { "weightedTeams": [] },
    "sixBadges":  { "weightedTeams": [] },
    "sevenBadges":{ "weightedTeams": [] },
    "eightBadges":{ "weightedTeams": [] },
    "postGame":   { "weightedTeams": [] }
  }
}
```

Tier selection = badge count; an empty tier falls back to the highest non-empty lower tier
(so authoring can start with `noBadge` only). `completedNodes` (node uuids) pre-completes
feat-board nodes on the spawned enemy — this is the enemy's **entire** power dial: stats,
abilities, passives *and form* all derive from replaying those nodes' rewards (D35).

## 12. `biomes/<id>.json` — BiomeDefinition

```jsonc
// biomes/forest.json
{
  "version": 1,
  "displayName": "Forest",
  "palette": {                       // voxel ids used by world realization
    "surface": "grass-block",
    "subsurface": "dirt-block",
    "deep": "stone-block",
    "flora": ["bush", "tall-grass"]
  },
  "worldgen": {
    "prefabs": {
      "scenery": [
        { "prefab": "oak-tree", "density": 0.15, "spacing": 5 },
        { "prefab": "flower-patch", "density": 6.0, "spacing": 1 }
      ]
    }
  },
  "encounterRules": [
    { "trigger": "bush",             // matched against voxel tags at the player cell
      "table": "forest-basic",
      "chancePerStep": 0.15 }
  ]
}
```

Each `worldgen.prefabs.scenery` density is the expected number of that prefab requested per
suitable world-plan cell. It is any non-negative number: `6.0` averages six requests per
cell, while `0.15` averages one request every six or seven cells. Poisson sampling keeps
the result natural and deterministic for a given seed. `spacing` is the minimum distance
between scenery centers in voxel columns; when omitted it defaults to the prefab's largest
horizontal dimension. Actual counts can be lower when spacing is saturated or around
roads, water, entities, and stairs. Scenery has no gameplay role; use ordinary multi-voxel
prefabs for trees, plants, rocks, and other biome dressing.

## 13. `prefabs/<id>.json` — reusable voxel structures

**Pure voxel content** — a house, a tree, a dungeon room — stamped into maps or generated
chunks at an anchor + orientation. No gameplay context (no biome, no spawn points); the
only non-voxel data is named **anchors**: local positions a consumer can attach meaning to
(a door cell to bind a portal to, a roof point for VFX…). Anchors are not part of the
content bounds and may be placed outside them.

Coordinates are relative to the prefab's **pivot** (optional `pivot` field, default
`[0, 0, 0]`): the cell the prefab is held by when stamped, and the fixed point of its
rotation. Any coordinate may be negative — layers below `y = 0` embed under the stamp
destination, replacing the terrain there (a house floor slab, a POI pedestal), while
`y = 0` stays the ground/walk level. There is no declared size: the bounding box is
deduced from the content, and the whole box is listed when stamping (empties carve).
Keep embedded layers fully filled or they punch holes in the terrain surface; to stretch
the carve box past the content (e.g. an extra layer of cleared air above a roof), place
one explicit empty cell there.

```jsonc
// prefabs/small-house.json
{
  "version": 1,
  "palette": { "0": null, "1": "stone-block", "2": "plank-block" },
  "fill": [ { "min": [0,-1,0], "max": [6,-1,5], "voxel": "1" } ], // floor slab, embeds below ground
  "cells": [ { "at": [3, 0, 0], "voxel": "0" } ],                 // sparse overrides (door hole)
  "anchors": [ { "name": "door", "at": [3, 0, 0] } ]
}
```

## 13b. `maps/<id>.json` — full playable spaces

A complete standalone space: the M1 testground, interiors (houses, tunnels, gym arenas),
handcrafted battle boards. Loaded whole (not streamed). This is where gameplay context
lives: markers, portals, biome.

```jsonc
// maps/m1-testground.json
{
  "version": 1,
  "size": [64, 16, 64],
  "palette": { "0": null, "1": "grass-block", "2": "dirt-block",
               "3": "stone-block", "4": "slope-grass", "5": "bush" },
  "fill": [ { "min": [0,0,0], "max": [63,1,63], "voxel": "2" },
            { "min": [0,2,0], "max": [63,2,63], "voxel": "1" } ],
  "cells": [
    { "at": [10, 3, 12], "voxel": "4", "orientation": "positiveZ", "flip": "positiveY" },
    { "at": [15, 3, 20], "voxel": "5" }
  ],
  "stamps": [                            // prefab placements, applied after cells
    { "prefab": "small-house", "at": [40, 3, 18], "orientation": "positiveX" }
  ],
  "markers": [                           // named gameplay positions
    { "name": "playerSpawn", "at": [32, 3, 32] },
    { "name": "healPoint",   "at": [30, 3, 30] }
  ],
  "portals": [                           // portal-to-portal links (D37)
    { "name": "house-door", "at": [43, 4, 18],
      "target": { "map": "small-house-interior", "portal": "entrance" } }
  ],
  "biome": "forest"                      // which encounter rules apply inside this map
}
```

A tunnel map has two portals (`west`, `east`) each targeting a different overworld door —
entering any portal places the player at its **target** portal's cell, so multi-exit
interiors need no "entry position" bookkeeping (D37). Generated interiors (caves, step 31)
produce the same structure in memory and bind their portals when instantiated.

## 14. `worldgen/default.json` — macro plan parameters

```jsonc
{
  "version": 1,
  "worldSize": [1024, 1024],            // macro-plan cells (1 cell = 1 voxel column)
  "landmass": { "falloff": "radial", "noiseFrequency": 0.004, "noiseAmplitude": 0.35,
                "seaLevel": 0.0, "detailFrequency": 0.02, "detailAmplitude": 0.1 },
  "height":   { "baseFrequency": 0.008, "mountainFrequency": 0.02, "mountainThreshold": 0.65,
                "maxHeight": 48 },
  "cities":   { "majorCount": 8, "minSpacing": 180, "coastalBias": 0.25,
                "satellitesPerCity": [1, 3], "satelliteRadius": 90 },
  "biomes":   ["forest", "desert", "tundra", "swamp", "volcano", "meadow", "coast", "highland"],
  "transport":{ "extraEdges": 3, "slopeCostFactor": 4.0, "riverCrossingCost": 25.0,
                "tunnelTriggerCost": 400.0 }
}
```

Exact semantics in [03-systems/world.md](03-systems/world.md) §generation.

## 15. `saves/<slot>.json` — save data (written by the game, versioned like definitions)

```jsonc
{
  "version": 1,
  "worldSeed": 123456789,
  "respawnPoint": [32, 3, 30],
  "player": {
    "position": [32.5, 3.0, 32.5],
    "team": [
      { "species": "sprout",
        "featBoard": { "nodes": [
          { "node": "c9f10d7b-2a45-4e8c-b1f3-6d0e9a8b7c6d",   // node uuid (D34)
            "completions": 1,
            "requirements": {                                  // keyed by requirement uuid
              "f7a2c4e6-1b3d-4a5f-8e9c-0d1b2a3c4d5e": { "progress": 40.0, "repeats": 1 }
            } } ] } },
      null, null, null, null, null
    ],
    "storage": []                        // same creature objects as team entries
  },
  "clearedGyms": ["gym-forest"],
  "clearedTrainers": ["trainer-017"]
}
```

Progress is **UUID-keyed** (node uuid → requirement uuid, D34) — robust against board
reordering/edits; unknown uuids in a save are dropped with a logged warning at load.
A creature saves **only** `species` + `featBoard` progress: attributes, abilities, passives
**and current form** are all recomputed by replaying feat rewards over base species stats
at load (D35; [03-systems/creatures-feats.md](03-systems/creatures-feats.md)).

---

## 16. Loader implementation notes (for step 01)

- `pg::JsonLoader::parseFile(path) -> nlohmann::json` (throws `JsonError` on IO/syntax).
- `pg::JsonReader` wraps an object + a path string; `require<T>(key)`, `optional<T>(key,
  default)`, `requireEnum(key, map)`, `child(key)`, `forbidUnknown({allowed keys})` — every
  error is thrown with `file:jsonPath` context. `forbidUnknown` is called by every definition
  parser (strictness, D10).
- Polymorphic factory: `pg::PolymorphicFactory<TBase>` — `registerType("damage", parseFn)`,
  `parse(reader)` dispatching on `"type"`. One factory instance per hierarchy, populated in
  one translation unit per hierarchy (`effects/effect_factory.cpp`, …).
- `pg::Registry<T>::load(dir, parseFn)` iterates `*.json` sorted by filename (deterministic
  numeric-id assignment for voxels), catches nothing (errors abort startup with the message).
