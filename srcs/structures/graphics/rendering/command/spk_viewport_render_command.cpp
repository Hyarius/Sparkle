#include "structures/graphics/rendering/command/spk_viewport_render_command.hpp"

#include <stdexcept>

#include "structures/graphics/rendering/context/spk_render_context.hpp"
#include "structures/system/device/window/spk_surface_state.hpp"

#include "structures/graphics/opengl/spk_opengl_viewport.hpp"
#include "structures/graphics/spk_uniform_buffer_object.hpp"

namespace spk
{
	ViewportCommand::ViewportCommand(const spk::Viewport &p_viewport) :
		_viewport(p_viewport)
	{
	}

	void ViewportCommand::execute(spk::RenderContext &p_renderContext)
	{
		if (p_renderContext.supportsOpenGLCommands() == false)
		{
			return;
		}

		std::shared_ptr<spk::SurfaceState> surfaceState = p_renderContext.surfaceState();
		if (surfaceState == nullptr)
		{
			throw std::runtime_error("spk::ViewportCommand::execute() - render context has no surface state");
		}

		_viewport.activate();
		spk::OpenGLViewport::apply(_viewport, *surfaceState);

		spk::UniformBufferObject &viewportBuffer = spk::Viewport::viewportUniformBuffer();
		const spk::Matrix4x4 &viewportMatrix = _viewport.matrix();
		viewportBuffer.edit(&viewportMatrix, sizeof(viewportMatrix));
		viewportBuffer.activate(p_renderContext);
	}
}
