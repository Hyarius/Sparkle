#include "type/spk_vertical_alignment.hpp"

namespace spk
{
	std::string toString(const VerticalAlignment& p_alignment)
	{
		switch (p_alignment)
		{
		case VerticalAlignment::Top:
			return "Top";
		case VerticalAlignment::Centered:
			return "Centered";
		case VerticalAlignment::Down:
			return "Down";
		}
		return "Unknown VerticalAlignment";
	}

	std::wstring toWstring(const VerticalAlignment& p_alignment)
	{
		switch (p_alignment)
		{
		case VerticalAlignment::Top:
			return L"Top";
		case VerticalAlignment::Centered:
			return L"Centered";
		case VerticalAlignment::Down:
			return L"Down";
		}
		return L"Unknown VerticalAlignment";
	}

	std::ostream& operator<<(std::ostream& p_stream, const VerticalAlignment& p_alignment)
	{
		p_stream << toString(p_alignment);
		return p_stream;
	}

	std::wostream& operator<<(std::wostream& p_stream, const VerticalAlignment& p_alignment)
	{
		p_stream << toWstring(p_alignment);
		return p_stream;
	}
}
