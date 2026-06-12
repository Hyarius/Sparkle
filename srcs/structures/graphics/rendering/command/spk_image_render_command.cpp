#include "structures/graphics/rendering/command/spk_image_render_command.hpp"

#include <memory>

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
	ImageRenderCommand::ImageRenderCommand(
		const spk::Texture& p_texture,
		spk::Texture::Section p_section,
		spk::Rect2D p_screenRect,
		float p_depth)
	{
		const spk::Vector2 topLeftUV     = p_section.anchor;
		const spk::Vector2 bottomLeftUV  = {p_section.anchor.x, p_section.anchor.y + p_section.size.y};
		const spk::Vector2 bottomRightUV = p_section.anchor + p_section.size;
		const spk::Vector2 topRightUV    = {p_section.anchor.x + p_section.size.x, p_section.anchor.y};

		auto mesh = std::make_shared<spk::TextureMesh2D>();
		mesh->addShape(
			{toPosition(p_screenRect.anchor,                                p_depth), topLeftUV},
			{toPosition({p_screenRect.left(),  p_screenRect.bottom()},      p_depth), bottomLeftUV},
			{toPosition({p_screenRect.right(), p_screenRect.bottom()},      p_depth), bottomRightUV},
			{toPosition({p_screenRect.right(), p_screenRect.top()},         p_depth), topRightUV});

		_textureCommand = std::make_unique<spk::DrawTextureMeshRenderCommand>(p_texture, mesh);
	}

	void ImageRenderCommand::execute(spk::RenderContext& p_renderContext)
	{
		_textureCommand->execute(p_renderContext);
	}
}
