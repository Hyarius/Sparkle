#pragma once

#include <GL/glew.h>

#include "structures/graphics/rendering/command/spk_render_command.hpp"
#include "structures/graphics/spk_uniform_buffer_object.hpp"
#include "structures/math/spk_matrix.hpp"

namespace spk
{
	class CameraUpdateRenderCommand : public spk::RenderCommand
	{
	public:
		CameraUpdateRenderCommand(GLuint p_bindingPoint, const spk::Matrix4x4 &p_viewProjection);

		void execute(spk::RenderContext &p_renderContext) override;

	private:
		spk::Matrix4x4 _viewProjection;
		spk::UniformBufferObject _cameraUBO;
	};
}
