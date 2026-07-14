# Implement progression definition registries

Add the immutable JSON definitions shared by Feat Progress and wild taming: a closed
battle-condition specification catalog, one-time Feat Board nodes, reward specifications,
evolution gates/choice groups, and inline taming profiles. Load Feat Boards through
`pg::Registries`, validate combat references now, and leave species-local form references
for the species validation pass in step 04.

This step defines and validates data. Do not evaluate battle events, persist progress,
complete nodes, derive creature stats, impress creatures, render a Feat Board, or build an
authoring tool.

---

# 1. Repository baseline and prerequisites

Implement after steps 01-02. Read both handoff reports and re-open the live parser and
registry files before editing.

Required existing contracts are:

* validated definition/local semantic IDs from step 01;
* fixed-point `BattleTime`, common `CreatureStat`, `DamageKind`, `BattleResource`, and
  `BattleOutcome` vocabulary;
* immutable ability, status, and battle-object registries from step 02;
* ordered, value-owned effect specifications with stable effect IDs;
* strict JSON parsing and local-then-commit `Registries::loadAll`;
* an empty checked-in `resources/data/featboards` directory;
* no implemented Feat definition, condition engine, taming definition, progress state, or
  species registry.

The GDD requires the same structured condition idea for Feat Requirements and taming. Do
not implement two unrelated evaluators or copy Unity's `FeatRequirement` class hierarchy.
The C++ specification is shared; the runtime evaluation context later binds whether the
subject is a persistent participant or a wild unit.

All progression `displayName` fields are 1-96 UTF-8 bytes and all user-facing
`description` fields are 1-512 UTF-8 bytes; neither may be all whitespace. Semantic IDs are
ASCII step-01 content IDs and are never trimmed, normalized, localized, or case-folded.

---

# 2. Required final behavior

After this step:

1. `Registries` exposes immutable `FeatBoardDefinition` values loaded by filename ID.
2. A closed recursive `ConditionSpec` can express all initial GDD Feat/taming examples.
3. Every leaf condition states its ability/turn/fight/game window, aggregation,
   comparison, actor perspective, and required qualifying-window count.
4. Condition, node, reward, and exclusive-group IDs are stable semantic strings, never
   array indices.
5. Feat nodes complete once; there is no repeatable-node/repeat-limit model.
6. The root is the only node allowed to have no requirements and starts completed later.
7. Adjacency, gates, rewards, and mutually exclusive evolution choices are fully authored.
8. Ability/status references are validated against step-02 registries.
9. Form IDs are syntax-validated but deliberately resolved only when a species selects the
   board in step 04.
10. `TamingProfileDefinition` uses the same `ConditionSpec` but is embedded/null in species
    JSON rather than placed in a separate registry.
11. Invalid data aborts the complete registry load transaction.
12. A minimal `training-board` loads and appears in the registry report.

There is no runtime progress checkpoint yet. The visible checkpoint is the expanded
registry report and exhaustive schema tests.

Extend `Registries` with the same const-owned pattern as the combat domains:

```cpp
Registry<FeatBoardDefinition> _featBoards;

[[nodiscard]] const Registry<FeatBoardDefinition> &featBoards() const noexcept;
```

The accessor exposes the immutable published registry only after the complete load
transaction succeeds. Do not expose the local parse registry or a mutable getter.

---

# 3. Shared condition perspective vocabulary

A condition is evaluated relative to one bound subject creature/unit. Define:

```cpp
enum class ConditionUnitSet
{
    Subject,
    SubjectAllies,
    SubjectTeam,
    OpponentTeam,
    Any
};

enum class ConditionEventRole
{
    Source,
    Target,
    Either
};
```

JSON literals are `subject`, `subjectAllies`, `subjectTeam`, `opponentTeam`, `any`, and
`source`, `target`, `either`.

Exact meaning:

```text
Feat evaluation:
    subject       = deployed persistent creature's BattleUnit
    subjectTeam   = Player
    opponentTeam  = Enemy

Taming evaluation:
    subject       = the tracked wild BattleUnit
    subjectTeam   = Enemy
    opponentTeam  = Player
```

`subjectAllies` excludes the subject; `subjectTeam` includes it. A leaf's `actor` identifies
which set must occupy its selected `role`. `counterpart` optionally filters the other side
of a source-target event. This makes “the owner heals an ally” and “the player team damages
this wild creature” ordinary data, not service special cases.

Do not use absolute `player`/`enemy` filters in condition JSON. Relative definitions remain
valid for both domains.

---

# 4. Windows, repetition, aggregation, and comparison

Define:

```cpp
enum class ConditionWindow
{
    Ability,
    Turn,
    Fight,
    Game
};

enum class WindowCountMode
{
    Cumulative,
    Consecutive
};

enum class ConditionAggregation
{
    Count,
    Sum,
    Maximum,
    Minimum
};

enum class ConditionComparison
{
    AtLeast,
    GreaterThan,
    AtMost,
    LessThan,
    Equal,
    Between
};
```

JSON uses `ability`, `turn`, `fight`, `game`; `cumulative`, `consecutive`; `count`, `sum`,
`maximum`, `minimum`; and `atLeast`, `greaterThan`, `atMost`, `lessThan`, `equal`, `between`.

Every leaf begins with common fields equivalent to:

```cpp
struct ConditionLeafHeader
{
    ConditionWindow window = ConditionWindow::Fight;
    ConditionUnitSet windowActor = ConditionUnitSet::Any;
    std::uint32_t requiredWindowCount = 1;
    WindowCountMode windowMode = WindowCountMode::Cumulative;
};
```

`ConditionSpec`, defined in section 5, owns `id` and `description` exactly once. The leaf
header deliberately does not repeat them. Its initializers only make programmatic aggregate
construction deterministic; the JSON parser still requires all four header keys on every
leaf and must diagnose a missing key rather than accepting an initializer as a schema
default.

`windowActor` is required on every leaf and selects which action/activation opens an
eligible short window:

* for `ability`, the committed cast source must belong to `windowActor`;
* for `turn`, the activation owner must belong to `windowActor`;
* `fight` and `game` require explicit `windowActor:"any"`; they cannot filter their one
  enclosing window;
* stable window identities are `(BattleId, BattleActionId)`, `(BattleId, TurnIndex)`,
  `BattleId`, and the persistent condition-owner key respectively.

Window semantics locked for step 15:

* `ability`: one successfully committed command/effect batch initiated by one ability;
* `turn`: one unit activation from ActivationStarted through ActivationEnded;
* `fight`: one terminal non-aborted battle;
* `game`: persistent lifetime accumulation across committed battles for Feats. For taming,
  it is reset at encounter construction and therefore means the whole current encounter;
* a leaf computes one metric inside a window and tests its comparison;
* a qualifying window increments the completed-window count once, never once per event;
* `cumulative` keeps previous qualifying windows;
* `consecutive` resets the streak to zero when an eligible window fails;
* the leaf completes when the selected count reaches `requiredWindowCount`;
* incomplete ability/turn/fight metric values do not leak into the next such window;
* for Feats, completed condition state always persists; a `game` leaf also persists its
  open metric/buckets, and an incomplete `fight` leaf persists closed qualifying-window
  count/consecutive streak across battles;
* incomplete ability/turn counts and all open ability/turn/fight metrics reset at the
  battle result boundary; taming advancement of every kind is encounter-local.

`requiredWindowCount` is `[1, 1000000]` for ability, turn, and fight. A `game` leaf is one
continuous persisted window, so it requires `requiredWindowCount:1` and
`windowMode:"cumulative"`; `consecutive` is invalid. A one-window leaf must still author
`windowMode` for uniform schema stability.

Integer event amounts and distances use signed 64-bit checked accumulation. No progress is
stored as a float percentage. Presentation can compute a ratio from advancement values.

---

# 5. Condition identity and recursive shape

Use the following exact supporting vocabulary and value shapes. These types intentionally
mirror the JSON field names; a JSON `type` discriminator is represented by the selected
variant alternative and is not stored a second time.

```cpp
enum class DamageKindFilter
{
    Any,
    Physical,
    Magical
};

enum class DamageComponent
{
    Total,
    Health,
    Shield
};

enum class PresenceFilter
{
    Any,
    Required,
    Forbidden
};

enum class StatusConditionAction
{
    Applied,
    Removed
};

enum class ShieldConditionAction
{
    Applied,
    Absorbed,
    Broken
};

enum class MovementFilter
{
    Voluntary,
    Displacement,
    Teleport,
    Any
};

enum class MovementDirection
{
    Any,
    TowardSource,
    AwayFromSource
};

enum class ResourceConditionAction
{
    Spent,
    Gained,
    Lost,
    NextActivationPenalty
};

enum class ResourceChangeReason
{
    AbilityCost,
    MovementCost,
    Effect,
    ActivationRefill,
    NextActivationPenaltyConsumption,
    EffectiveMaximumClamp
};

enum class PositionSample
{
    TurnStart,
    TurnEnd,
    AfterMove,
    ThroughoutWindow
};

enum class UnitRemovalReasonFilter
{
    Defeated,
    Impressed,
    Any
};

enum class RelativeBattleOutcome
{
    SubjectTeamVictory,
    OpponentTeamVictory,
    Draw,
    Completed
};

enum class SurvivorState
{
    Active,
    NotDefeated
};

struct MetricTest
{
    ConditionAggregation aggregation = ConditionAggregation::Count;
    ConditionComparison comparison = ConditionComparison::AtLeast;
    std::int64_t threshold = 0;
    std::optional<std::int64_t> maximum;
};

struct DamageConditionSpec
{
    ConditionLeafHeader leaf;
    ConditionUnitSet actor = ConditionUnitSet::Any;
    ConditionEventRole role = ConditionEventRole::Either;
    ConditionUnitSet counterpart = ConditionUnitSet::Any;
    DamageKindFilter kind = DamageKindFilter::Any;
    DamageComponent component = DamageComponent::Total;
    PresenceFilter targetHadShield = PresenceFilter::Any;
    std::vector<std::string> sourceAbilities;
    MetricTest metric;
};

struct HealingConditionSpec
{
    ConditionLeafHeader leaf;
    ConditionUnitSet actor = ConditionUnitSet::Any;
    ConditionEventRole role = ConditionEventRole::Either;
    ConditionUnitSet counterpart = ConditionUnitSet::Any;
    std::vector<std::string> sourceAbilities;
    MetricTest metric;
};

struct AbilityCastConditionSpec
{
    ConditionLeafHeader leaf;
    ConditionUnitSet actor = ConditionUnitSet::Any;
    std::vector<std::string> abilities;
    bool sameAbility = false;
    ConditionUnitSet affected = ConditionUnitSet::Any;
    std::uint32_t minimumAffectedUnits = 0;
    std::uint32_t minimumTargetDistance = 0;
    std::uint32_t maximumTargetDistance = 0;
    ConditionComparison comparison = ConditionComparison::AtLeast;
    std::int64_t threshold = 0;
};

struct StatusConditionSpec
{
    ConditionLeafHeader leaf;
    ConditionUnitSet actor = ConditionUnitSet::Any;
    ConditionEventRole role = ConditionEventRole::Either;
    ConditionUnitSet counterpart = ConditionUnitSet::Any;
    StatusConditionAction action = StatusConditionAction::Applied;
    std::vector<std::string> statuses;
    std::vector<std::string> tags;
    MetricTest metric;
};

struct ShieldConditionSpec
{
    ConditionLeafHeader leaf;
    ConditionUnitSet actor = ConditionUnitSet::Any;
    ConditionEventRole role = ConditionEventRole::Either;
    ConditionUnitSet counterpart = ConditionUnitSet::Any;
    ShieldConditionAction action = ShieldConditionAction::Applied;
    DamageKindFilter kind = DamageKindFilter::Any;
    MetricTest metric;
};

struct MovementConditionSpec
{
    ConditionLeafHeader leaf;
    ConditionUnitSet actor = ConditionUnitSet::Any;
    MovementFilter movement = MovementFilter::Any;
    MovementDirection direction = MovementDirection::Any;
    MetricTest metric;
};

struct ResourceConditionSpec
{
    ConditionLeafHeader leaf;
    ConditionUnitSet actor = ConditionUnitSet::Any;
    ResourceConditionAction action = ResourceConditionAction::Spent;
    BattleResource resource = BattleResource::ActionPoints;
    std::vector<ResourceChangeReason> reasons;
    MetricTest metric;
};

struct PositionConditionSpec
{
    ConditionLeafHeader leaf;
    PositionSample sample = PositionSample::TurnStart;
    ConditionUnitSet actor = ConditionUnitSet::Any;
    ConditionUnitSet relativeTo = ConditionUnitSet::Any;
    ConditionComparison comparison = ConditionComparison::AtMost;
    std::uint32_t distance = 0;
    std::optional<std::uint32_t> maximumDistance;
};

struct UnitRemovalConditionSpec
{
    ConditionLeafHeader leaf;
    ConditionUnitSet removed = ConditionUnitSet::Any;
    ConditionUnitSet creditedTo = ConditionUnitSet::Any;
    UnitRemovalReasonFilter reason = UnitRemovalReasonFilter::Any;
    ConditionComparison comparison = ConditionComparison::AtLeast;
    std::int64_t threshold = 0;
};

struct BattleOutcomeConditionSpec
{
    ConditionLeafHeader leaf;
    RelativeBattleOutcome outcome = RelativeBattleOutcome::Completed;
    bool requireSubjectActive = false;
};

struct SurvivorCountConditionSpec
{
    ConditionLeafHeader leaf;
    ConditionUnitSet units = ConditionUnitSet::Any;
    SurvivorState state = SurvivorState::Active;
    ConditionComparison comparison = ConditionComparison::AtLeast;
    std::uint32_t threshold = 0;
};

struct DamageAbsencePredicate
{
    ConditionUnitSet actor = ConditionUnitSet::Any;
    ConditionEventRole role = ConditionEventRole::Either;
    ConditionUnitSet counterpart = ConditionUnitSet::Any;
    DamageKindFilter kind = DamageKindFilter::Any;
    DamageComponent component = DamageComponent::Total;
    std::vector<std::string> sourceAbilities;
};

struct AbilityCastAbsencePredicate
{
    ConditionUnitSet actor = ConditionUnitSet::Any;
    std::vector<std::string> abilities;
};

struct StatusAbsencePredicate
{
    ConditionUnitSet actor = ConditionUnitSet::Any;
    ConditionEventRole role = ConditionEventRole::Either;
    ConditionUnitSet counterpart = ConditionUnitSet::Any;
    StatusConditionAction action = StatusConditionAction::Applied;
    std::vector<std::string> statuses;
    std::vector<std::string> tags;
};

struct MovementAbsencePredicate
{
    ConditionUnitSet actor = ConditionUnitSet::Any;
    MovementFilter movement = MovementFilter::Any;
};

using EventAbsencePredicate = std::variant<
    DamageAbsencePredicate,
    AbilityCastAbsencePredicate,
    StatusAbsencePredicate,
    MovementAbsencePredicate>;

struct EventAbsenceConditionSpec
{
    ConditionLeafHeader leaf;
    EventAbsencePredicate predicate;
};

struct ConditionSpec;

struct AllOfConditionSpec
{
    std::vector<ConditionSpec> children;
};

struct AnyOfConditionSpec
{
    std::vector<ConditionSpec> children;
};

using ConditionPayload = std::variant<
    DamageConditionSpec,
    HealingConditionSpec,
    AbilityCastConditionSpec,
    StatusConditionSpec,
    ShieldConditionSpec,
    MovementConditionSpec,
    ResourceConditionSpec,
    PositionConditionSpec,
    UnitRemovalConditionSpec,
    BattleOutcomeConditionSpec,
    SurvivorCountConditionSpec,
    EventAbsenceConditionSpec,
    AllOfConditionSpec,
    AnyOfConditionSpec>;

struct ConditionSpec
{
    std::string id;
    std::string description;
    ConditionPayload payload;
};
```

`std::vector<ConditionSpec>` is the owning indirection that makes the recursive type legal
in C++17 and later: `ConditionSpec` is forward-declared while the two composite alternatives
are declared, then completed after `ConditionPayload`. The tree has ordinary deep value
ownership, copy/move behavior, and no nullable child pointers. Every payload is therefore
complete when `ConditionPayload` is instantiated; the only incomplete type appears as a
`std::vector` element, which the standard permits with the default allocator.

`ConditionSpec` is the sole owner of `id` and `description`. A leaf alternative contains
one `ConditionLeafHeader`; a composite alternative contains only `children`. No alternative
stores the outer discriminator, identity, or description again. Likewise, the nested
`EventAbsencePredicate` is a second closed variant with exactly the four permitted predicate
types; it cannot hold a window, metric, child condition, or arbitrary negation.

`ConditionLeafHeader leaf`, `MetricTest metric`, and `ConditionPayload payload` are C++
composition only. Their members are flattened into the same condition JSON object shown in
section 7; there are no JSON `"leaf"`, `"metric"`, or `"payload"` keys. The
`EventAbsenceConditionSpec::predicate` member is different: it maps to the actual nested
JSON `"predicate"` object, whose own `"type"` selects one `EventAbsencePredicate`
alternative.

All fields shown in these value types are required JSON keys for their corresponding leaf
or predicate except `MetricTest::maximum` and
`PositionConditionSpec::maximumDistance`, which are omission-only optionals governed by
`between`. Empty ability/status/tag vectors are authored empty arrays with wildcard
semantics, not omitted keys. The numeric and Boolean initializers are deterministic C++
construction defaults only: strict parsing records key presence and rejects a missing
required key before constructing the value. Counts and bounded cell distances use
`std::uint32_t`; event-derived amounts and potentially long-lived counts use
`std::int64_t`; all accumulation uses checked signed 64-bit arithmetic.

`AbilityCastConditionSpec`, `UnitRemovalConditionSpec`, and
`SurvivorCountConditionSpec` carry one direct threshold and therefore reject
`ConditionComparison::Between`; they intentionally have no second operand. `Between` remains
valid only where the value shape supplies its paired upper bound: `MetricTest::maximum` or
`PositionConditionSpec::maximumDistance`.

The exact enum-to-JSON spellings are the lower-camel literals listed in sections 3, 4, and
7. `BattleResource` is the common step-01 enum and retains its existing JSON spellings; do
not introduce a progression-only duplicate.

Every condition at every nesting level has an ID following the step-01 grammar. IDs are
unique across the entire owning Feat Board or taming profile, not merely among siblings.
This gives diagnostics and persisted progress a stable key after arrays are reordered.

`allOf` and `anyOf` contain `children` and no additional window. They complete when all/any
children complete according to the children's own windows. Require at least two children.
Reject recursion deeper than `gameRules.battle.maxConditionDepth` and reject duplicate IDs
while descending. JSON is an acyclic tree because it embeds values; never add condition
references or a general boolean expression language.

---

# 6. Common metric fields

Amount-producing condition variants use:

`MetricTest`, declared with the concrete payloads in section 5 so those payloads are
complete before the variant alias, stores the authored `aggregation`, `comparison`,
`threshold`, and optional `maximum` fields.

Rules:

* `between` requires `maximum` and `threshold <= maximum`;
* other comparisons forbid `maximum`;
* `count` counts qualifying events and ignores event amount;
* `sum` adds effective/applied values, never requested/pre-mitigation values;
* `maximum`/`minimum` select one effective event value;
* negative thresholds are rejected for the initial catalog;
* amount/count/distance upper bound is `1,000,000,000` to keep checked totals practical.

Every parser validates which aggregations make sense. For example, absence has no metric,
while survivor count uses a direct comparison.

---

# 7. Leaf condition catalog

Every object calls `forbidUnknown` for common plus type-specific fields. Arrays of IDs are
deduplicated while preserving authored order; an empty filter array means any ID.

## 7.1 Damage

```json
{
  "id": "big-magic-hit",
  "type": "damage",
  "description": "Deal more than 80 magical HP damage in one ability.",
  "window": "ability",
  "windowActor": "subject",
  "requiredWindowCount": 1,
  "windowMode": "cumulative",
  "actor": "subject",
  "role": "source",
  "counterpart": "opponentTeam",
  "kind": "magical",
  "component": "health",
  "targetHadShield": "any",
  "sourceAbilities": [],
  "aggregation": "maximum",
  "comparison": "greaterThan",
  "threshold": 80
}
```

Fields:

* kind `any|physical|magical`;
* component `total|health|shield` selects applied total, HP, or shield absorption;
* targetHadShield `any|required|forbidden` reads the event's pre-hit
  `targetHadAnyShield` fact, regardless of damage-channel match;
* sourceAbilities filters semantic ability IDs;
* actor/role/counterpart follow section 3;
* standard metric test.

This consumes committed `Damage` values after mitigation and shield resolution. It
does not use authored base damage.

## 7.2 Healing

```json
{
  "id": "heal-allies",
  "type": "healing",
  "description": "Restore 30 effective HP to allies in a fight.",
  "window": "fight",
  "windowActor": "any",
  "requiredWindowCount": 1,
  "windowMode": "cumulative",
  "actor": "subject",
  "role": "source",
  "counterpart": "subjectAllies",
  "sourceAbilities": [],
  "aggregation": "sum",
  "comparison": "atLeast",
  "threshold": 30
}
```

Uses effective health restored after max-HP clamp. Overheal contributes zero. Fields are
actor/role/counterpart, ability IDs, and metric test.

## 7.3 Ability casts

```json
{
  "id": "repeat-one-ability",
  "type": "abilityCast",
  "description": "Use the same ability three times in one fight.",
  "window": "fight",
  "windowActor": "any",
  "requiredWindowCount": 1,
  "windowMode": "cumulative",
  "actor": "opponentTeam",
  "abilities": [],
  "sameAbility": true,
  "affected": "any",
  "minimumAffectedUnits": 0,
  "minimumTargetDistance": 0,
  "maximumTargetDistance": 64,
  "comparison": "atLeast",
  "threshold": 3
}
```

Consumes successful `AbilityCast` only. `sameAbility:true` maintains deterministic
per-ability buckets and qualifies when one bucket meets the comparison; ties are resolved
by ability ID for reporting. Distance is x/z source-to-anchor distance captured at cast;
minimum/maximum are inclusive and validated. `sameAbility:false` counts all filtered casts.

`affected` and `minimumAffectedUnits` filter distinct IDs from the cast's already captured
`affectedUnits`. `affected:"any"` plus zero disables this filter; otherwise at least the
positive minimum must match the subject-relative set. This lets a range condition require
that a cast actually affected an opponent and lets “get hit by N attacks” count one cast
window rather than each nested damage effect. The minimum is
`[0, 2 * gameRules.battle.teamCapacity]`; non-`any` filters require a positive value.

## 7.4 Status actions

```json
{
  "id": "cleanse-poison",
  "type": "status",
  "description": "Remove Poison from allies twice over the game.",
  "window": "game",
  "windowActor": "any",
  "requiredWindowCount": 1,
  "windowMode": "cumulative",
  "actor": "subject",
  "role": "source",
  "counterpart": "subjectAllies",
  "action": "removed",
  "statuses": [],
  "tags": ["poison"],
  "aggregation": "count",
  "comparison": "atLeast",
  "threshold": 2
}
```

Action is `applied|removed`; cleanse emits actual `StatusRemoved` values per removed
status instance/stack and therefore uses `removed`. Status IDs and tags are optional
filters but at least one may remain empty to mean any. `sum` uses actual stacks changed;
`count` counts status events. Source/target IDs and effective stack deltas are required in
the step-06/10 event payload.

## 7.5 Shields

```json
{
  "id": "absorb-while-guarded",
  "type": "shield",
  "description": "Absorb 40 physical damage with shields.",
  "window": "game",
  "windowActor": "any",
  "requiredWindowCount": 1,
  "windowMode": "cumulative",
  "actor": "subject",
  "role": "target",
  "counterpart": "any",
  "action": "absorbed",
  "kind": "physical",
  "aggregation": "sum",
  "comparison": "atLeast",
  "threshold": 40
}
```

Action is `applied|absorbed|broken`; kind is `any|physical|magical`. Applied/absorbed use
effective amount; broken permits only count aggregation.

## 7.6 Movement/displacement/teleport

```json
{
  "id": "travel",
  "type": "movement",
  "description": "Move ten cells voluntarily.",
  "window": "fight",
  "windowActor": "any",
  "requiredWindowCount": 1,
  "windowMode": "cumulative",
  "actor": "subject",
  "movement": "voluntary",
  "direction": "any",
  "aggregation": "sum",
  "comparison": "atLeast",
  "threshold": 10
}
```

Movement is `voluntary|displacement|teleport|any`; direction is
`any|towardSource|awayFromSource`. `any` direction is valid for every movement kind;
`towardSource`/`awayFromSource` are valid only for `displacement` or movement `any` because
voluntary/teleport aggregates do not carry that authored direction. Sum/maximum use
actual x/z cell distance. A multi-cell voluntary path emits one committed movement event
with total entered-cell count plus any ordered cell-entry object events.

## 7.7 Resources

```json
{
  "id": "spend-ap",
  "type": "resource",
  "description": "Spend twelve AP.",
  "window": "fight",
  "windowActor": "any",
  "requiredWindowCount": 1,
  "windowMode": "cumulative",
  "actor": "subject",
  "action": "spent",
  "resource": "actionPoints",
  "reasons": ["abilityCost"],
  "aggregation": "sum",
  "comparison": "atLeast",
  "threshold": 12
}
```

Action is `spent|gained|lost|nextActivationPenalty`. Every resource leaf requires a
non-empty, duplicate-free `reasons` array from this closed vocabulary:

```text
abilityCost | movementCost | effect | activationRefill |
nextActivationPenaltyConsumption | effectiveMaximumClamp
```

Validate reason/action compatibility: `spent` permits only the two cost reasons; `gained`
permits `effect`/`activationRefill`; `lost` permits `effect`, penalty consumption, or
effective-maximum clamp; `nextActivationPenalty` permits only `effect` and consumes the
applied accumulation event, not its later consumption. Values are actual pool change or
penalty applied, never the requested/clamped amount. This explicit filter prevents an
authored “gain AP from an effect” requirement from being farmed by ordinary activation
refills.

## 7.8 Position samples

```json
{
  "id": "approach-wild",
  "type": "position",
  "description": "End a move adjacent to the wild creature.",
  "window": "turn",
  "windowActor": "opponentTeam",
  "requiredWindowCount": 1,
  "windowMode": "cumulative",
  "sample": "afterMove",
  "actor": "opponentTeam",
  "relativeTo": "subject",
  "comparison": "atMost",
  "distance": 1
}
```

Sample is `turnStart|turnEnd|afterMove|throughoutWindow`; comparison is
`atMost|atLeast|equal|between`.
`between` additionally requires `maximumDistance`. Distance is the minimum Manhattan x/z
distance from the sampled actor to any active member of `relativeTo`; no member means the
sample does not qualify. Range is `[0, 128]`.

`throughoutWindow` is valid for ability, turn, and fight but forbidden for game. At an
eligible window's open, after every placement/move/displacement/teleport/swap/removal/
defeat/impression that can change a comparable pair, and at close, sample every active
member of `actor` against the closest active `relativeTo` member. A failed comparison
irreversibly fails that window. A moment with no comparable pair is neutral, but at least
one comparable sample is required. This expresses “stay at least N cells away for X
fights” rather than checking only the terminal frame.

## 7.9 Unit removal/defeat/impression

```json
{
  "id": "defeat-enemies",
  "type": "unitRemoval",
  "description": "Defeat three enemies.",
  "window": "game",
  "windowActor": "any",
  "requiredWindowCount": 1,
  "windowMode": "cumulative",
  "removed": "opponentTeam",
  "creditedTo": "subject",
  "reason": "defeated",
  "comparison": "atLeast",
  "threshold": 3
}
```

Reason is `defeated|impressed|any`. Feat kill content uses defeated. `RemovalReason::Impressed`
never satisfies defeated/kill credit. `creditedTo` may be any when credit is irrelevant.
Aggregation is implicitly count.

## 7.10 Battle outcome

```json
{
  "id": "win-and-survive",
  "type": "battleOutcome",
  "description": "Win while this creature remains active.",
  "window": "fight",
  "windowActor": "any",
  "requiredWindowCount": 2,
  "windowMode": "cumulative",
  "outcome": "subjectTeamVictory",
  "requireSubjectActive": true
}
```

Outcome is `subjectTeamVictory|opponentTeamVictory|draw|completed`. Only terminal
non-aborted `BattleEnded` qualifies. This type is fight-window only.

## 7.11 Survivor count

```json
{
  "id": "two-allies-live",
  "type": "survivorCount",
  "description": "Keep at least two player creatures active.",
  "window": "fight",
  "windowActor": "any",
  "requiredWindowCount": 1,
  "windowMode": "cumulative",
  "units": "opponentTeam",
  "state": "active",
  "comparison": "atLeast",
  "threshold": 2
}
```

State is `active|notDefeated`; active excludes defeated and impressed/removed units,
notDefeated includes impressed units for summary-only conditions. Evaluated from the
terminal snapshot and fight-window only.

## 7.12 Event absence and streaks

```json
{
  "id": "avoid-physical-damage",
  "type": "eventAbsence",
  "description": "Take no physical HP damage for two consecutive turns.",
  "window": "turn",
  "windowActor": "subject",
  "requiredWindowCount": 2,
  "windowMode": "consecutive",
  "predicate": {
    "type": "damage",
    "actor": "subject",
    "role": "target",
    "counterpart": "any",
    "kind": "physical",
    "component": "health",
    "sourceAbilities": []
  }
}
```

`EventAbsenceConditionSpec` qualifies when no committed event matching its closed predicate
occurs inside an eligible window. Supported predicate types are:

* `damage`: actor/role/counterpart, kind, component, ability IDs;
* `abilityCast`: actor, ability IDs;
* `status`: actor/role/counterpart, action, status IDs, tags;
* `movement`: actor and movement kind.

Predicates contain filters only, never a nested threshold/window. Reject other type names.
Do not implement a general negation operator.

## 7.13 Composition

```json
{
  "id": "support-or-defense",
  "type": "anyOf",
  "description": "Complete either support route.",
  "children": [
    { "id": "child-a", "type": "healing", "description": "...", "window": "fight", "windowActor": "any",
      "requiredWindowCount": 1, "windowMode": "cumulative",
      "actor": "subject", "role": "source", "counterpart": "subjectAllies",
      "sourceAbilities": [], "aggregation": "sum", "comparison": "atLeast", "threshold": 20 },
    { "id": "child-b", "type": "shield", "description": "...", "window": "fight", "windowActor": "any",
      "requiredWindowCount": 1, "windowMode": "cumulative",
      "actor": "subject", "role": "target", "counterpart": "any",
      "action": "absorbed", "kind": "any", "aggregation": "sum",
      "comparison": "atLeast", "threshold": 20 }
  ]
}
```

`allOf`/`anyOf` require at least two children. Completion/advancement retains each child by
ID. Do not collapse child state into min/max percentages.

---

# 8. Event vocabulary dependency

Condition specs parse before the runtime event classes exist, but their meaning is locked
to the typed event vocabulary implemented in steps 06-10. Those events must carry stable
IDs and effective values sufficient for the fields above:

```text
AbilityCast
    source, abilityId, sourceCell, anchorCell, targetDistance,
    captured affectedUnits in canonical order

Damage
    source?, target, abilityId?, effectId, kind,
    computedDamage, appliedToHealth, appliedToShield, targetHadAnyShield,
    targetHadMatchingShield

Healing
    source?, target, abilityId?, effectId, requestedHealing, appliedHealing

ShieldApplied / ShieldAbsorbed / ShieldBroken
    source?, target, abilityId?, effectId, kind, effective amount where applicable

StatusApplied / StatusRemoved
    source?, target, abilityId?, effectId, statusId, actual stack delta

ResourceSpent / ResourceChanged / NextActivationPenaltyApplied
    source?, target, resource, requested/effective amount, before/after where applicable

UnitMoved / UnitDisplaced / UnitTeleported
    unit, source?, from/to cells, actual distance, displacement direction

ActivationStarted / ActivationEnded
    unit, cell, closest active ally/enemy distances or snapshots from which to derive them

UnitRemoved / UnitDefeated
    unit, reason, credited source?, abilityId?, effectId?

BattleEnded
    outcome and terminal active/not-defeated counts by side
```

Every event also has sequence, batch identity, and fixed battle time. Requested damage,
overheal, blocked movement, rejected commands, previews, and presentation callbacks cannot
advance conditions. Later steps may store before/after snapshots instead of duplicating all
derived distances/counts, but the evaluator must receive the same immutable facts.

Do not introduce a separate Feat event bus as the source of truth. The ordered battle log
is authoritative.

---

# 9. Feat reward specification

Use a closed variant with stable reward IDs:

```cpp
struct BonusStatRewardSpec;
struct UnlockAbilityRewardSpec;
struct RemoveAbilityRewardSpec;
struct UnlockPassiveRewardSpec;
struct ChangeFormRewardSpec;

struct FeatRewardSpec
{
    std::string id;
    std::variant<BonusStatRewardSpec,
                 UnlockAbilityRewardSpec,
                 RemoveAbilityRewardSpec,
                 UnlockPassiveRewardSpec,
                 ChangeFormRewardSpec> payload;
};
```

Exact JSON types:

```json
{ "id": "health-up", "type": "bonusStat", "stat": "maxHealth", "amount": 5 }
{ "id": "faster", "type": "bonusStat", "stat": "stamina", "amountSeconds": -0.25 }
{ "id": "learn-guard", "type": "unlockAbility", "ability": "training-guard" }
{ "id": "forget-strike", "type": "removeAbility", "ability": "training-strike" }
{ "id": "gain-thorns", "type": "unlockPassive", "status": "thorns" }
{ "id": "become-bloom", "type": "changeForm", "form": "bloom" }
```

Rules:

* non-stamina stat amount is a non-zero signed integer in safe step-02 stat bounds;
* stamina uses only exact non-zero `amountSeconds` and forbids `amount`;
* ability/status references resolve during this step's cross-validation;
* unlockPassive cannot reference a `stun` status;
* form follows local content-ID grammar but cannot resolve until a species selects the
  board in step 04;
* reward IDs are unique across the entire board;
* rewards apply later in board node order, then reward array order;
* rewards are replayed from a clean species baseline and are never incrementally saved.

No reward grants XP, levels, items, currencies, encounter clears, arbitrary effects, or a
script.

---

# 10. Feat Board definition

Use:

```cpp
enum class FeatNodeKind
{
    Root,
    Stats,
    Ability,
    Passive,
    Evolution
};

struct FeatNodeDefinition
{
    std::string id;
    std::string displayName;
    std::string description;
    std::array<int, 2> position{};
    FeatNodeKind kind;
    std::vector<std::string> neighbours;
    std::optional<std::string> exclusiveGroup;
    std::uint32_t minimumCompletedNodes = 0;
    std::optional<std::string> fromForm;
    std::vector<ConditionSpec> requirements;
    std::vector<FeatRewardSpec> rewards;
};

struct FeatBoardDefinition
{
    std::string id;
    std::string displayName;
    std::string rootNodeId;
    std::vector<FeatNodeDefinition> nodes;
};
```

`nodes` vector order is authoritative for reward replay, simultaneous completion tie-breaks,
summaries, and deterministic tests. Lookup maps may be derived without replacing that order.

Exact compact example:

```json
{
  "version": 1,
  "displayName": "Training Board",
  "rootNode": "root",
  "nodes": [
    {
      "id": "root",
      "displayName": "Awakening",
      "description": "The creature's starting point.",
      "position": [0, 0],
      "kind": "root",
      "neighbours": ["learn-guard"],
      "minimumCompletedNodes": 0,
      "requirements": [],
      "rewards": []
    },
    {
      "id": "learn-guard",
      "displayName": "Measured Guard",
      "description": "Learn to protect an ally.",
      "position": [1, 0],
      "kind": "ability",
      "neighbours": ["root", "learn-snare"],
      "minimumCompletedNodes": 0,
      "requirements": [
        {
          "id": "take-damage",
          "type": "damage",
          "description": "Take ten HP damage over the game.",
          "window": "game",
          "windowActor": "any",
          "requiredWindowCount": 1,
          "windowMode": "cumulative",
          "actor": "subject",
          "role": "target",
          "counterpart": "opponentTeam",
          "kind": "any",
          "component": "health",
          "targetHadShield": "any",
          "sourceAbilities": [],
          "aggregation": "sum",
          "comparison": "atLeast",
          "threshold": 10
        }
      ],
      "rewards": [
        { "id": "unlock-guard", "type": "unlockAbility", "ability": "training-guard" }
      ]
    },
    {
      "id": "learn-snare",
      "displayName": "Prepared Ground",
      "description": "Learn to place a snare.",
      "position": [2, 0],
      "kind": "ability",
      "neighbours": ["learn-guard"],
      "minimumCompletedNodes": 1,
      "requirements": [
        {
          "id": "move-ten",
          "type": "movement",
          "description": "Move ten cells in one fight.",
          "window": "fight",
          "windowActor": "any",
          "requiredWindowCount": 1,
          "windowMode": "cumulative",
          "actor": "subject",
          "movement": "voluntary",
          "direction": "any",
          "aggregation": "sum",
          "comparison": "atLeast",
          "threshold": 10
        }
      ],
      "rewards": [
        { "id": "unlock-snare", "type": "unlockAbility", "ability": "training-snare" }
      ]
    }
  ]
}
```

Optional `exclusiveGroup` and `fromForm` are omission-only when absent. Explicit JSON `null`
is invalid in schema version 1 and receives the same path-aware type error everywhere.

---

# 11. Feat Board graph and semantic validation

Validate after parsing the complete board:

## 11.1 Identity and root

* version exactly 1;
* board filename ID and every local ID valid;
* display strings non-empty and bounded;
* node IDs unique;
* rootNode resolves to exactly one node of kind root;
* no other node has kind root;
* root is the only node allowed an empty requirements array;
* every non-root has at least one requirement;
* root has `minimumCompletedNodes == 0`, no `fromForm`, and no `exclusiveGroup`;
* nodes are one-time completions; reject any stale `repeatLimit`, `repeatCount`, or
  `repeatable` field.

Root rewards are allowed only when they are deterministic baseline rewards. They replay on
fresh construction; they cannot depend on events because root has no requirements.

## 11.2 Adjacency

* neighbor IDs exist;
* no self-edge or duplicate neighbor;
* edges are symmetric: if A names B, B must name A;
* the undirected graph is connected from root;
* positions may overlap in v1 because no editor exists, but warn/test diagnostics if useful;
* definition order, not traversal order, remains authoritative.

A node is active later when it is incomplete/unblocked, at least one neighbor is completed,
its `minimumCompletedNodes` and `fromForm` gates are satisfied, and its exclusive group has
not selected another node. Only nodes active at battle start may consume that battle's
events.

## 11.3 Evolution gates and choices

* `minimumCompletedNodes` is `[0, nodeCount-1]` and counts completed non-root nodes;
* `fromForm` is a valid local form ID resolved per species in step 04;
* `exclusiveGroup` is a valid local semantic ID;
* every exclusive group has at least two member nodes;
* group members are kind evolution, contain a `changeForm` reward, and use distinct target
  form IDs;
* an evolution node contains exactly one `changeForm` reward;
* nodes gated by a branch form must name that `fromForm`, preventing the sibling branch
  from activating them;
* if two active alternatives qualify in the same battle, earlier board-definition order
  wins. This runtime tie-break is fixed now.

Completing one group member permanently blocks every other member. Their descendants remain
locked through adjacency/form gates. Respec and branch reversal are out of scope.

## 11.4 Requirements/rewards

* condition IDs unique across all nodes and nested children;
* reward IDs unique across all nodes;
* recursion depth bounded;
* ability/status references exist;
* status/passive stun rule holds;
* kind/reward coherence: ability has an unlock/remove ability reward, passive has an
  unlockPassive reward, evolution has changeForm; stats has bonusStat; extra compatible
  rewards are allowed so a meaningful node may grant more than one result;
* all arrays preserve authored order.

Return contextual errors naming board, node, condition/reward, file, and path.

---

# 12. Inline taming profile

Define:

```cpp
struct TamingProfileDefinition
{
    std::vector<ConditionSpec> conditions;
};

[[nodiscard]] TamingProfileDefinition parseTamingProfile(JsonReader &p_reader);
```

It is not a registry definition and has no standalone ID/file. Step 04 embeds it in species:

```json
"tamingProfile": {
  "conditions": [
    {
      "id": "impress-with-magic",
      "type": "damage",
      "description": "Deal more than 8 magical HP damage in one ability.",
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
      "sourceAbilities": [],
      "aggregation": "maximum",
      "comparison": "greaterThan",
      "threshold": 8
    }
  ]
}
```

An absent/null profile means untameable. If present, conditions must be non-empty, IDs must
be unique through the tree, and normal references/limits apply. All top-level conditions
are ANDed. `anyOf` supplies explicit alternatives.

All four windows are accepted, but every advancement is battle-local for taming; even a
`game` window is reset when the encounter is constructed. Partial taming progress is never
saved. Step 16 binds subject/opponent perspective and performs immediate impression.

Do not add probability, items, capture actions, player input, or a standalone taming-profile
resource directory.

---

# 13. Load and validation order

Extend the step-02 transaction:

```text
load all existing world definitions into locals
load statuses, battle objects, abilities into locals
cross-validate combat graph
load featboards into local Registry<FeatBoardDefinition>
validate condition/reward syntax and combat IDs
publish nothing yet
move every local into Registries only after every current domain succeeds
```

Step 04 later inserts AI/species/encounters before publication. Preserve this single full
transaction; do not commit combat registries and then discover a bad Feat Board.

Form rewards and `fromForm` remain validated syntax plus deferred references. Record enough
path metadata or provide a validator that step 04 can call with `(board, species)` and still
produce the original board file/path.

Extend the successful load report with the Feat Board count only after commit.

---

# 14. Seed definition

Create:

```text
Playground/resources/data/featboards/training-board.json
```

Use the full valid example from section 10. It references the step-02 `training-guard` and
`training-snare` definitions. It deliberately contains no form reward so it is valid before
species exist. Step 04's training species selects it.

The checked-in file is bootstrap data, not the complete progression catalog. Step 19 adds
boards that exercise evolution and the full condition set.

---

# 15. Required tests

Add headless tests under `Playground/tests/conditions/` and
`Playground/tests/feats/`.

## 15.1 Condition parser catalog

Parse one valid golden object for every leaf, predicate, and composite alternative. Assert
all enum/value fields and stable authored order. Explicitly cover:

* all unit-set/role values;
* all four windows and both count modes;
* required `windowActor`, including ability/turn filtering and explicit `any` for
  fight/game;
* every aggregation/comparison;
* between maximum;
* same-ability buckets;
* affected-unit set/minimum filtering on ability casts;
* shielded damage component;
* displacement direction;
* every resource reason and each valid action/reason combination;
* `throughoutWindow` position invariants and every resample-causing event kind;
* event absence predicate variants;
* nested allOf/anyOf at the configured depth boundary.

## 15.2 Strict condition failures

Test missing/unknown/wrong fields, unknown types/enums, invalid IDs, duplicate nested IDs,
empty composite, bad window/count combination, impossible metric fields, missing/empty/
duplicate/incompatible resource reasons, invalid thresholds,
bad distance range, invalid `windowActor` for fight/game, invalid affected-unit minimum,
wrong window for `throughoutWindow`/battle outcome/survivors, and depth overflow. Assert
file/array path.

## 15.3 Combat references

Test known/unknown ability and status filters/rewards, duplicate IDs, passive stun rejection,
and that definition creation order does not affect validation.

## 15.4 Feat graph

Test:

* valid connected graph and preserved node order;
* missing/duplicate root, second root, non-root empty requirements;
* missing/asymmetric/self/duplicate edges and disconnected node;
* duplicate node/condition/reward IDs;
* stale repeatLimit rejected as unknown;
* minimum completed-node bounds;
* valid/invalid exclusive groups and distinct form targets;
* evolution missing/multiple changeForm rewards;
* simultaneous-choice tie policy documented as definition order;
* deferred form IDs survive parsing for step-04 validation.

## 15.5 Taming profile

Test absent/null handling through a species-like fixture, present non-empty profile, empty
failure, same shared parser/variant, relative opponent-team filters, all windows including
battle-local game, and duplicate/depth/reference failures.

## 15.6 Transaction/determinism

Load the real training board. Reverse file creation order in fixtures and assert registry
IDs/node vectors remain stable. Cause a bad board after valid combat definitions and prove
no member of a previously loaded `Registries` instance changes.

Do not test condition evaluation by constructing ad-hoc runtime events; step 15 owns that
behavior. Parser tests may verify the documented event-kind mapping as enum metadata.

---

# 16. Error and determinism requirements

* Every parser calls `forbidUnknown` at every object alternative.
* Unsupported board version is a hard error.
* Unknown condition/reward type lists valid literals in stable lexical order.
* Source paths survive recursive parsing and deferred form validation.
* Definition vectors retain JSON order; lookup maps do not change outcome order.
* No hash iteration, pointer identity, RNG, frame delta, or locale influences loading.
* Numeric progress specifications use checked integers/fixed time, never float percentages.
* The definition layer stores no mutable advancement.
* Composite recursion cannot overflow the call stack due to the configured depth limit.

---

# 17. Expected files and integration points

Add cohesive files equivalent to:

```text
Playground/srcs/conditions/condition_definition.hpp
Playground/srcs/conditions/condition_definition.cpp
Playground/srcs/conditions/condition_parser.cpp
Playground/srcs/feats/feat_reward_definition.hpp
Playground/srcs/feats/feat_board_definition.hpp
Playground/srcs/feats/feat_board_definition.cpp
Playground/srcs/taming/taming_profile_definition.hpp
Playground/srcs/taming/taming_profile_definition.cpp

Playground/tests/conditions/condition_definition_test.cpp
Playground/tests/feats/feat_board_definition_test.cpp
Playground/tests/feats/feat_reward_definition_test.cpp
Playground/tests/taming/taming_profile_definition_test.cpp
```

Modify `Registries` and implemented data-model docs. Add `training-board.json`.

Do not modify `GameContext`, create `CreatureUnit`, build a condition evaluator, touch the
world/board/widget, or add progress/save files. Step 04 owns species and persistent roster
state; steps 15/17 own evaluation/progression.

---

# 18. Documentation requirements

Document:

* relative subject/team vocabulary;
* exact ability/turn/fight/game windows, `windowActor`, stable window identities, and
  persistence intent;
* affected-unit cast filters and throughout-window position invariants;
* count modes, metrics, effective-value rule, and absence streaks;
* all condition and reward JSON alternatives;
* stable semantic IDs and definition-order guarantees;
* one-time root/adjacency activation model;
* minimum-completed/from-form evolution gates and exclusive choices;
* inline, battle-local taming profiles sharing the parser;
* deferred species-form validation;
* explicitly that evaluation and progression are not implemented yet.

Remove stale docs that describe UUID-only IDs, percentage advancement, repeatLimit,
independent Feat/taming factories with divergent schemas, or win-only progress.

---

# 19. Non-goals

Do not implement condition evaluation, event logs, window state, `FeatBoardProgress`, active
node snapshots, reward replay, derived creature state, evolution execution, taming trackers,
impression, Feat/taming UI, respec, repeatable nodes, XP/levels, save reconciliation,
authoring/editor tools, hidden lore, arbitrary predicates, or scripts.

Do not add a registry for conditions, rewards, nodes, or taming profiles. They are embedded
ordered definitions with stable local IDs.

---

# 20. Acceptance checklist

This step is complete only when:

* [ ] `Registries` loads and exposes Feat Boards transactionally;
* [ ] one shared closed `ConditionSpec` serves Feats and taming;
* [ ] all relative unit sets, windows, count modes, metrics, and comparisons are explicit;
* [ ] every leaf declares its window actor, casts can require captured affected units, and
      position conditions can enforce a whole-window invariant;
* [ ] the catalog expresses every GDD example listed in this prompt;
* [ ] stable IDs exist for nodes, all nested conditions, and rewards;
* [ ] root/graph/adjacency/gate invariants are enforced;
* [ ] nodes are one-time and stale repeat fields fail;
* [ ] ability/status references validate now and form refs defer contextually;
* [ ] exclusive evolution groups and definition-order tie policy validate;
* [ ] inline taming profiles are non-empty when present and use identical condition parsing;
* [ ] `training-board` loads with stable node order;
* [ ] invalid progression data leaves all registries unchanged;
* [ ] exhaustive parser/strictness/reference/graph tests pass;
* [ ] complete Playground tests/build remain green;
* [ ] no runtime evaluator/progress/taming behavior was added.

---

# 21. Required handoff report

Report:

1. exact condition/reward/board/profile C++ type and parser names;
2. every accepted type/enum literal and board schema version;
3. stable-ID scopes and graph validation algorithm;
4. event fields the step-06 vocabulary must expose;
5. deferred form-reference API step 04 must call;
6. registry load/commit order and training IDs;
7. files created/materially changed/deliberately untouched;
8. drift/adaptations;
9. build/test commands actually run and results;
10. registry load output actually observed;
11. all evaluation/progression/taming work explicitly deferred.
