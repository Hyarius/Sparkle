#pragma once

#include <functional>

#include "input/spk_events.hpp"
#include "application/module/spk_module.hpp"
#include "utils/spk_thread_safe_deque.hpp"

namespace spk
{
	class FrameModule : public IModule
	{
	public:
		using ProcessedEventCallback = std::function<bool(spk::FrameEventRecord&, bool)>;

	private:
		spk::ThreadSafeDeque<spk::FrameEventRecord> _events;

		bool _treatEvent(spk::FrameEventRecord& p_event);

	public:
		FrameModule();

		void pushEvent(spk::FrameEventRecord p_event);
		void processEvents(const ProcessedEventCallback& p_processedEventCallback = nullptr);
	};
}
