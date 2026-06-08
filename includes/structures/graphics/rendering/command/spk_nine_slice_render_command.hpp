#pragma once

#include <memory>

#include "structures/math/spk_rect_2d.hpp"
#include "structures/math/spk_vector2.hpp"
#include "structures/graphics/rendering/command/spk_draw_texture_mesh_render_command.hpp"
#include "structures/graphics/rendering/command/spk_render_command.hpp"
#include "structures/graphics/texture/spk_sprite_sheet.hpp"

namespace spk
{
	class NineSliceRenderCommand : public spk::RenderCommand
	{
	private:
		std::unique_ptr<spk::DrawTextureMeshRenderCommand> _textureCommand;

	public:
		NineSliceRenderCommand(
			const spk::SpriteSheet& p_spriteSheet,
			spk::Rect2D p_screenRect,
			spk::Vector2Int p_cornerSize,
			float p_depth = 0.0f);

		void execute(spk::RenderContext& p_renderContext) override;
	};
}
