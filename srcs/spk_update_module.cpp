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

		static spk::Timestamp _lastTimestamp = spk::TimeUtils::getTime();

		const spk::Timestamp currentTimestamp = spk::TimeUtils::getTime();

		spk::UpdateTick tick;
		tick.timestamp = currentTimestamp;
		tick.deltaTime = currentTimestamp - _lastTimestamp;
		tick.mouse = _mouse;
		tick.keyboard = _keyboard;

		_lastTimestamp = currentTimestamp;

		widget()->update(tick);
	}
}
