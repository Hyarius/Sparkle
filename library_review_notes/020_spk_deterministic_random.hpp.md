# Review 020 — `includes/structures/math/spk_deterministic_random.hpp`

- File: [includes/structures/math/spk_deterministic_random.hpp](<../includes/structures/math/spk_deterministic_random.hpp>)
- Done [X]
- Remarks and requested changes:

Refactor and clarify the deterministic hashing utilities so they provide an explicit, testable, cross-platform byte-level contract.

These functions are likely to influence procedural generation, persisted seeds, cached data, tests, or network-visible identifiers. Their exact output must therefore be treated as a versioned data-format contract rather than as an internal implementation detail.

Do not silently change existing deterministic outputs without first determining whether compatibility with previously generated data is required.

## Incorrect FNV-1a offset basis

The current constant is:

```cpp
inline constexpr std::uint64_t FnvOffset = 1469598103934665603ULL;
```

The standard 64-bit FNV-1a offset basis is:

```cpp
14695981039346656037ULL
```

The current value is missing the final digit `7`.

The prime is correct:

```cpp
1099511628211ULL
```

Therefore, the current `fnv1a()` function does not implement standard FNV-1a despite its name.

Resolve this explicitly.

### When backward compatibility is not required

Correct and rename the constants:

```cpp
inline constexpr std::uint64_t Fnv1a64OffsetBasis =
	14695981039346656037ULL;

inline constexpr std::uint64_t Fnv1a64Prime =
	1099511628211ULL;
```

Add known FNV-1a test vectors:

```text
""       -> 0xCBF29CE484222325
"a"      -> 0xAF63DC4C8601EC8C
"foobar" -> 0x85944171F73967E8
"hello"  -> 0xA430D84680AABD0B
```

## Separate algorithm identity from convenience naming

Names such as:

```cpp
StableHasher64
fnv1a
avalanche
```

currently mix three different concepts:

* A stable application-specific hash contract.
* A standard FNV-1a byte hash.
* A SplitMix64-style finalization step.

Make each algorithm's identity explicit.

A suitable organization is:

```cpp
namespace spk::deterministic
{
	namespace fnv1a64
	{
		inline constexpr std::uint64_t OffsetBasis = ...;
		inline constexpr std::uint64_t Prime = ...;

		[[nodiscard]]
		constexpr std::uint64_t hash(std::string_view p_value) noexcept;
	}

	class StableHasher64
	{
		// Application-level structured deterministic hasher.
	};

	[[nodiscard]]
	constexpr std::uint64_t avalanche64(std::uint64_t p_value) noexcept;

	[[nodiscard]]
	constexpr double toUnitInterval(std::uint64_t p_value) noexcept;
}
```

Another coherent naming scheme is acceptable, but do not imply conformance to a standard algorithm when the implementation intentionally differs.

## Define the exact byte contract

Cross-platform determinism requires more than avoiding `std::hash`.

Document exactly how each supported value is converted into bytes.

Do not hash raw object representations using:

```cpp
reinterpret_cast
std::as_bytes
sizeof(TType)
```

for arbitrary objects.

Raw object representations can include:

* Platform-dependent endianness.
* Padding bytes.
* Different type widths.
* Different enum representations.
* Non-canonical floating-point values.

Provide explicit mixing operations for the exact supported semantic types.

At minimum, consider:

```cpp
constexpr void mix(std::string_view p_value) noexcept;
constexpr void mix(std::uint32_t p_value) noexcept;
constexpr void mix(std::int32_t p_value) noexcept;
constexpr void mix(std::uint64_t p_value) noexcept;
constexpr void mix(std::int64_t p_value) noexcept;
```

Encode integers in one documented canonical byte order, preferably little-endian or big-endian, regardless of host architecture.

For example, a canonical little-endian unsigned 32-bit mix may conceptually process:

```cpp
mixByte(static_cast<std::uint8_t>(p_value >> 0U));
mixByte(static_cast<std::uint8_t>(p_value >> 8U));
mixByte(static_cast<std::uint8_t>(p_value >> 16U));
mixByte(static_cast<std::uint8_t>(p_value >> 24U));
```

Use one private byte-mixing primitive as the single source of truth:

```cpp
constexpr void _mixByte(std::uint8_t p_byte) noexcept;
```

## Current integer mixing is not FNV-1a byte mixing

The current integer overload performs:

```cpp
_value ^= static_cast<std::uint32_t>(p_value);
_value *= FnvPrime;
```

This folds the complete 32-bit value in one operation.

That behavior can be a valid application-specific deterministic transform, but it is not equivalent to mixing four bytes through FNV-1a.

Do not describe it as ordinary FNV integer hashing.

### Adopt canonical byte-wise integer encoding

Mix fixed-width integers byte by byte in a documented order.

Treat this as a deterministic-format migration because every affected output will change.

Do not switch between these approaches accidentally.

## Remove the platform-width `int` overload

The free function currently accepts:

```cpp
void mix(std::uint64_t &p_hash, int p_value) noexcept;
```

The width of `int` is not the appropriate basis for a cross-platform persistence contract.

Replace it with explicit fixed-width overloads:

```cpp
void mix(std::uint64_t &p_hash, std::int32_t p_value) noexcept;
void mix(std::uint64_t &p_hash, std::uint32_t p_value) noexcept;
```

Add 64-bit overloads when required by call sites.

Do not silently narrow arbitrary integral values to `std::int32_t`.

If convenience templates are added, constrain them carefully and map every supported type to an explicit canonical width.

## Preserve boundaries between mixed values

Repeated string mixing currently hashes the same byte stream as concatenation:

```cpp
StableHasher64 a;
a.mix("ab");
a.mix("c");

StableHasher64 b;
b.mix("a");
b.mix("bc");
```

Both operations process:

```text
abc
```

This is normal for a raw streaming hash, but it is unsafe when `StableHasher64` is intended to hash structured values.

### Structured-value model

Treat each public `mix()` call as one distinct structured value.

Represent the canonical encoding of a value with a small internal descriptor rather than exposing separate `mixTag()`, `mixSize()`, and `mixBytes()` operations.

For example:

```cpp
enum class ValueTag : std::uint8_t
{
	String,
	UInt32,
	Int32,
	UInt64,
	Int64
};

struct MixValue
{
	ValueTag tag;
	std::span<const std::byte> bytes;

	[[nodiscard]]
	constexpr std::uint64_t size() const noexcept
	{
		return static_cast<std::uint64_t>(bytes.size());
	}
};
```

The structure describes one already encoded value:

```text
[tag][fixed-width byte count][canonical bytes]
```

Do not store a separate writable `size` field when the size can be derived from the byte range. This prevents the declared size from disagreeing with the actual number of bytes.

Provide one private structured mixing operation, conceptually:

```cpp
constexpr void _mix(const MixValue &p_value) noexcept;
```

This operation must:

1. Mix the value tag using a documented fixed-width representation.
2. Mix the byte count as a canonical fixed-width integer.
3. Mix every encoded byte in order.

The existing typed overloads remain responsible for producing canonical bytes:

```cpp
constexpr void mix(std::string_view p_value) noexcept;
constexpr void mix(std::uint32_t p_value) noexcept;
constexpr void mix(std::int32_t p_value) noexcept;
constexpr void mix(std::uint64_t p_value) noexcept;
constexpr void mix(std::int64_t p_value) noexcept;
```

The internal structured operation does not replace the lowest-level byte primitive:

```cpp
constexpr void _mixByte(std::uint8_t p_byte) noexcept;
```

`_mixByte()` remains the single implementation point for the underlying FNV-1a state transition.

The structured encoding must make these inputs distinct:

```text
("ab", "c")
("a", "bc")
```

It must also distinguish values with equivalent raw bytes but different semantic types when their tags differ, such as a 32-bit integer and a four-byte string.

Do not use an undocumented separator character as the general solution. Arbitrary strings can contain any separator byte unless escaping or length-prefixing is formally defined.


## String encoding contract

`std::string_view` is a sequence of bytes, not a Unicode encoding contract.

Document whether semantic paths must be:

* ASCII.
* UTF-8.
* Another explicitly defined byte encoding.

For file paths or user-visible text, do not assume that visually identical Unicode strings always have identical byte representations.

Do not add automatic Unicode normalization inside this low-level utility unless the project already has a clear normalization policy.

## Refactor `deriveSeed()`

The current implementation is:

```cpp
return fnv1a(std::to_string(p_masterSeed) + "::" + std::string(p_semanticPath));
```

This has several problems:

* It allocates temporary strings.
* It converts the numeric seed into textual decimal form.
* Its format is only documented implicitly by the concatenation expression.
* It depends on the behavior currently hidden behind `fnv1a()`.
* It does not provide explicit algorithm versioning or domain separation.
* Changing punctuation or textual conversion would silently change every derived seed.

Replace it with an explicit deterministic format.

A preferred new-format design is conceptually:

```cpp
[[nodiscard]]
constexpr std::uint64_t deriveSeed(
	std::uint64_t p_masterSeed,
	std::string_view p_semanticPath) noexcept
{
	StableHasher64 hasher;

	hasher.mixDomain("spk.derive-seed.v1");
	hasher.mix(p_masterSeed);
	hasher.mix(p_semanticPath);

	return avalanche64(hasher.value());
}
```

Exact API details may differ, but the encoded components and version must be explicit.

The domain identifier prevents this operation from accidentally sharing the same input domain as unrelated stable hashes.

derivedSeed could be edited without thinking about old compatibility, edit anythiong needed without looking at previous execution

## Clarify the avalanche function

The constants and shifts in:

```cpp
avalanche(...)
```

match the SplitMix64 finalizer.

Rename or document it more precisely, for example:

```cpp
splitMix64Finalize(...)
```

or:

```cpp
avalanche64(...)
```

with documentation identifying the algorithm.

Make it clear that:

* It is a deterministic non-cryptographic bit mixer.
* It is not a random-number generator by itself.
* It is not a cryptographic hash.
* It maps `0` to `0`.
* Its exact output is part of the deterministic contract when persisted.

Make the function `constexpr`.

Add fixed golden tests.

## Clarify `unitInterval()`

The current conversion:

```cpp
static_cast<double>(p_hash >> 11U) *
	(1.0 / 9007199254740992.0)
```

uses the upper 53 bits and produces a value in:

```text
[0.0, 1.0)
```

Keep this behavior, but rename it to make the conversion explicit:

```cpp
toUnitInterval(...)
```

Document that:

* Zero maps to `0.0`.
* `UINT64_MAX` maps to the greatest representable grid point below `1.0`.
* The lower 11 bits are discarded.
* The input should already have suitable bit distribution.
* The function does not call `avalanche64()` automatically.

Make the function `constexpr`.

When bit-identical floating-point behavior across every supported platform is required, add an appropriate compile-time requirement for the project's floating-point assumptions, such as IEC 60559 binary64 support.

Do not return `1.0`.

## Make pure operations `constexpr`

The hashing and conversion operations are suitable for compile-time evaluation.

Where supported by the project's C++23 toolchain, make the following `constexpr`:

```cpp
StableHasher64()
StableHasher64(std::uint64_t)
mix(...)
value()
fnv1a64::hash(...)
avalanche64(...)
toUnitInterval(...)
```

Keep `noexcept` on operations that perform only fixed arithmetic and byte iteration.

This allows deterministic identifiers and golden values to be verified with `static_assert`.

## API duplication

The class API and the free `mix(std::uint64_t &, ...)` functions currently provide two ways to perform the same state mutation.

Inspect call sites before deciding whether both are useful.

Prefer one primary abstraction.

A coherent design may keep:

```cpp
StableHasher64
```

as the main incremental API and provide only focused one-shot helpers.

Do not let separate implementations drift.

Consider returning the updated hash instead of mutating through an output reference only when that better matches existing Sparkle conventions.

## Includes

The corrected header should approximately require:

```cpp
#include <cstdint>
#include <string_view>
```

Remove:

```cpp
#include <string>
```

Add `<limits>` only when floating-point or integer assumptions are checked.

Do not include broad convenience headers.

## Documentation

Document that the utilities are:

* Deterministic for the same canonical byte input.
* Independent of `std::hash`.
* Non-cryptographic.
* Not suitable for passwords, authentication, signatures, or adversarial collision resistance.
* Sensitive to any change in constants, byte order, type tags, length encoding, or domain identifiers.
* Versioned when their output is persisted or used to reproduce procedural content.

Document the precise range of `toUnitInterval()`.

Document whether `StableHasher64` is a raw byte-stream hasher or a structured-value hasher.

Do not claim generic cross-platform determinism without also defining integer widths, byte order, and string encoding.

## Tests for standard FNV-1a

When standard FNV-1a is exposed, add tests covering:

* Empty input.
* One-character input.
* Multiple-character input.
* Known public FNV-1a 64-bit vectors.
* Embedded null bytes inside `std::string_view`.
* Bytes above `0x7F`.
* Incremental byte mixing matching one-shot hashing.
* Compile-time evaluation with `static_assert`.

## Tests for deterministic integer encoding

Add tests covering:

* Zero.
* Maximum unsigned values.
* Positive signed values.
* Negative signed values.
* `INT32_MIN`.
* `INT32_MAX`.
* `UINT32_MAX`.
* `INT64_MIN`.
* `INT64_MAX`.
* `UINT64_MAX`.
* The documented canonical byte order.
* Identical golden values on every supported compiler.
* Distinction between different widths when structured type tagging is used.

Do not compute expected values with the same implementation under test. Use hard-coded golden values generated from the documented format.

## Tests for structured mixing

When value boundaries are encoded, add tests proving that these inputs differ:

```text
("ab", "c")
("a", "bc")
```

Also test:

```text
("", "abc")
("abc", "")
```

and different semantic types with equivalent raw numeric values when type tags are part of the format.

When raw streaming semantics are retained instead, add a test demonstrating and documenting that segmented and concatenated byte input intentionally produce the same result.

## Tests for seed derivation

Add golden tests covering:

* Master seed zero.
* Maximum master seed.
* Empty semantic path.
* ASCII semantic paths.
* Paths containing `:`.
* Paths containing embedded null bytes when supported.
* Repeated calls producing identical output.
* Different master seeds producing different outputs.
* Different semantic paths producing different outputs.
* Domain separation from ordinary string hashing.
* Legacy output compatibility when a legacy function is retained.
* Version 1 and version 2 producing intentionally distinct documented outputs.

## Tests for avalanche conversion

Add tests covering:

* Zero.
* One.
* Maximum `std::uint64_t`.
* Several hard-coded golden values.
* Compile-time evaluation.
* Repeated input returning identical output.

Do not use statistical randomness tests as a replacement for exact deterministic vectors.

## Tests for unit interval conversion

Add tests covering:

* `0` produces `0.0`.
* `UINT64_MAX` produces a value below `1.0`.
* Every tested result is greater than or equal to `0.0`.
* Every tested result is strictly less than `1.0`.
* Values differing only in the lower 11 bits produce the same result.
* A change in the retained upper 53 bits changes the result.
* Compile-time evaluation where supported.

## Scope restrictions

Do not replace this utility with:

* `std::hash`.
* A platform-specific hashing API.
* A cryptographic library unless cryptographic behavior is actually required.
* Hashing of arbitrary object memory.
* Locale-dependent textual serialization.
* Unversioned serialization of implementation-defined types.
* A random engine with hidden mutable global state.

Do not add generic template hashing for every type unless each supported type has an explicit canonical encoding.

The objective is not merely to produce repeatable-looking numbers. The objective is to define a small, precise, versioned deterministic format whose output can be reproduced intentionally across builds, platforms, and future code revisions.
