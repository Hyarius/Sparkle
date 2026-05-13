#include "spk_window_modules.hpp"

namespace spk
{
	RenderModule::RenderModule() = default;

	void RenderModule::render(const spk::RenderCommandBuilder& p_builder) const
	{
		for (const std::unique_ptr<spk::RenderCommand>& command : p_builder.commands())
		{
			if (command != nullptr)
			{
				command->execute();
			}
		}
	}
}
