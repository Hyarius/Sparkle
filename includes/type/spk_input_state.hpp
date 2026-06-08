#pragma once

#include <ostream>
#include <string>

namespace spk
{
	enum class InputState
	{
		Down,
		Up
	};

	std::string toString(const InputState &p_inputState);
	std::wstring toWstring(const InputState &p_inputState);

	std::ostream &operator<<(std::ostream &p_stream, const InputState &p_inputState);
	std::wostream &operator<<(std::wostream &p_stream, const InputState &p_inputState);
}