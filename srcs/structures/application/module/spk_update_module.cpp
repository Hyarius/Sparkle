#include "structures/application/module/spk_update_module.hpp"

#include "utils/spk_time_utils.hpp"

namespace spk
{
	UpdateModule::UpdateModule() = default;

	void UpdateModule::bindInputs(spk::Mouse *p_mouse, spk::Keyboard *p_keyboard)
	{
		_mouse = p_mouse;
		_keyboard = p_keyboard;
	}

	void UpdateModule::bindProfiler(spk::Profiler *p_profiler)
	{
		_profiler = p_profiler;
	}

	void UpdateModule::update()
	{
		if (widget() == nullptr)
		{
			return;
		}

		const spk::Timestamp currentTimestamp = spk::TimeUtils::getTime();

		spk::UpdateContext tick;
		tick.timestamp = currentTimestamp;
		tick.deltaTime = _lastTimestamp.has_value() == true ? currentTimestamp - _lastTimestamp.value() : 0_ms;
		tick.mouse = _mouse;
		tick.keyboard = _keyboard;
		tick.profiler = _profiler;

		_lastTimestamp = currentTimestamp;

		widget()->update(tick);
	}
}
