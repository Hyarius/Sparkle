#pragma once

#if defined(SPARKLE_GPU_BACKEND_OPENGL)

#include <memory>

#include "math/spk_rect_2d.hpp"
#include "rendering/render_command/spk_draw_texture_mesh_render_command.hpp"
#include "rendering/spk_render_command.hpp"
#include "rendering/spk_sprite_sheet.hpp"

namespace spk
{
	class DrawSpriteRenderCommand : public spk::RenderCommand
	{
	private:
		std::shared_ptr<spk::SpriteSheet> _spriteSheet;
		std::size_t _spriteID = 0;
		spk::Rect2D _screenRect;
		float _depth = 0.0f;

		mutable spk::Vector2UInt _cachedViewportSize{0, 0};
		mutable std::unique_ptr<spk::DrawTextureMeshRenderCommand> _textureCommand;

		[[nodiscard]] spk::TextureMesh2D _buildMesh(const spk::Vector2UInt& p_viewportSize) const;
		void _ensureTextureCommand() const;

	public:
		DrawSpriteRenderCommand(
			std::shared_ptr<spk::SpriteSheet> p_spriteSheet,
			std::size_t p_spriteID,
			spk::Rect2D p_screenRect,
			float p_depth = 0.0f);

		DrawSpriteRenderCommand(
			std::shared_ptr<spk::SpriteSheet> p_spriteSheet,
			const spk::Vector2UInt& p_spriteCoordinates,
			spk::Rect2D p_screenRect,
			float p_depth = 0.0f);

		void execute(spk::IRenderContext& p_renderContext) override;
	};
}

#endif
