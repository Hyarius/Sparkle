#pragma once

#include <memory>

#include "structures/math/spk_rect_2d.hpp"
#include "structures/graphics/rendering/command/spk_draw_texture_mesh_render_command.hpp"
#include "structures/graphics/rendering/command/spk_render_command.hpp"
#include "structures/graphics/texture/spk_texture.hpp"

namespace spk
{
	class ImageRenderCommand : public spk::RenderCommand
	{
	private:
		std::unique_ptr<spk::DrawTextureMeshRenderCommand> _textureCommand;

	public:
		ImageRenderCommand(
			const spk::Texture& p_texture,
			spk::Texture::Section p_section,
			spk::Rect2D p_screenRect,
			float p_depth = 0.0f);

		void execute(spk::RenderContext& p_renderContext) override;
	};
}
