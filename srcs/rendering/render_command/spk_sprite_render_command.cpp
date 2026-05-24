#include "rendering/render_command/spk_sprite_render_command.hpp"

#include <utility>

namespace
{
	[[nodiscard]] spk::Vector3 toPosition(const spk::Vector2Int& p_pixel, float p_depth)
	{
		return {
			static_cast<float>(p_pixel.x),
			static_cast<float>(p_pixel.y),
			p_depth
		};
	}
}

namespace spk
{
	SpriteRenderCommand::SpriteRenderCommand(
		const spk::SpriteSheet& p_spriteSheet,
		spk::Vector2UInt p_spriteCoordinates,
		spk::Rect2D p_screenRect,
		float p_depth)
	{
		const spk::SpriteSheet::Sprite& sprite = p_spriteSheet.sprite(p_spriteCoordinates);

		const spk::Vector2 topLeftUV     = sprite.anchor;
		const spk::Vector2 bottomLeftUV  = {sprite.anchor.x, sprite.anchor.y + sprite.size.y};
		const spk::Vector2 bottomRightUV = sprite.anchor + sprite.size;
		const spk::Vector2 topRightUV    = {sprite.anchor.x + sprite.size.x, sprite.anchor.y};

		spk::TextureMesh2D mesh;
		mesh.addShape(
			{toPosition(p_screenRect.anchor,                                p_depth), topLeftUV},
			{toPosition({p_screenRect.left(),  p_screenRect.bottom()},      p_depth), bottomLeftUV},
			{toPosition({p_screenRect.right(), p_screenRect.bottom()},      p_depth), bottomRightUV},
			{toPosition({p_screenRect.right(), p_screenRect.top()},         p_depth), topRightUV});

		_textureCommand = std::make_unique<spk::DrawTextureMeshRenderCommand>(
			static_cast<const spk::Texture&>(p_spriteSheet), std::move(mesh));
	}

	void SpriteRenderCommand::execute(spk::RenderContext& p_renderContext)
	{
		_textureCommand->execute(p_renderContext);
	}
}
