#pragma once

#include <ostream>
#include <string>

namespace spk
{
	enum class VerticalAlignment
	{
		Top,
		Centered,
		Down
	};

	std::string toString(const VerticalAlignment &p_alignment);
	std::wstring toWstring(const VerticalAlignment &p_alignment);

	std::ostream &operator<<(std::ostream &p_stream, const VerticalAlignment &p_alignment);
	std::wostream &operator<<(std::wostream &p_stream, const VerticalAlignment &p_alignment);
}
