#pragma once

#include "rendering/spk_render_command.hpp"
#include "rendering/spk_viewport.hpp"

namespace spk
{
	class ViewportCommand : public spk::RenderCommand
	{
	private:
		const spk::Viewport& _viewport;

	public:
		explicit ViewportCommand(const spk::Viewport& p_viewport);

		void execute(spk::RenderContext& p_renderContext) override;
	};
}
