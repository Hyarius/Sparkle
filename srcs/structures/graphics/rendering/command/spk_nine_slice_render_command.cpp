#include "structures/graphics/rendering/command/spk_nine_slice_render_command.hpp"

#include <array>
#include <memory>
#include <stdexcept>
#include <utility>

namespace
{
	[[nodiscard]] spk::Vector3 toPosition(int p_x, int p_y, float p_depth)
	{
		return {
			static_cast<float>(p_x),
			static_cast<float>(p_y),
			p_depth
		};
	}

	void validateArguments(
		const spk::SpriteSheet& p_spriteSheet,
		const spk::Rect2D& p_screenRect,
		const spk::Vector2Int& p_cornerSize)
	{
		if (p_spriteSheet.nbSprite() != spk::Vector2UInt{3, 3})
		{
			throw std::invalid_argument("NineSliceRenderCommand requires a 3x3 sprite sheet");
		}

		if (p_cornerSize.x < 0 || p_cornerSize.y < 0)
		{
			throw std::invalid_argument("NineSliceRenderCommand corner size cannot be negative");
		}

		if (static_cast<std::uint32_t>(p_cornerSize.x) > p_screenRect.width() / 2 ||
			static_cast<std::uint32_t>(p_cornerSize.y) > p_screenRect.height() / 2)
		{
			throw std::invalid_argument("NineSliceRenderCommand corner size does not fit the screen rectangle");
		}
	}
}

namespace spk
{
	NineSliceRenderCommand::NineSliceRenderCommand(
		const spk::SpriteSheet& p_spriteSheet,
		spk::Rect2D p_screenRect,
		spk::Vector2Int p_cornerSize,
		float p_depth)
	{
		validateArguments(p_spriteSheet, p_screenRect, p_cornerSize);

		const std::array<int, 4> xPositions = {
			p_screenRect.left(),
			p_screenRect.left() + p_cornerSize.x,
			p_screenRect.right() - p_cornerSize.x,
			p_screenRect.right()
		};
		const std::array<int, 4> yPositions = {
			p_screenRect.top(),
			p_screenRect.top() + p_cornerSize.y,
			p_screenRect.bottom() - p_cornerSize.y,
			p_screenRect.bottom()
		};

		auto mesh = std::make_shared<spk::TextureMesh2D>();
		mesh->reserve(9 * 4, 9 * 6);

		for (std::uint32_t y = 0; y < 3; ++y)
		{
			for (std::uint32_t x = 0; x < 3; ++x)
			{
				const spk::SpriteSheet::Sprite& sprite = p_spriteSheet.sprite({x, y});
				const spk::Vector2 topLeftUV = sprite.anchor;
				const spk::Vector2 bottomLeftUV = {sprite.anchor.x, sprite.anchor.y + sprite.size.y};
				const spk::Vector2 bottomRightUV = sprite.anchor + sprite.size;
				const spk::Vector2 topRightUV = {sprite.anchor.x + sprite.size.x, sprite.anchor.y};

				mesh->addShape(
					{toPosition(xPositions[x], yPositions[y], p_depth), topLeftUV},
					{toPosition(xPositions[x], yPositions[y + 1], p_depth), bottomLeftUV},
					{toPosition(xPositions[x + 1], yPositions[y + 1], p_depth), bottomRightUV},
					{toPosition(xPositions[x + 1], yPositions[y], p_depth), topRightUV});
			}
		}

		_textureCommand = std::make_unique<spk::DrawTextureMeshRenderCommand>(
			static_cast<const spk::Texture&>(p_spriteSheet), mesh);
	}

	void NineSliceRenderCommand::execute(spk::RenderContext& p_renderContext)
	{
		_textureCommand->execute(p_renderContext);
	}
}
