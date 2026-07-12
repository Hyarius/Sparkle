#pragma once

#include <array>
#include <cstdint>

#include <GL/glew.h>

#include "structures/graphics/rendering/command/spk_render_command.hpp"

namespace spk
{
	class ClearCommand : public spk::RenderCommand
	{
	private:
		std::array<float, 4> _color;
		GLbitfield _mask = GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT;
		float _depth = 1.0f;
		std::uint32_t _stencil = 0;

	public:
		explicit ClearCommand(
			const std::array<float, 4> &p_color = {0.05f, 0.06f, 0.08f, 1.0f},
			GLbitfield p_mask = GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT,
			float p_depth = 1.0f,
			std::uint32_t p_stencil = 0);
		[[nodiscard]] const std::array<float, 4> &color() const noexcept
		{
			return _color;
		}
		[[nodiscard]] GLbitfield mask() const noexcept
		{
			return _mask;
		}
		[[nodiscard]] float depth() const noexcept
		{
			return _depth;
		}
		[[nodiscard]] std::uint32_t stencil() const noexcept
		{
			return _stencil;
		}

		void execute(spk::RenderContext &p_renderContext) override;
	};
}
