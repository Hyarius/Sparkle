#include "input/spk_keyboard.hpp"

#include <array>
#include <string_view>
#include <utility>

namespace
{
	using KeyName = std::pair<spk::Keyboard::Key, std::string_view>;

	constexpr auto keyboardKeyNames = std::to_array<KeyName>({
		{spk::Keyboard::Backspace, "Backspace"},
		{spk::Keyboard::Tab, "Tab"},
		{spk::Keyboard::Clear, "Clear"},
		{spk::Keyboard::Return, "Return"},
		{spk::Keyboard::Shift, "Shift"},
		{spk::Keyboard::Control, "Control"},
		{spk::Keyboard::Alt, "Alt"},
		{spk::Keyboard::Pause, "Pause"},
		{spk::Keyboard::Capslock, "Capslock"},
		{spk::Keyboard::Escape, "Escape"},
		{spk::Keyboard::Convert, "Convert"},
		{spk::Keyboard::Non_convert, "Non_convert"},
		{spk::Keyboard::Accept, "Accept"},
		{spk::Keyboard::Mode_change, "Mode_change"},
		{spk::Keyboard::Space, "Space"},
		{spk::Keyboard::Prior, "Prior"},
		{spk::Keyboard::Next, "Next"},
		{spk::Keyboard::End, "End"},
		{spk::Keyboard::Home, "Home"},
		{spk::Keyboard::LeftArrow, "LeftArrow"},
		{spk::Keyboard::UpArrow, "UpArrow"},
		{spk::Keyboard::RightArrow, "RightArrow"},
		{spk::Keyboard::DownArrow, "DownArrow"},
		{spk::Keyboard::Select, "Select"},
		{spk::Keyboard::Print, "Print"},
		{spk::Keyboard::Execute, "Execute"},
		{spk::Keyboard::Snapshot, "Snapshot"},
		{spk::Keyboard::Insert, "Insert"},
		{spk::Keyboard::Delete, "Delete"},
		{spk::Keyboard::Help, "Help"},
		{spk::Keyboard::Key0, "Key0"},
		{spk::Keyboard::Key1, "Key1"},
		{spk::Keyboard::Key2, "Key2"},
		{spk::Keyboard::Key3, "Key3"},
		{spk::Keyboard::Key4, "Key4"},
		{spk::Keyboard::Key5, "Key5"},
		{spk::Keyboard::Key6, "Key6"},
		{spk::Keyboard::Key7, "Key7"},
		{spk::Keyboard::Key8, "Key8"},
		{spk::Keyboard::Key9, "Key9"},
		{spk::Keyboard::A, "A"},
		{spk::Keyboard::B, "B"},
		{spk::Keyboard::C, "C"},
		{spk::Keyboard::D, "D"},
		{spk::Keyboard::E, "E"},
		{spk::Keyboard::F, "F"},
		{spk::Keyboard::G, "G"},
		{spk::Keyboard::H, "H"},
		{spk::Keyboard::I, "I"},
		{spk::Keyboard::J, "J"},
		{spk::Keyboard::K, "K"},
		{spk::Keyboard::L, "L"},
		{spk::Keyboard::M, "M"},
		{spk::Keyboard::N, "N"},
		{spk::Keyboard::O, "O"},
		{spk::Keyboard::P, "P"},
		{spk::Keyboard::Q, "Q"},
		{spk::Keyboard::R, "R"},
		{spk::Keyboard::S, "S"},
		{spk::Keyboard::T, "T"},
		{spk::Keyboard::U, "U"},
		{spk::Keyboard::V, "V"},
		{spk::Keyboard::W, "W"},
		{spk::Keyboard::X, "X"},
		{spk::Keyboard::Y, "Y"},
		{spk::Keyboard::Z, "Z"},
		{spk::Keyboard::LeftWindows, "LeftWindows"},
		{spk::Keyboard::RightWindows, "RightWindows"},
		{spk::Keyboard::App, "App"},
		{spk::Keyboard::Sleep, "Sleep"},
		{spk::Keyboard::Numpad0, "Numpad0"},
		{spk::Keyboard::Numpad1, "Numpad1"},
		{spk::Keyboard::Numpad2, "Numpad2"},
		{spk::Keyboard::Numpad3, "Numpad3"},
		{spk::Keyboard::Numpad4, "Numpad4"},
		{spk::Keyboard::Numpad5, "Numpad5"},
		{spk::Keyboard::Numpad6, "Numpad6"},
		{spk::Keyboard::Numpad7, "Numpad7"},
		{spk::Keyboard::Numpad8, "Numpad8"},
		{spk::Keyboard::Numpad9, "Numpad9"},
		{spk::Keyboard::NumpadMultiply, "NumpadMultiply"},
		{spk::Keyboard::NumpadPlus, "NumpadPlus"},
		{spk::Keyboard::NumpadSeparator, "NumpadSeparator"},
		{spk::Keyboard::NumpadMinus, "NumpadMinus"},
		{spk::Keyboard::NumpadDecimal, "NumpadDecimal"},
		{spk::Keyboard::NumpadDivide, "NumpadDivide"},
		{spk::Keyboard::F1, "F1"},
		{spk::Keyboard::F2, "F2"},
		{spk::Keyboard::F3, "F3"},
		{spk::Keyboard::F4, "F4"},
		{spk::Keyboard::F5, "F5"},
		{spk::Keyboard::F6, "F6"},
		{spk::Keyboard::F7, "F7"},
		{spk::Keyboard::F8, "F8"},
		{spk::Keyboard::F9, "F9"},
		{spk::Keyboard::F10, "F10"},
		{spk::Keyboard::F11, "F11"},
		{spk::Keyboard::F12, "F12"},
		{spk::Keyboard::F13, "F13"},
		{spk::Keyboard::F14, "F14"},
		{spk::Keyboard::F15, "F15"},
		{spk::Keyboard::F16, "F16"},
		{spk::Keyboard::F17, "F17"},
		{spk::Keyboard::F18, "F18"},
		{spk::Keyboard::F19, "F19"},
		{spk::Keyboard::F20, "F20"},
		{spk::Keyboard::F21, "F21"},
		{spk::Keyboard::F22, "F22"},
		{spk::Keyboard::F23, "F23"},
		{spk::Keyboard::F24, "F24"},
		{spk::Keyboard::NumLock, "NumLock"},
		{spk::Keyboard::Scroll, "Scroll"},
		{spk::Keyboard::LeftShift, "LeftShift"},
		{spk::Keyboard::RightShift, "RightShift"},
		{spk::Keyboard::LeftControl, "LeftControl"},
		{spk::Keyboard::RightControl, "RightControl"},
		{spk::Keyboard::LeftMenu, "LeftMenu"},
		{spk::Keyboard::RightMenu, "RightMenu"},
		{spk::Keyboard::SemiColon, "SemiColon"},
		{spk::Keyboard::Plus, "Plus"},
		{spk::Keyboard::Comma, "Comma"},
		{spk::Keyboard::Minus, "Minus"},
		{spk::Keyboard::Period, "Period"},
		{spk::Keyboard::QuestionMark, "QuestionMark"},
		{spk::Keyboard::Tilde, "Tilde"},
		{spk::Keyboard::LeftBracket, "LeftBracket"},
		{spk::Keyboard::VerticalLine, "VerticalLine"},
		{spk::Keyboard::RightBracket, "RightBracket"},
		{spk::Keyboard::Quote, "Quote"},
		{spk::Keyboard::Unknown, "Unknown"},
		{spk::Keyboard::AngleBracket, "AngleBracket"},
		{spk::Keyboard::Process, "Process"},
	});
} // namespace

namespace spk
{
	std::string toString(Keyboard::Key p_key)
	{
		for (const auto &[key, name] : keyboardKeyNames)
		{
			if (key == p_key)
			{
				return std::string(name);
			}
		}

		return "Unknown";
	}

	std::wstring toWstring(Keyboard::Key p_key)
	{
		const std::string result = toString(p_key);

		return std::wstring(result.begin(), result.end());
	}

	std::optional<Keyboard::Key> fromString(std::string_view p_keyName)
	{
		for (const auto &[key, name] : keyboardKeyNames)
		{
			if (name == p_keyName)
			{
				return key;
			}
		}

		return std::nullopt;
	}

	std::ostream &operator<<(std::ostream &p_stream, Keyboard::Key p_key)
	{
		p_stream << toString(p_key);
		return p_stream;
	}

	std::wostream &operator<<(std::wostream &p_stream, Keyboard::Key p_key)
	{
		p_stream << toWstring(p_key);
		return p_stream;
	}
} // namespace spk
