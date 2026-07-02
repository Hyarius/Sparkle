#pragma once

#include <array>

#include <GL/glew.h>

#include "structures/graphics/geometry/spk_color.hpp"
#include "structures/graphics/rendering/command/spk_render_command.hpp"
#include "structures/graphics/spk_uniform_buffer_object.hpp"
#include "structures/math/spk_vector3.hpp"

namespace pg
{
	class LightUpdateRenderCommand : public spk::RenderCommand
	{
	private:
		static constexpr GLuint LightBinding = 3;

		struct LightData
		{
			std::array<float, 4> direction{};
			std::array<float, 4> color{};
			std::array<float, 4> ambient{};
		};
		static_assert(sizeof(LightData) == 48);

		LightData _data;
		spk::UniformBufferObject _lightUBO;

	public:
		LightUpdateRenderCommand(
			const spk::Vector3 &p_direction,
			const spk::Color &p_color,
			float p_ambient);

		void execute(spk::RenderContext &p_renderContext) override;
	};
}
