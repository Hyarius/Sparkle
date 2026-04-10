#include "spk_window_modules.hpp"

namespace spk
{
	FrameModule::FrameModule() = default;

	void FrameModule::_treatEvent(const spk::Event& p_event)
	{
		if (_windowHost != nullptr)
		{
			if (const auto* payload = p_event.getIf<spk::WindowResizedPayload>(); payload != nullptr)
			{
				_windowHost->notifyFrameResized(payload->rect);
			}
		}

		if (widget() != nullptr)
		{
			widget()->dispatchFrameEvent(p_event);
		}
	}

	void FrameModule::bindWindowHost(spk::WindowHost* p_windowHost)
	{
		_windowHost = p_windowHost;

		if (_windowHost == nullptr)
		{
			_frameEventContract.resign();
			return;
		}

		_frameEventContract = _windowHost->subscribeToFrameEvents(
			[this](const spk::Event& p_event)
			{
				_treatEvent(p_event);
			});
	}
}
