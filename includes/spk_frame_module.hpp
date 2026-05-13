#pragma once

#include <functional>

#include "spk_events.hpp"
#include "spk_module.hpp"
#include "spk_thread_safe_deque.hpp"

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
