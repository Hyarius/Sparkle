#pragma once

#include <cstdint>

#include "spk_cached_data.hpp"
#include "spk_time_unit.hpp"

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

		Duration(const Duration& p_other);
		Duration(Duration&& p_other) noexcept;

		Duration& operator=(const Duration& p_other);
		Duration& operator=(Duration&& p_other) noexcept;

		long long nanoseconds() const noexcept;
		long long milliseconds() const;
		double seconds() const;

		Duration operator-() const noexcept;

		Duration operator+(const Duration& p_other) const noexcept;
		Duration operator-(const Duration& p_other) const noexcept;

		Duration& operator+=(const Duration& p_other) noexcept;
		Duration& operator-=(const Duration& p_other) noexcept;

		bool operator==(const Duration& p_other) const noexcept;
		bool operator!=(const Duration& p_other) const noexcept;
		bool operator<(const Duration& p_other) const noexcept;
		bool operator>(const Duration& p_other) const noexcept;
		bool operator<=(const Duration& p_other) const noexcept;
		bool operator>=(const Duration& p_other) const noexcept;
	};

	Duration operator""_s(long double p_value);
	Duration operator""_s(unsigned long long p_value);
	Duration operator""_ms(unsigned long long p_value);
	Duration operator""_ns(unsigned long long p_value);
}