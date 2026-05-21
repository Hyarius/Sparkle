#include "rendering/render_command/spk_draw_sprite_render_command.hpp"

#if defined(SPARKLE_GPU_BACKEND_OPENGL)

#include <array>
#include <stdexcept>
#include <utility>

#include <GL/glew.h>

namespace
{
	[[nodiscard]] spk::Vector2UInt currentViewportSize()
	{
		std::array<GLint, 4> viewport{};
		glGetIntegerv(GL_VIEWPORT, viewport.data());
		return {
			static_cast<unsigned int>(viewport[2]),
			static_cast<unsigned int>(viewport[3])
		};
	}

	[[nodiscard]] spk::Vector3 toClipSpace(
		const spk::Vector2Int& p_pixel,
		const spk::Vector2UInt& p_viewportSize,
		float p_depth)
	{
		const float x = (2.0f * static_cast<float>(p_pixel.x) / static_cast<float>(p_viewportSize.x)) - 1.0f;
		const float y = 1.0f - (2.0f * static_cast<float>(p_pixel.y) / static_cast<float>(p_viewportSize.y));
		return {x, y, p_depth};
	}
}

namespace spk
{
	DrawSpriteRenderCommand::DrawSpriteRenderCommand(
		std::shared_ptr<spk::SpriteSheet> p_spriteSheet,
		std::size_t p_spriteID,
		spk::Rect2D p_screenRect,
		float p_depth) :
		_spriteSheet(std::move(p_spriteSheet)),
		_spriteID(p_spriteID),
		_screenRect(p_screenRect),
		_depth(p_depth)
	{
	}

	DrawSpriteRenderCommand::DrawSpriteRenderCommand(
		std::shared_ptr<spk::SpriteSheet> p_spriteSheet,
		const spk::Vector2UInt& p_spriteCoordinates,
		spk::Rect2D p_screenRect,
		float p_depth) :
		DrawSpriteRenderCommand(
			p_spriteSheet,
			p_spriteSheet == nullptr ? 0 : p_spriteSheet->spriteID(p_spriteCoordinates),
			p_screenRect,
			p_depth)
	{
	}

	spk::TextureMesh2D DrawSpriteRenderCommand::_buildMesh(const spk::Vector2UInt& p_viewportSize) const
	{
		const spk::SpriteSheet::Sprite& sprite = _spriteSheet->sprite(_spriteID);
		const spk::Vector2 topLeftUV = sprite.anchor;
		const spk::Vector2 bottomLeftUV = {sprite.anchor.x, sprite.anchor.y + sprite.size.y};
		const spk::Vector2 bottomRightUV = sprite.anchor + sprite.size;
		const spk::Vector2 topRightUV = {sprite.anchor.x + sprite.size.x, sprite.anchor.y};

		const spk::Vector2Int topLeft = _screenRect.anchor;
		const spk::Vector2Int bottomLeft = {_screenRect.left(), _screenRect.bottom()};
		const spk::Vector2Int bottomRight = {_screenRect.right(), _screenRect.bottom()};
		const spk::Vector2Int topRight = {_screenRect.right(), _screenRect.top()};

		spk::TextureMesh2D mesh;
		mesh.addShape(
			{toClipSpace(topLeft, p_viewportSize, _depth), topLeftUV},
			{toClipSpace(bottomLeft, p_viewportSize, _depth), bottomLeftUV},
			{toClipSpace(bottomRight, p_viewportSize, _depth), bottomRightUV},
			{toClipSpace(topRight, p_viewportSize, _depth), topRightUV});
		return mesh;
	}

	void DrawSpriteRenderCommand::_ensureTextureCommand() const
	{
		if (_spriteSheet == nullptr)
		{
			throw std::runtime_error("DrawSpriteRenderCommand requires a valid sprite sheet");
		}

		const spk::Vector2UInt viewportSize = currentViewportSize();
		if (viewportSize.x == 0 || viewportSize.y == 0)
		{
			throw std::runtime_error("DrawSpriteRenderCommand requires a valid viewport");
		}

		if (_textureCommand == nullptr || _cachedViewportSize != viewportSize)
		{
			_cachedViewportSize = viewportSize;
			_textureCommand = std::make_unique<spk::DrawTextureMeshRenderCommand>(
				_spriteSheet,
				_buildMesh(viewportSize));
		}
	}

	void DrawSpriteRenderCommand::execute(spk::IRenderContext& p_renderContext)
	{
		_ensureTextureCommand();
		_textureCommand->execute(p_renderContext);
	}
}

#endif
