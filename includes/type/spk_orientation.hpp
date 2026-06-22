#pragma once

#include <ostream>
#include <string>

namespace spk
{
	enum class Orientation
	{
		Horizontal,
		Vertical
	};

	std::string toString(const Orientation &p_orientation);
	std::wstring toWstring(const Orientation &p_orientation);

	std::ostream &operator<<(std::ostream &p_stream, const Orientation &p_orientation);
	std::wostream &operator<<(std::wostream &p_stream, const Orientation &p_orientation);
}
