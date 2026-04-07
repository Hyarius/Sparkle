#include "spk_duration.hpp"

#include <limits>
#include <stdexcept>

namespace spk
{
	namespace
	{
		long long clampLongDoubleToLongLong(const long double p_value)
		{
			const long double minimumValue = static_cast<long double>(std::numeric_limits<long long>::min());
			const long double maximumValue = static_cast<long double>(std::numeric_limits<long long>::max());

			if (p_value < minimumValue || p_value > maximumValue)
			{
				throw std::out_of_range("spk::Duration: value out of range");
			}

			return static_cast<long long>(p_value);
		}
	}

	long long Duration::_toNanoseconds(const long double p_value, const TimeUnit p_unit)
	{
		switch (p_unit)
		{
		case TimeUnit::Second:
			return clampLongDoubleToLongLong(p_value * 1'000'000'000.0L);

		case TimeUnit::Millisecond:
			return clampLongDoubleToLongLong(p_value * 1'000'000.0L);

		case TimeUnit::Nanosecond:
			return clampLongDoubleToLongLong(p_value);

		default:
			throw std::invalid_argument("spk::Duration: unknown TimeUnit");
		}
	}

	void Duration::_rebindCaches()
	{
		_millisecondsCache.configure(
			[this]()
			{
				return _nanoseconds / 1'000'000LL;
			});

		_secondsCache.configure(
			[this]()
			{
				return static_cast<double>(_nanoseconds) / 1'000'000'000.0;
			});
	}

	Duration::Duration()
	{
		_rebindCaches();
	}

	Duration::Duration(const long double p_value, const TimeUnit p_unit) :
		_nanoseconds(_toNanoseconds(p_value, p_unit))
	{
		_rebindCaches();
	}

	Duration::Duration(const Duration& p_other) :
		_nanoseconds(p_other._nanoseconds)
	{
		_rebindCaches();
	}

	Duration::Duration(Duration&& p_other) noexcept :
		_nanoseconds(p_other._nanoseconds)
	{
		_rebindCaches();
	}

	Duration& Duration::operator=(const Duration& p_other)
	{
		if (this != &p_other)
		{
			_nanoseconds = p_other._nanoseconds;
			_rebindCaches();
		}

		return *this;
	}

	Duration& Duration::operator=(Duration&& p_other) noexcept
	{
		if (this != &p_other)
		{
			_nanoseconds = p_other._nanoseconds;
			_rebindCaches();
		}

		return *this;
	}

	long long Duration::nanoseconds() const noexcept
	{
		return _nanoseconds;
	}

	long long Duration::milliseconds() const
	{
		return _millisecondsCache.get();
	}

	double Duration::seconds() const
	{
		return _secondsCache.get();
	}

	Duration Duration::operator-() const noexcept
	{
		Duration result;
		result._nanoseconds = -_nanoseconds;
		result._rebindCaches();
		return result;
	}

	Duration Duration::operator+(const Duration& p_other) const noexcept
	{
		Duration result;
		result._nanoseconds = _nanoseconds + p_other._nanoseconds;
		result._rebindCaches();
		return result;
	}

	Duration Duration::operator-(const Duration& p_other) const noexcept
	{
		Duration result;
		result._nanoseconds = _nanoseconds - p_other._nanoseconds;
		result._rebindCaches();
		return result;
	}

	Duration& Duration::operator+=(const Duration& p_other) noexcept
	{
		_nanoseconds += p_other._nanoseconds;
		_rebindCaches();
		return *this;
	}

	Duration& Duration::operator-=(const Duration& p_other) noexcept
	{
		_nanoseconds -= p_other._nanoseconds;
		_rebindCaches();
		return *this;
	}

	bool Duration::operator==(const Duration& p_other) const noexcept
	{
		return _nanoseconds == p_other._nanoseconds;
	}

	bool Duration::operator!=(const Duration& p_other) const noexcept
	{
		return !(*this == p_other);
	}

	bool Duration::operator<(const Duration& p_other) const noexcept
	{
		return _nanoseconds < p_other._nanoseconds;
	}

	bool Duration::operator>(const Duration& p_other) const noexcept
	{
		return _nanoseconds > p_other._nanoseconds;
	}

	bool Duration::operator<=(const Duration& p_other) const noexcept
	{
		return _nanoseconds <= p_other._nanoseconds;
	}

	bool Duration::operator>=(const Duration& p_other) const noexcept
	{
		return _nanoseconds >= p_other._nanoseconds;
	}

	Duration operator""_s(const long double p_value)
	{
		return Duration(p_value, TimeUnit::Second);
	}

	Duration operator""_s(const unsigned long long p_value)
	{
		return Duration(static_cast<long double>(p_value), TimeUnit::Second);
	}

	Duration operator""_ms(const unsigned long long p_value)
	{
		return Duration(static_cast<long double>(p_value), TimeUnit::Millisecond);
	}

	Duration operator""_ns(const unsigned long long p_value)
	{
		return Duration(static_cast<long double>(p_value), TimeUnit::Nanosecond);
	}
}