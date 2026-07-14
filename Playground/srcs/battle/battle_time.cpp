#include "battle/battle_time.hpp"

#include <cmath>
#include <limits>
#include <stdexcept>

namespace pg
{
	namespace
	{
		using Rep = BattleTime::Rep;

		constexpr Rep RepMinimum = std::numeric_limits<Rep>::min();
		constexpr Rep RepMaximum = std::numeric_limits<Rep>::max();

		// int64 has no portable overflow builtin across the compilers this repository is
		// built with, so every operation is guarded before it is performed.
		[[nodiscard]] Rep checkedAdd(Rep p_left, Rep p_right, const char *p_operation)
		{
			if ((p_right > 0 && p_left > RepMaximum - p_right) || (p_right < 0 && p_left < RepMinimum - p_right))
			{
				throw std::overflow_error(std::string("BattleTime ") + p_operation + " overflowed the tick range");
			}
			return p_left + p_right;
		}

		[[nodiscard]] Rep checkedSubtract(Rep p_left, Rep p_right, const char *p_operation)
		{
			if ((p_right < 0 && p_left > RepMaximum + p_right) || (p_right > 0 && p_left < RepMinimum + p_right))
			{
				throw std::overflow_error(std::string("BattleTime ") + p_operation + " overflowed the tick range");
			}
			return p_left - p_right;
		}

		[[nodiscard]] Rep checkedMultiply(Rep p_left, Rep p_right, const char *p_operation)
		{
			const auto fail = [p_operation]() {
				throw std::overflow_error(std::string("BattleTime ") + p_operation + " overflowed the tick range");
			};

			if (p_left == 0 || p_right == 0)
			{
				return 0;
			}
			// -1 is the one factor whose negation can leave the range, and it escapes the
			// division tests below because RepMinimum / -1 already overflows.
			if (p_left == -1)
			{
				if (p_right == RepMinimum)
				{
					fail();
				}
				return -p_right;
			}
			if (p_right == -1)
			{
				if (p_left == RepMinimum)
				{
					fail();
				}
				return -p_left;
			}

			if (p_left > 0)
			{
				if (p_right > 0 ? (p_left > RepMaximum / p_right) : (p_right < RepMinimum / p_left))
				{
					fail();
				}
			}
			else
			{
				if (p_right > 0 ? (p_left < RepMinimum / p_right) : (p_left < RepMaximum / p_right))
				{
					fail();
				}
			}
			return p_left * p_right;
		}
	}

	BattleTime BattleTime::fromSecondsExact(double p_seconds)
	{
		if (!std::isfinite(p_seconds))
		{
			throw std::invalid_argument("battle seconds must be a finite number");
		}

		const double scaled = p_seconds * static_cast<double>(TicksPerSecond);
		const double rounded = std::nearbyint(scaled);
		if (std::abs(scaled - rounded) > SecondsTolerance)
		{
			throw std::invalid_argument(
				"battle seconds must be a whole number of milliseconds (a multiple of 0.001)");
		}

		// -2^63 is exactly representable as a double; 2^63 is the first value above the tick
		// range, so a strict compare against it keeps the cast defined.
		constexpr double LowerBound = -9223372036854775808.0;
		constexpr double UpperBound = 9223372036854775808.0;
		if (rounded < LowerBound || rounded >= UpperBound)
		{
			throw std::overflow_error("battle seconds are outside the representable tick range");
		}

		return fromTicks(static_cast<Rep>(rounded));
	}

	double BattleTime::secondsForDisplay() const noexcept
	{
		return static_cast<double>(_ticks) / static_cast<double>(TicksPerSecond);
	}

	BattleTime BattleTime::operator+(const BattleTime &p_other) const
	{
		return fromTicks(checkedAdd(_ticks, p_other._ticks, "addition"));
	}

	BattleTime BattleTime::operator-(const BattleTime &p_other) const
	{
		return fromTicks(checkedSubtract(_ticks, p_other._ticks, "subtraction"));
	}

	BattleTime BattleTime::operator-() const
	{
		return fromTicks(checkedSubtract(0, _ticks, "negation"));
	}

	BattleTime BattleTime::operator*(Rep p_factor) const
	{
		return fromTicks(checkedMultiply(_ticks, p_factor, "multiplication"));
	}

	BattleTime &BattleTime::operator+=(const BattleTime &p_other)
	{
		_ticks = checkedAdd(_ticks, p_other._ticks, "addition");
		return *this;
	}

	BattleTime &BattleTime::operator-=(const BattleTime &p_other)
	{
		_ticks = checkedSubtract(_ticks, p_other._ticks, "subtraction");
		return *this;
	}

	BattleTime &BattleTime::operator*=(Rep p_factor)
	{
		_ticks = checkedMultiply(_ticks, p_factor, "multiplication");
		return *this;
	}

	BattleTime clamp(const BattleTime &p_value, const BattleTime &p_minimum, const BattleTime &p_maximum)
	{
		if (p_minimum > p_maximum)
		{
			throw std::invalid_argument("BattleTime clamp received an inverted range");
		}
		if (p_value < p_minimum)
		{
			return p_minimum;
		}
		if (p_value > p_maximum)
		{
			return p_maximum;
		}
		return p_value;
	}

	double ratioForDisplay(const BattleTime &p_value, const BattleTime &p_total) noexcept
	{
		if (p_total.ticks() == 0)
		{
			return 0.0;
		}
		return static_cast<double>(p_value.ticks()) / static_cast<double>(p_total.ticks());
	}

	BattleTime requireBattleSeconds(JsonReader &p_reader, const std::string &p_key, BattleTimeSign p_sign)
	{
		const double seconds = p_reader.require<double>(p_key);

		BattleTime result;
		try
		{
			result = BattleTime::fromSecondsExact(seconds);
		} catch (const std::exception &exception)
		{
			throw JsonError(p_reader.file(), p_reader.pathFor(p_key), exception.what());
		}

		if (p_sign == BattleTimeSign::Positive && result.ticks() <= 0)
		{
			throw JsonError(p_reader.file(), p_reader.pathFor(p_key), "expected a duration greater than zero");
		}
		if (p_sign == BattleTimeSign::NonNegative && result.ticks() < 0)
		{
			throw JsonError(p_reader.file(), p_reader.pathFor(p_key), "expected a duration of zero or more");
		}

		return result;
	}
}
