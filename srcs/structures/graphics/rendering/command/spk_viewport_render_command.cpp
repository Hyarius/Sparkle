#include "structures/graphics/rendering/command/spk_viewport_render_command.hpp"

#include <stdexcept>

#include "structures/graphics/rendering/context/spk_render_context.hpp"
#include "structures/system/device/window/spk_surface_state.hpp"

#include "structures/graphics/opengl/spk_opengl_gpu_data_buffer_center.hpp"
#include "structures/graphics/opengl/spk_opengl_viewport.hpp"
#include "structures/graphics/opengl/spk_opengl_uniform_buffer_object.hpp"

namespace
{
	constexpr GLuint ViewportUniformBindingPoint = 0;

	spk::UniformBufferObject& viewportUniformBuffer()
	{
		if (spk::GPUDataBufferCenter::contains(spk::GPUDataBufferCenter::ViewportBlockName) == false)
		{
			auto buffer = std::make_shared<spk::UniformBufferObject>(
				ViewportUniformBindingPoint,
				spk::BufferObject::Usage::DynamicDraw,
				sizeof(spk::Matrix4x4));
			spk::GPUDataBufferCenter::addUBO(
				spk::GPUDataBufferCenter::ViewportBlockName,
				buffer);
			return *buffer;
		}

		return spk::GPUDataBufferCenter::getUBO(spk::GPUDataBufferCenter::ViewportBlockName);
	}
}

namespace spk
{
	ViewportCommand::ViewportCommand(const spk::Viewport& p_viewport) :
		_viewport(p_viewport)
	{
	}

	void ViewportCommand::execute(spk::RenderContext& p_renderContext)
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

		spk::UniformBufferObject& viewportBuffer = viewportUniformBuffer();
		const spk::Matrix4x4& viewportMatrix = _viewport.matrix();
		viewportBuffer.edit(&viewportMatrix, sizeof(viewportMatrix));
		viewportBuffer.activate();
	}
}
