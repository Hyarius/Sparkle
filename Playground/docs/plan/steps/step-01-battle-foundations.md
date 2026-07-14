# Implement battle foundations and locked contracts

Prepare the small, final-purpose foundation that every later battle step will share:
deterministic fixed-point time and random draws, stable value IDs, common battle enums,
validated content identifiers, the expanded game-rules schema, and one accurate build/test
workflow. This step must not implement a board, battle unit, scheduler, command, effect,
mode, HUD, or save system.

This is an implementation task. Modify Playground, migrate its configuration, add focused
tests and documentation, and keep the existing exploration/world prototype green.

---

# 1. Repository baseline

Implement against the live repository, not an imagined restoration of the old battle
branch. Reinspect every named file before editing.

At the `6ae3151` planning baseline:

* `PlaygroundCore`, `SparklePlayground`, and `PlaygroundTests` already exist;
* `Playground/CMakeLists.txt` recursively adds Playground `.cpp` files to
  `PlaygroundCore`, excluding only `main.cpp`;
* `Playground/tests/CMakeLists.txt` already discovers GTest cases and labels them
  `playground`;
* the root `test` preset enables both Sparkle tests and Playground tests;
* `pg::JsonReader`, `pg::Registry<T>`, and `spk::loadJsonDirectory` already exist and must
  not be rebuilt;
* `pg::deterministic` aliases Sparkle's stable FNV/avalanche helpers;
* `randomWorldSeed()` legitimately uses `std::random_device` only to create a new run seed;
* `GameRules` currently contains only `maxVerticalTraversalGap` and the `hovered`/`invalid`
  mask coordinates;
* `config/game-rules.json` is version 1;
* `GameContext` contains only exploration events and world state;
* there is no battle time, battle RNG, battle ID, creature instance ID, battle-side enum,
  or battle module in the checked-out implementation;
* `docs/howto/build-and-run.md` incorrectly claims the `playground` preset builds
  `PlaygroundTests` and an `EreliaTools` executable. Neither statement is true.

Do not reintroduce a generic JSON loader, registry, observable resource, service locator,
or polymorphic factory as “foundation” work. Later steps add only the domain facilities
they actually need.

---

# 2. Required final behavior

After this step:

1. All simulated battle time has one signed integer representation and never advances from
   render-frame delta.
2. Authored seconds convert to that representation by one strict, tested rule.
3. A battle-local deterministic RNG can produce reproducible integers without modulo bias.
4. Persistent creatures, battle sessions, battle units, and battle objects have distinct
   value ID types; zero/empty sentinel values cannot masquerade as valid IDs.
5. Definition filenames and definition-local semantic IDs use one validated grammar.
6. Common battle-side, damage-kind, resource, stat, and outcome enums have one spelling.
7. `game-rules.json` version 2 carries the structural limits and battle tunables required
   by steps 02-20 while preserving current exploration values.
8. `Registries::loadAll` remains all-or-none when the new rules file is invalid.
9. The documented test commands match the actual CMake graph.
10. Both the existing world/exploration suite and new foundation tests pass.

No visible battle is expected. The checkpoint is a deterministic foundation test trace and
an ordinary exploration boot.

---

# 3. Scope and module boundary

All new code belongs to namespace `pg` under `Playground/srcs`. Do not edit `includes/` or
`srcs/`; none of these game contracts belongs in reusable `spk::` yet.

Use cohesive files equivalent to:

```text
Playground/srcs/core/content_id.hpp
Playground/srcs/core/content_id.cpp
Playground/srcs/battle/battle_time.hpp
Playground/srcs/battle/battle_time.cpp
Playground/srcs/battle/battle_ids.hpp
Playground/srcs/battle/battle_rng.hpp
Playground/srcs/battle/battle_rng.cpp
Playground/srcs/battle/battle_types.hpp
```

Header-only strong numeric IDs are acceptable. Parsing, checked arithmetic, and RNG
algorithms should have `.cpp` definitions when that keeps error handling and tests clear.
Do not create an all-purpose `battle_common.hpp` dumping ground.

---

# 4. Fixed-point `BattleTime`

## 4.1 Representation

Define a value type equivalent to:

```cpp
class BattleTime
{
public:
    using Rep = std::int64_t;
    static constexpr Rep TicksPerSecond = 1000;

    constexpr BattleTime() noexcept = default;
    [[nodiscard]] static constexpr BattleTime fromTicks(Rep p_ticks) noexcept;
    [[nodiscard]] static BattleTime fromSecondsExact(double p_seconds);

    [[nodiscard]] constexpr Rep ticks() const noexcept;
    [[nodiscard]] double secondsForDisplay() const noexcept;

    // equality, three-way comparison, checked +/-, checked multiplication by integer
};
```

One tick is exactly one millisecond. `BattleTime{}` is a valid zero duration, not an
invalid sentinel. Runtime bars, delays, timeline status durations, and stamina all use this
type. Presentation may convert it to `double` only for display/interpolation.

Do not use `float`, `std::chrono::duration<double>`, wall-clock timestamps, or
`spk::UpdateContext` elapsed time as authoritative battle time.

## 4.2 JSON seconds conversion

Authored JSON uses human-readable seconds. Convert with this exact contract:

```text
scaled = seconds * 1000
accepted only when:
    seconds is finite
    scaled is inside int64 range
    scaled is within 1e-9 of an integer
ticks = that integer
```

This deliberately rejects values with sub-millisecond precision instead of silently
rounding them. `4`, `4.0`, `0.125`, and `-0.250` are representable; `0.0005`, NaN,
infinity, and overflow are not. Domain parsers add their own sign rule: stamina and
durations require positive values, effect deltas may be negative, and optional delays may
permit zero.

Provide a JSON helper that preserves `file:path` diagnostics, for example:

```cpp
enum class BattleTimeSign
{
    Positive,
    NonNegative,
    Any
};

[[nodiscard]] BattleTime requireBattleSeconds(
    JsonReader &p_reader,
    const std::string &p_key,
    BattleTimeSign p_sign);
```

Do not make callers catch an uncontextualized `std::invalid_argument` and rebuild the path.

## 4.3 Arithmetic

Authoritative time arithmetic must detect signed overflow. It may throw
`std::overflow_error` with an operation-specific message. Required operations are:

* checked addition and subtraction;
* multiplication by a signed integer for definition-derived totals;
* clamp to `[zero, maximum]` as an explicit free function;
* ratio-for-display that never feeds simulation state.

Do not add arbitrary floating multiplication. Later percentage modifiers use integer
permille arithmetic and an explicitly documented rounding rule.

---

# 5. Stable identifiers

Do not use raw pointers, container indices, display names, `std::hash`, or a UUID API that
cannot parse its own persisted text.

## 5.1 Strong numeric encounter-local IDs

Use distinct wrappers equivalent to:

```cpp
template <typename TTag>
class BattleNumericId
{
    std::uint32_t _value = 0;

public:
    constexpr BattleNumericId() noexcept = default;
    explicit constexpr BattleNumericId(std::uint32_t p_value);
    [[nodiscard]] constexpr bool valid() const noexcept;
    [[nodiscard]] constexpr std::uint32_t value() const noexcept;
    auto operator<=>(const BattleNumericId &) const = default;
};

using BattleId       = BattleNumericId<struct BattleIdTag>;
using BattleUnitId   = BattleNumericId<struct BattleUnitIdTag>;
using BattleObjectId = BattleNumericId<struct BattleObjectIdTag>;
```

`0` is invalid and construction from zero must be rejected at the checked public boundary
or be restricted to the default constructor. Valid allocation starts at 1 and checks
overflow before incrementing. Keep the types non-interchangeable.

Unit/object allocation policy is owned by the eventual battle/session, not by a
process-global static. Step 06 assigns unit IDs in player roster order followed by enemy
roster order and object IDs in creation order. `BattleId` is not allocated from those
counters: step 06 deterministically derives its one session value from the exact combat
seed using the algorithm below.

```cpp
const std::uint64_t mixed = pg::deterministic::deriveSeed(p_combatSeed, "battle-id/v1");
std::uint32_t value = static_cast<std::uint32_t>(mixed) ^
                      static_cast<std::uint32_t>(mixed >> 32U);
if (value == 0U)
    value = 1U;
return BattleId(value);
```

At most one `BattleSession` is authoritative in v1, so this 32-bit ID scopes IDs/events
inside that session; it is not claimed to be a globally collision-free save identity.
Archived/replayed battles are disambiguated by the stable encounter identity and step-18
result token. Never collision-probe against process history, because that would make replay
depend on which battles happened earlier. Golden-test this derivation across separate
session constructions.

## 5.2 Persistent creature IDs

Define `CreatureInstanceId` as a validated string value. Its canonical text is:

```text
creature-XXXXXXXXXXXXXXXX
```

where `XXXXXXXXXXXXXXXX` is exactly 16 lowercase hexadecimal digits encoding a non-zero
`std::uint64_t` serial, padded with leading zeroes. Examples:

```text
creature-0000000000000001
creature-000000000000002a
```

Required API shape:

```cpp
class CreatureInstanceId
{
public:
    [[nodiscard]] static CreatureInstanceId fromSerial(std::uint64_t p_serial);
    [[nodiscard]] static CreatureInstanceId parse(std::string_view p_text);
    [[nodiscard]] std::uint64_t serial() const noexcept;
    [[nodiscard]] std::string_view string() const noexcept;
    [[nodiscard]] bool valid() const noexcept;
    auto operator<=>(const CreatureInstanceId &) const = default;
};
```

Reject uppercase hex, missing padding, extra characters, zero, signs, whitespace, and
overflow. Generation is deterministic from `PlayerData::nextCreatureSerial` in step 04/18;
never call `random_device` to identify a recruit.

## 5.3 Definition and local semantic IDs

Definition filename stems, Feat node IDs, condition IDs, effect IDs, AI rule IDs, team IDs,
and trigger IDs follow:

```regex
[a-z0-9]+(?:-[a-z0-9]+)*
```

Length is 1-64 bytes. ASCII only. No leading/trailing/consecutive hyphens, uppercase,
underscore, whitespace, slash, dot, or locale-dependent case folding.

Supply a helper such as:

```cpp
void requireContentId(std::string_view p_value,
                      const std::filesystem::path &p_file,
                      std::string p_jsonPath,
                      std::string_view p_kind);
```

Use it for all newly introduced domains. Do not retroactively reject existing world/voxel
data in this step; those loaders have their own compatibility surface. Registry definition
IDs still come from filename stems and are never redundantly authored inside the JSON root.

Local IDs are scoped by their owning definition. `root` in two different Feat Boards is
valid; duplicate local IDs in one board are not.

---

# 6. Common battle vocabulary

Lock these enum spellings now so JSON, events, effects, AI, Feats, and presentation do not
invent synonyms later:

```cpp
enum class BattleSide      { Player, Enemy };
enum class EncounterKind   { Wild, Trainer, Gym, Special, Debug };
enum class DamageKind      { Physical, Magical };
enum class BattleResource  { ActionPoints, MovementPoints };
enum class CreatureStat
{
    MaxHealth,
    Strength,
    MagicPower,
    Armor,
    Resistance,
    MaxActionPoints,
    MaxMovementPoints,
    Stamina,
    Range
};
enum class BattleOutcome   { Undecided, PlayerVictory, PlayerDefeat, Draw, Aborted };
```

JSON literals are lower camel case:

```text
player, enemy
wild, trainer, gym, special, debug
physical, magical
actionPoints, movementPoints
maxHealth, strength, magicPower, armor, resistance,
maxActionPoints, maxMovementPoints, stamina, range
```

The magical offense field is `magicPower`, never `ability`, `magic`, or `spellPower`.
“Ability” remains the authored command definition. `Range` is a stat bonus that extends an
ability's maximum range only.

Do not add accuracy, evasion, critical chance, elemental types, initiative speed, mana,
energy, armor penetration, resistance penetration, level, or XP.

Put enum parse/name tables in one battle-side module or the owning parser; never duplicate
slightly different maps across definitions.

---

# 7. Deterministic battle RNG

## 7.1 Seed derivation

Step 12 is the sole owner of encounter child-seed derivation. It builds one non-empty
stable instance identity containing encounter definition, world cell or debug identity,
and persisted ordinal, then calls the existing helper exactly as follows:

```cpp
const std::uint64_t encounterResolutionSeed = pg::deterministic::deriveSeed(
    p_worldSeed, p_stableEncounterIdentity + "/encounter-resolution");
const std::uint64_t combatSeed = pg::deterministic::deriveSeed(
    p_worldSeed, p_stableEncounterIdentity + "/combat");
```

`BattleSession` receives `combatSeed` in its descriptor and constructs
`BattleRng(combatSeed)` directly. It must not derive another `battle/` seed. This keeps
encounter resolution draws independent of combat draws and prevents double derivation.
Do not hash pointers, frame counters, wall time, or `std::hash<std::string>`.

## 7.2 Generator contract

Implement a small value-owned `BattleRng` using a fully specified algorithm, preferably
SplitMix64:

```cpp
class BattleRng
{
public:
    explicit BattleRng(std::uint64_t p_seed) noexcept;
    [[nodiscard]] std::uint64_t nextU64() noexcept;
    [[nodiscard]] std::uint64_t uniformBelow(std::uint64_t p_exclusiveUpperBound);
    [[nodiscard]] std::size_t drawCount() const noexcept;
};
```

Specify SplitMix64 constants and shifts in code comments and golden tests. Increment the
draw count exactly once per `nextU64` call. `uniformBelow(0)` is an error. Use rejection
sampling so every bounded value has equal probability; do not use `next % bound` unless the
rejected prefix has first been removed.

Do not expose a mutable global RNG. A `BattleSession` will own one. Invalid commands in
later steps validate before invoking anything that draws, so they leave both state and
`drawCount` unchanged.

Provide no floating random API unless a later step proves it necessary. Encounter chances
use integer permille and bounded integers.

---

# 8. `game-rules.json` version 2

Replace the current file with this exact root shape and preserve its current exploration
gap and current atlas cells:

```json
{
  "version": 2,
  "maxVerticalTraversalGap": 0.5,
  "battle": {
    "teamCapacity": 6,
    "abilitySlotCapacity": 8,
    "defaultBoardSize": [11, 11],
    "deploymentDepth": 2,
    "minimumStaminaSeconds": 0.1,
    "mitigationScale": 100,
    "maxCommandsPerActivation": 64,
    "maxAiCommandsPerActivation": 32,
    "maxEffectChainDepth": 32,
    "maxConditionDepth": 8
  },
  "overlayMasks": {
    "deployment": [0, 0],
    "movement": [1, 0],
    "path": [2, 0],
    "abilityRange": [3, 0],
    "areaOfEffect": [0, 1],
    "losBlocked": [1, 1],
    "hovered": [2, 1],
    "invalid": [3, 1]
  }
}
```

Use a typed substructure equivalent to:

```cpp
struct BattleGameRules
{
    static constexpr std::size_t RequiredTeamCapacity = 6;

    std::size_t teamCapacity = RequiredTeamCapacity;
    std::size_t abilitySlotCapacity = 8;
    std::array<int, 2> defaultBoardSize{11, 11};
    int deploymentDepth = 2;
    BattleTime minimumStamina = BattleTime::fromTicks(100);
    std::int64_t mitigationScale = 100;
    std::size_t maxCommandsPerActivation = 64;
    std::size_t maxAiCommandsPerActivation = 32;
    std::size_t maxEffectChainDepth = 32;
    std::size_t maxConditionDepth = 8;
};

struct GameRules
{
    double maxVerticalTraversalGap = 0.0;
    BattleGameRules battle;
    std::map<std::string, std::array<int, 2>> overlayMasks;
};
```

Equivalent strong dimensions are welcome. Keep existing call sites for
`maxVerticalTraversalGap` and `overlayMasks.at("hovered"/"invalid")` working.

---

# 9. Game-rules validation

The parser must call `forbidUnknown` at the root, `battle`, and `overlayMasks` levels and
reject any version other than 2. Validate before returning:

* `maxVerticalTraversalGap` is finite and non-negative;
* `teamCapacity` is exactly 6. This is a serialized invariant, not a tunable that may
  disagree with `std::array<..., 6>`;
* `abilitySlotCapacity` is exactly 8 in schema version 1; reject every other value so the
  runtime loadout, AI enumeration, and eight-slot HUD cannot drift;
* board width and depth are odd integers in `[5, 31]`;
* `deploymentDepth >= 1` and two deployment strips leave at least one neutral row;
* `minimumStaminaSeconds` is exactly representable, positive, and at least one tick;
* `mitigationScale` is an integer in `[1, 1000000]`; every later mitigation multiply/add
  uses checked wide arithmetic before conversion to the declared result type;
* command limits are positive and `maxAiCommandsPerActivation <=
  maxCommandsPerActivation`;
* effect-chain depth is 1-256;
* condition depth is 1-32;
* every mask coordinate is exactly two non-negative integers;
* all eight required mask keys are present and no other key is accepted.

Do not retain compatibility parsing for version 1. The resource file is migrated in the
same commit, and an unsupported version must fail clearly.

`Registries::loadAll` already parses into local values and publishes only after success.
Preserve that property: an invalid rules file cannot partially replace `_gameRules` or any
other registry.

---

# 10. Locked numeric conventions for later steps

Document and test these conventions even though their consumers arrive later:

* probabilities and ratios used by rules are integer permille (`1000 == 1.0`);
* authored encounter chance is `0..1000`, inclusive;
* stat additions are signed checked integers;
* integer formula division truncates toward zero unless a later effect contract explicitly
  specifies another rounding mode;
* board coordinates use existing `spk::Vector3Int`; range distance ignores `y`;
* externally visible cell ordering is `(z, x, y)` ascending unless a board step specifies a
  more semantic primary key;
* externally visible unit tie order is Player side first, then encounter roster order, then
  `BattleUnitId`;
* definitions and runtime values never use unordered-container iteration as an outcome
  order.

Do not implement damage, range, or tie selection here. These statements prevent later
steps from selecting incompatible primitive representations.

---

# 11. Correct the build and test workflow

Update `Playground/docs/howto/build-and-run.md` to describe the checked-in CMake graph,
without claiming an `EreliaTools` target exists.

The canonical commands from repository root after this step are:

```powershell
cmake --preset test
cmake --build build/test --target PlaygroundTests SparklePlayground
ctest --test-dir build/test --output-on-failure -L playground
./build/test/Playground/SparklePlayground.exe --resource-path Playground/resources
```

The direct test executable is useful while debugging but is not the only documented test
path:

```powershell
./build/test/Playground/tests/PlaygroundTests.exe
```

On the current Ninja layout, verify the actual output path before documenting it if the
repository has drifted. Do not add duplicate build presets merely to preserve the stale
documentation. Updating the `test` preset description to mention both suites is acceptable;
do not alter unrelated compiler/toolchain settings.

---

# 12. Required tests

Add focused headless tests under `Playground/tests/core/` and
`Playground/tests/battle/`. Do not require a window or GL context.

## 12.1 Battle time

Test:

* zero/default construction;
* exact conversions for `4`, `0.1`, `0.125`, and `-0.250` seconds;
* rejection of sub-millisecond values;
* rejection of non-finite and overflowing values;
* positive/non-negative/any JSON sign policies with exact file/path diagnostics;
* ordering and equality;
* checked addition, subtraction, multiplication, clamp, and every overflow edge;
* display conversion never mutates ticks.

## 12.2 IDs

Test:

* numeric ID default is invalid and valid IDs compare by value;
* distinct strong ID types cannot be accidentally assigned (compile-time assertion where
  practical);
* exact `BattleId` fold for fixed combat seeds, zero-fold remapping to one through a test
  seam, and identical results in independently constructed sessions;
* creature serial 1 and `0x2a` produce the exact canonical strings above;
* parse/format round trip at `uint64_t` maximum;
* rejection of zero, uppercase, short/long, sign, whitespace, invalid hex, and overflow;
* definition/local content ID grammar boundaries and 64/65-byte cases.

## 12.3 RNG

Golden-test the first several SplitMix64 results for at least two seeds. Test:

* two generators with the same seed have identical values and draw counts;
* a different seed diverges;
* `uniformBelow(1)` is always zero;
* invalid bound zero fails without advancing the generator;
* rejection sampling increments the draw count by the number of actual draws;
* a deterministic small-domain sample remains within bounds;
* the exact encounter-resolution/combat child derivations change with world seed, semantic
  identity, and domain suffix while remaining identical across independent runs.

Do not write a statistical/flaky randomness test.

## 12.4 Game rules

Load the real v2 file and assert every typed value. Generate temporary JSON fixtures that
independently reject:

* wrong version;
* missing and unknown fields at every object level;
* wrong team capacity;
* even/small/large board dimensions;
* invalid deployment depth;
* unrepresentable or non-positive minimum stamina;
* bad command/depth bounds;
* missing, unknown, negative, or malformed masks.

Add or extend a transactional `Registries::loadAll` test proving failed reload leaves a
previously loaded `Registries` value unchanged.

## 12.5 Existing behavior

Run the entire label, not only a filtered foundation test. Existing world generation,
voxel, traversal, fluid, widget-lifetime, and CLI tests must stay green.

---

# 13. Error and determinism requirements

* JSON errors name the source file, exact JSON path, bad value, and expected constraint.
* Parser helpers catch conversion/overflow errors and rethrow `JsonError`; raw STL errors
  must not escape startup without context.
* IDs and time values are value-owned and trivially copyable/movable where practical.
* Never depend on locale for ID parsing or formatting.
* Never serialize `uint64_t` through a JSON double. Step 18 writes world seeds as decimal
  strings and creature IDs in their canonical text.
* Never advance a battle RNG in a validation/query function.
* No test depends on wall time or the order of files returned by the filesystem.

---

# 14. Expected files

Add the cohesive foundation files and tests described above. Materially modify:

```text
Playground/srcs/core/game_rules.hpp
Playground/srcs/core/game_rules.cpp
Playground/resources/data/config/game-rules.json
Playground/docs/howto/build-and-run.md
```

Possibly update the `test` preset's inaccurate description. The source glob should discover
new `.cpp` files; reconfigure rather than manually enumerating them unless the live CMake
has changed.

Do not modify:

```text
includes/
srcs/
Playground/srcs/board/
Playground/srcs/world/
Playground/srcs/game_scene_widget.*
```

except for a minimal include/call-site adaptation forced by the typed `GameRules` change.

---

# 15. Documentation requirements

Document in implemented-system docs, not only code comments:

* one millisecond fixed-point time and strict authored conversion;
* which randomness is allowed for a new world versus a battle;
* canonical creature and content ID formats;
* exact shared enum/JSON spellings;
* the game-rules v2 schema and invariant bounds;
* the real configure/build/test/run commands.

Mark the battle types as foundations only. Do not claim a scheduler, battle loop, or save
format exists yet.

---

# 16. Non-goals

Do not implement battle/session state, board data, occupancy, pathfinding, placement,
events, scheduler phases, commands, effects, statuses at runtime, AI planning, modes,
encounter triggering, creature views, HUD, Feat evaluation, taming, result commits, save
files, authoring tools, hot reload, or engine promotion.

Do not add a generic UUID library, a scripting language, an ECS battle component, or a
global RNG service.

---

# 17. Acceptance checklist

This step is complete only when:

* [ ] battle time uses signed integer milliseconds everywhere introduced by this step;
* [ ] authored sub-millisecond/non-finite/overflow values fail contextually;
* [ ] checked time arithmetic is covered at its boundaries;
* [ ] persistent and encounter-local ID types cannot be confused;
* [ ] creature instance IDs parse and format canonically from deterministic serials;
* [ ] new definition/local IDs use the locked kebab grammar;
* [ ] SplitMix64 output and unbiased bounded draws have golden tests;
* [ ] step-12 child seeds derive only from world seed plus stable semantic identity/domain,
  and the session consumes the exact combat seed once;
* [ ] `BattleId` uses the fixed combat-seed fold and no process-history collision probe;
* [ ] all common enum and JSON spellings match this prompt;
* [ ] game-rules v2 loads and all invalid constraints fail;
* [ ] registry reload remains transactional;
* [ ] stale build/tool claims are removed from documentation;
* [ ] `PlaygroundTests` and `SparklePlayground` build from the `test` tree;
* [ ] `ctest -L playground` passes;
* [ ] an exploration smoke run still boots with the migrated resource file;
* [ ] no later battle subsystem was prematurely implemented.

---

# 18. Required handoff report

Report:

1. the exact `BattleTime`, ID, content-ID, RNG, enum, and `BattleGameRules` APIs;
2. the exact SplitMix64 golden sequence and time-conversion tolerance used;
3. files created and materially changed;
4. any live-repository drift and the adaptation made;
5. configure/build/test commands actually run and their results;
6. the exploration smoke scenario actually run, or why it was skipped;
7. the game-rules schema version and compatibility consequence;
8. any remaining limitation explicitly assigned to a later step.

Do not report a visual battle checkpoint; none exists in this foundation step.
