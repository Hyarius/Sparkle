#pragma once

#include "spk_render_command_builder.hpp"

namespace spk
{
	class RenderModule
	{
	public:
		RenderModule();

		void render(const spk::RenderCommandBuilder& p_builder) const;
	};
}
