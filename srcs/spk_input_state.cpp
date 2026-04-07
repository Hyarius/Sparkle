#include "spk_input_state.hpp"

namespace spk
{
	std::string toString(const InputState &p_inputState)
	{
		switch (p_inputState)
		{
		case InputState::Up:
			return "Up";
		case InputState::Down:
			return "Down";
		default:
			return "Unknow InputState";
		}
	}

	std::wstring toWstring(const InputState &p_inputState)
	{
		switch (p_inputState)
		{
		case InputState::Up:
			return L"Up";
		case InputState::Down:
			return L"Down";
		default:
			return L"Unknow InputState";
		}
	}

	std::ostream &operator<<(std::ostream &p_stream, const InputState &p_inputState)
	{
		p_stream << toString(p_inputState);
		return p_stream;
	}

	std::wostream &operator<<(std::wostream &p_stream, const InputState &p_inputState)
	{
		p_stream << toWstring(p_inputState);
		return p_stream;
	}
}