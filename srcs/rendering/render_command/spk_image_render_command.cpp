#include "rendering/render_command/spk_image_render_command.hpp"

#if defined(SPARKLE_GPU_BACKEND_OPENGL)

#include <array>
#include <stdexcept>

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
	ImageRenderCommand::ImageRenderCommand(
		const spk::Texture& p_texture,
		spk::Texture::Section p_section,
		spk::Rect2D p_screenRect,
		float p_depth) :
		_texture(p_texture),
		_section(p_section),
		_screenRect(p_screenRect),
		_depth(p_depth)
	{
	}

	spk::TextureMesh2D ImageRenderCommand::_buildMesh(const spk::Vector2UInt& p_viewportSize) const
	{
		const spk::Vector2 topLeftUV = _section.anchor;
		const spk::Vector2 bottomLeftUV = {_section.anchor.x, _section.anchor.y + _section.size.y};
		const spk::Vector2 bottomRightUV = _section.anchor + _section.size;
		const spk::Vector2 topRightUV = {_section.anchor.x + _section.size.x, _section.anchor.y};

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

	void ImageRenderCommand::_ensureTextureCommand() const
	{
		const spk::Vector2UInt viewportSize = currentViewportSize();
		if (viewportSize.x == 0 || viewportSize.y == 0)
		{
			throw std::runtime_error("ImageRenderCommand requires a valid viewport");
		}

		if (_textureCommand == nullptr || _cachedViewportSize != viewportSize)
		{
			_cachedViewportSize = viewportSize;
			_textureCommand = std::make_unique<spk::DrawTextureMeshRenderCommand>(
				_texture,
				_buildMesh(viewportSize));
		}
	}

	void ImageRenderCommand::execute(spk::IRenderContext& p_renderContext)
	{
		_ensureTextureCommand();
		_textureCommand->execute(p_renderContext);
	}
}

#endif
