# Review 005 — `includes/structures/container/spk_uuid.hpp`

- File: [includes/structures/container/spk_uuid.hpp](<../includes/structures/container/spk_uuid.hpp>)
- Done [X]
- Remarks and requested changes:


Refactor `spk::UUID` with focused API and correctness improvements while preserving its current lightweight design.

Do not redesign the class architecture. The current 16-byte storage model is appropriate and should remain unchanged.

### Core API improvements

Make simple operations `constexpr` and `noexcept` where possible:

```cpp
constexpr UUID() noexcept = default;
explicit constexpr UUID(Storage p_bytes) noexcept;

[[nodiscard]]
static constexpr UUID null() noexcept;

[[nodiscard]]
constexpr const Storage &bytes() const noexcept;

[[nodiscard]]
constexpr bool isNull() const noexcept;
```

Pass `Storage` by value in the constructor because it is only 16 bytes.

Keep:

```cpp
static UUID generate();
std::string toString() const;
```

### Comparison operators

Remove the explicit `operator!=`.

Replace the current comparison operators with a defaulted three-way comparison:

```cpp
[[nodiscard]]
constexpr auto operator<=>(const UUID &) const noexcept = default;
```

This should provide equality, inequality, and ordering based on the stored bytes.

Add the required `<compare>` include.

### Parsing support

Add UUID string parsing.

Expose:

```cpp
[[nodiscard]]
static UUID fromString(std::string_view p_string);

[[nodiscard]]
static std::optional<UUID> tryParse(
	std::string_view p_string) noexcept;
```

Use the canonical format:

```text
xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx
```

Parsing rules:

* Require exactly 36 characters.
* Require hyphens at positions `8`, `13`, `18`, and `23`.
* Accept uppercase and lowercase hexadecimal digits.
* Reject every invalid character or malformed input.
* `fromString()` must throw `std::invalid_argument` on invalid input.
* `tryParse()` must return `std::nullopt` and never throw.

The following round trip must work:

```cpp
const spk::UUID original = spk::UUID::generate();
const std::string text = original.toString();
const spk::UUID parsed = spk::UUID::fromString(text);

assert(parsed == original);
```

### UUID metadata

Add a version accessor:

```cpp
[[nodiscard]]
constexpr std::uint8_t version() const noexcept;
```

It should return the high four bits of byte index `6`.

Optionally add a variant check:

```cpp
[[nodiscard]]
constexpr bool hasRFCVariant() const noexcept;
```

It should verify that the two highest bits of byte index `8` match the RFC UUID variant.

Keep the current version-4 generation behavior:

```cpp
bytes[6] = static_cast<std::uint8_t>(
	(bytes[6] & 0x0Fu) | 0x40u);

bytes[8] = static_cast<std::uint8_t>(
	(bytes[8] & 0x3Fu) | 0x80u);
```

### Generation

Keep the current thread-local generator approach:

```cpp
static thread_local std::mt19937_64 generator(
	std::random_device{}());
```

The UUIDs are intended for engine entities, resources, and runtime identifiers, not for security-sensitive tokens.

The byte-generation implementation may be simplified or optimized, but readability should remain the priority.

### Hash correction

Keep the `std::hash<spk::UUID>` specialization, but correct the FNV-1a offset basis.

Use a fixed `std::uint64_t` accumulator:

```cpp
std::uint64_t result = 14695981039346656037ull;

for (const std::uint8_t byte : p_uuid.bytes())
{
	result ^= static_cast<std::uint64_t>(byte);
	result *= 1099511628211ull;
}
```

Return the result directly on 64-bit platforms.

On platforms where `std::size_t` is smaller than 64 bits, fold the upper bits into the lower bits before conversion:

```cpp
if constexpr (sizeof(std::size_t) >= sizeof(std::uint64_t))
{
	return static_cast<std::size_t>(result);
}
else
{
	return static_cast<std::size_t>(
		result ^ (result >> 32u));
}
```

### Constants and includes

Add a public size constant:

```cpp
static constexpr std::size_t Size = 16;
using Storage = std::array<std::uint8_t, Size>;
```

Add the necessary includes:

```cpp
#include <compare>
#include <optional>
#include <string_view>
```

Remove unused includes where applicable.

### Expected public interface

The final class should approximately expose:

```cpp
class UUID
{
public:
	static constexpr std::size_t Size = 16;
	using Storage = std::array<std::uint8_t, Size>;

private:
	Storage _bytes{};

public:
	constexpr UUID() noexcept = default;
	explicit constexpr UUID(Storage p_bytes) noexcept;

	[[nodiscard]]
	static UUID generate();

	[[nodiscard]]
	static constexpr UUID null() noexcept;

	[[nodiscard]]
	static UUID fromString(std::string_view p_string);

	[[nodiscard]]
	static std::optional<UUID> tryParse(
		std::string_view p_string) noexcept;

	[[nodiscard]]
	constexpr const Storage &bytes() const noexcept;

	[[nodiscard]]
	constexpr bool isNull() const noexcept;

	[[nodiscard]]
	constexpr std::uint8_t version() const noexcept;

	[[nodiscard]]
	constexpr bool hasRFCVariant() const noexcept;

	[[nodiscard]]
	std::string toString() const;

	[[nodiscard]]
	constexpr auto operator<=>(const UUID &) const noexcept = default;
};
```

Preserve the existing stream operator:

```cpp
std::ostream &operator<<(
	std::ostream &p_stream,
	const UUID &p_uuid);
```

Add tests covering:

* Default construction produces a null UUID.
* `null()` produces a null UUID.
* Generated UUIDs are non-null.
* Generated UUIDs report version `4`.
* Generated UUIDs use the RFC variant.
* `toString()` produces the canonical 36-character format.
* Uppercase and lowercase UUID strings can be parsed.
* Invalid lengths, hyphen positions, and hexadecimal characters are rejected.
* `toString()` and `fromString()` round-trip correctly.
* Equality, inequality, and ordering work.
* Equal UUIDs produce equal hashes.