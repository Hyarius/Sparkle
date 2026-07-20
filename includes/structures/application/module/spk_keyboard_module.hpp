#pragma once

#include "structures/application/module/spk_module.hpp"
#include "structures/system/device/input/spk_keyboard.hpp"
#include "structures/system/event/spk_events.hpp"
#include "structures/system/thread/spk_thread_safe_queue.hpp"

namespace spk
{
	class KeyboardModule : public IModule
	{
	private:
		spk::Keyboard _keyboard;
		spk::ThreadSafeQueue<spk::KeyboardEventRecord> _events;

	private:
		void _treatEvent(spk::KeyboardEventRecord &p_event);

	public:
		KeyboardModule();

		void pushEvent(spk::KeyboardEventRecord p_event);
		void processEvents();

		[[nodiscard]] spk::Keyboard &keyboard();
		[[nodiscard]] const spk::Keyboard &keyboard() const;
	};
}
