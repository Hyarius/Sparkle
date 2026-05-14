#include "spk_update_module.hpp"

#include "spk_time_utils.hpp"

namespace spk
{
	UpdateModule::UpdateModule() = default;

	void UpdateModule::bindInputs(spk::Mouse* p_mouse, spk::Keyboard* p_keyboard)
	{
		_mouse = p_mouse;
		_keyboard = p_keyboard;
	}

	void UpdateModule::update()
	{
		if (widget() == nullptr)
		{
			return;
		}

		const spk::Timestamp currentTimestamp = spk::TimeUtils::getTime();

		spk::UpdateTick tick;
		tick.timestamp = currentTimestamp;
		tick.deltaTime = _lastTimestamp.has_value() == true ? currentTimestamp - _lastTimestamp.value() : 0_ms;
		tick.mouse = _mouse;
		tick.keyboard = _keyboard;

		_lastTimestamp = currentTimestamp;

		widget()->update(tick);
	}
}
