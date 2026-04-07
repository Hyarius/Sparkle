#pragma once

#include <cstdint>

#include "spk_cached_data.hpp"
#include "spk_duration.hpp"
#include "spk_time_unit.hpp"

namespace spk
{
	class Timestamp
	{
	private:
		long long _nanoseconds = 0;

		mutable CachedData<long long> _millisecondsCache;
		mutable CachedData<double> _secondsCache;

		static long long _toNanoseconds(long double p_value, TimeUnit p_unit);
		void _rebindCaches();

	public:
		Timestamp();
		Timestamp(long double p_value, TimeUnit p_unit);
		Timestamp(const Duration& p_duration);

		Timestamp(const Timestamp& p_other);
		Timestamp(Timestamp&& p_other) noexcept;

		Timestamp& operator=(const Timestamp& p_other);
		Timestamp& operator=(Timestamp&& p_other) noexcept;

		double seconds() const;
		long long milliseconds() const;
		long long nanoseconds() const noexcept;

		Duration operator-(const Timestamp& p_other) const;
		Timestamp operator+(const Duration& p_other) const noexcept;
		Timestamp operator-(const Duration& p_other) const noexcept;

		Timestamp& operator+=(const Duration& p_other) noexcept;
		Timestamp& operator-=(const Duration& p_other) noexcept;

		bool operator==(const Timestamp& p_other) const noexcept;
		bool operator!=(const Timestamp& p_other) const noexcept;
		bool operator<(const Timestamp& p_other) const noexcept;
		bool operator>(const Timestamp& p_other) const noexcept;
		bool operator<=(const Timestamp& p_other) const noexcept;
		bool operator>=(const Timestamp& p_other) const noexcept;
	};
}