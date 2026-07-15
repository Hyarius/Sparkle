# Combat plan — progress ledger

The **state of what has actually been built**, step by step, as a compact API surface:
enough to implement the next step without re-reading the headers of every earlier one.
The [step index](steps/README.md) says what *should* be done; this file says what *is*
done and what its real signatures are.

**How to use it.** Before implementing step N, read this ledger for steps 1..N-1 instead
of opening their headers. Open a header only when you need a body, a constant, or a detail
the ledger deliberately omits.

**Authority.** When this ledger and the code disagree, the code wins — the ledger drifted
and should be corrected. Signatures here are transcribed from the real headers, but a later
step may have extended an API; treat a surprising absence as "check the header", not "it
does not exist".

**Maintenance.** The last task of every step is to append/refresh its section here from the
headers that step actually landed. A stale ledger is worse than none, because the next step
will trust it. (This is the one plan-doc file the step workflow does keep current; the
per-step `docs/NN-*.md` system docs are still skipped.)

Status: **steps 01–07 complete** on branch `BattleImplementation`.
Next unimplemented step: **08 — movement and targeting**.

---

## Conventions that hold across every step

- All battle code is `namespace pg` under `Playground/srcs`. Nothing is promoted into
  `spk::`. Headless: `battle/ abilities/ statuses/ ai/ feats/ taming/ conditions/
  encounters/ creatures/ player/ board/` build and test without a window or GL context.
- **Definitions are immutable values; cross-references are validated string ids** (never
  pointers). A definition holds no runtime state (no current HP/AP/MP, position, stacks).
- **Text is a translation key, not a sentence.** Definitions carry `displayNameKey` /
  `descriptionKey` (and condition/effect carry `descriptionKey`); `pg::Translator` resolves
  them at display time, a missing key renders as itself. Icons are `spk::Vector2Int`.
  Shared authored-metadata helpers live in `core/definition_fields.hpp`:
  `requireDisplayNameKey`, `requireDescriptionKey`, `requireIcon`, `requireTags`,
  `requireVersion`, `requireBoundedText`, `requireIntegerInRange`.
- **`DefinitionSource {std::filesystem::path file; std::string jsonPath;}`** rides on most
  definitions so a *second-phase* cross-reference error can still name the authored line.
- **`pg::Registry<T>`** (deterministic `std::map`, filename-stem ids) + `spk::loadJsonDirectory`
  already exist. A domain parser leaves `id` empty; the loader assigns the filename stem.
- **`Registries::loadAll` is one transaction**: parse everything into locals, cross-validate
  the whole graph, then publish every member by move. A failed load leaves the previously
  published `Registries` untouched. Cross-registry reference checks live in
  `core/combat_definition_validation.hpp` (two-phase: parse ids, then resolve once all
  registries exist — so authored status/object/ability cycles need no load order).
- **Closed variants, not heap hierarchies.** A JSON `"type"` string selects a
  compile-time `std::variant` alternative; an unknown type is a hard error listing the
  accepted literals. Every enum has a `xxxNames()` `std::map<std::string, Enum>` table +
  `toString`.
- **Determinism**: battle-local `BattleRng` only; no `random_device`/wall-clock/pointer
  order/unordered iteration in a rule. Checked integer arithmetic; ordered containers or
  explicitly sorted outputs. **Never `spk::UUID` for battle identity.**

### Build / test (from repo root)

```powershell
cmake --preset test
cmake --build build/test --target PlaygroundTests SparklePlayground
ctest --test-dir build/test --output-on-failure -L playground
./build/test/Playground/SparklePlayground.exe --resource-path Playground/resources
```

Tests live under `Playground/tests/<domain>/`; the harness pins the source resource tree.

---

## Step 01 — Battle foundations (`battle/`, `core/`)

Deterministic time/RNG/ids, the shared enum vocabulary, and game-rules v2. No board, unit,
scheduler, effect, or session yet.

Files: `battle/battle_time.*`, `battle/battle_ids.hpp`, `battle/battle_rng.*`,
`battle/battle_types.*`, `core/content_id.*`, `core/creature_instance_id.*`,
`core/game_rules.*`; `resources/data/config/game-rules.json` (v2).

```cpp
// battle_time.hpp — signed-millisecond fixed point; simulation never reads frame delta.
class BattleTime {
  using Rep = std::int64_t; static constexpr Rep TicksPerSecond = 1000;
  static constexpr double SecondsTolerance = 1e-9;
  static constexpr BattleTime fromTicks(Rep) noexcept;
  static BattleTime fromSecondsExact(double);          // throws on non-finite / sub-ms / overflow
  Rep ticks() const; bool isZero() const; double secondsForDisplay() const;
  BattleTime operator+ - * (checked, throws std::overflow_error); ==, <=> };
BattleTime clamp(v, lo, hi); double ratioForDisplay(v, total);   // free fns
enum class BattleTimeSign { Positive, NonNegative, Any };
BattleTime requireBattleSeconds(JsonReader&, key, BattleTimeSign);  // parser: adds file/path

// battle_ids.hpp — 0 is the invalid sentinel; tags keep the three types unassignable.
template<class Tag> class BattleNumericId { uint32_t value(); bool valid(); ==,<=>;
    explicit ctor throws std::invalid_argument on 0 };
using BattleId / BattleUnitId / BattleObjectId = BattleNumericId<...>;
template<class Tag> class BattleNumericIdAllocator { BattleNumericId<Tag> allocate(); };  // starts at 1
using BattleUnitIdAllocator / BattleObjectIdAllocator = ...;
BattleId deriveBattleId(uint64_t combatSeed);        // session id is a pure fn of the seed,
BattleId battleIdFromMixedSeed(uint64_t mixed);      // not a counter — replay keeps its id

// battle_rng.hpp — SplitMix64, owned by value; the session owns one, there is no global.
class BattleRng { explicit BattleRng(uint64_t seed);
  uint64_t nextU64(); uint64_t uniformBelow(uint64_t exclusiveUpper);  // unbiased; throws on 0
  size_t drawCount(); uint64_t state(); };   // a validation/query path must not advance it

// battle_types.hpp — the whole shared vocabulary, spelled once.
enum class BattleSide {Player,Enemy};  EncounterKind {Wild,Trainer,Gym,Special,Debug};
  DamageKind {Physical,Magical};  BattleResource {ActionPoints,MovementPoints};
  CreatureStat {MaxHealth,Strength,MagicPower,Armor,Resistance,MaxActionPoints,
                MaxMovementPoints,Stamina,Range};   // magical offense is magicPower
  BattleOutcome {Undecided,PlayerVictory,PlayerDefeat,Draw,Aborted};
BattleSide opposingSide(BattleSide);   // + xxxNames() tables and toString for each

// content_id.hpp — [a-z0-9]+(?:-[a-z0-9]+)*, 1..64 ASCII bytes, lower-case only.
constexpr size_t MaxContentIdLength = 64;
bool isContentId(sv);  void requireContentId(value, file, jsonPath, kind);

// creature_instance_id.hpp — "creature-" + 16 lower-hex, non-zero uint64 serial.
class CreatureInstanceId { static fromSerial(uint64_t); static parse(sv);   // both throw on bad
  uint64_t serial(); string_view string(); bool valid(); ==,<=>; };         // trivially copyable

// game_rules.hpp — v2 config; structural values are serialized invariants, not tunables.
struct BattleGameRules { static constexpr RequiredTeamCapacity=6, RequiredAbilitySlotCapacity=8,
  MinimumBoardSide=5, MaximumBoardSide=31, MaximumEffectChainDepth=256, MaximumConditionDepth=32;
  size_t teamCapacity, abilitySlotCapacity; array<int,2> defaultBoardSize; int deploymentDepth;
  BattleTime minimumStamina; int64_t mitigationScale; size_t maxCommandsPerActivation,
  maxAiCommandsPerActivation, maxEffectChainDepth, maxConditionDepth; };
struct GameRules { double maxVerticalTraversalGap; BattleGameRules battle;
  std::map<std::string, std::array<int,2>> overlayMasks; };
GameRules parseGameRules(JsonReader&);   // Registries::gameRules()
```

Invariants: 1 tick = 1 ms; sub-ms/non-finite JSON seconds are rejected, never rounded.
Probabilities/ratios are integer permille (1000 == 1.0). Board coords are `spk::Vector3Int`;
range distance ignores y. Enum JSON literals are lowerCamel.

## Step 02 — Combat definition registries (`abilities/`, `statuses/`, `battle_objects/`)

Immutable ability/status/battle-object JSON, a closed effect-payload variant, durations,
target filters, range/area geometry — data only, nothing executes.
`Registries::statuses() / abilities() / battleObjects()`.

Files: `abilities/{ability_definition,effect_definition,targeting_definition,duration_spec}.*`,
`statuses/status_definition.*`, `battle_objects/battle_object_definition.*`,
`core/combat_definition_validation.*`. Seed: `resources/data/{abilities,statuses,battle-objects}/training-*.json`.

```cpp
// duration_spec.hpp
struct TimelineDurationSpec {BattleTime duration;};  OwnerActivationsDurationSpec {uint32_t count;};
  InfiniteDurationSpec {};
using DurationSpec = variant<Timeline, OwnerActivations, Infinite>;   enum class DurationKind{...};

// targeting_definition.hpp — anchorFilter and affectedFilter are SEPARATE authored questions.
struct TargetFilter { bool allowSelf, allowAllies, allowEnemies, allowDefeated, allowEmptyCell; };
enum class RangeShape {Self,Diamond,OrthogonalLine,DiagonalLine};
struct RangeDefinition { RangeShape shape; int minimum, maximum; bool requiresLineOfSight; };
enum class AreaShape {Single,Diamond,Square,Cross,Line};
struct AreaDefinition { AreaShape shape; int radius; };   // cells NOT enumerated yet (step 08)

// effect_definition.hpp — closed EffectPayload, 14 alternatives, no expression/script.
enum class EffectApplyTo {SourceUnit,PrimaryUnit,AffectedUnits,AnchorCell,AffectedCells};
DamageEffectSpec{kind,base,strengthRatioPermille,magicPowerRatioPermille}; HealEffectSpec{...};
ApplyShieldEffectSpec{kind,amount,DurationSpec}; ApplyStatusEffectSpec{status,stacks,DurationSpec};
RemoveStatusEffectSpec{status,stacks}; CleanseEffectSpec{tags}; ChangeResourceEffectSpec{resource,delta};
ApplyNextActivationPenaltyEffectSpec{resource,amount}; AdjustTurnBarEffectSpec{BattleTime delta};
DisplaceEffectSpec{DisplaceDirection,distance}; SwapWithSourceEffectSpec{}; TeleportSourceEffectSpec{};
PlaceObjectEffectSpec{object,DurationSpec}; RemoveObjectsEffectSpec{tags};
using EffectPayload = variant<...the 14...>;
struct EffectApplication { string id; EffectApplyTo applyTo; bool requiresLivingSource;
  EffectPayload payload; DefinitionSource source; };   // id unique across the whole owner
vector<EffectApplication> parseEffects(reader,key);  void requireUniqueEffectIds(effects, set& seen);

// status_definition.hpp — a passive IS an infinite status; no separate passive registry.
enum class StatusReapplyPolicy{AddStacks,ReplaceStacks}; DurationRefreshPolicy{Replace,KeepLonger,Extend};
  StatModifierOperation{Add,MultiplyPermille};
struct StatModifierSpec{id,CreatureStat stat,operation,int64 value,bool perStack};  // stamina add = valueMilliseconds
enum class StatusHook {Applied,ActivationStart,ActivationEnd,AfterAbilityCast,AfterDamageDealt,
  AfterDamageTaken,AfterHealingDone,AfterHealingReceived,AfterVoluntaryMove,Removed};
struct StatusHookSpec{id,StatusHook hook,vector<EffectApplication> effects};
struct StatusDefinition{id,displayNameKey,descriptionKey,Vector2Int icon,vector<string> tags,
  uint32 maxStacks, reapplyPolicy, durationRefreshPolicy, vector<StatModifierSpec> modifiers,
  vector<StatusHookSpec> hooks};
constexpr string_view StunTag="stun";  bool isStunStatus(const StatusDefinition&);

// battle_object_definition.hpp
enum class BattleObjectTrigger {UnitEnteredCell,UnitLeftCell,UnitEndedMoveOnCell,
  UnitActivationStartedOnCell,UnitActivationEndedOnCell};
struct BattleObjectTriggerSpec{id,trigger,TargetFilter,uint32 maxTriggers/*0=unlimited*/,
  bool removeWhenExhausted, vector<EffectApplication> effects};
struct BattleObjectDefinition{id,displayNameKey,descriptionKey,icon,tags,
  bool blocksMovement, bool blocksLineOfSight, vector<BattleObjectTriggerSpec> triggers};
```

Invariants: friendly fire is never inferred — anchorFilter and affectedFilter both say so.
`changeResource` (current pool) vs `applyNextActivationPenalty` (next refill) stay distinct.
A `stun` status may only be applied with a finite `timeline` duration. Effect ids are stable
semantic identities, unique across their whole owner (across all hooks/triggers).

## Step 03 — Progression definition registries (`conditions/`, `feats/`, `taming/`)

One shared closed `ConditionSpec` for both Feats and taming; Feat Boards; rewards; inline
taming profiles. `Registries::featBoards()`. Definition layer only — nothing evaluates.

Files: `conditions/condition_definition.*` (+ `condition_parser.cpp`),
`feats/{feat_reward_definition,feat_board_definition}.*`, `taming/taming_profile_definition.*`.
Seed: `resources/data/featboards/training-board.json`.

```cpp
// condition_definition.hpp — relative to a bound subject, never an absolute side.
enum class ConditionUnitSet {Subject,SubjectAllies,SubjectTeam,OpponentTeam,Any};
  ConditionEventRole{Source,Target,Either}; ConditionWindow{Ability,Turn,Fight,Game};
  WindowCountMode{Cumulative,Consecutive}; ConditionAggregation{Count,Sum,Maximum,Minimum};
  ConditionComparison{AtLeast,GreaterThan,AtMost,LessThan,Equal,Between};
  // + DamageKindFilter, DamageComponent, PresenceFilter, StatusConditionAction,
  //   ShieldConditionAction, MovementFilter/Direction, ResourceConditionAction,
  //   ResourceChangeReason, PositionSample, UnitRemovalReasonFilter, RelativeBattleOutcome,
  //   SurvivorState
struct ConditionLeafHeader {ConditionWindow window; ConditionUnitSet windowActor;
  uint32 requiredWindowCount; WindowCountMode windowMode;};   // all four required on every leaf
struct MetricTest {ConditionAggregation aggregation; ConditionComparison comparison;
  int64 threshold; optional<int64> maximum;};                 // maximum only for "between"
// 12 leaf specs: Damage, Healing, AbilityCast, Status, Shield, Movement, Resource, Position,
//   UnitRemoval, BattleOutcome (fight-only), SurvivorCount (fight-only), EventAbsence.
using EventAbsencePredicate = variant<Damage/AbilityCast/Status/Movement AbsencePredicate>; // filters only
struct AllOfConditionSpec{vector<ConditionSpec> children}; AnyOfConditionSpec{...}; // >=2 children
using ConditionPayload = variant<...12 leaves + AllOf + AnyOf...>;
struct ConditionSpec { string id; string descriptionKey; ConditionPayload payload;
  DefinitionSource source; };   // sole owner of id; id unique across the WHOLE board/profile
struct ConditionLimits {size_t maxDepth; size_t teamCapacity;};
ConditionLimits conditionLimits(const GameRules&);
struct ConditionParseState {ConditionLimits limits; set<string> ids;};   // ids accumulate across trees
ConditionSpec parseConditionSpec(reader, state, depth=1);
vector<ConditionSpec> parseConditions(reader, key, state, allowEmpty);
struct ConditionReferences {vector<ConditionReference> abilities, statuses;};   // {id, source}
void collectConditionReferences(conditions, ConditionReferences&);   // resolved in combat_definition_validation

// feat_reward_definition.hpp — closed variant, replayed from baseline (step 17).
BonusStatRewardSpec{CreatureStat stat,int64 amount,BattleTime staminaDelta}; UnlockAbilityRewardSpec{ability};
  RemoveAbilityRewardSpec{ability}; UnlockPassiveRewardSpec{status /*never a stun*/}; ChangeFormRewardSpec{form};
using FeatRewardPayload = variant<...5...>;   struct FeatRewardSpec{id, FeatRewardPayload payload, source};

// feat_board_definition.hpp
enum class FeatNodeKind {Root,Stats,Ability,Passive,Evolution};
struct FeatNodeDefinition{id,displayNameKey,descriptionKey,array<int,2> position,FeatNodeKind kind,
  vector<string> neighbours/*undirected, symmetric*/, optional<string> exclusiveGroup,
  uint32 minimumCompletedNodes, optional<string> fromForm, vector<ConditionSpec> requirements,
  vector<FeatRewardSpec> rewards, source};
struct FeatBoardDefinition{id,displayNameKey,rootNodeId,vector<FeatNodeDefinition> nodes,source}; // nodes order authoritative
FeatBoardDefinition parseFeatBoardDefinition(reader, const ConditionLimits&);
void validateFeatBoardFormReferences(board, set<string> formIds, speciesId);  // deferred to step 04
set<string> featBoardFormReferences(board);

// taming_profile_definition.hpp — embedded in a species (step 04), not a registry.
struct TamingProfileDefinition { vector<ConditionSpec> conditions; };  // ANDed; present => non-empty
TamingProfileDefinition parseTamingProfile(reader, const ConditionLimits&);
```

Invariants: only the root may have empty requirements and starts completed; adjacency is
undirected/symmetric/connected-from-root; nodes complete once (stale `repeatLimit` fails).
`Between` is rejected where there is no paired upper bound. Ability/status refs validate now;
form refs defer to step 04. No condition/reward/node registry — all embedded with stable ids.

## Step 04 — Species, AI, encounters, roster (`creatures/`, `ai/`, `encounters/`, `player/`, `board/`)

The rest of the immutable graph plus the persistent player model. `Registries::species() /
aiBehaviours() / encounters() / battleBoards() / newGame()`. `GameContext` gains `PlayerData player`.

Files: `creatures/{creature_attributes,creature_species_definition,creature_state_derivation,
creature_unit}.*`, `feats/feat_board_progress.*`, `ai/ai_definition.*`,
`board/handcrafted_battle_board_definition.*`, `encounters/{encounter_definition,encounter_resolver}.*`,
`player/{player_roster,player_data,new_game_definition}.*`; `world/biome_definition.*` → v2
(adds `optional<string> wildEncounterTableId`). Seed: `creatures/training-sprout.json`,
`ai/training-aggressive.json`, `encounter-tables/{training-wild,debug-battle}.json`,
`config/new-game.json`.

```cpp
// creature_attributes.hpp — the nine authored numbers, no level/roll/scaling.
struct CreatureAttributes {int64 maxHealth,strength,magicPower,armor,resistance,
  maxActionPoints,maxMovementPoints; BattleTime stamina; int64 range;};   // lower stamina = faster
constexpr int64 MaximumCreatureAttribute = 1000000;
CreatureAttributes parseCreatureAttributes(reader, const GameRules&);
void requireValidAttributes(attrs, rules, DefinitionSource, owner);
void addToAttribute(attrs&, CreatureStat, int64 amount, const BattleTime& staminaDelta);  // checked, no clamp

// creature_species_definition.hpp
struct PlaceholderVisual {array<uint8,4> tint; uint16 scalePermille;};   // colored cube only
struct CreatureFormDefinition {id,displayNameKey,uint32 tier,PlaceholderVisual presentation;};
struct CreatureSpeciesDefinition {id,displayNameKey,descriptionKey,CreatureAttributes baseAttributes,
  vector<string> defaultAbilityIds, defaultPassiveStatusIds, string featBoardId,
  optional<TamingProfileDefinition> tamingProfile/*absent=untameable*/, string defaultFormId,
  vector<CreatureFormDefinition> forms, source;
  const CreatureFormDefinition* tryForm(sv);};
CreatureSpeciesDefinition parseCreatureSpeciesDefinition(reader, const GameRules&, const ConditionLimits&);

// feat_board_progress.hpp — the persistent half of a creature; keys are stable ids, not indices.
struct PersistentConditionAdvancement {conditionId,bool completed,uint32 qualifyingWindows,
  uint32 consecutiveWindowStreak, optional<int64> gameMetric, map<string,int64> gameBuckets;};
struct FeatRequirementProgress {requirementId, vector<PersistentConditionAdvancement> conditionAdvancement;};
struct FeatNodeProgress {nodeId,bool completed, vector<FeatRequirementProgress> requirements;};
struct FeatBoardProgress {boardId, vector<FeatNodeProgress> nodes, map<string,string> chosenExclusiveGroups;
  const FeatNodeProgress* tryNode(sv); bool isCompleted(sv);};
FeatBoardProgress makeFreshFeatBoardProgress(board);          // root completed, nothing else
FeatBoardProgress makePresetFeatBoardProgress(board, startingFormId, span<const string> completedNodeIds);

// creature_state_derivation.hpp — the ONE deterministic replay (no second derivation path).
struct DerivedCreatureState {CreatureAttributes attributes; vector<string> abilityIds,passiveStatusIds;
  string formId;};   // a cache; step 18 omits it from saves and rebuilds it
struct DerivationContext {const GameRules*; const Registry<StatusDefinition>*; const Registry<AbilityDefinition>*;
  const Registry<FeatBoardDefinition>*; const Registry<CreatureSpeciesDefinition>*;};  // borrows; also usable from load-txn locals
DerivationContext derivationContextOf(const Registries&);
DerivedCreatureState deriveCreatureState(species, board, progress, const DerivationContext&);   // + Registries overload

// creature_unit.hpp — what the player owns; step 06 projects a session-local BattleUnit from this.
struct CreatureUnit {CreatureInstanceId id; string speciesId; FeatBoardProgress featProgress;
  DerivedCreatureState derived;};   // derived is a rebuildable cache, never authoritative
CreatureUnit makeCreatureUnit(id, speciesId, span<const string> completedNodeIds, ctx);   // ctx = DerivationContext or Registries
void rebuildDerivedState(CreatureUnit&, ctx);

// ai_definition.hpp — ordered intent, no callback/script; step 11 evaluates via the same gateway.
enum class AIUnitSelector {Self,NearestEnemy,NearestAlly,LowestHealth{Enemy,Ally},HighestHealth{Enemy,Ally}};
// conditions: AIAlways / AIHealthRatio / AIResource / AIDistance / AIStatus(hasStatus|hasStatusTag) / AIAbilityAffordable
// anchors:    AIUnitAnchor / AISelfCellAnchor / AIBestAreaAnchor / AINearestLegalCellToAnchor
// decisions:  AICastAbility(abilityId,AIAnchorSpec) / AIMoveToward / AIMoveAway / AIEndTurn
using AIConditionSpec / AIAnchorSpec / AIDecisionSpec = variant<...>;
struct AIRuleDefinition {id, vector<AIConditionSpec> conditions, AIDecisionSpec decision, source;};
struct AIBehaviourDefinition {id, displayNameKey, vector<AIRuleDefinition> rules, source;};
AIBehaviourDefinition parseAIBehaviourDefinition(reader);
vector<string> aiCastAbilityReferences(behaviour);   // encounter checks these vs the spawn's derived loadout

// handcrafted_battle_board_definition.hpp — Gym/Special arena geometry (materialised in step 05).
struct HandcraftedBattleBoardDefinition {id,displayNameKey,spk::Vector3Int size,string geometryPrefabId,
  spk::VoxelOrientation playerApproach, int deploymentDepth, source;};
HandcraftedBattleBoardDefinition parseHandcraftedBattleBoardDefinition(reader, const Registry<PrefabDefinition>&);
BoardDeploymentStrip player/enemyDeploymentStrip(board);   // validated at identity transform

// encounter_definition.hpp — kind locks the schema (see step index §5).
using OpponentPlacementPolicy = variant<Fixed{cells}, ByLine{rowsFromEnemyEdge,order}, SeededRandom{}>;
using EncounterBoardPolicyDefinition = variant<LiveWorldBoardPolicyDefinition{size,depth,placement},
  HandcraftedBoardPolicyDefinition{definitionId,placement}>;
struct EncounterSpawnDefinition {id,speciesId,aiBehaviourId,vector<string> completedFeatNodeIds,source};
struct EncounterTeamDefinition {id,displayNameKey,uint64 weight,vector<EncounterSpawnDefinition> members};
struct EncounterTierDefinition {uint32 tier, vector<EncounterTeamDefinition> teams;};   // sparse
struct EncounterDefinition {id,displayNameKey,EncounterKind kind,bool allowsTaming,bool repeatable,
  uint32 triggerChancePermille, EncounterBoardPolicyDefinition board, vector<EncounterTierDefinition> tiers, source;};
constexpr MaximumProgressionTier=9, MaximumClearedGymTier=8;   // tier 9 reserved postgame
array<int,2> effectiveBoardSize(board, boards);  int effectiveDeploymentDepth(board, boards);
size_t largestTeamSize(encounter);
EncounterDefinition parseEncounterDefinition(reader, const Registry<HandcraftedBattleBoardDefinition>&);

// encounter_resolver.hpp — pure, headless; produces a recipe, materialises nothing.
struct ResolvedEncounter {encounterDefinitionId,EncounterKind kind,bool allowsTaming,repeatable,
  uint32 resolvedTier, string teamId, EncounterBoardPolicyDefinition board,
  vector<EncounterSpawnDefinition> enemyRoster;};
const EncounterTierDefinition& selectEncounterTier(def, uint32 progressionTier);   // greatest tier <= request
const EncounterTeamDefinition& selectWeightedTeam(tier, BattleRng&);
bool rollEncounterTrigger(def, BattleRng&);    // exactly one uniformBelow(1000), only for wild-bush
ResolvedEncounter resolveEncounter(def, uint32 progressionTier, BattleRng&);

// player_roster.hpp — exactly six nullable slots + ordered PC box; every op is id-based & atomic.
class PlayerRoster { static constexpr TeamCapacity=6;   // static_assert == BattleGameRules::RequiredTeamCapacity
  using Team = array<optional<CreatureUnit>, 6>;
  struct Placement {bool inTeam; size_t index;};
  const Team& team(); const vector<CreatureUnit>& storage();
  const CreatureUnit* find(id); CreatureUnit* findMutable(id); bool contains(id); size_t size();
  Placement add(CreatureUnit);   moveTeamToStorage(id); moveStorageToTeam(id,slot);
  void swapTeamSlots(a,b); swapTeamAndStorage(teamId,storedId);   CreatureUnit remove(id);
  void validate(ctx); };   // ctx = DerivationContext or Registries; checks derived == fresh derivation

// player_data.hpp — a value, not a service; a failing caller works on a copy.
struct PlayerData {PlayerRoster roster; uint64 nextCreatureSerial=1, encounterSerial=0;
  spk::Vector3Int playerCell, lastHealPoint; set<string> clearedTrainerIds,clearedGymIds,clearedSpecialEncounterIds;
  CreatureInstanceId allocateCreatureId();   // reserves by advancing; throws without advancing
  uint32 encounterTier();};                  // cleared-gym count, clamped to 8
PlayerRoster::Placement createAndAddCreature(PlayerData&, speciesId, span<const string> nodes, ctx);

// new_game_definition.hpp
struct NewGameCreatureDefinition {speciesId, vector<string> completedFeatNodeIds;};
struct NewGameDefinition {vector<NewGameCreatureDefinition> starterTeam, source;};   // order = slot order
NewGameDefinition parseNewGameDefinition(reader);
PlayerData makeNewPlayerData(def, ctx);   // starters -> slots 0..N-1, serials 1..N
```

Invariants: encounter kind↔board pairing is locked (wild/trainer→liveWorld, gym/special→
handcrafted, debug→either); repeatability is kind-locked. Every generated overworld biome
(v2) references a valid wild+repeatable table. Roster ops are all-or-none; a failed recruit
reserves no serial. No enemy class — spawns are recipes projected through ordinary derivation.

## Step 05 — Tactical board seam (`board/`)

The headless logical board every battle receives: a window over live voxels or an owned
handcrafted grid, frozen traversal for the battle's lifetime, stable occupancy, weighted
paths, 3D LoS, deployment zones, and the reusable test fixture. **`BoardData` is the only
terrain/nav/occupancy surface later battle rules get** — they never touch `VoxelWorld`.

Files: `board/{board_cell,board_data,board_occupancy,board_builder,deployment_layout,
board_line_of_sight,weighted_path}.*`; modified `board/cell_source.*`, `board/pathfinder.*`,
`voxel/voxel_data.hpp` (+ optional `int movementCost = 1;`). Fixture:
`tests/fixtures/board_fixture.*` (ASCII layers; `Handcrafted` source, id `test-flat-board`).

```cpp
// board_cell.hpp — the canonical battle coordinate: board-local SOLID SUPPORT voxel.
using BoardCell = spk::Vector3Int;   using BoardCellLess = CellPositionLess;   // no BoardCellId
enum class BoardSourceKind {LiveWorld, Handcrafted};
struct BoardSourceDescriptor {BoardSourceKind kind; optional<string> definitionId; <=>;};  // live=>no id, hand=>id
optional<int> tryAdd/trySubtract(int,int);  optional<Vector3Int> tryAdd/trySubtract(v,v);   // checked

// weighted_path.hpp — canonical deterministic movement.
struct WeightedPath {vector<BoardCell> cells/*incl source+dest*/; int totalCost/*excludes source*/;};
struct TraversalCostQuery {function<optional<int>(const Vector3Int&)> enterCost;};   // nullopt=blocked, else >0
TraversalCostQuery uniformCostQuery(int cost=1);
optional<WeightedPath> findWeightedPath(graph, source, dest, budget, query);
map<BoardCell,int,BoardCellLess> floodWeighted(graph, source, budget, query);
// tie key: (total cost, entered-cell count, lexicographic full cell sequence). from==to => {from}@0.

// board_occupancy.hpp — stable ids; one unit per cell, any number of objects stacked.
enum class BoardMutationError {InvalidId,UnknownUnit,UnknownObject,DestinationNotStandable,
  DestinationOccupied,UnitAlreadyPlacedElsewhere,ObjectAlreadyPlacedElsewhere,CannotSwapSameUnit};
struct BoardMutationResult {bool accepted, changed; optional<BoardMutationError> error;
  static ok(changed); static rejected(error);};    // accepted==(!error); rejected changes nothing
class BoardOccupancy { explicit BoardOccupancy(const TraversalGraph&);   // captures standable set once
  bool isStandable(cell);
  optional<BattleUnitId> unitAt(cell); optional<BoardCell> cellOf(BattleUnitId);
  span<const BattleObjectId> objectsAt(cell)/*ascending id*/; optional<BoardCell> cellOf(BattleObjectId);
  size_t unitCount/objectCount(); bool invariantsHold();
  // mutators (placeUnit/moveUnit/swapUnits/removeUnit/placeObject/removeObject) — command path only
};

// deployment_layout.hpp — approach = player's entry edge; enemy on the far one.
struct DeploymentStrips {bool axisIsZ; int player{Min,Max}, enemy{Min,Max}; isPlayerRow/isEnemyRow;};
DeploymentStrips deploymentStrips(size, playerApproach, stripDepth);   // scan candidates before a graph exists
struct DeploymentLayout {int stripDepth; VoxelOrientation playerApproachDirection; Vector2Int boardSize;
  vector<BoardCell> playerCells, enemyCells;   // centre-first, then perpendicular, then Y, then BoardCellLess
  contains(side,cell); cells(side); isDeploymentAxisZ();
  deploymentAxisCoordinate(cell); perpendicularCoordinate(cell); rowIndexFromOuterEdge(side,cell);};
DeploymentLayout buildDeploymentLayout(graph, size, playerApproach, stripDepth);

// board_line_of_sight.hpp — deterministic 3D DDA over ICellSource (not VoxelMap).
constexpr string_view LineOfSightTransparentTag = "losTransparent";  constexpr float DefaultBoardEyeHeight=0.65f;
struct LineOfSightResult {bool clear; optional<Vector3Int> firstBlockingVoxel;};
class IBoardLineOfSightExtraBlockers { virtual bool blocksVoxel(const Vector3Int&) const; };  // step 10 objects
class BoardLineOfSight { static LineOfSightResult trace(board, fromSupport, toSupport,
  const IBoardLineOfSightExtraBlockers* =nullptr, float eyeHeight=DefaultBoardEyeHeight); };
// Solid & not losTransparent blocks; endpoint columns ignored; supercover Amanatides-Woo, no corner leak.

// board_builder.hpp — plan/pin/pause/build-once (live) OR materialise a prefab (handcrafted).
struct WorldBoardRequest {Vector3Int encounterSupportCell; Vector2Int size; int minWorldY,maxWorldY;
  VoxelOrientation approachDirection; int deploymentDepth; size_t playerSeatCount,enemySeatCount;};
struct WorldBoardPlan {request; vector<Vector3Int> candidateWorldAnchors, requiredChunkCoordinates;
  Vector3Int pinWindowOriginChunk, pinWindowRange;};   // planWorld READS NOTHING
struct WorldBoardPlanResult {optional<WorldBoardPlan> plan; optional<BoardBuildError> error;};
class BoardBuilder {
  static WorldBoardPlanResult planWorld(request);
  static BoardBuildResult buildWorld(const VoxelWorld&, plan, double maxVerticalGap);
  static BoardBuildResult buildHandcrafted(def, const PrefabDefinition& geometry, const VoxelRegistry&,
    size_t playerSeats, enemySeats, double maxVerticalGap);   // identity transform, validate before writing
  static BoardBuildResult buildFrozenSource(...);   // shared tail; the fixture calls this too
};

// board_data.hpp — the frozen board.
struct BoardExtent {Vector2Int size; TraversalBounds traversalBounds;};
struct RequiredChunkStamp {Vector3Int coordinates; uint64 contentRevision;};
struct LiveBoardTerrainStamp {const VoxelWorld* world; weak_ptr<const void> mapLifetime;
  vector<RequiredChunkStamp> requiredChunks;};   // NOT VoxelWorld::revision() — per-required-chunk only
enum class BoardBuildErrorCode {InvalidRequest,NumericOverflow,RequiredChunkUnavailable,
  NoFeasibleWorldAnchor,InvalidHandcraftedDefinition,PrefabVoxelOutOfBounds,TraversalBuildFailed,
  InsufficientDeploymentCapacity};
struct BoardBuildError {code, boardOrEncounterId, optional<string> prefabId, optional<Vector3Int> cell,
  size_t required/availablePlayerSeats, required/availableEnemySeats, diagnosticDetail;};
struct BoardBuildResult {optional<BoardData> board; optional<BoardBuildError> error;};  // exactly one set
class BoardData {
  struct Parts {source; shared_ptr<const ICellSource> cells; Vector3Int presentationOrigin;
    optional<Vector3Int> liveWorldAnchor; BoardExtent extent; TraversalGraph navigation;
    DeploymentLayout deployment; vector<BoardCell> borderCells; optional<LiveBoardTerrainStamp> liveTerrainStamp;};
  explicit BoardData(Parts);   // throws if the source invariants are broken
  sourceDescriptor(); cells(); navigation(); occupancy(); deployment(); extent();
  presentationOrigin(); liveWorldAnchor(); liveTerrainStamp(); borderCells();
  bool isInsideColumn(x,z); isInside(cell); isStandable(cell); isBorderCell(cell);
  int movementCost(cell);   // throws on non-standable/unresolved — never silently 1
  Vector3Int toPresentationCell(cell); optional<BoardCell> fromPresentationCell(pres);  // no clamp/downward scan
  Vector3Int toLiveWorldCell(cell); BoardCell fromLiveWorldCell(world);   // LIVE-ONLY, throw on a handcrafted board
  spk::Vector3 unitPresentationPosition(cell);
  bool terrainIsCurrent(); void requireCurrentTerrain();   // live abort vs handcrafted always-current
  friend class BoardMutation; };
class IBoardMovementBlockers { virtual bool blocksMovement(const BoardCell&) const; };   // step 10
TraversalCostQuery boardMovementQuery(board, BattleUnitId mover, const IBoardMovementBlockers* =nullptr);
class BoardMutation { explicit BoardMutation(BoardData&);   // the ONLY writable handle onto occupancy
  placeUnit/moveUnit/swapUnits/removeUnit/placeObject/removeObject; };
```

Invariants: local `y` == world voxel Y for a live board (only x/z rebased); `walkHeightAtCenter`
already returns the top, so unit placement adds no extra `+1`. A handcrafted board may never
pass as live (zero presentation origin is not liveness). Terrain that moved under a live board
is a controlled abort, never a silent rebuild. This step consumes **no** battle RNG. Occupancy
mutators are gated behind `BoardMutation`; UI/AI/snapshots get const `BoardData`.

---

## Step 06 — Battle session, units, placement, and event log (`battle/`)

One headless `BattleSession` privately owns a `BattleContext`: copied descriptor, `BoardData`,
ordered `BattleUnit`s, battle RNG, phase/outcome state, counters, append-only event log, and
authoritative/publish batch stores. No mutable context escapes the session. `BoardSourceDescriptor`
is copied unchanged into the descriptor, `BattleStarted`, snapshots, and the material digest;
handcrafted boards are always terrain-current while live boards can terminate with
`RequiredTerrainChanged`.

Files: `battle/{battle_unit,battle_context,battle_session,battle_command,battle_command_result,`
`battle_event,battle_event_log,battle_snapshot,battle_state_digest,battle_phase,battle_sequence_ids}.*`,
`battle/placement/placement_resolver.*`; coverage begins in `tests/battle/battle_session_test.cpp`.

```cpp
// battle_unit.hpp — homogeneous session-local projection, never written back to CreatureUnit.
struct BattleParticipantSeed { BattleSide side; uint32_t rosterOrder;
  optional<CreatureInstanceId> persistentCreatureId; optional<string> encounterSpawnMemberId;
  string speciesId, formId; CreatureAttributes attributes; vector<string> abilityIds, passiveStatusIds;
  optional<string> aiBehaviourId; vector<string> inheritedCompletedFeatNodeIds; };
class BattleUnit { static BattleUnit project(BattleUnitId, const BattleParticipantSeed&);
  BattleUnitId id(); BattleSide side(); uint32_t rosterOrder(); int health(), maxHealth(), actionPoints(),
  movementPoints(); bool placed(), isActiveCombatant(); RemovalReason removalReason(); };
// active == placed && health > 0 && removalReason == None; Impressed is never Defeated.

// battle_session.hpp — construction and the only public mutation gateway.
struct BattleConstructionRequest { BattleDescriptor descriptor; vector<BattleParticipantSeed> participants;
  OpponentPlacementPolicy enemyPlacement; };
static BattleConstructionResult BattleSession::create(BattleConstructionRequest, const Registries&, BoardData);
CommandResult submit(const BattleCommand&, CommandIssuer);
BattleSnapshot snapshot() const; vector<BattleEvent> copyEvents(EventRange) const;
vector<CommittedBattleBatch> takeCommittedBatches(); const vector<CommittedBattleBatch>& archivedBatches() const;
uint64_t gameplayProgressDigest() const;

// battle_command.hpp — final value command shapes (only placement/confirmation execute in step 06).
using BattleCommand = variant<PlaceUnitCommand{BattleUnitId, BoardCell},
  ConfirmDeploymentCommand{BattleSide}, MoveCommand{BattleUnitId, BoardCell},
  CastAbilityCommand{BattleUnitId, string, BoardCell}, EndTurnCommand{BattleUnitId, EndTurnRequestCause}>;
enum class CommandController { System, Player, EnemyAi, DebugAutoplay };
using CommandResult = variant<AcceptedCommand{BattleActionId, BattleBatchId, EventRange},
  RejectedCommand{CommandRejection}, AbortedCommand{BattleAbortReason, BattleBatchId, EventRange}>;
```

Construction validates descriptor/source pairing, seed provenance/references/loadouts/attributes,
and deployment capacity before publishing; IDs allocate player roster order first, then enemy
spawn order. It auto-places enemies with fixed, by-line, or partial-Fisher–Yates seeded-random
policy (one RNG draw per enemy only after capacity is known) and auto-confirms the enemy side.
Player placement/reposition/friendly-swap and player confirmation go through `submit`; rejected
commands change no state, log, counter, or RNG draw. Move/cast/end-turn are declared but return
`CommandUnavailable` until later steps.

`BattleEvent` is a value-owned variant with `BattleEventHeader` (`BattleId`, sequence, batch,
time, optional turn/action, category, origin). The full lifecycle/gameplay vocabulary is declared
now; Step 06 emits `BattleStarted`, deployment changes/confirmation, and technical-abort terminal
events. A committed batch owns before/after snapshots and a half-open event range. Normal commits
reserve one batch and three event IDs for an abort tail; an exhausted preflight rolls back and
commits a no-action `TechnicalAbort` batch. `ResourceChangeReason` now lives in shared
`battle_types.hpp`, so condition filters and logged resource changes use the same enum.

## Step 07 — Stamina scheduler and phase machine (`battle/scheduler/`)

`BattleSession::advanceUntilActivation()` is legal only in `AwaitingActivation`; it returns
`SchedulerCallResult` (`SchedulerAdvanceResult` or `RejectedSchedulerAdvance`) and uses the
same non-reentrant session guard as commands. `StaminaScheduler` jumps exact `BattleTime` by
the minimum time-to-ready (or the future step-10 timeline-boundary seam), filling only active,
non-stunned units and clamping at effective stamina. No scheduler path reads wall/render time
or battle RNG.

```cpp
enum class SchedulerStop { ActivationReady, Terminal, Aborted };
enum class SchedulerRejection { WrongPhase, SessionBusy };
struct SchedulerAdvanceResult { SchedulerStop stop; optional<BattleUnitId> activeUnit;
  vector<BattleBatchId> committedBatches; };
using SchedulerCallResult = variant<SchedulerAdvanceResult, RejectedSchedulerAdvance>;
SchedulerCallResult BattleSession::advanceUntilActivation();
BattleOutcome evaluateBattleOutcome(const BattleContext&) noexcept;
struct BattleTerminalRecord { BattleId battleId; BattleOutcome outcome;
  optional<BattleAbortReason> abortReason; };
```

Ready selection is explicitly `(Player before Enemy, rosterOrder, BattleUnitId)`. A selected
unit enters `Activation` with monotonic `TurnIndex` (never max), receives AP then MP refill,
then each one-shot penalty is subtracted and cleared, and finally emits `ActivationStarted`.
Scheduler batches have no action id; `EndTurnCommand` uses the normal command action id,
emits `ActivationEnded`, resets only the actor's bar, and returns to `AwaitingActivation`.
Time stays frozen through activation. Player can explicitly end player turns; EnemyAi can end
enemy turns; DebugAutoplay has the player-side autoplay end causes. Move/cast remain unavailable.

Player confirmation now emits `DeploymentConfirmed` then `DeploymentCompleted` in one command
batch and transitions to `AwaitingActivation` without pumping the scheduler. `BattleSnapshot`
captures `nextTurn` and `resolvedNonEndCommands`; the latter is reserved for the later generic
non-end command cap and intentionally remains outside `gameplayProgressDigest`. The timeline
seam currently has no runtime deadlines; no eligible filler/deadline produces the terminal
`TechnicalAbort{SchedulerNoFutureProgress}` batch, and a zero-time no-progress boundary uses
`TimelineBoundaryMadeNoProgress`.

## Next: step 08 — movement and targeting

[`steps/step-08-movement-and-targeting.md`](steps/step-08-movement-and-targeting.md): add the
first accepted non-end commands to the active-turn gateway. Scheduler/activation ownership,
resource pools, turn bars, phase transitions, command batches, and end-turn semantics above are
the baseline; do not advance time while resolving movement or targeting.
