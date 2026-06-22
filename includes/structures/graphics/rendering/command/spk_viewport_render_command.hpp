#pragma once

#include "structures/graphics/rendering/command/spk_render_command.hpp"
#include "structures/graphics/rendering/state/spk_viewport.hpp"

namespace spk
{
	class ViewportCommand : public spk::RenderCommand
	{
	private:
		spk::Viewport _viewport;

	public:
		explicit ViewportCommand(const spk::Viewport &p_viewport);

		void execute(spk::RenderContext &p_renderContext) override;
	};
}
