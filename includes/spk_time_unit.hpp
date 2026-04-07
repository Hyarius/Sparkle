#pragma once

#include <ostream>
#include <string>

namespace spk
{
	enum class TimeUnit
	{
		Second,
		Millisecond,
		Nanosecond
	};

	std::string toString(const TimeUnit &p_timeUnit);
	std::wstring toWstring(const TimeUnit &p_timeUnit);

	std::ostream &operator<<(std::ostream &p_stream, const TimeUnit &p_timeUnit);
	std::wostream &operator<<(std::wostream &p_stream, const TimeUnit &p_timeUnit);
}