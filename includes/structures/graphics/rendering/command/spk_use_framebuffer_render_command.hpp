#pragma once

#include "structures/graphics/rendering/command/spk_render_command.hpp"
#include "structures/graphics/rendering/state/spk_viewport.hpp"

namespace spk
{
	class FrameBufferObject;

	// Binds a render target and applies its viewport (glViewport + glScissor +
	// projection UBO). When _target is null it binds the default framebuffer
	// (the screen) and applies the viewport against the window surface; otherwise
	// it binds the FBO and applies the viewport against the FBO's own surface, so
	// the Y-flip references the correct target height.
	class UseFrameBufferRenderCommand : public spk::RenderCommand
	{
	private:
		const spk::FrameBufferObject *_target;
		spk::Viewport _viewport;

	public:
		UseFrameBufferRenderCommand(const spk::FrameBufferObject *p_target, const spk::Viewport &p_viewport);
		[[nodiscard]] const spk::FrameBufferObject *target() const noexcept
		{
			return _target;
		}
		[[nodiscard]] const spk::Viewport &viewport() const noexcept
		{
			return _viewport;
		}

		void execute(spk::RenderContext &p_renderContext) override;
	};
}
