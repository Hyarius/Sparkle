#pragma once

#include <GL/glew.h>

#include "structures/graphics/rendering/command/spk_render_command.hpp"
#include "structures/graphics/spk_uniform_buffer_object.hpp"
#include "structures/graphics/rendering/spk_scene_lighting_gpu_data.hpp"

namespace spk
{
	class CameraUpdateRenderCommand : public spk::RenderCommand
	{
	public:
		CameraUpdateRenderCommand(GLuint p_bindingPoint, const spk::Matrix4x4 &p_viewProjection);
		CameraUpdateRenderCommand(GLuint p_bindingPoint, const spk::Matrix4x4 &p_viewProjection, const spk::Matrix4x4 &p_view);

		void execute(spk::RenderContext &p_renderContext) override;

	private:
		spk::CameraGpuData _camera;
		spk::UniformBufferObject _cameraUBO;
	};
}
