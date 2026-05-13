#include "spk_keyboard_module.hpp"

#include <utility>

namespace spk
{
	KeyboardModule::KeyboardModule() = default;

	void KeyboardModule::_treatEvent(spk::KeyboardEventRecord& p_event)
	{
		if (widget() == nullptr)
		{
			return;
		}

		std::visit(
			spk::Overloaded{
				[this](spk::KeyPressedRecord& p_record)
				{
					_keyboard[p_record.key] = spk::InputState::Down;
				},
				[this](spk::KeyReleasedRecord& p_record)
				{
					_keyboard[p_record.key] = spk::InputState::Up;
				},
				[this](spk::TextInputRecord& p_record)
				{
					_keyboard.glyph = p_record.glyph;
				}
			},
			p_event);

		widget()->dispatchKeyboardEvent(p_event, _keyboard);
	}

	void KeyboardModule::pushEvent(spk::KeyboardEventRecord p_event)
	{
		_events.pushBack(std::move(p_event));
	}

	void KeyboardModule::processEvents()
	{
		spk::KeyboardEventRecord event;

		while (_events.popFront(event) == true)
		{
			_treatEvent(event);
		}
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
