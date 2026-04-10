#include "spk_window_modules.hpp"

namespace spk
{
	MouseModule::MouseModule() = default;

	void MouseModule::_treatEvent(const spk::Event& p_event)
	{
		if (const auto* payload = p_event.getIf<spk::MouseMovedPayload>(); payload != nullptr)
		{
			_mouse.position = payload->position;
			_mouse.deltaPosition = payload->delta;
		}
		else if (const auto* payload = p_event.getIf<spk::MouseWheelScrolledPayload>(); payload != nullptr)
		{
			_mouse.wheel += payload->delta.y;
		}
		else if (const auto* payload = p_event.getIf<spk::MouseButtonPressedPayload>(); payload != nullptr)
		{
			_mouse[payload->button] = spk::InputState::Down;
		}
		else if (const auto* payload = p_event.getIf<spk::MouseButtonReleasedPayload>(); payload != nullptr)
		{
			_mouse[payload->button] = spk::InputState::Up;
		}

		if (widget() != nullptr)
		{
			widget()->dispatchMouseEvent(p_event);
		}
	}

	void MouseModule::bindWindowHost(spk::WindowHost* p_windowHost)
	{
		_windowHost = p_windowHost;

		if (_windowHost == nullptr)
		{
			_mouseEventContract.resign();
			return;
		}

		_mouseEventContract = _windowHost->subscribeToMouseEvents(
			[this](const spk::Event& p_event)
			{
				_treatEvent(p_event);
			});
	}

	spk::Mouse& MouseModule::mouse()
	{
		return _mouse;
	}

	const spk::Mouse& MouseModule::mouse() const
	{
		return _mouse;
	}
}
