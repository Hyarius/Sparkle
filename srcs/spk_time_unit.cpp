#include "spk_time_unit.hpp"

namespace spk
{
	std::string toString(const TimeUnit &p_timeUnit)
	{
		switch (p_timeUnit)
		{
		case TimeUnit::Second:
			return ("Second");
		case TimeUnit::Millisecond:
			return ("Millisecond");
		case TimeUnit::Nanosecond:
			return ("Nanosecond");
		}
		return ("Unknown TimeUnit");
	}

	std::wstring toWstring(const TimeUnit &p_timeUnit)
	{
		switch (p_timeUnit)
		{
		case TimeUnit::Second:
			return (L"Second");
		case TimeUnit::Millisecond:
			return (L"Millisecond");
		case TimeUnit::Nanosecond:
			return (L"Nanosecond");
		}
		return (L"Unknown TimeUnit");
	}

	std::ostream &operator<<(std::ostream &p_stream, const TimeUnit &p_timeUnit)
	{
		p_stream << toString(p_timeUnit);
		return p_stream;
	}

	std::wostream &operator<<(std::wostream &p_stream, const TimeUnit &p_timeUnit)
	{
		p_stream << toWstring(p_timeUnit);
		return p_stream;
	}
}