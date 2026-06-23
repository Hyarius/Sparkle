#pragma once

#include <chrono>
#include <cstdint>
#include <iosfwd>
#include <string>

#include "structures/container/spk_cached_data.hpp"
#include "type/spk_time_unit.hpp"

namespace spk
{
	class Duration
	{
	private:
		long long _nanoseconds = 0;

		mutable CachedData<long long> _millisecondsCache;
		mutable CachedData<double> _secondsCache;

		static long long _toNanoseconds(long double p_value, TimeUnit p_unit);
		void _rebindCaches();

	public:
		Duration();
		Duration(long double p_value, TimeUnit p_unit);
		template <typename TRep, typename TPeriod>
		explicit Duration(const std::chrono::duration<TRep, TPeriod> &p_duration) :
			_nanoseconds(std::chrono::duration_cast<std::chrono::nanoseconds>(p_duration).count())
		{
			_rebindCaches();
		}

		Duration(const Duration &p_other);
		Duration(Duration &&p_other) noexcept;

		Duration &operator=(const Duration &p_other);
		Duration &operator=(Duration &&p_other) noexcept;

		long long nanoseconds() const noexcept;
		long long milliseconds() const;
		double seconds() const;
		std::chrono::nanoseconds toChronoNanoseconds() const noexcept;
		template <typename TDuration>
		[[nodiscard]] TDuration toChrono() const noexcept
		{
			return std::chrono::duration_cast<TDuration>(toChronoNanoseconds());
		}

		Duration operator-() const noexcept;

		Duration operator+(const Duration &p_other) const noexcept;
		Duration operator-(const Duration &p_other) const noexcept;

		Duration &operator+=(const Duration &p_other) noexcept;
		Duration &operator-=(const Duration &p_other) noexcept;

		bool operator==(const Duration &p_other) const noexcept;
		bool operator!=(const Duration &p_other) const noexcept;
		bool operator<(const Duration &p_other) const noexcept;
		bool operator>(const Duration &p_other) const noexcept;
		bool operator<=(const Duration &p_other) const noexcept;
		bool operator>=(const Duration &p_other) const noexcept;

		[[nodiscard]] std::string toString(TimeUnit p_unit = TimeUnit::Nanosecond) const;
		[[nodiscard]] std::wstring toWstring(TimeUnit p_unit = TimeUnit::Nanosecond) const;

		friend std::ostream &operator<<(std::ostream &p_stream, const Duration &p_duration);
		friend std::wostream &operator<<(std::wostream &p_stream, const Duration &p_duration);
	};

	Duration operator""_s(long double p_value);
	Duration operator""_s(unsigned long long p_value);
	Duration operator""_ms(unsigned long long p_value);
	Duration operator""_ns(unsigned long long p_value);
}
