#pragma once

#include <array>
#include <ostream>
#include <string>

#include "spk_input_state.hpp"

namespace spk
{
	struct Keyboard
	{
		enum Key
		{
			Backspace = 8,
			Tab = 9,
			Clear = 12,
			Return = 13,
			Shift = 16,
			Control = 17,
			Alt = 18,
			Pause = 19,
			Capslock = 20,
			Escape = 27,
			Convert = 28,
			Non_convert = 29,
			Accept = 30,
			Mode_change = 31,
			Space = 32,
			Prior = 33,
			Next = 34,
			End = 35,
			Home = 36,
			LeftArrow = 37,
			UpArrow = 38,
			RightArrow = 39,
			DownArrow = 40,
			Select = 41,
			Print = 42,
			Execute = 43,
			Snapshot = 44,
			Insert = 45,
			Delete = 46,
			Help = 47,
			Key0 = 48,
			Key1 = 49,
			Key2 = 50,
			Key3 = 51,
			Key4 = 52,
			Key5 = 53,
			Key6 = 54,
			Key7 = 55,
			Key8 = 56,
			Key9 = 57,
			A = 65,
			B = 66,
			C = 67,
			D = 68,
			E = 69,
			F = 70,
			G = 71,
			H = 72,
			I = 73,
			J = 74,
			K = 75,
			L = 76,
			M = 77,
			N = 78,
			O = 79,
			P = 80,
			Q = 81,
			R = 82,
			S = 83,
			T = 84,
			U = 85,
			V = 86,
			W = 87,
			X = 88,
			Y = 89,
			Z = 90,
			LeftWindows = 91,
			RightWindows = 92,
			App = 93,
			Sleep = 95,
			Numpad0 = 96,
			Numpad1 = 97,
			Numpad2 = 98,
			Numpad3 = 99,
			Numpad4 = 100,
			Numpad5 = 101,
			Numpad6 = 102,
			Numpad7 = 103,
			Numpad8 = 104,
			Numpad9 = 105,
			NumpadMultiply = 106,
			NumpadPlus = 107,
			NumpadSeparator = 108,
			NumpadMinus = 109,
			NumpadDecimal = 110,
			NumpadDivide = 111,
			F1 = 112,
			F2 = 113,
			F3 = 114,
			F4 = 115,
			F5 = 116,
			F6 = 117,
			F7 = 118,
			F8 = 119,
			F9 = 120,
			F10 = 121,
			F11 = 122,
			F12 = 123,
			F13 = 124,
			F14 = 125,
			F15 = 126,
			F16 = 127,
			F17 = 128,
			F18 = 129,
			F19 = 130,
			F20 = 131,
			F21 = 132,
			F22 = 133,
			F23 = 134,
			F24 = 135,
			NumLock = 144,
			Scroll = 145,
			LeftShift = 160,
			RightShift = 161,
			LeftControl = 162,
			RightControl = 163,
			LeftMenu = 164,
			RightMenu = 165,
			SemiColon = 186,
			Plus = 187,
			Comma = 188,
			Minus = 189,
			Period = 190,
			QuestionMark = 191,
			Tilde = 192,
			LeftBracket = 219,
			VerticalLine = 220,
			RightBracket = 221,
			Quote = 222,
			Unknown = 223,
			AngleBracket = 226,
			Process = 229
		};

		static constexpr size_t NbKey = 255;

		std::array<InputState, NbKey> keys;
		char32_t glyph = U'\0';

		Keyboard()
		{
			for (auto &state : keys)
			{
				state = InputState::Up;
			}

			glyph = U'\0';
		}

		InputState &operator[](const Key p_key)
		{
			return keys[static_cast<size_t>(p_key)];
		}

		const InputState &operator[](const Key p_key) const
		{
			return keys[static_cast<size_t>(p_key)];
		}
	};

	std::string toString(const Keyboard::Key &p_key);
	std::wstring toWstring(const Keyboard::Key &p_key);

	std::ostream &operator<<(std::ostream &p_stream, const Keyboard::Key &p_key);
	std::wostream &operator<<(std::wostream &p_stream, const Keyboard::Key &p_key);

}