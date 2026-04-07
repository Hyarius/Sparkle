#pragma once

#include <array>
#include <ostream>
#include <string>

#include "spk_vector2.hpp"
#include "spk_input_state.hpp"

namespace spk
{
	struct Mouse
	{
		enum Button
		{
			Right,
			Middle,
			Left
		};
		static inline const size_t NbButton = 3;

		std::array<InputState, NbButton> buttons;
		spk::Vector2Int position = {0, 0};
		spk::Vector2Int deltaPosition = {0, 0};
		float wheel = 0;

		Mouse()
		{
			for (auto &state : buttons)
			{
				state = InputState::Up;
			}
		}

		InputState &operator[](const Button p_button)
		{
			return buttons[static_cast<size_t>(p_button)];
		}

		const InputState &operator[](const Button p_button) const
		{
			return buttons[static_cast<size_t>(p_button)];
		}
	};

	std::string toString(const Mouse::Button &p_button);
	std::wstring toWstring(const Mouse::Button &p_button);

	std::ostream &operator<<(std::ostream &p_stream, const Mouse::Button &p_button);
	std::wostream &operator<<(std::wostream &p_stream, const Mouse::Button &p_button);
}