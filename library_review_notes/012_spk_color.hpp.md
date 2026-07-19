# Review 012 — `includes/structures/graphics/geometry/spk_color.hpp`

- File: [includes/structures/graphics/geometry/spk_color.hpp](<../includes/structures/graphics/geometry/spk_color.hpp>)
- Done [X]
- Remarks and requested changes:

Refactor and complete `spk::Color` while preserving its lightweight four-channel floating-point representation.

## Channel ordering

Add a channel-order enum:

```cpp
enum class ChannelOrder
{
	RGBA,
	ARGB,
	BGRA,
	ABGR
};
```

Use one four-channel enum rather than separate three-channel and four-channel enums.

For four-channel input, the order describes all supplied values directly.

Examples:

```cpp
Color(1.0f, 0.5f, 0.0f, 1.0f, ChannelOrder::RGBA);
Color(1.0f, 1.0f, 0.5f, 0.0f, ChannelOrder::ARGB);
Color(0.0f, 0.5f, 1.0f, 1.0f, ChannelOrder::BGRA);
```

For three-channel input, remove the alpha component from the declared order and assign alpha to `1.0f`.

Therefore:

```text
RGBA without alpha -> RGB
ARGB without alpha -> RGB
BGRA without alpha -> BGR
ABGR without alpha -> BGR
```

This rule must be used consistently by constructors, hexadecimal parsing, and hexadecimal output without alpha.

## Construction

Replace the existing constructor with two explicit channel-count overloads:

```cpp
constexpr Color(
	float p_first,
	float p_second,
	float p_third,
	ChannelOrder p_order = ChannelOrder::RGBA);

constexpr Color(
	float p_first,
	float p_second,
	float p_third,
	float p_fourth,
	ChannelOrder p_order = ChannelOrder::RGBA);
```

Normal usage must remain simple:

```cpp
Color red(1.0f, 0.0f, 0.0f);
Color translucentRed(1.0f, 0.0f, 0.0f, 0.5f);
```

Use private helpers to reorder the supplied values into the internal `r`, `g`, `b`, and `a` members.

## Constructor validation

Every channel supplied to a public constructor must be:

* Finite.
* Greater than or equal to `0.0f`.
* Less than or equal to `1.0f`.

Throw `std::invalid_argument` when a supplied channel is NaN, infinite, below zero, or above one.

Example:

```cpp
Color(1.2f, 0.0f, 0.0f); // Throws.
Color(0.0f, -0.1f, 0.0f); // Throws.
```

Centralize validation in a private constexpr helper:

```cpp
static constexpr void _validateChannel(float p_value);
```

Do not silently clamp constructor arguments.

The public members may remain directly accessible for compatibility. Document that constructor validation does not prevent callers or arithmetic operations from later producing non-normalized channel values.

## Hexadecimal parsing

Remove the string constructor:

```cpp
Color(std::string_view);
```

Hexadecimal conversion must be explicit and follow the conventions used by the rest of the library.

Add:

```cpp
[[nodiscard]]
static constexpr Color fromHex(
	std::string_view p_hexCode,
	ChannelOrder p_order = ChannelOrder::RGBA);
```

Support:

```text
#RRGGBB
#RRGGBBAA
```

The letters in those examples represent positions, not a fixed RGBA interpretation. The actual interpretation is controlled by `ChannelOrder`.

Examples:

```cpp
Color::fromHex("#FF8000", ChannelOrder::RGBA);
// R = FF, G = 80, B = 00, A = FF

Color::fromHex("#0080FF", ChannelOrder::BGRA);
// B = 00, G = 80, R = FF, A = FF

Color::fromHex("#FFFF8000", ChannelOrder::ARGB);
// A = FF, R = FF, G = 80, B = 00
```

Parsing rules:

* Require a leading `#`.
* Require exactly six or eight hexadecimal digits.
* Accept uppercase and lowercase hexadecimal characters.
* For six-digit input, use the declared order with the alpha component removed.
* For six-digit input, assign alpha to `1.0f`.
* For eight-digit input, use the complete declared four-channel order.
* `fromHex()` throws `std::invalid_argument` for malformed input.

Avoid duplicating parsing logic between the two methods.

## Hexadecimal stream manipulator

Do not add a public member such as:

```cpp
color.toHex();
```

Instead, add a stream manipulator that allows:

```cpp
std::cout << spk::toHex << color << std::endl;
```

The default manipulator configuration must:

* Include alpha.
* Use `ChannelOrder::RGBA`.
* Produce uppercase hexadecimal output.
* Include the leading `#`.

Example:

```cpp
Color color(1.0f, 0.5f, 0.0f, 1.0f);

std::cout << spk::toHex << color;
// #FF8000FF
```

Make the manipulator configurable:

```cpp
std::cout
	<< spk::toHex(ChannelOrder::ARGB)
	<< color;
```

Allow alpha to be omitted:

```cpp
std::cout
	<< spk::toHex(ChannelOrder::RGBA, false)
	<< color;
```

A suitable API is:

```cpp
struct ColorHexManipulator
{
	ChannelOrder order = ChannelOrder::RGBA;
	bool includeAlpha = true;

	[[nodiscard]]
	constexpr ColorHexManipulator operator()(
		ChannelOrder p_order,
		bool p_includeAlpha = true) const noexcept;
};

inline constexpr ColorHexManipulator toHex{};
```

This must support both:

```cpp
stream << spk::toHex << color;
```

and:

```cpp
stream << spk::toHex(ChannelOrder::BGRA, false) << color;
```

Use stream-local state, such as `std::ios_base::xalloc()` and `iword()`, to store the selected color output mode.

The hexadecimal formatting mode may persist like standard stream manipulators such as `std::hex`.

Add a second manipulator to restore the normal component representation:

```cpp
inline constexpr ColorComponentManipulator toDecimal{};
```

Expected usage:

```cpp
std::cout
	<< spk::toHex
	<< color
	<< ' '
	<< spk::toDecimal
	<< color;
```

Expected result:

```text
#FF8000FF Color(1, 0.5, 0, 1)
```

Do not permanently modify unrelated stream formatting flags.

## Hexadecimal channel conversion

When writing hexadecimal output:

* Clamp each channel to `[0, 1]`.
* Convert each channel to `[0, 255]`.
* Round to the nearest integer.
* Use uppercase hexadecimal digits.
* Always output exactly two digits per included channel.

For non-finite values produced after construction:

* NaN becomes `0`.
* Negative infinity becomes `0`.
* Positive infinity becomes `255`.

The output order must follow the selected `ChannelOrder`.

When alpha is omitted, use the selected order with the alpha component removed.

Hexadecimal parsing and output represent encoded normalized channel values. Do not perform implicit sRGB-to-linear or linear-to-sRGB conversion.

## Default stream representation

Update the `spk::Color` string-formatting API so `toString()` explicitly supports both decimal and hexadecimal representations, as well as configurable channel ordering.

## Formatting enum

Add:

```cpp
enum class Format
{
	Decimal,
	Hexadecimal
};
```

## `toString()` API

Replace the current parameterless method:

```cpp
[[nodiscard]]
std::string toString() const;
```

with:

```cpp
[[nodiscard]]
std::string toString(
	Format p_format = Format::Decimal,
	ChannelOrder p_order = ChannelOrder::RGBA) const;
```

The first parameter controls the numeric representation.

The second parameter controls the order in which the color channels are written.

## Decimal formatting

Decimal output must include the selected channel-order name so the meaning of each value remains explicit.

For a color containing:

```cpp
r = 1.0f;
g = 0.5f;
b = 0.0f;
a = 1.0f;
```

the expected results are:

```cpp
color.toString();
// "RGBA(1, 0.5, 0, 1)"
```

```cpp
color.toString(
	Color::Format::Decimal,
	Color::ChannelOrder::ARGB);
// "ARGB(1, 1, 0.5, 0)"
```

```cpp
color.toString(
	Color::Format::Decimal,
	Color::ChannelOrder::BGRA);
// "BGRA(0, 0.5, 1, 1)"
```

Every decimal representation must include all four channels, including alpha.

## Hexadecimal formatting

Hexadecimal output must:

* Include all four channels.
* Follow the requested `ChannelOrder`.
* Use uppercase hexadecimal digits.
* Include the leading `#`.
* Write exactly two digits per channel.
* Clamp computed channel values to `[0, 1]` before conversion.
* Round normalized channel values to the nearest integer in `[0, 255]`.

Expected results:

```cpp
color.toString(Color::Format::Hexadecimal);
// "#FF8000FF"
```

```cpp
color.toString(
	Color::Format::Hexadecimal,
	Color::ChannelOrder::ARGB);
// "#FFFF8000"
```

```cpp
color.toString(
	Color::Format::Hexadecimal,
	Color::ChannelOrder::BGRA);
// "#0080FFFF"
```

## Shared formatting implementation

Do not duplicate the channel-ordering and formatting logic between:

* `toString()`
* The normal stream operator
* The hexadecimal stream manipulator

Introduce one internal formatting function, such as:

```cpp
void _writeToStream(
	std::ostream &p_stream,
	Format p_format,
	ChannelOrder p_order) const;
```

This helper must:

* Select the channels in the requested order.
* Write either decimal or hexadecimal output.
* Be the single source of truth for color formatting.

Implement `toString()` using a local stream:

```cpp
std::string Color::toString(
	Format p_format,
	ChannelOrder p_order) const
{
	std::ostringstream stream;
	_writeToStream(stream, p_format, p_order);
	return stream.str();
}
```

## Stream consistency

The ordinary stream operator should produce the default representation:

```cpp
std::cout << color;
```

Equivalent to:

```cpp
color.toString(
	Color::Format::Decimal,
	Color::ChannelOrder::RGBA);
```

The hexadecimal stream manipulator should produce the same output as `toString()` with matching parameters:

```cpp
std::cout
	<< spk::toHex(Color::ChannelOrder::BGRA)
	<< color;
```

must match:

```cpp
color.toString(
	Color::Format::Hexadecimal,
	Color::ChannelOrder::BGRA);
```

The hexadecimal manipulator should use a constructed manipulator object:

```cpp
spk::toHex()
spk::toHex(Color::ChannelOrder::ARGB)
spk::toHex(Color::ChannelOrder::BGRA)
```

Do not use a fixed global manipulator object such as:

```cpp
inline constexpr ColorHexManipulator toHex{};
```

## Independence from stream state

`toString()` must depend only on its explicit arguments.

It must never inherit formatting state previously applied to another stream.

For example:

```cpp
std::cout << spk::toHex() << color;

const std::string result = color.toString();
```

`result` must still be:

```text
RGBA(1, 0.5, 0, 1)
```

## Tests

Add tests covering:

* Default decimal RGBA output.
* Decimal ARGB output.
* Decimal BGRA output.
* Decimal ABGR output.
* Default hexadecimal RGBA output.
* Hexadecimal output for every supported channel order.
* Decimal output always includes the order name.
* Both formats always include alpha.
* Hexadecimal output uses uppercase and fixed-width channels.
* Hexadecimal conversion rounds correctly.
* Hexadecimal conversion clamps out-of-range computed values.
* `toString()` matches stream output when using the same format and order.
* `toString()` is independent from external stream formatting state.

## Channel access

Keep:

```cpp
[[nodiscard]]
constexpr std::array<float, 4> values() const noexcept;
```

The returned order must always be:

```text
r, g, b, a
```

regardless of the order used during construction.

Add:

```cpp
[[nodiscard]]
constexpr float &operator[](std::size_t p_index);

[[nodiscard]]
constexpr const float &operator[](std::size_t p_index) const;
```

Use:

```text
0 -> r
1 -> g
2 -> b
3 -> a
```

Throw `std::out_of_range` for an invalid index.

## Arithmetic operations

Add component-wise operations:

```cpp
constexpr Color &operator+=(const Color &p_other) noexcept;
constexpr Color &operator-=(const Color &p_other) noexcept;
constexpr Color &operator*=(const Color &p_other) noexcept;
constexpr Color &operator*=(float p_scalar) noexcept;
constexpr Color &operator/=(float p_scalar);
```

Add the corresponding non-mutating operators.

Semantics:

* `Color * Color` performs component-wise multiplication.
* Scalar operations affect all channels, including alpha.
* Division by zero throws `std::domain_error`.
* Arithmetic operations do not automatically clamp.
* Arithmetic results may therefore be outside `[0, 1]`.
* Use `clamp()` or `clamped()` when normalized output is required.

Because the public constructor validates its arguments, provide a private unchecked construction helper for internal arithmetic operations:

```cpp
[[nodiscard]]
static constexpr Color _fromUnchecked(
	float p_red,
	float p_green,
	float p_blue,
	float p_alpha) noexcept;
```

Do not route arithmetic results through the validating public constructor.

## Interpolation

Add:

```cpp
[[nodiscard]]
static constexpr Color lerp(
	const Color &p_from,
	const Color &p_to,
	float p_ratio) noexcept;
```

Use component-wise interpolation.

## Comparison

Add exact equality:

```cpp
[[nodiscard]]
constexpr bool operator==(const Color &) const noexcept = default;
constexpr bool operator!=(const Color &) const noexcept = default;
```

For floating point comparaison, use the spk::ApproxValue<float>, foundable here : includes\structures\math\spk_approx_value.hpp

## Named colors

Add:

```cpp
static const Color White;
static const Color Black;
static const Color Transparent;
static const Color Red;
static const Color Green;
static const Color Blue;
```

Define them as inline constants after the class definition if necessary:

```cpp
inline constexpr Color Color::White(1, 1, 1, 1);
inline constexpr Color Color::Black(0, 0, 0, 1);
inline constexpr Color Color::Transparent(0, 0, 0, 0);
inline constexpr Color Color::Red(1, 0, 0, 1);
inline constexpr Color Color::Green(0, 1, 0, 1);
inline constexpr Color Color::Blue(0, 0, 1, 1);
```

## Includes and file organization

Replace `<iostream>` with narrower headers.

The declaration header should approximately require:

```cpp
#include <array>
#include <cstddef>
#include <iosfwd>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
```

The implementation may require:

```cpp
#include <algorithm>
#include <cmath>
#include <iomanip>
#include <ostream>
#include <sstream>
```

Move non-constexpr stream-manipulator implementation into the `.cpp` file.

## Tests

Add tests covering:

* Default construction produces opaque white.
* Three-channel construction defaults alpha to one.
* Four-channel RGBA construction.
* ARGB construction.
* BGRA construction.
* ABGR construction.
* Three-channel ordering removes the alpha position correctly.
* Constructors reject negative values.
* Constructors reject values above one.
* Constructors reject NaN and infinity.
* Six-digit hexadecimal parsing.
* Eight-digit hexadecimal parsing.
* Every supported `ChannelOrder`.
* Six-digit parsing defaults alpha to one.
* Uppercase and lowercase hexadecimal digits.
* Invalid lengths, prefixes, and characters.
* `tryParseHex()` returns `nullopt`.
* Default `spk::toHex` output is RGBA with alpha.
* Configured hexadecimal output order.
* Hexadecimal output without alpha.
* Hexadecimal output rounds channel values correctly.
* Hexadecimal output clamps non-normalized computed values.
* `spk::toDecimal` restores normal stream output.
* Stream formatting does not corrupt unrelated flags.
* `values()` always returns RGBA order.
* Indexed channel access.
* Invalid indexed access.
* Normalization detection.
* Clamping of finite and non-finite values.
* Alpha helpers.
* Component-wise and scalar arithmetic.
* Division by zero.
* Arithmetic does not clamp.
* Linear interpolation and extrapolation.
* Clamped interpolation.
* Exact and approximate equality.
* Named color constants.
* `toString()` always uses the component representation.

Preserve a convenient default RGBA API while allowing explicit channel-order conversion wherever external formats require ARGB, BGRA, or ABGR.
