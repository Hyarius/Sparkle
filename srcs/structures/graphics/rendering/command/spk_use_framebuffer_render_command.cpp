#include "structures/graphics/rendering/command/spk_use_framebuffer_render_command.hpp"

#include <stdexcept>

#include "structures/graphics/opengl/spk_opengl_framebuffer_object.hpp"
#include "structures/graphics/opengl/spk_opengl_viewport.hpp"
#include "structures/graphics/rendering/context/spk_render_context.hpp"
#include "structures/graphics/spk_framebuffer_object.hpp"
#include "structures/graphics/spk_uniform_buffer_object.hpp"
#include "structures/system/device/window/spk_surface_state.hpp"

namespace spk
{
	UseFrameBufferRenderCommand::UseFrameBufferRenderCommand(
		const spk::FrameBufferObject *p_target,
		const spk::Viewport &p_viewport) :
		_target(p_target),
		_viewport(p_viewport)
	{
	}

	UseFrameBufferRenderCommand::UseFrameBufferRenderCommand(
		std::shared_ptr<const spk::FrameBufferObject> p_target,
		const spk::Viewport &p_viewport) :
		_ownedTarget(std::move(p_target)),
		_target(_ownedTarget.get()),
		_viewport(p_viewport)
	{
	}

	void UseFrameBufferRenderCommand::execute(spk::RenderContext &p_renderContext)
	{
		if (p_renderContext.supportsOpenGLCommands() == false)
		{
			return;
		}

		const spk::SurfaceState *surfaceState = nullptr;

		if (_target != nullptr)
		{
			_target->gpu(p_renderContext).bind();
			surfaceState = &_target->surfaceState();
		}
		else
		{
			spk::OpenGL::FrameBufferObject::bindDefault();

			std::shared_ptr<spk::SurfaceState> contextSurface = p_renderContext.surfaceState();
			if (contextSurface == nullptr)
			{
				throw std::runtime_error("spk::UseFrameBufferRenderCommand::execute() - render context has no surface state");
			}
			surfaceState = contextSurface.get();
		}

		_viewport.activate();
		spk::OpenGL::Viewport::apply(_viewport, *surfaceState);

		spk::UniformBufferObject &viewportBuffer = spk::Viewport::viewportUniformBuffer();
		const spk::Matrix4x4 &viewportMatrix = _viewport.matrix();
		viewportBuffer.edit(&viewportMatrix, sizeof(viewportMatrix));
		viewportBuffer.activate(p_renderContext);
	}
}
