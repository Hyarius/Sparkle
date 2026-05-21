#pragma once

#include "input/spk_events.hpp"
#include "application/module/spk_module.hpp"
#include "input/spk_mouse.hpp"
#include "utils/spk_thread_safe_deque.hpp"

namespace spk
{
	class MouseModule : public IModule
	{
	private:
		spk::Mouse _mouse;
		spk::ThreadSafeDeque<spk::MouseEventRecord> _events;

	private:
		void _treatEvent(spk::MouseEventRecord& p_event);

	public:
		MouseModule();

		void pushEvent(spk::MouseEventRecord p_event);
		void processEvents();

		[[nodiscard]] spk::Mouse& mouse();
		[[nodiscard]] const spk::Mouse& mouse() const;
	};
}
