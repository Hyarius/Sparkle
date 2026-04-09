#include "spk_window_modules.hpp"

namespace spk
{
	FrameModule::FrameModule() = default;

	void FrameModule::_treatEvent(const spk::Event& p_event)
	{
		if (_window != nullptr)
		{
			if (const auto* payload = p_event.getIf<spk::WindowResizedPayload>(); payload != nullptr)
			{
				_window->notifyFrameResized(payload->rect);
			}
		}

		if (widget() != nullptr)
		{
			widget()->dispatchFrameEvent(p_event);
		}
	}

	void FrameModule::bindWindow(spk::Window* p_window)
	{
		_window = p_window;

		if (_window == nullptr)
		{
			_frameEventContract.resign();
			return;
		}

		_frameEventContract = _window->subscribeToFrameEvents(
			[this](const spk::Event& p_event)
			{
				_treatEvent(p_event);
			});
	}
}
