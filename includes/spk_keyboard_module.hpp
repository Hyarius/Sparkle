#pragma once

#include "spk_events.hpp"
#include "spk_keyboard.hpp"
#include "spk_module.hpp"
#include "spk_thread_safe_deque.hpp"

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
