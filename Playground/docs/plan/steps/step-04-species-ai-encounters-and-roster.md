# Implement species, AI, encounters, and persistent roster state

Complete the immutable definition graph needed to construct a battle and add the smallest
persistent player model needed to own creatures before/after encounters. Implement species
and forms, deterministic derived creature state, ordered-rule AI definitions, tiered
encounter definitions and resolution, immutable handcrafted-battle-board definitions,
biome-to-wild-table links, a JSON new-game roster, six active slots, and PC storage.

This step still does not construct a tactical board or run a battle. Do not add battle
units, commands, effects at runtime, AI planning, encounter triggers, modes, save files,
widgets, or authoring tools.

---

# 1. Repository baseline and prerequisites

Implement after steps 01-03 and read every handoff report. Reinspect the live repository;
paths below are the intended architecture, not permission to overwrite drift.

Required prior contracts:

* fixed `BattleTime`, stable content IDs, `CreatureInstanceId`, common stat/side/encounter
  enums, and deterministic `BattleRng`;
* strict, sorted, local-then-commit registry loading;
* immutable status/passive, ability, effect, and battle-object definitions;
* `FeatBoardDefinition`, shared `ConditionSpec`, rewards, evolution gates, and inline
  `TamingProfileDefinition` parsing;
* no creature species, form, AI behavior, encounter, player data, roster, storage, or save
  implementation at the original baseline;
* empty checked-in `resources/data/ai`, `battle-boards`, `creatures`, and
  `encounter-tables` directories;
* existing version-1 biome files and `BiomeDefinition` with no encounter-table link;
* `main.cpp` currently default-constructs `GameContext` after loading `Registries`;
* `GameContext` currently owns exploration event/world state only.

The Unity project demonstrates the separation between `CreatureSpecies`, persistent
`CreatureUnit`, encounter `EncounterUnit`, and runtime `BattleUnit`. Preserve that
separation without copying ScriptableObjects, serialized-reference inheritance, global
services, GUID registries, or editor dictionaries.

Across the new v1 domains, `displayName` is 1-96 UTF-8 bytes and `description` is 1-512
UTF-8 bytes; neither may be all whitespace. Stable IDs use the step-01 ASCII grammar and
are never trimmed or case-folded. Parsers enforce these same bounds instead of each domain
quietly choosing another limit.

---

# 2. Required final behavior

After this step:

1. `Registries` exposes immutable AI, species, handcrafted battle-board, and encounter
   definitions plus one immutable new-game definition.
2. A species owns base attributes, default abilities/passives, a Feat Board reference,
   optional inline taming profile, forms, and placeholder form presentation data.
3. A creature has no level or random stat roll; its derived form/stats/loadout are a pure
   replay from species baseline plus completed Feat nodes.
4. A persistent `CreatureUnit` stores stable instance/species/progress identity and only a
   rebuildable derived cache.
5. `PlayerRoster` has exactly six nullable active slots and ordered unbounded PC storage.
6. Every roster operation is ID-based, uniqueness-checked, and all-or-none.
7. New-game JSON creates the starter team and deterministic creature serials; no starter is
   hard-coded in `main.cpp`.
8. AI JSON is an ordered list of closed state conditions and decisions; it cannot mutate
   state or bypass the future command gateway.
9. Encounter JSON supplies kind/taming/repeatability, one strict live-world or handcrafted
   board-policy alternative, sparse progression tiers, weighted teams, ordered spawn
   members, AI, and pre-completed nodes.
10. Every generated overworld biome has an explicit wild encounter-table reference.
11. Encounter chance/team selection uses the battle RNG and stable integer weights.
12. The entire old+new definition graph validates before any registry/member is published.
13. The executable prints the starter roster and all new registry counts, then still boots
    into exploration.

There is no battle view. The visible checkpoint is the validated starter-roster diagnostic.

---

# 3. Creature attributes and same-baseline rule

Define one immutable/current-derived attribute value:

```cpp
struct CreatureAttributes
{
    std::int64_t maxHealth = 1;
    std::int64_t strength = 0;
    std::int64_t magicPower = 0;
    std::int64_t armor = 0;
    std::int64_t resistance = 0;
    std::int64_t maxActionPoints = 1;
    std::int64_t maxMovementPoints = 1;
    BattleTime stamina = BattleTime::fromTicks(1000);
    std::int64_t range = 0;
};
```

Exact JSON:

```json
"attributes": {
  "maxHealth": 24,
  "strength": 6,
  "magicPower": 3,
  "armor": 2,
  "resistance": 2,
  "maxActionPoints": 6,
  "maxMovementPoints": 4,
  "staminaSeconds": 4.0,
  "range": 0
}
```

Validation:

* health/AP/MP are positive integers up to 1,000,000;
* strength, magicPower, armor, resistance, and range are non-negative up to 1,000,000;
* stamina is positive, exactly representable fixed time, and not below
  `gameRules.battle.minimumStamina`;
* all reward additions/multiplications use checked arithmetic and revalidate these bounds;
* lower stamina means faster.

“Same baseline” means a fresh creature uses the authored base values with no level, XP,
individual value, nature, random roll, or encounter scaling. It does not require unrelated
species to have identical numbers.

Do not add penetration, critical, elemental, accuracy/evasion, experience, or hidden stats.
Passive behavior belongs to status definitions.

---

# 4. Forms and placeholder presentation

Use a species-local form value:

```cpp
struct PlaceholderVisual
{
    std::array<std::uint8_t, 4> tint{255, 255, 255, 255};
    std::uint16_t scalePermille = 1000;
};

struct CreatureFormDefinition
{
    std::string id;
    std::string displayName;
    std::uint32_t tier = 0;
    PlaceholderVisual presentation;
};
```

JSON forms are an ordered array, not a dynamic-key object:

```json
"forms": [
  {
    "id": "base",
    "displayName": "Training Sprout",
    "tier": 0,
    "presentation": {
      "tint": [90, 190, 105, 255],
      "scalePermille": 1000
    }
  }
]
```

Validate unique local IDs, non-empty display names, tier `[0, 100]`, four integer color
channels `[0,255]`, and scale `[250,3000]`. Preserve array order.

This data deliberately supports colored placeholder cubes in step 14 without coupling the
headless definition to `spk::Color`, a renderer, texture, model, animation, VFX, or audio.
Final asset metadata and authoring tools remain out of scope.

---

# 5. Species definition and JSON schema

Use:

```cpp
struct CreatureSpeciesDefinition
{
    std::string id;
    std::string displayName;
    std::string description;
    CreatureAttributes baseAttributes;
    std::vector<std::string> defaultAbilityIds;
    std::vector<std::string> defaultPassiveStatusIds;
    std::string featBoardId;
    std::optional<TamingProfileDefinition> tamingProfile;
    std::string defaultFormId;
    std::vector<CreatureFormDefinition> forms;
};
```

Exact v1 example:

```json
{
  "version": 1,
  "displayName": "Training Sprout",
  "description": "A development creature used to exercise battle systems.",
  "attributes": {
    "maxHealth": 24,
    "strength": 6,
    "magicPower": 3,
    "armor": 2,
    "resistance": 2,
    "maxActionPoints": 6,
    "maxMovementPoints": 4,
    "staminaSeconds": 4.0,
    "range": 0
  },
  "defaultAbilities": ["training-strike"],
  "defaultPassives": [],
  "featBoard": "training-board",
  "defaultForm": "base",
  "forms": [
    {
      "id": "base",
      "displayName": "Training Sprout",
      "tier": 0,
      "presentation": {
        "tint": [90, 190, 105, 255],
        "scalePermille": 1000
      }
    }
  ],
  "tamingProfile": {
    "conditions": [
      {
        "id": "approach-carefully",
        "type": "position",
        "description": "End a move next to this creature.",
        "window": "turn",
        "requiredWindowCount": 1,
        "windowMode": "cumulative",
        "sample": "afterMove",
        "actor": "opponentTeam",
        "relativeTo": "subject",
        "comparison": "atMost",
        "distance": 1
      }
    ]
  }
}
```

An absent or explicit null `tamingProfile` means untameable. A present profile must pass the
step-03 parser and be non-empty.

---

# 6. Species cross-validation

After combat and Feat registries exist, validate:

* version exactly 1 and strict fields;
* filename/content ID valid;
* default abilities are non-empty, unique, known, and at most
  `abilitySlotCapacity`;
* default passives are unique known statuses and none has reserved tag `stun`;
* Feat Board exists;
* forms are non-empty/unique and default form exists;
* exactly one tier-0 form exists and it is the default;
* form tier cannot decrease along a board's authored `changeForm` reward order;
* every board `fromForm` and `changeForm` ID exists in this species;
* every exclusive evolution group's target forms are distinct;
* inline taming ability/status filters exist;
* the conservative union of default abilities plus every `unlockAbility` reward contains no
  more than `abilitySlotCapacity` unique IDs. This avoids adding an unimplemented move-slot
  selection system; remove/branch rewards may make actual sets smaller but not larger;
* all possible reward arithmetic stays within validated attribute bounds. When exact branch
  enumeration is needed, enumerate board choices in definition order with a test-bounded
  graph rather than using unchecked worst-case sums.

A Feat Board may be shared by species only if all of its form references validate for each
species. Errors name both species and board/node/reward paths.

Definitions never contain current form, unlocked loadout, current HP, or progression.

---

# 7. Persistent progress shape established for creatures

Step 15/17 will evaluate and advance conditions, but persistent creatures need stable
definition-keyed storage now. Establish values equivalent to:

```cpp
struct PersistentConditionAdvancement
{
    std::string conditionId;
    bool completed = false;
    std::uint32_t qualifyingWindows = 0;
    std::uint32_t consecutiveWindowStreak = 0;
    std::optional<std::int64_t> gameMetric;
    std::map<std::string, std::int64_t> gameBuckets;
};

struct FeatRequirementProgress
{
    std::string requirementId;
    // One stable-ID entry for the top-level condition and every nested child.
    std::vector<PersistentConditionAdvancement> conditionAdvancement;
};

struct FeatNodeProgress
{
    std::string nodeId;
    bool completed = false;
    std::vector<FeatRequirementProgress> requirements;
};

struct FeatBoardProgress
{
    std::string boardId;
    std::vector<FeatNodeProgress> nodes;
    std::map<std::string, std::string> chosenExclusiveGroups;
};
```

Equivalent recursive advancement storage is acceptable. Requirements:

* keys are stable authored IDs, never indices;
* externally visible iteration follows board/node/condition definition order;
* completion of any condition is persistent once reached;
* an incomplete `fight` condition persists its count of closed qualifying fight windows
  and its consecutive-fight streak, so requirements such as two qualifying fights work;
* a `game` condition additionally persists its open metric and per-ID buckets;
* open ability/turn/fight metrics and incomplete ability/turn window counts are
  battle-result temporaries and are absent/reset at the result boundary;
* every nested `allOf`/`anyOf` child has its own condition-ID entry; do not collapse a
  composite requirement into one scalar;
* fresh progress creates every current node/condition entry, completes the root exactly
  once, and leaves all others incomplete;
* completed-node presets reconstruct/validate exclusive choices instead of trusting a
  contradictory map;
* no percentage float is stored.

Implement:

```cpp
[[nodiscard]] FeatBoardProgress makeFreshFeatBoardProgress(
    const FeatBoardDefinition &p_board);

[[nodiscard]] FeatBoardProgress makePresetFeatBoardProgress(
    const FeatBoardDefinition &p_board,
    std::span<const std::string> p_completedNodeIds);
```

The preset function is used for encounter opponents. Root is implicit and must not need to
appear in JSON. Reject unknown nodes, duplicates, impossible disconnected completion,
unsatisfied minimum/from-form gates, and multiple choices in one exclusive group. It does
not pretend the enemy fulfilled runtime requirements; the encounter author is explicitly
selecting a legal completed-node state.

Step 17 may refine advancement fields to match the implemented condition engine, but must
retain these identity/persistence rules without a parallel progress model.

---

# 8. Derived creature state and reward replay seam

Definitions and progress derive a battle-ready immutable value:

```cpp
struct DerivedCreatureState
{
    CreatureAttributes attributes;
    std::vector<std::string> abilityIds;
    std::vector<std::string> passiveStatusIds;
    std::string formId;
};

[[nodiscard]] DerivedCreatureState deriveCreatureState(
    const CreatureSpeciesDefinition &p_species,
    const FeatBoardDefinition &p_board,
    const FeatBoardProgress &p_progress,
    const Registries &p_registries);
```

Implement the final deterministic replay order now because enemy presets and step-06 battle
projections require it:

```text
attributes = species.baseAttributes
abilities  = species.defaultAbilityIds, unique in authored order
passives   = species.defaultPassiveStatusIds, unique in authored order
form       = species.defaultFormId

for node in board definition order:
    if progress says completed:
        for reward in authored order:
            bonusStat      -> checked addition
            unlockAbility  -> append only if absent
            removeAbility  -> erase if present
            unlockPassive  -> append only if absent
            changeForm     -> replace form

validate final attributes, IDs, form, and ability capacity
return value
```

Replay is pure, deterministic, and idempotent. It does not mutate a battle unit or apply a
reward incrementally. Step 17 reuses this function after evaluating new completions and adds
the progression transaction/summary tests; it must not create a second derivation path.

---

# 9. Persistent `CreatureUnit`

Use a value type equivalent to:

```cpp
struct CreatureUnit
{
    CreatureInstanceId id;
    std::string speciesId;
    FeatBoardProgress featProgress;

    // Rebuildable convenience cache; never authoritative save data.
    DerivedCreatureState derived;
};
```

Construction goes through a checked factory:

```cpp
[[nodiscard]] CreatureUnit makeCreatureUnit(
    CreatureInstanceId p_id,
    std::string p_speciesId,
    std::span<const std::string> p_completedNodeIds,
    const Registries &p_registries);

void rebuildDerivedState(CreatureUnit &p_unit, const Registries &p_registries);
```

The factory resolves species/board, builds fresh/preset progress, derives state, and either
returns one complete value or throws/returns an error without exposing a partial object.

Do not store level, XP, AI, battle side, current form independently, current HP/AP/MP,
position, statuses, shields, turn-bar fill, or taming progress. Step 06 projects a
session-local `BattleUnit`; step 18 intentionally omits `derived` from saves and rebuilds it.

---

# 10. Six-slot roster and PC storage

Use exactly six active slots:

```cpp
class PlayerRoster
{
public:
    static constexpr std::size_t TeamCapacity = 6;
    using Team = std::array<std::optional<CreatureUnit>, TeamCapacity>;

private:
    Team _team;
    std::vector<CreatureUnit> _storage;

public:
    [[nodiscard]] const Team &team() const noexcept;
    [[nodiscard]] const std::vector<CreatureUnit> &storage() const noexcept;
    [[nodiscard]] const CreatureUnit *find(CreatureInstanceId) const noexcept;
    [[nodiscard]] CreatureUnit *findMutable(CreatureInstanceId) noexcept;

    // checked ID-based team/storage operations
};
```

Required operations:

* add a new creature to the lowest empty team slot, otherwise append storage;
* move team creature to storage;
* move storage creature to a specified empty team slot;
* swap two team slots;
* swap a team and storage creature;
* remove by ID only for explicit administrative/test use, returning the removed value;
* validate the whole roster.

Expose result values describing destination/index and errors; do not return a long-lived
pointer into `_storage` across mutations.

All operations are transactional. Prepare/validate against a copy or perform every
potentially failing check before mutation. On failure, team and storage order are unchanged.

Roster invariants:

* team array is always length six;
* each occupied value has valid ID/species/progress/derived state;
* instance IDs are unique across team and storage;
* storage order is stable insertion order;
* empty team slots are ordinary `nullopt`, not dummy creatures;
* no raw definition pointers;
* capacity comes from the compile-time/GDD invariant and `game-rules.json` must equal six.

Team editing UI and boxes/pages are later work; this step supplies headless operations.

---

# 11. `PlayerData` and deterministic serial allocation

Add persistent run/player value state equivalent to:

```cpp
struct PlayerData
{
    PlayerRoster roster;
    std::uint64_t nextCreatureSerial = 1;
    std::uint64_t encounterSerial = 0;

    spk::Vector3Int playerCell{};
    spk::Vector3Int lastHealPoint{};

    std::set<std::string> clearedTrainerIds;
    std::set<std::string> clearedGymIds;
    std::set<std::string> clearedSpecialEncounterIds;

    [[nodiscard]] CreatureInstanceId allocateCreatureId();
    [[nodiscard]] std::uint32_t encounterTier() const noexcept;
};
```

`allocateCreatureId` formats `nextCreatureSerial`, checks non-zero/overflow/collision, and
reserves it by advancing the counter on the working `PlayerData`. A higher-level
`createAndAddCreature` operation constructs/validates the unit before publishing that
working value. Callers that may fail work on a copy, so discarding the copy also discards a
reserved serial; step 18 uses that transaction boundary. No random UUID.

`encounterTier()` is the cleared-gym count clamped to `[0,8]`; numeric tier 9 is reserved
for future postgame state. Do not store a separate badge count that can drift. Step 12/18
persist/use `encounterSerial` for stable repeat encounter identities.

`playerCell` and `lastHealPoint` are initialized to the generated spawn by scene/bootstrap
once it is known. The JSON new-game definition does not hard-code a world cell.

Add `PlayerData player` to `GameContext`. Construct it from registries before constructing
`GameSceneWidget`, while preserving `WorldContext` teardown rules. Step 12 later adds the
authoritative mode/world-seed/world-plan fields.

---

# 12. New-game JSON

Add a single immutable config:

```text
Playground/resources/data/config/new-game.json
```

Schema:

```json
{
  "version": 1,
  "starterTeam": [
    {
      "species": "training-sprout",
      "completedFeatNodes": []
    }
  ]
}
```

Use:

```cpp
struct NewGameCreatureDefinition
{
    std::string speciesId;
    std::vector<std::string> completedFeatNodeIds;
};

struct NewGameDefinition
{
    std::vector<NewGameCreatureDefinition> starterTeam;
};

[[nodiscard]] PlayerData makeNewPlayerData(
    const NewGameDefinition &p_definition,
    const Registries &p_registries);
```

Validation:

* version exactly 1 and strict fields;
* 1-6 starter entries;
* known species and legal completed-node presets;
* root is implicit;
* no authored instance IDs;
* array order becomes team slot order;
* IDs allocate serials 1..N and `nextCreatureSerial == N+1`;
* initial storage/cleared sets empty and encounter serial zero;
* every derived state rebuilds successfully.

Store `NewGameDefinition` in `Registries` or an equivalently immutable bootstrap catalog
loaded inside the same transaction. Provide a const getter. Do not conflate it with a save
file; step 18 defines saves separately.

---

# 13. Ordered-rule AI definitions

AI definitions describe intent only. Step 11 evaluates them against snapshots and submits
ordinary commands. Do not store callbacks or execute logic while parsing.

Use:

```cpp
enum class AIUnitSelector
{
    Self,
    NearestEnemy,
    NearestAlly,
    LowestHealthEnemy,
    LowestHealthAlly,
    HighestHealthEnemy,
    HighestHealthAlly
};

enum class AIInclusiveComparison
{
    AtMost,
    AtLeast
};

enum class AIBestAreaPreference
{
    Enemies,
    Allies
};

struct AIAlwaysCondition
{
    // Explicit empty value: JSON type "always" owns no hidden state.
};

struct AIHealthRatioCondition
{
    AIUnitSelector selector = AIUnitSelector::Self;
    AIInclusiveComparison comparison = AIInclusiveComparison::AtMost;
    std::uint16_t permille = 0; // validated [0, 1000]
};

struct AIResourceCondition
{
    BattleResource resource = BattleResource::ActionPoints;
    int amount = 0; // validated [0, INT_MAX]
};

struct AIDistanceCondition
{
    AIUnitSelector selector = AIUnitSelector::NearestEnemy;
    AIInclusiveComparison comparison = AIInclusiveComparison::AtMost;
    std::uint16_t cells = 0; // validated [0, 128]
};

struct AIStatusIdReference
{
    std::string statusId;
};

struct AIStatusTagReference
{
    std::string tag;
};

using AIStatusReference = std::variant<
    AIStatusIdReference,
    AIStatusTagReference>;

struct AIStatusCondition
{
    AIUnitSelector selector = AIUnitSelector::Self;
    AIStatusReference reference = AIStatusIdReference{};
    bool present = true;
};

struct AIAbilityAffordableCondition
{
    std::string abilityId;
};

using AIConditionSpec = std::variant<
    AIAlwaysCondition,
    AIHealthRatioCondition,
    AIResourceCondition,
    AIDistanceCondition,
    AIStatusCondition,
    AIAbilityAffordableCondition>;

struct AIUnitAnchor
{
    AIUnitSelector selector = AIUnitSelector::Self;
};

struct AISelfCellAnchor
{
    // Explicit empty value: the acting unit is supplied by the planner context.
};

struct AIBestAreaAnchor
{
    AIBestAreaPreference preferred = AIBestAreaPreference::Enemies;
};

struct AINearestLegalCellToAnchor
{
    AIUnitSelector selector = AIUnitSelector::NearestEnemy;
};

using AIAnchorSpec = std::variant<
    AIUnitAnchor,
    AISelfCellAnchor,
    AIBestAreaAnchor,
    AINearestLegalCellToAnchor>;

struct AICastAbilityDecision
{
    std::string abilityId;
    AIAnchorSpec anchor = AISelfCellAnchor{}; // owned value, never a borrowed JSON node
};

struct AIMoveTowardDecision
{
    AIUnitSelector selector = AIUnitSelector::NearestEnemy;
    std::uint32_t maximumMovementPoints = 0; // 0 means all current MP; else [1, 1000000]
};

struct AIMoveAwayDecision
{
    AIUnitSelector selector = AIUnitSelector::NearestEnemy;
    std::uint32_t maximumMovementPoints = 0; // 0 means all current MP; else [1, 1000000]
};

struct AIEndTurnDecision
{
    // Explicit empty value: the acting unit is supplied by the planner context.
};

using AIDecisionSpec = std::variant<
    AICastAbilityDecision,
    AIMoveTowardDecision,
    AIMoveAwayDecision,
    AIEndTurnDecision>;

struct AIRuleDefinition
{
    std::string id;
    std::vector<AIConditionSpec> conditions;
    AIDecisionSpec decision;
};

struct AIBehaviourDefinition
{
    std::string id;
    std::string displayName;
    std::vector<AIRuleDefinition> rules;
};
```

These are value types. The in-class initializers prevent uninitialized storage; they do not
make JSON members optional. The strict parser still requires every member shown by the
selected JSON alternative, rejects unknown members, and validates before publication.
`AIStatusReference` is the one closed representation of the two status queries:

```text
type: "hasStatus"     + status -> AIStatusIdReference{statusId}
type: "hasStatusTag"  + tag    -> AIStatusTagReference{tag}
```

Exactly one representation is constructed, so a condition can never carry both a status ID
and a tag or neither. Status IDs resolve against the candidate status registry built inside
the same all-registry load transaction. Tags use the step-01 content-ID grammar and must
occur on at least one status definition in that candidate graph; this catches misspelled AI
tags before the graph is published. `AICastAbilityDecision` owns its fully parsed
`AIAnchorSpec` by value. No AI definition retains a JSON node, registry pointer, callback,
or runtime battle object.

Selector ties always use encounter roster order, then `BattleUnitId`. Nearest distance is
Manhattan x/z. Health ratios are integer permille computed from effective current/max HP.

---

# 14. AI JSON catalog

Exact v1 root:

```json
{
  "version": 1,
  "displayName": "Training Aggressive",
  "rules": [
    {
      "id": "strike-nearest",
      "conditions": [
        { "type": "abilityAffordable", "ability": "training-strike" }
      ],
      "decision": {
        "type": "castAbility",
        "ability": "training-strike",
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

Empty conditions mean true/AND identity. `always` exists for explicit readability but may
not be combined with other conditions in one rule.

Condition alternatives:

```json
{ "type": "always" }
{ "type": "healthRatio", "selector": "self", "comparison": "atMost", "permille": 500 }
{ "type": "resourceAtLeast", "resource": "actionPoints", "amount": 2 }
{ "type": "distance", "selector": "nearestEnemy", "comparison": "atMost", "cells": 3 }
{ "type": "hasStatus", "selector": "self", "status": "training-guarded", "present": true }
{ "type": "hasStatusTag", "selector": "nearestEnemy", "tag": "poison", "present": true }
{ "type": "abilityAffordable", "ability": "training-strike" }
```

`AIInclusiveComparison` maps exactly to `atMost|atLeast`; health permille is `[0,1000]`.
Resource amount is a non-negative runtime-pool integer and must fit `int`; a JSON value above
`INT_MAX` fails contextually rather than narrowing. Distance cells are `[0,128]`.
`hasStatus` and `hasStatusTag` map to the distinct `AIStatusReference` alternatives above.
`abilityAffordable` checks current costs only; full target legality belongs to decision
planning/gateway.

Anchor alternatives:

```json
{ "type": "unit", "selector": "nearestEnemy" }
{ "type": "selfCell" }
{ "type": "bestArea", "preferred": "enemies" }
{ "type": "nearestLegalCellTo", "selector": "nearestEnemy" }
```

`AIBestAreaPreference` maps exactly to `enemies|allies`; step 11 enumerates legal anchors, maximizes
preferred affected active units, minimizes non-preferred affected units, then uses canonical
cell order. It never ignores the ability's affected filter.

Decision alternatives:

```json
{ "type": "castAbility", "ability": "training-strike", "anchor": { ... } }
{ "type": "moveToward", "selector": "nearestEnemy", "maximumMovementPoints": 0 }
{ "type": "moveAway", "selector": "nearestEnemy", "maximumMovementPoints": 0 }
{ "type": "endTurn" }
```

Zero maximum means all currently available MP; otherwise `[1,1000000]`. Movement decisions
choose among legal gateway paths/cells later and never call occupancy directly.

AI validation requires version 1, non-empty rules, unique rule IDs, strict alternative
fields, valid referenced abilities/statuses, and a final unconditional/end-turn fallback.
It may conservatively reject an AI with no possible termination path. Rules remain in JSON
order. If conditions match but a decision produces no legal command, step 11 continues to
the next rule. After each successful command, evaluation restarts at rule zero.

No random rule, weighted choice, mode dictionary, script, utility score expression, or
AI-only effect is allowed in v1.

---

# 15. Battle-board and encounter definition values

Use `resources/data/battle-boards` for reusable handcrafted arena definitions and the
existing `resources/data/encounter-tables` directory for encounters. Expose each through its
own immutable registry getter. Board definitions describe authored geometry and deployment;
encounter board policies choose that geometry and enemy placement. They do not contain
runtime occupancy or mutable battle state.

```cpp
enum class OpponentPlacementKind { Fixed, ByLine, SeededRandom };
enum class OpponentLineOrder { CenterOut, LeftToRight, RightToLeft };

struct FixedOpponentPlacementPolicy
{
    std::vector<std::array<int, 2>> cells; // authored board-local [x,z] order
};

struct ByLineOpponentPlacementPolicy
{
    int rowsFromEnemyEdge = 0;
    OpponentLineOrder order = OpponentLineOrder::CenterOut;
};

struct SeededRandomOpponentPlacementPolicy {};

using OpponentPlacementPolicy = std::variant<
    FixedOpponentPlacementPolicy,
    ByLineOpponentPlacementPolicy,
    SeededRandomOpponentPlacementPolicy>;

struct LiveWorldBoardPolicyDefinition
{
    std::array<int, 2> size{11, 11};
    int deploymentDepth = 2;
    OpponentPlacementPolicy opponentPlacement;
};

struct HandcraftedBattleBoardDefinition
{
    std::string id;
    std::string displayName;
    spk::Vector3Int size; // x,y,z
    std::string geometryPrefabId;
    spk::VoxelOrientation playerApproach;
    int deploymentDepth = 0;
};

struct HandcraftedBoardPolicyDefinition
{
    std::string definitionId;
    OpponentPlacementPolicy opponentPlacement;
};

using EncounterBoardPolicyDefinition = std::variant<
    LiveWorldBoardPolicyDefinition,
    HandcraftedBoardPolicyDefinition>;

struct EncounterSpawnDefinition
{
    std::string id;
    std::string speciesId;
    std::string aiBehaviourId;
    std::vector<std::string> completedFeatNodeIds;
};

struct EncounterTeamDefinition
{
    std::string id;
    std::string displayName;
    std::uint64_t weight = 1;
    std::vector<EncounterSpawnDefinition> members;
};

struct EncounterTierDefinition
{
    std::uint32_t tier = 0;
    std::vector<EncounterTeamDefinition> teams;
};

struct EncounterDefinition
{
    std::string id;
    std::string displayName;
    EncounterKind kind;
    bool allowsTaming = false;
    bool repeatable = true;
    std::uint32_t triggerChancePermille = 1000;
    EncounterBoardPolicyDefinition board;
    std::vector<EncounterTierDefinition> tiers;
};
```

The handcrafted board file is strict version 1. This is an exact complete example:

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

The only allowed top-level keys are exactly `version`, `displayName`, `size`,
`geometryPrefab`, `playerApproach`, and `deploymentDepth`; all six are required. The stable
board ID comes from the filename through the common registry loader. Validation is exact:

* `version` is the integer `1`;
* `size` is exactly three integers; X and Z are odd and each in `[5,31]`, while Y is in
  `[3,64]`;
* compute `size.x * size.y * size.z` with checked wide multiplication before any grid
  allocation, even though the v1 limits are deliberately small;
* `geometryPrefab` is a non-empty stable ID resolving to an existing strict
  `PrefabDefinition` parsed by the normal prefab registry; battle boards do not introduce a
  second voxel/palette/prefab parser;
* `playerApproach` is one of the four horizontal orientation literals
  `positiveX|negativeX|positiveZ|negativeZ`;
* `deploymentDepth` is positive and `2 * deploymentDepth` is at most the board extent on
  the approach axis, using checked arithmetic;
* every listed prefab voxel, including an explicitly authored empty/carve cell, lies inside
  `[0,size.x) x [0,size.y) x [0,size.z)` when applied with the identity transform. Identity
  here means `PositiveZ`, no flip, and no translation: pass `prefab.pivot()` as Sparkle's
  stamp destination so the applied position equals the authored `Prefab::Voxel::position`.
  Reject the board/reference if even one cell is outside; never rely on `Prefab::applyTo`
  silently clipping it.

Expose a cohesive parser equivalent to:

```cpp
[[nodiscard]] HandcraftedBattleBoardDefinition parseHandcraftedBattleBoardDefinition(
    JsonReader &p_reader,
    const Registry<PrefabDefinition> &p_prefabs);
```

Load files in canonical filename/ID order with the common local registry transaction. The
parser returns owned values and performs prefab lookup only for validation; the definition
stores `geometryPrefabId`, never a pointer/reference into the temporary prefab registry.

This step parses and cross-validates the board/prefab graph but does not yet allocate the
arena grid or navigation. Step 05 performs the same bounds check defensively, materializes
the validated `PrefabDefinition` into an empty owned grid at that exact identity transform,
and validates standability/deployment capacity.

---

# 16. Encounter JSON schema

Exact wild example:

```json
{
  "version": 1,
  "displayName": "Training Wilds",
  "kind": "wild",
  "allowsTaming": true,
  "repeatable": true,
  "triggerChancePermille": 1000,
  "board": {
    "type": "liveWorld",
    "size": [11, 11],
    "deploymentDepth": 2,
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
          "id": "lone-sprout",
          "displayName": "Lone Training Sprout",
          "weight": 1,
          "members": [
            {
              "id": "sprout-a",
              "species": "training-sprout",
              "ai": "training-aggressive",
              "completedFeatNodes": []
            }
          ]
        }
      ]
    }
  ]
}
```

The board object is a strict discriminator alternative. A `liveWorld` object allows exactly
`type`, `size`, `deploymentDepth`, and `opponentPlacement`, all required. A handcrafted
object allows exactly the following three required keys:

```json
{
  "board": {
    "type": "handcrafted",
    "definition": "verdant-trial-arena",
    "opponentPlacement": {
      "type": "byLine",
      "rowsFromEnemyEdge": 0,
      "order": "centerOut"
    }
  }
}
```

`definition` resolves in `battleBoards()`. A handcrafted encounter may not repeat `size`,
`deploymentDepth`, or `playerApproach`; those come from the referenced board definition.
Conversely, a live-world alternative may not carry `definition`. Placement validation uses
the effective size/depth from the live object or referenced handcrafted definition.

Kind literals use step 01. Validation:

* `allowsTaming:true` only with `wild`; non-wild must be false;
* repeatability is kind-locked despite remaining explicit/self-describing in JSON: `wild`
  and `debug` require `repeatable:true`; `trainer`, `gym`, and `special` require
  `repeatable:false`, matching permanent clear-on-victory persistence;
* debug encounters cannot tame;
* chance is `[0,1000]` and only used for biome-triggered wild encounters;
* `wild` and `trainer` require the `liveWorld` board alternative;
* `gym` and `special` require the `handcrafted` board alternative;
* `debug` may use either alternative, but still obeys that alternative's complete schema;
* board dimensions/deployment obey their exact bounds and every handcrafted definition
  resolves before encounter publication;
* tier array non-empty, strictly increasing, unique, values `[0,9]`, and first tier 0;
* tier 9 is reserved postgame; current gym count reaches only 0-8;
* every tier has at least one team and every team weight is an integer in
  `[1, 1000000000]`;
* weight sum is checked in `uint64_t`;
* every team has 1-6 members and stable unique member IDs;
* member order is authoritative enemy roster/tie order;
* species/AI/nodes exist and preset progress is legal;
* every `castAbility` referenced by that member's AI exists in its derived ability list;
* a wild species may be untameable; that simply creates no tracker later.

## 16.1 Placement alternatives

By-line:

```json
{ "type": "byLine", "rowsFromEnemyEdge": 0, "order": "centerOut" }
```

`rowsFromEnemyEdge` is an integer in `[0, board.deploymentDepth - 1]`. It selects the first
eligible row at that offset from the enemy's outer edge; enumeration then proceeds row by row
toward the board centre without wrapping back toward the edge. Within each row, order is
`centerOut|leftToRight|rightToLeft`: smaller board-local perpendicular coordinate is left;
left/right orders are monotonic; center-out sorts by doubled distance from the row's
coordinate midpoint and chooses the smaller coordinate on a tie. Multiple vertical support
cells in one x/z column use increasing y then `BoardCellLess`. The policy must expose enough
logical row positions for the largest team at definition validation; after the concrete live
or handcrafted board is built, step 06 must also find enough standable zone cells or session
construction fails without placement/RNG mutation.

Fixed:

```json
{ "type": "fixed", "cells": [[5, 10], [4, 10], [6, 10]] }
```

Cells are zero-based local board `[x,z]`, unique, inside the effective X/Z extent, and the
authored list must cover the largest team in any tier. For a handcrafted alternative, the
referenced definition supplies `playerApproach`, so definition validation also requires every
cell in its enemy deployment strip. A live-world approach is supplied only by the triggering
movement; its fixed cells are checked against that concrete enemy strip during board/session
construction, and an incompatible approach fails entry without placement or RNG mutation.
Once a concrete board exists, a column with multiple standable support cells selects
increasing y then `BoardCellLess`; if it has none, session construction rejects the placement.
Resolved cells must remain unique. Members use the first N resolved cells.

Seeded random:

```json
{ "type": "seededRandom" }
```

Seeded random selects without replacement from canonical legal enemy-zone cells using the
battle-local RNG. It never uses `random_device` or render time.

Enemy placement is authored; player placement remains manual in step 06/12.

---

# 17. Tier and weighted-team resolution

Provide pure headless functions equivalent to:

```cpp
[[nodiscard]] const EncounterTierDefinition &selectEncounterTier(
    const EncounterDefinition &p_definition,
    std::uint32_t p_progressionTier);

[[nodiscard]] const EncounterTeamDefinition &selectWeightedTeam(
    const EncounterTierDefinition &p_tier,
    BattleRng &p_rng);

struct ResolvedEncounter
{
    std::string encounterDefinitionId;
    EncounterKind kind;
    bool allowsTaming = false;
    bool repeatable = true;
    std::uint32_t resolvedTier = 0;
    std::string teamId;
    EncounterBoardPolicyDefinition board;
    std::vector<EncounterSpawnDefinition> enemyRoster;
};

[[nodiscard]] ResolvedEncounter resolveEncounter(...);
```

Tier selection chooses the greatest authored `tier <= requested`; sparse tiers therefore
fall back to the highest lower tier. A requested value above 9 is rejected at the public
progression boundary; it is never clamped or wrapped.

Weighted selection:

```text
total = checked sum of positive weights in authored order
ticket = rng.uniformBelow(total)
walk teams in authored order, subtracting weight
select first containing ticket
```

Chance for a Bush-trigger request uses one bounded-sampling call:

```text
roll = rng.uniformBelow(1000)
trigger iff roll < triggerChancePermille
```

When the chance function is called it invokes `uniformBelow(1000)` exactly once even for 0
or 1000. Because `uniformBelow` is unbiased rejection sampling, that one call consumes one
or more raw `nextU64` draws; `BattleRng::drawCount()` must report the actual seed-dependent
number. Debug/named direct encounters skip the chance call and proceed to one
`uniformBelow(totalWeight)` team-selection call. Definition validation guarantees
selection cannot fail after bounded sampling.

Resolution returns copies/IDs, not definition pointers that outlive a transaction. The board
alternative and handcrafted definition ID are copied unchanged; resolution does not
materialize geometry, construct battle units, or allocate persistent creature IDs.

---

# 18. Biome-to-wild encounter link

Migrate `BiomeDefinition` and every checked-in biome file to version 2. Add:

```cpp
std::optional<std::string> wildEncounterTableId;
```

Top-level JSON field:

```json
"wildEncounterTable": "training-wild"
```

Rules:

* version-2 parser includes the field in `forbidUnknown`;
* every overworld biome with a `worldgen` block requires a non-empty valid encounter ID;
* interior-only/non-worldgen biomes omit it; explicit JSON `null` is invalid in version 2;
* reference validation occurs after the encounter registry loads;
* referenced encounter kind must be wild and repeatable;
* no encounter fields are added to `VoxelDefinition`;
* Bush detection later remains `VoxelData::hasTag("Bush")`; biome selects the team.

For this bootstrap step, point every generated overworld biome to `training-wild`. Step 19
replaces links with biome-appropriate content. Leave cave/interior content without a wild
table until the interior encounter policy is implemented.

This is an intentional schema migration: reject biome version 1 after all checked-in files
are converted. Update docs/tests accordingly.

---

# 19. Full registry load and validation transaction

Extend `Registries` with:

```cpp
Registry<AIBehaviourDefinition> _aiBehaviours;
Registry<CreatureSpeciesDefinition> _species;
Registry<HandcraftedBattleBoardDefinition> _battleBoards;
Registry<EncounterDefinition> _encounters;
NewGameDefinition _newGame;

[[nodiscard]] const Registry<AIBehaviourDefinition> &aiBehaviours() const noexcept;
[[nodiscard]] const Registry<CreatureSpeciesDefinition> &species() const noexcept;
[[nodiscard]] const Registry<HandcraftedBattleBoardDefinition> &battleBoards() const noexcept;
[[nodiscard]] const Registry<EncounterDefinition> &encounters() const noexcept;
[[nodiscard]] const NewGameDefinition &newGame() const noexcept;
```

Use this semantic order while preserving dependencies already required by world loading:

```text
parse game rules
parse all existing world/voxel/prefab/biome definitions into locals
parse statuses, battle objects, abilities into locals
parse/validate feat boards against combat locals
parse/validate AI against combat locals
parse species against combat + feat locals
parse/validate handcrafted battle boards against voxel + prefab locals
parse encounters against species + AI + feat + handcrafted-board locals
validate biome wild-table IDs against encounter locals
parse/validate new-game config against species/derived-state locals
run one final graph/invariant validation
publish every member by move
print counts
```

Where current dependencies require a different parse order, keep raw IDs and perform the
semantic validation in the final phase. Prefab/voxel locals must exist before battle-board
reference and applied-coordinate validation; battle-board locals must exist before encounter
kind/policy validation. Never publish `_species` or `_battleBoards` and then fail on an
encounter, biome, or starter.

All cross-reference errors retain owning file/path. Definitions store IDs after validation,
not raw pointers.

---

# 20. Required bootstrap content

Add:

```text
Playground/resources/data/battle-boards/.gitkeep
Playground/resources/data/creatures/training-sprout.json
Playground/resources/data/ai/training-aggressive.json
Playground/resources/data/encounter-tables/training-wild.json
Playground/resources/data/encounter-tables/debug-battle.json
Playground/resources/data/config/new-game.json
```

Use sections 5, 12, 14, and 16 exactly.

The battle-board registry is intentionally empty at this bootstrap checkpoint because the
two step-04 encounters are live-world encounters. The loader and schema are real, tested,
and ready for the first authored Gym/Special arena in step 19; do not add a fake unused
arena merely to make the count non-zero.

`debug-battle.json` is a one-team live-world encounter:

* `kind: "debug"`;
* `allowsTaming: false`;
* `repeatable: true`;
* same 11x11 by-line-placement board;
* tier 0 with one `training-sprout` using `training-aggressive`.

This named ID is the F8/CLI target in step 12. It must go through the same encounter
resolver as wild content, except it skips Bush chance/taming.

Migrate every generated-world biome to reference `training-wild`. Do not add three final
species or production balance here; step 19 owns the complete small content pack.

---

# 21. Starter bootstrap integration

In `main.cpp`, after successful registry load and before `GameSceneWidget` construction:

1. create `PlayerData` from `registries.newGame()`;
2. put it in `GameContext` through a constructor or explicit value move;
3. let scene construction initialize player/heal world cells when the generated spawn is
   successfully committed;
4. print one concise deterministic line per occupied starter slot, for example:

```text
Starter roster [0]: creature-0000000000000001 -> training-sprout (base)
```

Do not log raw addresses or derived object dumps. Map-only/check-stairs paths do not need to
construct player state unless the live main organization makes it harmless.

If scene construction fails, ordinary stack unwinding destroys the value state; no global
service remains.

---

# 22. Required species/derivation tests

Add tests under `Playground/tests/creatures/`.

Test:

* every attribute boundary and exact stamina parsing;
* strict species/form/presentation schemas;
* missing/duplicate/unknown default IDs;
* passive stun rejection;
* default form/tier rules;
* board/form/fromForm/changeForm cross-reference errors;
* ability-capacity union;
* absent/null/valid/invalid taming profiles;
* fresh root progress and preset-node legality;
* every reward kind in deterministic board/reward order;
* checked stat overflow/underflow;
* unlock/remove idempotence and stable order;
* pure repeated derivation produces byte/value-identical results;
* two same-species units with different progress remain independent;
* no level/random scaling appears.

---

# 23. Required roster/new-game tests

Add tests under `Playground/tests/player/`.

Test:

* team always has six slots;
* new-game entries receive serial IDs/order exactly;
* next serial advances correctly and overflow/collision fails without mutation;
* add fills lowest empty slot then PC;
* every move/swap operation and invalid index/occupied target/missing ID case;
* duplicate IDs across team/storage rejected;
* storage order stable;
* failed operation leaves a deep-equal roster;
* lookup never uses species as identity;
* copied `PlayerData` can be mutated transactionally without changing original;
* new-game missing/unknown/too many/duplicate-invalid entries fail contextually;
* derived cache rebuilds and is not treated as authoritative progress.

---

# 24. Required AI tests

Add parser/validator tests under `Playground/tests/ai/` for every condition, selector,
anchor, and decision alternative. Test:

* strict unknown/missing/type/version handling;
* unique/stable rule order;
* empty condition AND identity;
* explicit always combination rejection;
* ratio/resource/distance/movement bounds;
* known/unknown ability/status/tag references;
* final termination fallback;
* deterministic selector tie contract documented as roster then unit ID;
* AI definition itself consumes no RNG and mutates no state.

Do not implement planner behavior tests here; step 11 owns snapshot evaluation and common
command submission.

---

# 25. Required encounter/biome tests

Add tests under `Playground/tests/encounters/` and extend biome registry tests.

## 25.1 Schema/reference validation

For handcrafted board files, test the exact allowed-key set, missing keys, unsupported
version, non-array/wrong-length/non-integer size, every min/max/parity boundary, checked
volume, every approach literal, invalid deployment depth on both deployment axes, missing
prefab, negative/upper-bound prefab coordinates, and an explicitly empty carve cell outside
the board. Prove validation uses identity placement (`destination == prefab.pivot()`) and
rejects instead of clipping. In every failure case, a previously loaded complete
`Registries` value—including its prior battle-board registry—remains unchanged.

For encounters, test every kind, every legal/illegal kind-to-board pairing, strict fields for
both board alternatives, missing handcrafted definition, taming/repeatability contradiction,
chance boundary, board bound, placement alternative, tier order/range, zero/overflow weight,
team/member count, duplicate local IDs, unknown species/AI/node, illegal completed-node set,
and AI ability absent from derived loadout.

## 25.2 Resolution goldens

Test sparse tier fallback for requests 0-9; exact weighted boundaries with a controllable
golden RNG; roster order preservation; chance comparison at 0/1/999/1000; exact draw-count
contract; same seed/input gives identical `ResolvedEncounter`; different seed can choose a
different weighted team without flaky assertions.

## 25.3 Placement data

Validate by-line rows/order, fixed cells/count/zone/uniqueness, seeded-random no-extra-fields,
and effective size/depth lookup through both board-policy alternatives. No placement or
navigation construction is executed in this step.

## 25.4 Biome migration

Load every real biome as v2. Assert every `worldgen` biome resolves a wild repeatable table,
interior behavior is explicit, version 1 fails, unknown/non-wild references fail, and a bad
biome/encounter leaves the prior complete `Registries` unchanged.

---

# 26. Complete integration tests and smoke run

Add one real-resource integration test that:

1. loads all registries;
2. creates new player data;
3. asserts slot 0 exact ID/species/form/stats/default ability;
4. resolves `debug-battle` at tier 0 with a fixed seed;
5. asserts enemy member species/AI/derived loadout and the `liveWorld` board alternative;
6. asserts the checked-in battle-board registry is empty but its getter/transaction exists;
7. verifies no registry definition changed during derivation/resolution.

Run the complete Playground test label. Smoke-run the executable with a fixed world seed and
resource path, observe registry counts plus starter line, and verify exploration remains
usable. Do not claim a battle starts yet.

---

# 27. Error handling and determinism requirements

* Every definition/nested alternative is strict and versioned.
* Missing references fail startup, never first use.
* All graph validation completes before registry publication.
* Species/AI/handcrafted-board/encounter/roster values own strings/scalars/vectors, not
  reader values or pointers.
* Definition vectors, roster slots, storage, rules, tiers, teams, members, forms, abilities,
  passives, and rewards preserve documented order.
* Lookup acceleration cannot alter outcome order.
* Derived state uses checked integers/fixed time and no RNG.
* Encounter randomness comes only from passed `BattleRng`; invalid definitions/requests draw
  nothing.
* Failed roster/new-game operations allocate no ID and do not advance serials.
* Diagnostics name definition IDs plus file/JSON path.

---

# 28. Expected files and integration points

Add cohesive files equivalent to:

```text
Playground/srcs/creatures/creature_attributes.hpp
Playground/srcs/creatures/creature_species_definition.hpp
Playground/srcs/creatures/creature_species_definition.cpp
Playground/srcs/creatures/creature_unit.hpp
Playground/srcs/creatures/creature_state_derivation.hpp
Playground/srcs/creatures/creature_state_derivation.cpp
Playground/srcs/feats/feat_board_progress.hpp
Playground/srcs/player/player_roster.hpp
Playground/srcs/player/player_roster.cpp
Playground/srcs/player/player_data.hpp
Playground/srcs/player/new_game_definition.hpp
Playground/srcs/player/new_game_definition.cpp
Playground/srcs/ai/ai_definition.hpp
Playground/srcs/ai/ai_definition.cpp
Playground/srcs/board/handcrafted_battle_board_definition.hpp
Playground/srcs/board/handcrafted_battle_board_definition.cpp
Playground/srcs/encounters/encounter_definition.hpp
Playground/srcs/encounters/encounter_definition.cpp
Playground/srcs/encounters/encounter_resolver.hpp
Playground/srcs/encounters/encounter_resolver.cpp
```

Modify:

```text
Playground/srcs/core/registries.hpp/.cpp
Playground/srcs/core/game_context.hpp/.cpp
Playground/srcs/world/biome_definition.hpp/.cpp
Playground/srcs/main.cpp
Playground/resources/data/biomes/*.json
Playground/resources/data/battle-boards/.gitkeep
Playground/docs/02-data-model.md
```

Add tests/content described above. Equivalent organization is acceptable; avoid one-class
wrapper files with no cohesion.

Do not modify Sparkle engine code, tactical board/pathfinder, world generation algorithms,
GameSceneWidget rendering/input beyond initializing spawn cells if needed, or any UI.

---

# 29. Documentation requirements

Document the implemented subset:

* species/base/form/taming JSON schema and exact attribute names;
* placeholder presentation block and its intentionally temporary scope;
* definition versus persistent creature versus battle-unit separation;
* stable Feat progress IDs and derived-cache non-authority;
* exact reward replay order;
* six team slots, PC ordering, ID allocation, and new-game schema;
* complete AI JSON condition/decision catalog and future common-command requirement;
* handcrafted battle-board schema, prefab identity/bounds validation, and registry getter;
* encounter kind/tier/weight/board/placement schemas and deterministic selection;
* biome v2 `wildEncounterTable` migration;
* complete registry load/reference order;
* explicit statement that battle construction/triggers/planning are not implemented yet.

Remove stale docs that store form/stats/abilities independently, use Unity GUIDs,
hard-code badge-named tier fields, imply random level scaling, or claim biome encounters
already run.

---

# 30. Non-goals

Do not implement `BattleUnit`, board data, occupancy, deployment execution, scheduler,
command validation/resolution, effect/status/object runtime, AI evaluation/planning,
exploration Bush triggers, `ModeManager`, presentation, Feat event evaluation, taming runtime,
result commits, save serialization, heal-center interaction, team-management UI, content
authoring tools, final creature catalog/art, levels/XP, items, type charts, or random enemy
stat scaling.

Do not create separate enemy creature classes. Encounter spawns are immutable recipes;
runtime enemies are ordinary derived creature projections in step 06.

---

# 31. Acceptance checklist

This step is complete only when:

* [ ] AI/species/handcrafted-board/encounter registries and new-game getter load
  transactionally;
* [ ] species schema uses the exact attributes and `magicPower` spelling;
* [ ] all default/board/form/taming references validate;
* [ ] placeholder visuals are headless values, not renderer assets/pointers;
* [ ] fresh/preset progress is stable-ID based with root implicit;
* [ ] pure reward replay derives form/stats/abilities/passives deterministically;
* [ ] `CreatureUnit` persists only ID/species/progress plus non-authoritative cache;
* [ ] roster has exactly six nullable slots and ordered storage;
* [ ] every roster operation is ID-based and atomic;
* [ ] new-game JSON allocates deterministic canonical creature IDs;
* [ ] AI definitions are ordered closed data and have a termination fallback;
* [ ] handcrafted board JSON is strict, prefab-backed, identity-bounded, and not yet
  materialized as runtime navigation;
* [ ] encounter kind/taming/repeatability/tier/team/member invariants hold;
* [ ] Wild/Trainer use live-world boards, Gym/Special use handcrafted boards, and Debug may
  use either without discriminator-field leakage;
* [ ] weighted/chance resolution is reproducible with exact bounded-call counts and the
  actual rejection-sampling raw draw count;
* [ ] encounter member AI can only cast derived abilities;
* [ ] every generated-world biome is v2 and references a valid wild table;
* [ ] `training-sprout`, `training-aggressive`, `training-wild`, and `debug-battle` load;
* [ ] integration test constructs exact starter and resolved debug recipe;
* [ ] all parser/derivation/roster/AI/encounter/biome tests pass;
* [ ] full Playground build/tests stay green and exploration smoke-run works;
* [ ] no tactical battle runtime or presentation was implemented.

---

# 32. Required handoff report

Report:

1. exact type/header/getter names for species, progress, derived state, roster, player, AI,
   handcrafted board, encounter, resolver, and new game;
2. exact JSON versions, enum/type literals, directory names, and biome migration consequence;
3. reward replay and completed-node preset algorithms;
4. creature ID allocation and roster transaction behavior;
5. AI selector/anchor/decision catalog and ordering tie-break;
6. handcrafted prefab identity/bounds contract plus encounter tier/chance/weight RNG draw
   contract and board-policy variant types;
7. all content IDs and biome links added;
8. files created/materially changed/deliberately untouched;
9. repository drift/adaptations;
10. commands actually run and results;
11. exact registry/starter smoke output and exploration scenario observed;
12. work explicitly deferred to board/session/runtime steps.
