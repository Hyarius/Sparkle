#pragma once

#include <ostream>
#include <string>

namespace spk
{
	enum class HorizontalAlignment
	{
		Left,
		Centered,
		Right
	};

	std::string toString(const HorizontalAlignment &p_alignment);
	std::wstring toWstring(const HorizontalAlignment &p_alignment);

	std::ostream &operator<<(std::ostream &p_stream, const HorizontalAlignment &p_alignment);
	std::wostream &operator<<(std::wostream &p_stream, const HorizontalAlignment &p_alignment);
}
