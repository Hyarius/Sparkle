#pragma once

#include <cstddef>

#include <GL/glew.h>

#include "structures/graphics/rendering/command/spk_render_command.hpp"
#include "structures/graphics/spk_directional_light.hpp"
#include "structures/graphics/spk_uniform_buffer_object.hpp"

namespace spk
{
	class DirectionalLightUpdateRenderCommand : public spk::RenderCommand
	{
	private:
		spk::DirectionalLight _light;
		spk::UniformBufferObject _lightUBO;

	public:
		DirectionalLightUpdateRenderCommand(
			std::size_t p_bindingPoint,
			spk::DirectionalLight p_light);

		void execute(spk::RenderContext &p_renderContext) override;
	};
}
