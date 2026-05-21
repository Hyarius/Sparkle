#pragma once

#include <optional>

#include "input/spk_keyboard.hpp"
#include "application/module/spk_module.hpp"
#include "input/spk_mouse.hpp"
#include "time/spk_timestamp.hpp"

namespace spk
{
	class UpdateModule : public IModule
	{
	private:
		std::optional<spk::Timestamp> _lastTimestamp;
		spk::Mouse* _mouse = nullptr;
		spk::Keyboard* _keyboard = nullptr;

	public:
		UpdateModule();

		void bindInputs(spk::Mouse* p_mouse, spk::Keyboard* p_keyboard);
		void update();
	};
}
