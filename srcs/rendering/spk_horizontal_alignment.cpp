#include "rendering/spk_horizontal_alignment.hpp"

namespace spk
{
	std::string toString(const HorizontalAlignment& p_alignment)
	{
		switch (p_alignment)
		{
		case HorizontalAlignment::Left:
			return "Left";
		case HorizontalAlignment::Centered:
			return "Centered";
		case HorizontalAlignment::Right:
			return "Right";
		}
		return "Unknown HorizontalAlignment";
	}

	std::wstring toWstring(const HorizontalAlignment& p_alignment)
	{
		switch (p_alignment)
		{
		case HorizontalAlignment::Left:
			return L"Left";
		case HorizontalAlignment::Centered:
			return L"Centered";
		case HorizontalAlignment::Right:
			return L"Right";
		}
		return L"Unknown HorizontalAlignment";
	}

	std::ostream& operator<<(std::ostream& p_stream, const HorizontalAlignment& p_alignment)
	{
		p_stream << toString(p_alignment);
		return p_stream;
	}

	std::wostream& operator<<(std::wostream& p_stream, const HorizontalAlignment& p_alignment)
	{
		p_stream << toWstring(p_alignment);
		return p_stream;
	}
}
