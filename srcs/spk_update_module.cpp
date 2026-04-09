#include "spk_window_modules.hpp"

namespace spk
{
	UpdateModule::UpdateModule() = default;

	void UpdateModule::update(const spk::UpdateTick& p_tick)
	{
		if (widget() != nullptr)
		{
			widget()->update(p_tick);
		}
	}
}
