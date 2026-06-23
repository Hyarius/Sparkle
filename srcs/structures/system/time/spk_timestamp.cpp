#include "structures/system/time/spk_timestamp.hpp"

#include <chrono>
#include <limits>
#include <ostream>
#include <sstream>
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
				throw std::out_of_range("spk::Timestamp: value out of range");
			}

			return static_cast<long long>(p_value);
		}
	}

	long long Timestamp::_toNanoseconds(const long double p_value, const TimeUnit p_unit)
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
			throw std::invalid_argument("spk::Timestamp: unknown TimeUnit");
		}
	}

	void Timestamp::_rebindCaches()
	{
		_millisecondsCache.configure(
			[this]() {
				return _nanoseconds / 1'000'000LL;
			});

		_secondsCache.configure(
			[this]() {
				return static_cast<double>(_nanoseconds) / 1'000'000'000.0;
			});
	}

	Timestamp::Timestamp()
	{
		_nanoseconds = std::chrono::duration_cast<std::chrono::nanoseconds>(
						   std::chrono::steady_clock::now().time_since_epoch())
						   .count();

		_rebindCaches();
	}

	Timestamp::Timestamp(const long double p_value, const TimeUnit p_unit) :
		_nanoseconds(_toNanoseconds(p_value, p_unit))
	{
		_rebindCaches();
	}

	Timestamp::Timestamp(const Duration &p_duration) :
		_nanoseconds(p_duration.nanoseconds())
	{
		_rebindCaches();
	}

	Timestamp::Timestamp(const Timestamp &p_other) :
		_nanoseconds(p_other._nanoseconds)
	{
		_rebindCaches();
	}

	Timestamp::Timestamp(Timestamp &&p_other) noexcept :
		_nanoseconds(p_other._nanoseconds)
	{
		_rebindCaches();
	}

	Timestamp &Timestamp::operator=(const Timestamp &p_other)
	{
		if (this != &p_other)
		{
			_nanoseconds = p_other._nanoseconds;
			_rebindCaches();
		}

		return *this;
	}

	Timestamp &Timestamp::operator=(Timestamp &&p_other) noexcept
	{
		if (this != &p_other)
		{
			_nanoseconds = p_other._nanoseconds;
			_rebindCaches();
		}

		return *this;
	}

	double Timestamp::seconds() const
	{
		return _secondsCache.get();
	}

	long long Timestamp::milliseconds() const
	{
		return _millisecondsCache.get();
	}

	long long Timestamp::nanoseconds() const noexcept
	{
		return _nanoseconds;
	}

	Duration Timestamp::operator-(const Timestamp &p_other) const
	{
		return Duration(static_cast<long double>(_nanoseconds - p_other._nanoseconds), TimeUnit::Nanosecond);
	}

	Timestamp Timestamp::operator+(const Duration &p_other) const noexcept
	{
		Timestamp result(*this);
		result._nanoseconds += p_other.nanoseconds();
		result._rebindCaches();
		return result;
	}

	Timestamp Timestamp::operator-(const Duration &p_other) const noexcept
	{
		Timestamp result(*this);
		result._nanoseconds -= p_other.nanoseconds();
		result._rebindCaches();
		return result;
	}

	Timestamp &Timestamp::operator+=(const Duration &p_other) noexcept
	{
		_nanoseconds += p_other.nanoseconds();
		_rebindCaches();
		return *this;
	}

	Timestamp &Timestamp::operator-=(const Duration &p_other) noexcept
	{
		_nanoseconds -= p_other.nanoseconds();
		_rebindCaches();
		return *this;
	}

	bool Timestamp::operator==(const Timestamp &p_other) const noexcept
	{
		return _nanoseconds == p_other._nanoseconds;
	}

	bool Timestamp::operator!=(const Timestamp &p_other) const noexcept
	{
		return !(*this == p_other);
	}

	bool Timestamp::operator<(const Timestamp &p_other) const noexcept
	{
		return _nanoseconds < p_other._nanoseconds;
	}

	bool Timestamp::operator>(const Timestamp &p_other) const noexcept
	{
		return _nanoseconds > p_other._nanoseconds;
	}

	bool Timestamp::operator<=(const Timestamp &p_other) const noexcept
	{
		return _nanoseconds <= p_other._nanoseconds;
	}

	bool Timestamp::operator>=(const Timestamp &p_other) const noexcept
	{
		return _nanoseconds >= p_other._nanoseconds;
	}

	std::string Timestamp::toString(const TimeUnit p_unit) const
	{
		std::ostringstream outputStream;

		switch (p_unit)
		{
		case TimeUnit::Second:
			outputStream << seconds() << "s";
			break;

		case TimeUnit::Millisecond:
			outputStream << milliseconds() << "ms";
			break;

		case TimeUnit::Nanosecond:
			outputStream << nanoseconds() << "ns";
			break;

		default:
			throw std::invalid_argument("spk::Timestamp: unknown TimeUnit");
		}

		return outputStream.str();
	}

	std::wstring Timestamp::toWstring(const TimeUnit p_unit) const
	{
		std::wostringstream outputStream;

		switch (p_unit)
		{
		case TimeUnit::Second:
			outputStream << seconds() << L"s";
			break;

		case TimeUnit::Millisecond:
			outputStream << milliseconds() << L"ms";
			break;

		case TimeUnit::Nanosecond:
			outputStream << nanoseconds() << L"ns";
			break;

		default:
			throw std::invalid_argument("spk::Timestamp: unknown TimeUnit");
		}

		return outputStream.str();
	}

	std::ostream &operator<<(std::ostream &p_stream, const Timestamp &p_timestamp)
	{
		p_stream << p_timestamp.toString();
		return p_stream;
	}

	std::wostream &operator<<(std::wostream &p_stream, const Timestamp &p_timestamp)
	{
		p_stream << p_timestamp.toWstring();
		return p_stream;
	}
}
