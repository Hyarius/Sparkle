#include "spk_window_modules.hpp"

#include <stdexcept>

#include "spk_time_utils.hpp"

namespace spk
{
	IModule::~IModule() = default;

	void IModule::bind(spk::Widget* p_widget)
	{
		_widget = p_widget;
	}

	spk::Widget* IModule::widget()
	{
		return _widget;
	}

	const spk::Widget* IModule::widget() const
	{
		return _widget;
	}

	MouseModule::MouseModule() = default;

	void MouseModule::_treatEvent(const spk::Event& p_event)
	{
		if (widget() == nullptr)
		{
			return;
		}

		if (const auto* payload = p_event.getIf<spk::MouseEnteredPayload>(); payload != nullptr)
		{
			_mouse.position = payload->position;
			_mouse.deltaPosition = {0, 0};
			widget()->dispatchMouseEvent(p_event);
			return;
		}

		if (const auto* payload = p_event.getIf<spk::MouseLeftPayload>(); payload != nullptr)
		{
			_mouse.position = payload->position;
			_mouse.deltaPosition = {0, 0};
			widget()->dispatchMouseEvent(p_event);
			return;
		}

		if (const auto* payload = p_event.getIf<spk::MouseMovedPayload>(); payload != nullptr)
		{
			_mouse.deltaPosition = payload->position - _mouse.position;
			_mouse.position = payload->position;
			widget()->dispatchMouseEvent(p_event);
			return;
		}

		if (const auto* payload = p_event.getIf<spk::MouseWheelScrolledPayload>(); payload != nullptr)
		{
			_mouse.wheel += payload->delta.y;
			widget()->dispatchMouseEvent(p_event);
			return;
		}

		if (const auto* payload = p_event.getIf<spk::MouseButtonPressedPayload>(); payload != nullptr)
		{
			_mouse[payload->button] = spk::InputState::Down;
			widget()->dispatchMouseEvent(p_event);
			return;
		}

		if (const auto* payload = p_event.getIf<spk::MouseButtonReleasedPayload>(); payload != nullptr)
		{
			_mouse[payload->button] = spk::InputState::Up;
			widget()->dispatchMouseEvent(p_event);
			return;
		}

		if (p_event.getIf<spk::MouseButtonDoubleClickedPayload>() != nullptr)
		{
			widget()->dispatchMouseEvent(p_event);
			return;
		}
	}

	void MouseModule::pushEvent(const spk::Event& p_event)
	{
		_treatEvent(p_event);
	}

	spk::Mouse& MouseModule::mouse()
	{
		return _mouse;
	}

	const spk::Mouse& MouseModule::mouse() const
	{
		return _mouse;
	}

	KeyboardModule::KeyboardModule() = default;

	void KeyboardModule::_treatEvent(const spk::Event& p_event)
	{
		if (widget() == nullptr)
		{
			return;
		}

		if (const auto* payload = p_event.getIf<spk::KeyPressedPayload>(); payload != nullptr)
		{
			_keyboard[payload->key] = spk::InputState::Down;
			widget()->dispatchKeyboardEvent(p_event);
			return;
		}

		if (const auto* payload = p_event.getIf<spk::KeyReleasedPayload>(); payload != nullptr)
		{
			_keyboard[payload->key] = spk::InputState::Up;
			widget()->dispatchKeyboardEvent(p_event);
			return;
		}

		if (const auto* payload = p_event.getIf<spk::TextInputPayload>(); payload != nullptr)
		{
			_keyboard.glyph = payload->glyph;
			widget()->dispatchKeyboardEvent(p_event);
			return;
		}
	}

	void KeyboardModule::pushEvent(const spk::Event& p_event)
	{
		_treatEvent(p_event);
	}

	spk::Keyboard& KeyboardModule::keyboard()
	{
		return _keyboard;
	}

	const spk::Keyboard& KeyboardModule::keyboard() const
	{
		return _keyboard;
	}

	FrameModule::FrameModule() = default;

	void FrameModule::_treatEvent(const spk::Event& p_event)
	{
		if (widget() == nullptr)
		{
			return;
		}

		if (const auto* payload = p_event.getIf<spk::WindowResizedPayload>(); payload != nullptr)
		{
			widget()->setGeometry(payload->rect.atOrigin());
		}

		widget()->dispatchFrameEvent(p_event);
	}

	void FrameModule::pushEvent(const spk::Event& p_event)
	{
		_treatEvent(p_event);
	}

	UpdateModule::UpdateModule() = default;

	void UpdateModule::bindInputs(spk::Mouse* p_mouse, spk::Keyboard* p_keyboard)
	{
		_mouse = p_mouse;
		_keyboard = p_keyboard;
	}

	void UpdateModule::update()
	{
		if (widget() == nullptr)
		{
			return;
		}

		const spk::Timestamp currentTimestamp = spk::TimeUtils::getTime();

		spk::UpdateTick tick;
		tick.timestamp = currentTimestamp;
		tick.deltaTime = (_lastTimestamp.has_value() == true ? currentTimestamp - _lastTimestamp.value() : 0_ms);
		tick.mouse = _mouse;
		tick.keyboard = _keyboard;

		_lastTimestamp = currentTimestamp;

		widget()->update(tick);
	}

	RenderModule::RenderModule() = default;

	void RenderModule::render(const spk::RenderCommandBuilder& p_builder) const
	{
		for (const std::unique_ptr<spk::RenderCommand>& command : p_builder.commands())
		{
			if (command != nullptr)
			{
				command->execute();
			}
		}
	}
}
