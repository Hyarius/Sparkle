#include "spk_mouse.hpp"

namespace spk
{
	std::string toString(const Mouse::Button &p_button)
	{
		switch (p_button)
		{
		case Mouse::Right:
			return "Right";
		case Mouse::Middle:
			return "Middle";
		case Mouse::Left:
			return "Left";
		default:
			return ("Unknow mouse button");
		}
	}

	std::wstring toWstring(const Mouse::Button &p_button)
	{
		switch (p_button)
		{
		case Mouse::Right:
			return L"Right";
		case Mouse::Middle:
			return L"Middle";
		case Mouse::Left:
			return L"Left";
		default:
			return (L"Unknow mouse button");
		}
	}

	std::ostream &operator<<(std::ostream &p_stream, const Mouse::Button &p_button)
	{
		p_stream << toString(p_button);
		return p_stream;
	}

	std::wostream &operator<<(std::wostream &p_stream, const Mouse::Button &p_button)
	{
		p_stream << toWstring(p_button);
		return p_stream;
	}
}