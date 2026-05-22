#include "rendering/render_command/spk_viewport_render_command.hpp"

#include <stdexcept>

#include "opengl/spk_opengl_gpu_data_buffer_center.hpp"
#include "opengl/spk_opengl_uniform_buffer_object.hpp"
#include "rendering/spk_render_context.hpp"
#include "window/spk_surface_state.hpp"

#if defined(_WIN32) && defined(SPARKLE_GPU_BACKEND_OPENGL)
#include "opengl/spk_opengl_render_context.hpp"
namespace
{
	constexpr GLuint ViewportUniformBindingPoint = 0;

	spk::OpenGL::UniformBufferObject& viewportUniformBuffer()
	{
		if (spk::OpenGL::GPUDataBufferCenter::contains(spk::OpenGL::GPUDataBufferCenter::ViewportBlockName) == false)
		{
			auto buffer = std::make_shared<spk::OpenGL::UniformBufferObject>(
				ViewportUniformBindingPoint,
				spk::OpenGL::BufferObject::Usage::DynamicDraw,
				sizeof(spk::Matrix4x4));
			spk::OpenGL::GPUDataBufferCenter::addUBO(
				spk::OpenGL::GPUDataBufferCenter::ViewportBlockName,
				buffer);
			return *buffer;
		}

		return spk::OpenGL::GPUDataBufferCenter::getUBO(spk::OpenGL::GPUDataBufferCenter::ViewportBlockName);
	}
}
#endif

namespace spk
{
	ViewportCommand::ViewportCommand(const spk::Viewport& p_viewport) :
		_viewport(p_viewport)
	{
	}

	void ViewportCommand::execute(spk::IRenderContext& p_renderContext)
	{
#if defined(_WIN32) && defined(SPARKLE_GPU_BACKEND_OPENGL)
		if (dynamic_cast<spk::OpenGL::RenderContext*>(&p_renderContext) == nullptr)
		{
			return;
		}
#endif

		std::shared_ptr<spk::ISurfaceState> surfaceState = p_renderContext.surfaceState();
		if (surfaceState == nullptr)
		{
			throw std::runtime_error("spk::ViewportCommand::execute() - render context has no surface state");
		}

		_viewport.activate(*surfaceState);

#if defined(_WIN32) && defined(SPARKLE_GPU_BACKEND_OPENGL)
		spk::OpenGL::UniformBufferObject& viewportBuffer = viewportUniformBuffer();
		const spk::Matrix4x4& viewportMatrix = _viewport.matrix();
		viewportBuffer.edit(&viewportMatrix, sizeof(viewportMatrix));
		viewportBuffer.activate();
#endif
	}
}
