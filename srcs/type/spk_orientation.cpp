#include "type/spk_orientation.hpp"

namespace spk
{
	std::string toString(const Orientation& p_orientation)
	{
		switch (p_orientation)
		{
		case Orientation::Horizontal:
			return "Horizontal";
		case Orientation::Vertical:
			return "Vertical";
		}
		return "Unknown Orientation";
	}

	std::wstring toWstring(const Orientation& p_orientation)
	{
		switch (p_orientation)
		{
		case Orientation::Horizontal:
			return L"Horizontal";
		case Orientation::Vertical:
			return L"Vertical";
		}
		return L"Unknown Orientation";
	}

	std::ostream& operator<<(std::ostream& p_stream, const Orientation& p_orientation)
	{
		p_stream << toString(p_orientation);
		return p_stream;
	}

	std::wostream& operator<<(std::wostream& p_stream, const Orientation& p_orientation)
	{
		p_stream << toWstring(p_orientation);
		return p_stream;
	}
}
