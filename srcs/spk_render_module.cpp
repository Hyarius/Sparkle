#include "spk_window_modules.hpp"

namespace spk
{
	RenderModule::RenderModule() = default;

	void RenderModule::render()
	{
		if (widget() != nullptr)
		{
			widget()->render();
		}
	}
}
