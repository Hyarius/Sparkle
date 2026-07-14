# System — Battle foundations

**Status: foundations only.** This document describes contracts that later steps build on.
There is no board, battle session, scheduler, command, effect, status, AI, HUD or save
format yet, and nothing here creates one. Landed by
[plan step 01](../plan/steps/step-01-battle-foundations.md).

Everything below lives in `namespace pg` under `Playground/srcs`. None of it is in `spk::`:
these are game contracts, and the promotion checkpoints come much later in the plan.

---

## 1. Time is fixed point — `battle/battle_time.hpp`

`pg::BattleTime` is a signed count of **milliseconds** (`std::int64_t`, `TicksPerSecond =
1000`). Every simulated duration is one — turn-bar progress, stamina, delays, status
durations.

```cpp
BattleTime stamina = BattleTime::fromTicks(2500);   // 2.5 s
BattleTime total   = stamina * 3;                   // checked
double bar = ratioForDisplay(elapsed, total);       // display only
```

- `BattleTime{}` is a **valid zero duration**, never an invalid sentinel.
- Simulation never advances from a render-frame delta, a wall clock, `std::chrono::duration
  <double>` or `spk::UpdateContext` elapsed time. Two runs fed the same commands must
  produce identical tick values on every machine.
- Presentation converts to `double` one way, via `secondsForDisplay()` / `ratioForDisplay()`.
  Nothing feeds those results back into simulation state.

### Authored seconds → ticks

JSON is authored in human-readable seconds and converted by one strict rule:

```
scaled = seconds * 1000
accepted only when   seconds is finite
                     scaled is inside the int64 range
                     scaled is within 1e-9 of an integer
ticks = that integer
```

Sub-millisecond precision is **rejected, not rounded**: `4`, `4.0`, `0.125` and `-0.250` are
representable; `0.0005`, NaN, infinity and overflow are not. Parsers add a sign rule on top
via `requireBattleSeconds(reader, key, sign)`, which rethrows every failure as a `JsonError`
carrying the file and the exact JSON path:

| `BattleTimeSign` | Used for |
|---|---|
| `Positive` | stamina, durations |
| `NonNegative` | optional delays |
| `Any` | effect deltas, which may be negative |

### Arithmetic

`+`, `-`, unary `-` and `* integer` are **checked** and throw `std::overflow_error` naming
the operation. `clamp(value, min, max)` is a free function, so no call site can mistake
saturation for checked arithmetic. There is deliberately no floating multiplication: later
percentage modifiers use integer permille with an explicit rounding rule (§5).

---

## 2. Randomness

Two kinds, and they must not be confused:

| Source | Allowed for | Never for |
|---|---|---|
| `std::random_device` (`pg::randomWorldSeed`) | minting the seed of a **brand new world**, once | anything else |
| `pg::BattleRng` (SplitMix64) | every draw inside a battle | — |
| `pg::deterministic::deriveSeed` | child seeds from the world seed + a stable semantic identity | — |

A battle is replayable from its seed alone, so there is **no global RNG**: a `BattleSession`
will own its `BattleRng` by value. `spk::UUID::generate()` is off-limits for battle identity
for the same reason — it draws from a process-global `mt19937_64`.

### `battle/battle_rng.hpp`

SplitMix64, with its constants and shifts pinned by golden tests (they are part of the
replay contract, not a tunable):

```cpp
BattleRng rng(combatSeed);
std::uint64_t value  = rng.nextU64();
std::uint64_t sixSided = rng.uniformBelow(6);   // unbiased, rejection sampled
std::size_t   drawn  = rng.drawCount();
```

`uniformBelow(0)` throws **before drawing**, so a rejected call leaves the stream untouched.
Bounded draws discard the biased `2^64 mod bound` prefix rather than using a raw `% bound`.
`drawCount()` counts every actual draw, rejections included: an invalid command validates
before it draws, so it must leave both state and draw count unchanged. There is no floating
random API — encounter chances use integer permille and bounded integers.

### Child seeds (owned by step 12, shape locked here)

```cpp
encounterResolutionSeed = deriveSeed(worldSeed, stableIdentity + "/encounter-resolution");
combatSeed              = deriveSeed(worldSeed, stableIdentity + "/combat");
```

The session receives `combatSeed` and constructs `BattleRng(combatSeed)` **directly** — it
never derives a second seed from it. Encounter-resolution draws stay independent of combat
draws. Nothing hashes a pointer, a frame counter, wall time or `std::hash<std::string>`.

---

## 3. Identifiers

Never a raw pointer, a container index, a display name, a `std::hash`, or a UUID that cannot
parse its own persisted text.

### Encounter-local — `battle/battle_ids.hpp`

`BattleId`, `BattleUnitId` and `BattleObjectId` are `BattleNumericId<Tag>`: a `std::uint32_t`
in a tag-distinct wrapper, so they cannot be assigned to one another. `0` is the one invalid
value; constructing from zero throws at the checked boundary, and in a constant expression it
does not compile.

Unit and object ids are allocated from a counter the **session** owns (never a process-global
static, which would make a replay depend on which battles ran before it). Allocation starts
at 1 and is checked before it increments. Step 06 assigns unit ids in player-roster order
then enemy-roster order, and object ids in creation order.

`BattleId` is *not* allocated. It is a pure fold of the combat seed:

```cpp
mixed = deriveSeed(combatSeed, "battle-id/v1");
value = uint32(mixed) ^ uint32(mixed >> 32);   // 0 remaps to 1
```

so an archived battle keeps its id when replayed. It is never collision-probed against
process history. At most one session is authoritative in v1, so this 32-bit value scopes ids
and events **inside** that session; it is not a globally unique save identity. Archived
battles are disambiguated by their encounter identity and the step-18 result token.

### Persistent creatures — `core/creature_instance_id.hpp`

`CreatureInstanceId` is a validated string value whose canonical text is the persisted form:

```
creature-0000000000000001
creature-000000000000002a
```

`creature-` plus exactly 16 lower-case hex digits, zero padded, encoding a **non-zero**
`uint64` serial. It parses back to the identical serial. Uppercase, missing padding, signs,
whitespace, wrong length, non-hex digits, and zero are all rejected, without locale-dependent
case folding. Serials come from `PlayerData::nextCreatureSerial` (steps 04/18) and are
deterministic: a recruit is **never** identified by `std::random_device`.

### Authored content — `core/content_id.hpp`

Definition filename stems and definition-local semantic ids (Feat nodes, conditions, effects,
AI rules, teams, triggers) share one grammar:

```
[a-z0-9]+(?:-[a-z0-9]+)*        1–64 ASCII bytes
```

No leading, trailing or consecutive hyphens; no uppercase, underscore, whitespace, slash or
dot. `requireContentId(value, file, jsonPath, kind)` reports violations with the file, the
path, the value and the grammar. Local ids are scoped by their owning definition — `root` may
appear in two Feat Boards, but not twice in one.

Registry definition ids still come from the **filename stem** and are never re-authored inside
the JSON root. Existing world/voxel data is not retroactively held to this grammar; it has its
own compatibility surface.

---

## 4. Shared vocabulary — `battle/battle_types.hpp`

One spelling each, so JSON, events, effects, AI, Feats and presentation cannot invent
synonyms. JSON literals are lower camel case, and the parse/name tables live in this one
module.

| Enum | JSON literals |
|---|---|
| `BattleSide` | `player`, `enemy` |
| `EncounterKind` | `wild`, `trainer`, `gym`, `special`, `debug` |
| `DamageKind` | `physical`, `magical` |
| `BattleResource` | `actionPoints`, `movementPoints` |
| `CreatureStat` | `maxHealth`, `strength`, `magicPower`, `armor`, `resistance`, `maxActionPoints`, `maxMovementPoints`, `stamina`, `range` |
| `BattleOutcome` | `undecided`, `playerVictory`, `playerDefeat`, `draw`, `aborted` |

The magical offense stat is **`magicPower`** — never `ability`, `magic` or `spellPower`.
"Ability" stays the name of an authored command definition. `Range` is a stat bonus that
extends an ability's maximum range and nothing else.

Deliberately **absent**, and not to be added without revisiting the design: accuracy, evasion,
critical chance, elemental types, initiative speed, mana, energy, armor/resistance
penetration, level, XP.

---

## 5. Locked numeric conventions

Their consumers arrive in later steps; the representations are fixed now so those steps
cannot pick incompatible primitives.

- Probabilities and ratios in rules are **integer permille** (`1000 == 1.0`).
- Authored encounter chance is `0..1000` inclusive.
- Stat additions are signed **checked** integers.
- Integer formula division **truncates toward zero** unless an effect contract explicitly
  specifies another rounding mode.
- Board coordinates use `spk::Vector3Int`; range distance ignores `y`.
- Externally visible cell ordering is `(z, x, y)` ascending unless a board step specifies a
  more semantic primary key.
- Externally visible unit tie order is Player side first, then encounter roster order, then
  `BattleUnitId`.
- Unordered-container iteration is **never** an outcome order.

---

## 6. Error and determinism rules

- JSON errors name the source file, the exact JSON path, the bad value and the expected
  constraint.
- Parser helpers catch conversion/overflow errors and rethrow `JsonError`; a raw STL exception
  must not escape startup without context.
- IDs and time values are value-owned and trivially copyable.
- ID parsing and formatting never depend on locale.
- A `uint64_t` is never serialized through a JSON double: step 18 writes world seeds as
  decimal **strings** and creature ids in their canonical text.
- A battle RNG never advances in a validation or query function.
- No test depends on wall time or on filesystem ordering.

---

## 7. Where this is exercised

`Playground/tests/battle/` — `battle_time_test.cpp`, `battle_ids_test.cpp`,
`battle_rng_test.cpp` (golden SplitMix64 vectors, child-seed derivation).
`Playground/tests/core/` — `content_id_test.cpp`, `creature_instance_id_test.cpp`,
`game_rules_test.cpp`, `registries_transaction_test.cpp`.

All headless: no window, no GL context.
