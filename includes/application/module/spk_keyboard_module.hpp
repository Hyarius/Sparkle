#pragma once

#include "input/spk_events.hpp"
#include "input/spk_keyboard.hpp"
#include "application/module/spk_module.hpp"
#include "utils/spk_thread_safe_deque.hpp"

namespace spk
{
	class KeyboardModule : public IModule
	{
	private:
		spk::Keyboard _keyboard;
		spk::ThreadSafeDeque<spk::KeyboardEventRecord> _events;

	private:
		void _treatEvent(spk::KeyboardEventRecord& p_event);

	public:
		KeyboardModule();

		void pushEvent(spk::KeyboardEventRecord p_event);
		void processEvents();

		[[nodiscard]] spk::Keyboard& keyboard();
		[[nodiscard]] const spk::Keyboard& keyboard() const;
	};
}
