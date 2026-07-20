#include "structures/application/module/spk_frame_module.hpp"

#include <utility>

namespace spk
{
	FrameModule::FrameModule() = default;

	bool FrameModule::_treatEvent(spk::FrameEventRecord &p_event)
	{
		if (widget() == nullptr)
		{
			return false;
		}

		if (auto *event = spk::getIf<spk::WindowResizedRecord>(p_event); event != nullptr)
		{
			widget()->_onResize(event->rect.atOrigin());
			widget()->invalidateRenderUnitTree();
		}

		return widget()->_dispatchFrameEvent(p_event);
	}

	void FrameModule::pushEvent(spk::FrameEventRecord p_event)
	{
		_events.push(std::move(p_event));
	}

	void FrameModule::processEvents(const ProcessedEventCallback &p_processedEventCallback)
	{
		while (std::optional<spk::FrameEventRecord> event = _events.tryPop())
		{
			const bool isConsumed = _treatEvent(*event);

			if (p_processedEventCallback != nullptr)
			{
				if (p_processedEventCallback(*event, isConsumed) == false)
				{
					return;
				}
			}
		}
	}
}
