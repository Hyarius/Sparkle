#pragma once

#include <optional>

#include "structures/application/module/spk_module.hpp"
#include "structures/system/device/input/spk_keyboard.hpp"
#include "structures/system/device/input/spk_mouse.hpp"
#include "structures/system/time/spk_timestamp.hpp"

namespace spk
{
	class UpdateModule : public IModule
	{
	private:
		std::optional<spk::Timestamp> _lastTimestamp;
		spk::Mouse *_mouse = nullptr;
		spk::Keyboard *_keyboard = nullptr;

	public:
		UpdateModule();

		void bindInputs(spk::Mouse *p_mouse, spk::Keyboard *p_keyboard);
		void update();
	};
}
