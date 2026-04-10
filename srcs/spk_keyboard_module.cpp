#include "spk_window_modules.hpp"

namespace spk
{
	KeyboardModule::KeyboardModule() = default;

	void KeyboardModule::_treatEvent(const spk::Event& p_event)
	{
		if (const auto* payload = p_event.getIf<spk::KeyPressedPayload>(); payload != nullptr)
		{
			_keyboard[payload->key] = spk::InputState::Down;
		}
		else if (const auto* payload = p_event.getIf<spk::KeyReleasedPayload>(); payload != nullptr)
		{
			_keyboard[payload->key] = spk::InputState::Up;
		}
		else if (const auto* payload = p_event.getIf<spk::TextInputPayload>(); payload != nullptr)
		{
			_keyboard.glyph = payload->glyph;
		}

		if (widget() != nullptr)
		{
			widget()->dispatchKeyboardEvent(p_event);
		}
	}

	void KeyboardModule::bindWindowHost(spk::WindowHost* p_windowHost)
	{
		_windowHost = p_windowHost;

		if (_windowHost == nullptr)
		{
			_keyboardEventContract.resign();
			return;
		}

		_keyboardEventContract = _windowHost->subscribeToKeyboardEvents(
			[this](const spk::Event& p_event)
			{
				_treatEvent(p_event);
			});
	}

	spk::Keyboard& KeyboardModule::keyboard()
	{
		return _keyboard;
	}

	const spk::Keyboard& KeyboardModule::keyboard() const
	{
		return _keyboard;
	}
}
