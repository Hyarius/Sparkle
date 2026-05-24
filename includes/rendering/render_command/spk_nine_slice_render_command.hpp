#pragma once

#if defined(SPARKLE_GPU_BACKEND_OPENGL)

#include <memory>

#include "math/spk_rect_2d.hpp"
#include "math/spk_vector2.hpp"
#include "rendering/render_command/spk_draw_texture_mesh_render_command.hpp"
#include "rendering/spk_render_command.hpp"
#include "rendering/spk_sprite_sheet.hpp"

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

		void execute(spk::IRenderContext& p_renderContext) override;
	};
}

#endif
