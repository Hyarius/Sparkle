#include "spk_frame_module.hpp"

#include <utility>

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

	void FrameModule::pushEvent(spk::FrameEventRecord p_event)
	{
		_events.pushBack(std::move(p_event));
	}

	void FrameModule::processEvents(const ProcessedEventCallback& p_processedEventCallback)
	{
		spk::FrameEventRecord event;

		while (_events.popFront(event) == true)
		{
			const bool isConsumed = _treatEvent(event);

			if (p_processedEventCallback != nullptr)
			{
				if (p_processedEventCallback(event, isConsumed) == false)
				{
					return;
				}
			}
		}
	}
}
