#pragma once

#include <optional>

#include "spk_keyboard.hpp"
#include "spk_module.hpp"
#include "spk_mouse.hpp"
#include "spk_timestamp.hpp"

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
