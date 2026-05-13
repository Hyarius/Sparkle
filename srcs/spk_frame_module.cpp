#include "spk_window_modules.hpp"

namespace spk
{
	FrameModule::FrameModule() = default;

	bool FrameModule::_treatEvent(spk::FrameEventRecord& p_event)
	{
		if (widget() == nullptr)
		{
			return false;
		}

		if (auto* event = spk::getIf<spk::WindowResizedRecord>(p_event); event != nullptr)
		{
			widget()->setGeometry(event->rect.atOrigin());
		}

		return widget()->dispatchFrameEvent(p_event);
	}

	bool FrameModule::pushEvent(spk::FrameEventRecord& p_event)
	{
		return _treatEvent(p_event);
	}
}
