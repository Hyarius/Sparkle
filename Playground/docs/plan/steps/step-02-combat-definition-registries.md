# Implement combat definition registries

Add the immutable, strictly validated JSON vocabulary for abilities, statuses/passives,
effect applications, durations, and interactive battle objects. Load every domain through
`pg::Registries` with a two-phase reference validator so cyclic status/effect/object
references are safe and startup remains transactional.

This step defines data only. Do not implement casts, damage, statuses at runtime, traps,
board occupancy, commands, battle events, AI, widgets, or authoring tools.

---

# 1. Repository baseline and prerequisites

Implement after step 01 and read its handoff report before editing.

The live baseline provides:

* `pg::JsonReader` with contextual typed reads, child readers, arrays, enum mapping, and
  `forbidUnknown`;
* `pg::Registry<T>` / `spk::DefinitionRegistry<T>` backed by deterministic `std::map`;
* `spk::loadJsonDirectory`, which loads direct `.json` children by filename and merges only
  after the directory succeeds;
* `pg::Registries::loadAll`, which builds all registries in locals and publishes at the end;
* empty `resources/data/abilities`, `statuses`, and the other future game-data domains;
* no checked-in combat definition, effect hierarchy, battle object, or battle runtime;
* step 01's `BattleTime`, content-ID grammar, common battle enums, and game-rule limits.

The generic loader already solves file discovery, filename IDs, sorted iteration, duplicate
IDs, and per-directory transactional loading. Reuse it. Do not add a competing registry
base, JSON library, reflection layer, or `ScriptableObject` emulation.

The Unity draft is evidence for desired effects, but its implicit execution context is not
the contract. Playground must explicitly state whether an effect operates on the source,
the primary target, every captured target, the anchor cell, or every captured cell.

---

# 2. Required final behavior

After this step:

1. `Registries` exposes immutable status, ability, and battle-object registries.
2. Every definition ID is its validated filename stem.
3. Every JSON object rejects unknown fields and unsupported versions with a contextual
   `JsonError`.
4. Effects are closed, value-owned variants with stable semantic IDs; JSON never contains
   executable expressions.
5. Ability anchor legality and affected-target legality are separate authored filters.
6. Statuses declare stacking, stat modifiers, reserved tags, and ordered hook effects.
7. Battle objects declare blocking and ordered unit-trigger effects.
8. Direct current AP/MP changes and next-activation penalties are different effect types.
9. Durations distinguish timeline time, owner activations, and infinity.
10. Cross-definition IDs are parsed first and validated only after all three registries
    exist, so legitimate status/object cycles do not impose an impossible load order.
11. A failed parse or cross-reference leaves the previously published `Registries` object
    untouched.
12. Minimal `training-*` seed data lets the executable print non-zero registry counts.

No effect is executed yet. The visible checkpoint is a concise registry load report, not a
battle or damage number.

---

# 3. Ownership and representation rules

Definitions are immutable values owned by `Registries`. They may contain string IDs and
value variants, but never:

* current HP/AP/MP;
* a board position or occupancy pointer;
* a current status stack/duration;
* a shield instance or battle-object instance;
* a `BattleUnit*`, widget, component, entity, RNG, callback, or `std::function`;
* a raw pointer to another definition.

Keep cross-references as validated string IDs. This avoids invalidation and naturally
supports cycles such as:

```text
status A hook -> apply status B
status B hook -> place object C
object C trigger -> remove status A
```

Runtime code later resolves IDs through the immutable registries. Do not eagerly replace
them with owning pointers or copy definitions into a session.

Prefer `std::variant` payloads over a heap hierarchy. An equivalent closed tagged union is
acceptable, but `type` must map to a compile-time-known C++ alternative and unknown types
must be hard errors listing accepted literals.

Common authored metadata uses exact bounds: `displayName` is 1-96 UTF-8 bytes,
`description` is 1-512 UTF-8 bytes, neither may be all whitespace, icon coordinates are
two integers in `[0, 4095]`, and tags use the step-01 content-ID grammar. Reject duplicate
tags; do not trim or case-fold identifiers during loading.

---

# 4. Source layout and public registry contract

Use cohesive files equivalent to:

```text
Playground/srcs/abilities/ability_definition.hpp
Playground/srcs/abilities/ability_definition.cpp
Playground/srcs/abilities/effect_definition.hpp
Playground/srcs/abilities/effect_definition.cpp
Playground/srcs/abilities/targeting_definition.hpp
Playground/srcs/abilities/targeting_definition.cpp
Playground/srcs/statuses/status_definition.hpp
Playground/srcs/statuses/status_definition.cpp
Playground/srcs/battle_objects/battle_object_definition.hpp
Playground/srcs/battle_objects/battle_object_definition.cpp
Playground/srcs/core/combat_definition_validation.hpp
Playground/srcs/core/combat_definition_validation.cpp
```

Equivalent organization is acceptable. Do not put every parser and variant in one giant
translation unit.

Extend `Registries` with values/accessors equivalent to:

```cpp
Registry<StatusDefinition> _statuses;
Registry<AbilityDefinition> _abilities;
Registry<BattleObjectDefinition> _battleObjects;

[[nodiscard]] const Registry<StatusDefinition> &statuses() const noexcept;
[[nodiscard]] const Registry<AbilityDefinition> &abilities() const noexcept;
[[nodiscard]] const Registry<BattleObjectDefinition> &battleObjects() const noexcept;
```

Create `resources/data/battle-objects/`. Keep the existing hyphenated resource-domain name;
C++ paths may use `battle_objects` to remain valid identifiers.

---

# 5. Shared duration specification

Define:

```cpp
struct TimelineDurationSpec        { BattleTime duration; };
struct OwnerActivationsDurationSpec { std::uint32_t count = 0; };
struct InfiniteDurationSpec        {};

using DurationSpec = std::variant<
    TimelineDurationSpec,
    OwnerActivationsDurationSpec,
    InfiniteDurationSpec>;
```

JSON forms are exact:

```json
{ "type": "timeline", "seconds": 2.5 }
{ "type": "ownerActivations", "count": 2 }
{ "type": "infinite" }
```

Validation:

* each form calls `forbidUnknown` for only its documented keys;
* timeline seconds are positive and exactly representable by `BattleTime`;
* owner-activation count is in `[1, 1000000]`;
* infinite has no extra field;
* durations are values, never wall-clock deadlines.

Timeline duration advances only when the scheduler advances simulated time. Owner-
activation duration decrements at the end of its bearer's completed ordinary activation.
If defeat or impression removes the active bearer, removal cleanup destroys its transient
statuses and owner-activation objects, discards the captured duration IDs, and performs no
decrement. Terminal/technical structural closes also do not decrement. The later runtime
owns the exact removal event order.

Locked stun rule: a status tagged `stun` may be applied only with a finite `timeline`
duration. `ownerActivations` is invalid because a stunned unit cannot reliably consume its
own turn to expire the stun; infinite stun is also invalid for this playable series. The
cross-validator checks every `applyStatus` reference after status tags are known.

---

# 6. Target filters

Use one relative filter value for both anchors and affected occupants:

```cpp
struct TargetFilter
{
    bool allowSelf = false;
    bool allowAllies = false;
    bool allowEnemies = false;
    bool allowDefeated = false;
    bool allowEmptyCell = false;
};
```

JSON requires all five booleans; do not infer friendly fire:

```json
{
  "allowSelf": false,
  "allowAllies": false,
  "allowEnemies": true,
  "allowDefeated": false,
  "allowEmptyCell": false
}
```

Semantics are relative to the ability caster or battle object's owning side:

* `self` means the caster/source unit itself;
* allies exclude self;
* enemies are active units on the opposing side;
* `allowDefeated` is reserved for a future revive schema and must be `false` in every v1
  definition; a filter that relies on it for its only allowed relationship is invalid;
* `allowEmptyCell` controls whether an unoccupied cell may be an anchor/affected cell;
* battle objects are not units and never make a cell satisfy an occupant relation.

Require at least one of `allowSelf|allowAllies|allowEnemies|allowEmptyCell`. An ability
authors two filters:

```text
anchorFilter    -> whether the chosen anchor cell is legal
affectedFilter  -> which occupants/cells in the already captured AoE are eligible
```

An enemy-only anchor does not imply enemy-only AoE. Both must say so. This is the locked
friendly-fire contract.

---

# 7. Range and area geometry definitions

Define:

```cpp
enum class RangeShape
{
    Self,
    Diamond,
    OrthogonalLine,
    DiagonalLine
};

struct RangeDefinition
{
    RangeShape shape = RangeShape::Diamond;
    int minimum = 0;
    int maximum = 0;
    bool requiresLineOfSight = false;
};

enum class AreaShape
{
    Single,
    Diamond,
    Square,
    Cross,
    Line
};

struct AreaDefinition
{
    AreaShape shape = AreaShape::Single;
    int radius = 0;
};
```

JSON literals are `self`, `diamond`, `orthogonalLine`, `diagonalLine`, and `single`,
`diamond`, `square`, `cross`, `line`.

Locked x/z geometry:

* `diamond`: Manhattan distance `abs(dx) + abs(dz)`;
* `orthogonalLine`: `dx == 0 || dz == 0`;
* `diagonalLine`: `abs(dx) == abs(dz)`;
* `self`: anchor is source cell;
* AoE `single`: anchor only and radius must be 0;
* AoE `diamond`: Manhattan radius around anchor;
* AoE `square`: Chebyshev radius around anchor;
* AoE `cross`: anchor plus cardinal rays up to radius;
* AoE `line`: forward ray beginning at the anchor and oriented in the source-to-anchor
  dominant x/z direction. Include anchor plus offsets `1..radius`; exact absolute ties choose
  x, the selected coordinate's sign gives direction, and a zero x/z delta returns anchor
  exactly once.

Elevation never changes distance. LoS remains a 3D voxel query. `CreatureStat::Range`
extends only `RangeDefinition::maximum`; it never lowers `minimum` or enlarges AoE.

Validation:

* minimum/maximum are in `[0, 64]` and minimum <= maximum;
* self requires both zero;
* a `self` range requires `anchorFilter.allowSelf == true`;
* non-self maximum must be positive;
* radius is in `[0, 16]`;
* single requires radius zero; non-single may use radius zero as a single-cell degenerate
  shape if useful.

Do not calculate cells in this step; step 08 implements and tests the algorithms.

---

# 8. Explicit effect execution scope

Every effect entry has common metadata:

```cpp
enum class EffectApplyTo
{
    SourceUnit,
    PrimaryUnit,
    AffectedUnits,
    AnchorCell,
    AffectedCells
};

struct EffectApplication
{
    std::string id;
    EffectApplyTo applyTo;
    bool requiresLivingSource = true;
    EffectPayload payload;
};
```

JSON literals are `sourceUnit`, `primaryUnit`, `affectedUnits`, `anchorCell`, and
`affectedCells`. Every effect object requires `id`, `type`, `applyTo`, and
`requiresLivingSource` plus its type fields.

The eventual execution context has these exact meanings:

```text
ability:
    sourceUnit      caster
    primaryUnit     occupant of the anchor when one exists
    affectedUnits   captured AoE occupants after affectedFilter
    anchorCell      chosen anchor
    affectedCells   captured AoE cells after affectedFilter empty-cell policy

status hook:
    sourceUnit      original applier stable ID when present; passive owner for a passive
    primaryUnit     bearer for applied/activation/removed/after-cast/after-move;
                    original event target for after-damage-dealt/after-healing-done;
                    original event source for after-damage-taken/after-healing-received
                    when that source ID exists, otherwise the bearer
    affectedUnits   [primaryUnit]
    anchorCell      bearer's current cell, or last occupied cell for a copied removed hook
    affectedCells   [primaryUnit current/last cell] when available, otherwise [anchorCell]

battle-object trigger:
    sourceUnit      object creator when still addressable
    primaryUnit     triggering unit
    affectedUnits   [triggering unit]
    anchorCell      object cell
    affectedCells   [object cell]
```

The status-hook mapping above is the schema contract later implemented in step 10; do not
replace it with a blanket “all hooks target the bearer” rule. A copied source ID does not
imply that source is still active: `requiresLivingSource` decides whether execution may
continue.

The resolver captures targets/cells once per cast in canonical order. Effects do not
re-query an expanding AoE after displacement. An already captured cast continues after a
source defeat unless the next effect has `requiresLivingSource: true`.

Parser validation rejects a unit effect with a cell-only scope, a cell effect with a
unit-only scope, `teleportSource` outside `anchorCell`, and `swapWithSource` outside
`primaryUnit`. Do not silently reinterpret an invalid scope.

Effect IDs use the step-01 content-ID grammar and are unique across the owning definition,
including effects nested in different status hooks or object triggers. They are stable
semantic event/debug identities, not array indices.

---

# 9. Effect payload catalog

Use a closed `EffectPayload` variant containing the following alternatives and no general
expression/script payload.

## 9.1 Damage

```json
{
  "id": "impact",
  "type": "damage",
  "applyTo": "affectedUnits",
  "requiresLivingSource": true,
  "kind": "physical",
  "base": 5,
  "strengthRatioPermille": 1000,
  "magicPowerRatioPermille": 0
}
```

Value shape:

```cpp
struct DamageEffectSpec
{
    DamageKind kind;
    std::int64_t base = 0;
    std::int32_t strengthRatioPermille = 0;
    std::int32_t magicPowerRatioPermille = 0;
};
```

`base` is `[0, 1000000]`; ratios are `[0, 10000]`; at least one of base/ratios is positive.
Step 09 owns the checked damage formula. Physical uses Armor and magical uses Resistance;
the ratios remain explicit so unusual hybrid authored effects are data, not special C++.

## 9.2 Healing

```json
{
  "id": "restore",
  "type": "heal",
  "applyTo": "affectedUnits",
  "requiresLivingSource": true,
  "base": 4,
  "strengthRatioPermille": 0,
  "magicPowerRatioPermille": 750
}
```

Use `HealEffectSpec` with the same bounds and positivity rule. Healing does not revive in
v1 and clamps at effective maximum health.

## 9.3 Shield

```json
{
  "id": "physical-ward",
  "type": "applyShield",
  "applyTo": "affectedUnits",
  "requiresLivingSource": true,
  "kind": "physical",
  "amount": 8,
  "duration": { "type": "ownerActivations", "count": 2 }
}
```

`ApplyShieldEffectSpec` carries damage kind, positive amount up to 1,000,000, and a
`DurationSpec`. Matching shields absorb before HP. Shield runtime order is step 09/10.

## 9.4 Apply/remove/cleanse status

```json
{
  "id": "apply-poison",
  "type": "applyStatus",
  "applyTo": "affectedUnits",
  "requiresLivingSource": true,
  "status": "poison",
  "stacks": 1,
  "duration": { "type": "timeline", "seconds": 4.0 }
}
```

```json
{
  "id": "remove-poison",
  "type": "removeStatus",
  "applyTo": "affectedUnits",
  "requiresLivingSource": false,
  "status": "poison",
  "stacks": 1
}
```

```json
{
  "id": "cleanse-toxins",
  "type": "cleanse",
  "applyTo": "affectedUnits",
  "requiresLivingSource": true,
  "tags": ["poison", "toxin"]
}
```

Stacks are `[1, 1000000]`. Cleanse tags are non-empty validated content-like tag strings,
deduplicated case-sensitively, and remove all matching status instances in later-defined
stable instance order. Status IDs are cross-validated after all registries load.

## 9.5 Current resources versus next-activation penalty

Current pool mutation:

```json
{
  "id": "drain-current-ap",
  "type": "changeResource",
  "applyTo": "affectedUnits",
  "requiresLivingSource": true,
  "resource": "actionPoints",
  "delta": -2
}
```

Next activation penalty:

```json
{
  "id": "slow-next-move",
  "type": "applyNextActivationPenalty",
  "applyTo": "affectedUnits",
  "requiresLivingSource": false,
  "resource": "movementPoints",
  "amount": 1
}
```

`changeResource` has a non-zero signed delta in `[-1000000, 1000000]` and only changes the
current AP/MP pool. `applyNextActivationPenalty.amount` is in `[1, 1000000]`; accumulation
uses checked `int64_t` arithmetic and subtracts from the refill at exactly the next
activation start, then is consumed. It cannot be authored as
a positive “bonus”; bonuses/max changes use statuses and stat modifiers. This distinction
must survive through events and tests.

## 9.6 Turn-bar adjustment

```json
{
  "id": "delay-turn",
  "type": "adjustTurnBar",
  "applyTo": "affectedUnits",
  "requiresLivingSource": true,
  "deltaSeconds": -0.5
}
```

`AdjustTurnBarEffectSpec::delta` is a non-zero exact `BattleTime`. Positive adds fill and
brings readiness closer; negative removes fill and delays. It never changes authored
stamina/max duration. Clamp the runtime bar to `[0, effective stamina]` in step 09.

## 9.7 Displacement

```json
{
  "id": "push",
  "type": "displace",
  "applyTo": "affectedUnits",
  "requiresLivingSource": true,
  "direction": "awayFromSource",
  "distance": 2
}
```

Direction is `awayFromSource` or `towardSource`; distance is `[1, 64]`. Step 09 moves one
cardinal cell at a time along the dominant source-target x/z axis, x winning ties, stopping
at the first invalid/occupied cell. Displacement costs no MP.

## 9.8 Swap with source

```json
{
  "id": "transpose",
  "type": "swapWithSource",
  "applyTo": "primaryUnit",
  "requiresLivingSource": true
}
```

No extra fields. It requires one living source and primary unit and an atomic legal swap.

## 9.9 Teleport source to anchor

```json
{
  "id": "blink",
  "type": "teleportSource",
  "applyTo": "anchorCell",
  "requiresLivingSource": true
}
```

No absolute destination is authored. The ability-selected anchor is the destination. This
effect executes once per cast, not once per AoE cell.

## 9.10 Place/remove battle objects

```json
{
  "id": "place-snare",
  "type": "placeObject",
  "applyTo": "anchorCell",
  "requiresLivingSource": true,
  "object": "training-snare-object",
  "duration": { "type": "ownerActivations", "count": 3 }
}
```

```json
{
  "id": "clear-snares",
  "type": "removeObjects",
  "applyTo": "affectedCells",
  "requiresLivingSource": false,
  "tags": ["trap"]
}
```

Object ID and tags are validated. A placed instance snapshots creator ID/side, cell,
duration, trigger counts, and its immutable definition ID later; none belongs in the
definition.

---

# 10. Ability definition

Use a value shape equivalent to:

```cpp
struct AbilityCost
{
    int actionPoints = 0;
    int movementPoints = 0;
};

struct AbilityDefinition
{
    std::string id;
    std::string displayName;
    std::string description;
    std::array<int, 2> icon{};
    AbilityCost cost;
    RangeDefinition range;
    AreaDefinition area;
    TargetFilter anchorFilter;
    TargetFilter affectedFilter;
    std::vector<EffectApplication> effects;
};
```

Exact v1 example:

```json
{
  "version": 1,
  "displayName": "Training Strike",
  "description": "A simple adjacent physical attack.",
  "icon": [0, 0],
  "cost": { "actionPoints": 2, "movementPoints": 0 },
  "range": {
    "shape": "diamond",
    "minimum": 1,
    "maximum": 1,
    "requiresLineOfSight": true
  },
  "area": { "shape": "single", "radius": 0 },
  "anchorFilter": {
    "allowSelf": false,
    "allowAllies": false,
    "allowEnemies": true,
    "allowDefeated": false,
    "allowEmptyCell": false
  },
  "affectedFilter": {
    "allowSelf": false,
    "allowAllies": false,
    "allowEnemies": true,
    "allowDefeated": false,
    "allowEmptyCell": false
  },
  "effects": [
    {
      "id": "impact",
      "type": "damage",
      "applyTo": "affectedUnits",
      "requiresLivingSource": true,
      "kind": "physical",
      "base": 4,
      "strengthRatioPermille": 1000,
      "magicPowerRatioPermille": 0
    }
  ]
}
```

Validation:

* version exactly 1;
* filename/content ID valid;
* display name and description non-empty, bounded UTF-8 strings;
* icon values non-negative;
* AP/MP costs in `[0, 1000000]`;
* range/area/filter rules above;
* at least one effect;
* unique effect IDs;
* every effect scope compatible with its payload;
* no duplicate/unknown root or nested field.

Zero-cost abilities are allowed because statuses/passives may author them, but later the
activation command cap and no-state-change guard prevent infinite loops. Parsing cannot
prove an effect changes state for every target.

---

# 11. Status/passive definition

A passive is an infinite status applied when a `BattleUnit` is projected. Do not create a
separate passive registry or executable passive base class.

Use:

```cpp
enum class StatusReapplyPolicy { AddStacks, ReplaceStacks };
enum class DurationRefreshPolicy { Replace, KeepLonger, Extend };
enum class StatModifierOperation { Add, MultiplyPermille };

struct StatModifierSpec
{
    std::string id;
    CreatureStat stat;
    StatModifierOperation operation;
    std::int64_t value = 0;
    bool perStack = false;
};

enum class StatusHook
{
    Applied,
    ActivationStart,
    ActivationEnd,
    AfterAbilityCast,
    AfterDamageDealt,
    AfterDamageTaken,
    AfterHealingDone,
    AfterHealingReceived,
    AfterVoluntaryMove,
    Removed
};

struct StatusHookSpec
{
    std::string id;
    StatusHook hook;
    std::vector<EffectApplication> effects;
};

struct StatusDefinition
{
    std::string id;
    std::string displayName;
    std::string description;
    std::array<int, 2> icon{};
    std::vector<std::string> tags;
    std::uint32_t maxStacks = 1;
    StatusReapplyPolicy reapplyPolicy;
    DurationRefreshPolicy durationRefreshPolicy;
    std::vector<StatModifierSpec> modifiers;
    std::vector<StatusHookSpec> hooks;
};
```

Exact example:

```json
{
  "version": 1,
  "displayName": "Training Guard",
  "description": "Raises Armor while active.",
  "icon": [1, 0],
  "tags": ["buff", "guard"],
  "stacking": {
    "maxStacks": 1,
    "reapply": "replaceStacks",
    "durationRefresh": "replace"
  },
  "modifiers": [
    {
      "id": "armor-bonus",
      "stat": "armor",
      "operation": "add",
      "value": 3,
      "perStack": false
    }
  ],
  "hooks": []
}
```

Hook literals are `applied`, `activationStart`, `activationEnd`, `afterAbilityCast`,
`afterDamageDealt`, `afterDamageTaken`, `afterHealingDone`, `afterHealingReceived`,
`afterVoluntaryMove`, and `removed`.

Every non-empty `hooks` entry has exactly `{id, when, effects}`. For example:

```json
{
  "id": "restore-on-start",
  "when": "activationStart",
  "effects": [
    {
      "id": "restore-one-ap",
      "type": "changeResource",
      "applyTo": "primaryUnit",
      "requiresLivingSource": false,
      "resource": "actionPoints",
      "delta": 1
    }
  ]
}
```

The parser maps `when` to `StatusHookSpec::hook`; `hook` is not an accepted JSON key.

Modifier rules:

* IDs are unique across modifiers;
* `add` uses the stat's unit. For Stamina, value is milliseconds; authoring a stamina add
  in JSON uses the explicit field `valueMilliseconds`, not `value`. Other stats use
  integer `value`;
* `multiplyPermille` uses integer `value`, where 1000 is unchanged, and permits `[1,10000]`;
* additive `value` and `valueMilliseconds` are signed integers in
  `[-1000000, 1000000]`; zero is rejected;
* for `add`, `perStack:true` multiplies the signed additive value by current stack count
  with checked arithmetic; for `multiplyPermille`, it applies the authored factor once per
  stack in sequence, truncating after each application—it never multiplies the factor by
  the stack count;
* effective maximum HP/AP/MP clamp to at least one; effective stamina clamps to
  `gameRules.battle.minimumStamina`; armor/resistance, strength/magicPower, and range clamp
  to at least zero in runtime;
* lower Stamina is faster.

Status validation also requires `maxStacks` in `[1, 1000000]`, unique non-empty tags, unique hook IDs, non-empty effect
lists inside authored hooks, unique effect IDs across all hooks, and at most one hook entry
for a given hook literal. The reserved tag `stun` pauses turn-bar fill; a definition tagged
`stun` cannot be granted as a permanent passive and every `applyStatus` reference to it
must use finite timeline duration.

Do not add pre-damage mutation callbacks. Stat modifiers and ordered post-event hooks are
enough for v1 and avoid hidden damage-formula mutation.

---

# 12. Battle-object definition

Define:

```cpp
enum class BattleObjectTrigger
{
    UnitEnteredCell,
    UnitLeftCell,
    UnitEndedMoveOnCell,
    UnitActivationStartedOnCell,
    UnitActivationEndedOnCell
};

struct BattleObjectTriggerSpec
{
    std::string id;
    BattleObjectTrigger trigger;
    TargetFilter targetFilter;
    std::uint32_t maxTriggers = 0; // 0 = unlimited during instance lifetime
    bool removeWhenExhausted = false;
    std::vector<EffectApplication> effects;
};

struct BattleObjectDefinition
{
    std::string id;
    std::string displayName;
    std::string description;
    std::array<int, 2> icon{};
    std::vector<std::string> tags;
    bool blocksMovement = false;
    bool blocksLineOfSight = false;
    std::vector<BattleObjectTriggerSpec> triggers;
};
```

Example:

```json
{
  "version": 1,
  "displayName": "Training Snare",
  "description": "Penalizes the next movement refill of an enemy entering its cell.",
  "icon": [2, 0],
  "tags": ["trap", "snare"],
  "blocksMovement": false,
  "blocksLineOfSight": false,
  "triggers": [
    {
      "id": "on-enter",
      "when": "unitEnteredCell",
      "targetFilter": {
        "allowSelf": false,
        "allowAllies": false,
        "allowEnemies": true,
        "allowDefeated": false,
        "allowEmptyCell": false
      },
      "maxTriggers": 1,
      "removeWhenExhausted": true,
      "effects": [
        {
          "id": "movement-penalty",
          "type": "applyNextActivationPenalty",
          "applyTo": "primaryUnit",
          "requiresLivingSource": false,
          "resource": "movementPoints",
          "amount": 1
        }
      ]
    }
  ]
}
```

Trigger literals are `unitEnteredCell`, `unitLeftCell`, `unitEndedMoveOnCell`,
`unitActivationStartedOnCell`, and `unitActivationEndedOnCell`. Require at least one trigger,
unique trigger IDs, unique tags, compatible target filters, and unique effect IDs across
the object. Every trigger filter requires `allowDefeated:false`, `allowEmptyCell:false`, and
at least one of `allowSelf|allowAllies|allowEnemies`. `maxTriggers == 0` requires
`removeWhenExhausted == false`; a nonzero value is in `[1, 1000000]`, and exhaustion means
the instance has fired that many times. An object's placement duration is authored by
the placing effect, not duplicated here.

Objects may coexist with a unit but v1 permits at most one instance of a given object
definition per cell/source pair; step 10 locks runtime stacking. Blocking objects cannot be
placed on an occupied cell. These are documented runtime contracts, not implemented here.

---

# 13. Strict polymorphic parsing

Implement one dispatcher per closed hierarchy or an equivalent `std::map<string, parseFn>`
local to its translation unit. Requirements:

* read `type` first;
* unknown type errors list the accepted literals in stable lexical order;
* the selected parser calls `forbidUnknown` with common plus alternative-specific fields;
* every nested error retains the array path, such as `$.effects[2].duration.seconds`;
* do not catch and downgrade an invalid effect to a no-op;
* do not store unparsed JSON for later runtime interpretation.

If `JsonReader` lacks a helper needed for nullable/variant values, add the narrow helper to
Playground or parse its `Value` carefully. Do not edit Sparkle's reader merely for one game
schema unless separately approved.

---

# 14. Two-phase load and cross-validation

Do not load statuses, then immediately resolve their references before abilities/objects
exist. Use:

```text
phase 1: parse values
    loadedStatuses
    loadedAbilities
    loadedBattleObjects

phase 2: validate the complete graph
    every effect reference exists
    every referenced status/object obeys contextual rules
    every definition-local ID is unique

only after every old and new domain validates:
    move locals into Registries members
```

The three directories may be parsed in the order statuses, battle objects, abilities for a
stable load report, but semantic correctness must not depend on it.

Cross-validator checks every nested effect in abilities, status hooks, and object triggers:

* status ID exists for apply/remove;
* object ID exists for placement;
* cleanse/remove-object tags are valid and non-empty;
* stun applications use finite timeline duration;
* status/object self-reference and cycles are allowed;
* source-liveness/scope compatibility is valid;
* no definition references an ID from the wrong registry.

Reference errors must name both the owning definition/effect ID and the original JSON path.
Retain source-location metadata during phase 1 when necessary; do not emit a generic
`unknown id` detached from its file.

There is no topological sort and no recursive expansion. Runtime `maxEffectChainDepth` plus
event re-entry guards in step 10 protect triggered cycles.

---

# 15. Seed content required by this step

Add small, deliberately development-named data that later steps can reference:

```text
resources/data/abilities/training-strike.json
resources/data/abilities/training-guard.json
resources/data/abilities/training-snare.json
resources/data/statuses/training-guarded.json
resources/data/battle-objects/training-snare-object.json
```

Use the schemas above:

* `training-strike` deals physical damage to one enemy;
* `training-guard` targets self/ally and applies `training-guarded` for two owner
  activations;
* `training-snare` targets an empty cell and places `training-snare-object` for three owner
  activations;
* `training-guarded` adds Armor;
* the snare applies one point of next-activation MP penalty to the first enemy entering.

All anchor/affected booleans and effect scopes must be explicit. These definitions are
bootstrap content, not a promised final balance pack. Step 19 may add/replace playable
species content but should retain these IDs if tests use the real resource tree.

Do not add production creature lore, final icons, models, animations, VFX, or sound.

---

# 16. Load report

Extend the existing successful `Registries::loadAll` diagnostic without dumping full
definitions:

```text
Loaded ... N statuses, M abilities, and K battle objects
```

Keep output deterministic and emit it only after the complete transactional commit. A
failed load prints only the caught contextual startup error from the existing main path;
never print a misleading partial-success count.

---

# 17. Required parser and validation tests

Add tests under:

```text
Playground/tests/abilities/
Playground/tests/statuses/
Playground/tests/battle_objects/
Playground/tests/core/
```

Use temporary directories/files for isolation and the real resource tree for one smoke
load. Never modify checked-in resources during a test.

## 17.1 Successful shape tests

Parse one golden definition for every effect alternative, every duration, every target
filter category, every range/AoE shape, every modifier operation, every status hook, and
every object trigger. Assert the concrete `std::variant` alternative and every field.

## 17.2 Strictness tests

For roots and nested alternatives test:

* unsupported/missing version;
* missing required field;
* unknown field;
* wrong scalar/array/object type;
* unknown enum/type with stable known-value list;
* invalid content ID and duplicate local IDs;
* bad numeric boundary and non-finite/unrepresentable seconds;
* empty required arrays/tags/strings;
* incompatible effect scope/payload;
* duplicate effects across separate hooks/triggers.

Assert useful file and JSON paths, not only that an exception occurred.

## 17.3 Ability contracts

Test anchor and affected filters remain distinct; range/AoE constraints; self-range
constraints; MP/AP bounds; empty effects; unique effect order; and exact preservation of
authored effect order.

## 17.4 Status contracts

Test stack policies, duration refresh policies, tag uniqueness, modifier units, stamina
modifier field selection, duplicate hook rejection, and reserved stun validation.

## 17.5 Object contracts

Test trigger limits, unlimited/removal contradiction, blocking flags, target filters,
trigger/effect order, and all five trigger kinds. `unitLeftCell` observes the unit that just
left after the occupancy transition has committed; the object's anchor remains its own cell.

## 17.6 Cross-reference graph

Build fixtures proving:

* ability -> status succeeds;
* ability -> object succeeds;
* status -> status succeeds;
* status -> object -> status cycle succeeds without recursion at load;
* every missing status/object fails at its owning effect path;
* stun with owner-activation/infinite duration fails;
* a non-stun status with each duration succeeds;
* an invalid second load leaves all previously loaded registry counts/IDs unchanged.

## 17.7 Deterministic loading

Create definitions in reverse filesystem creation order and assert `ids()` is lexical and
load output/counts are stable. Do not assume directory iteration order.

---

# 18. Error handling and determinism requirements

* Invalid definitions abort startup; there is no warning-and-skip mode.
* Empty combat directories are structurally allowed for isolated tests, but checked-in
  seed content must make all three non-empty.
* Definitions contain no unordered map whose iteration enters gameplay. Use vectors for
  authored order and ordered maps only for lookup.
* `std::variant` order is not a serialized type number; JSON uses explicit strings.
* Ratios and probabilities use integers, not runtime float accumulation.
* Parsing and validation consume no RNG.
* Effect IDs, hook IDs, and trigger IDs are preserved in diagnostic/event-ready values.
* Unknown future fields are errors until the domain version changes.

---

# 19. Expected files and integration points

Add definition/parser/validator files, tests, the battle-object resource directory, and the
five seed definitions. Modify:

```text
Playground/srcs/core/registries.hpp
Playground/srcs/core/registries.cpp
Playground/docs/02-data-model.md          # implemented subset only
```

Update other implemented-system docs only where they currently claim these registries
already exist or use conflicting field names.

Do not modify `GameContext`, `GameSceneWidget`, the world generator, board/navigation code,
engine code, or widgets. No runtime caller should need an ability yet.

---

# 20. Documentation requirements

Document:

* exact v1 schemas and filename-ID convention;
* `magicPower` and integer permille ratios;
* execution-scope contexts;
* anchor versus affected filters;
* duration semantics and the stun restriction;
* current resource change versus next-activation penalty;
* status-as-passive model;
* two-phase validation and why cycles remain string IDs;
* every implemented effect/status/object type;
* explicitly that no resolver/runtime exists yet.

Replace stale aspirational examples that use `targetProfile`, implicit per-target effects,
`magic`, float effect ratios, `durationTurns: -1`, or absolute teleport destinations.

---

# 21. Non-goals

Do not implement effect execution, formulas, status instances, shields, traps, object
occupancy, command validation, previews, battle events, board code, scheduler, AI, creature
species, progression evaluation, VFX/animation/audio fields, rules-text generation,
authoring tools, hot reload, general scripts, revive, steal-resource, accuracy, critical
hits, elemental types, or items.

Do not add an effects registry: effects are ordered embedded value specifications with
stable local IDs.

---

# 22. Acceptance checklist

This step is complete only when:

* [ ] all three registries load through `Registries::loadAll` and have const getters;
* [ ] definitions remain immutable values with string cross-references;
* [ ] every JSON object is versioned and strict;
* [ ] all effect alternatives and duration forms parse into closed variants;
* [ ] each effect has stable ID, explicit scope, and explicit source-liveness requirement;
* [ ] anchor and affected filters cannot be conflated;
* [ ] AP/MP current changes and next-activation penalties are distinct;
* [ ] status stacking/modifier/hook data is fully validated;
* [ ] battle-object blocking/trigger data is fully validated;
* [ ] stun duration/passive constraints fail at load;
* [ ] cross-registry cycles load without pointer recursion;
* [ ] missing references have owning file/path diagnostics;
* [ ] a failed full reload leaves prior registries unchanged;
* [ ] the five seed files load and the report is non-zero;
* [ ] all required parser/strictness/reference/determinism tests pass;
* [ ] the full Playground label and executable build remain green;
* [ ] no combat runtime or presentation was added.

---

# 23. Required handoff report

Report:

1. exact final C++ definition/variant names and header paths;
2. every accepted JSON type/enum literal and schema version;
3. registry getter names and final load/validation order;
4. cross-reference cycles tested and the source-location strategy used;
5. seed content IDs added;
6. files created/materially changed and deliberately untouched;
7. repository drift/adaptations;
8. commands actually run and results;
9. registry-load smoke output actually observed;
10. runtime behavior explicitly left for steps 08-10.
