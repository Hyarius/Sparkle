#pragma once

#include <functional>

#include "structures/application/module/spk_module.hpp"
#include "structures/system/event/spk_events.hpp"
#include "structures/system/thread/spk_thread_safe_queue.hpp"

namespace spk
{
	class FrameModule : public IModule
	{
	public:
		using ProcessedEventCallback = std::function<bool(spk::FrameEventRecord &, bool)>;

	private:
		spk::ThreadSafeQueue<spk::FrameEventRecord> _events;

		bool _treatEvent(spk::FrameEventRecord &p_event);

	public:
		FrameModule();

		void pushEvent(spk::FrameEventRecord p_event);
		void processEvents(const ProcessedEventCallback &p_processedEventCallback = nullptr);
	};
}
