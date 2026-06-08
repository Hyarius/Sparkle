#pragma once

#include "structures/system/event/spk_events.hpp"
#include "structures/application/module/spk_module.hpp"
#include "structures/system/device/input/spk_mouse.hpp"
#include "structures/system/thread/spk_thread_safe_deque.hpp"

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
