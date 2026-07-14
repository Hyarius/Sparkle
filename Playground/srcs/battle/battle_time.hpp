#pragma once

#include "core/json.hpp"

#include <compare>
#include <cstdint>
#include <string>

namespace pg
{
	// Foundation only: no scheduler, no battle loop, no runtime status exists yet.
	//
	// Every simulated battle duration - turn-bar progress, stamina, delays, status
	// durations - is a signed count of milliseconds. Simulation never reads a render-frame
	// delta, a wall clock or a float: two runs fed the same commands must produce the same
	// tick values on every machine. Presentation may convert to double, one way, for
	// display and interpolation.
	class BattleTime
	{
	public:
		using Rep = std::int64_t;
		static constexpr Rep TicksPerSecond = 1000;

		// Authored seconds are accepted only when seconds * 1000 lands within this distance
		// of a whole number, so 0.0005 s is rejected rather than silently rounded away.
		static constexpr double SecondsTolerance = 1e-9;

		constexpr BattleTime() noexcept = default;

		[[nodiscard]] static constexpr BattleTime fromTicks(Rep p_ticks) noexcept
		{
			BattleTime result;
			result._ticks = p_ticks;
			return result;
		}

		// Throws std::invalid_argument when p_seconds is not finite or is not a whole number
		// of milliseconds, std::overflow_error when the scaled value leaves the tick range.
		// Prefer requireBattleSeconds() from a parser: it adds the file/path context.
		[[nodiscard]] static BattleTime fromSecondsExact(double p_seconds);

		[[nodiscard]] constexpr Rep ticks() const noexcept
		{
			return _ticks;
		}

		[[nodiscard]] constexpr bool isZero() const noexcept
		{
			return _ticks == 0;
		}

		[[nodiscard]] double secondsForDisplay() const noexcept;

		[[nodiscard]] constexpr bool operator==(const BattleTime &p_other) const noexcept = default;
		[[nodiscard]] constexpr auto operator<=>(const BattleTime &p_other) const noexcept = default;

		// Checked arithmetic: authoritative time may never wrap silently. Each operation
		// throws std::overflow_error naming itself.
		[[nodiscard]] BattleTime operator+(const BattleTime &p_other) const;
		[[nodiscard]] BattleTime operator-(const BattleTime &p_other) const;
		[[nodiscard]] BattleTime operator-() const;
		[[nodiscard]] BattleTime operator*(Rep p_factor) const;

		BattleTime &operator+=(const BattleTime &p_other);
		BattleTime &operator-=(const BattleTime &p_other);
		BattleTime &operator*=(Rep p_factor);

	private:
		Rep _ticks = 0;
	};

	[[nodiscard]] inline constexpr BattleTime battleTimeZero() noexcept
	{
		return BattleTime{};
	}

	// Clamp is an explicit free function so no call site can mistake a saturating result for
	// checked arithmetic. Throws std::invalid_argument when p_minimum > p_maximum.
	[[nodiscard]] BattleTime clamp(const BattleTime &p_value, const BattleTime &p_minimum, const BattleTime &p_maximum);

	// Display only - never feed the result back into simulation state. Returns 0.0 for a
	// zero total instead of dividing by zero.
	[[nodiscard]] double ratioForDisplay(const BattleTime &p_value, const BattleTime &p_total) noexcept;

	// Sign policy a parser applies on top of the shared representation rule: stamina and
	// durations are Positive, optional delays are NonNegative, effect deltas are Any.
	enum class BattleTimeSign
	{
		Positive,
		NonNegative,
		Any
	};

	// Reads an authored seconds value and rethrows every conversion failure as a JsonError
	// carrying the source file and the exact JSON path, so callers never rebuild the path.
	[[nodiscard]] BattleTime requireBattleSeconds(JsonReader &p_reader, const std::string &p_key, BattleTimeSign p_sign);
}
