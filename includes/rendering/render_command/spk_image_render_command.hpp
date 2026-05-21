#pragma once

#if defined(SPARKLE_GPU_BACKEND_OPENGL)

#include <memory>

#include "math/spk_rect_2d.hpp"
#include "math/spk_vector2.hpp"
#include "rendering/render_command/spk_draw_texture_mesh_render_command.hpp"
#include "rendering/spk_render_command.hpp"
#include "rendering/spk_texture.hpp"
#include "rendering/spk_texture_mesh_2d.hpp"

namespace spk
{
	class ImageRenderCommand : public spk::RenderCommand
	{
	private:
		const spk::Texture& _texture;
		spk::Texture::Section _section;
		spk::Rect2D _screenRect;
		float _depth = 0.0f;

		mutable spk::Vector2UInt _cachedViewportSize{0, 0};
		mutable std::unique_ptr<spk::DrawTextureMeshRenderCommand> _textureCommand;

		[[nodiscard]] spk::TextureMesh2D _buildMesh(const spk::Vector2UInt& p_viewportSize) const;
		void _ensureTextureCommand() const;

	public:
		ImageRenderCommand(
			const spk::Texture& p_texture,
			spk::Texture::Section p_section,
			spk::Rect2D p_screenRect,
			float p_depth = 0.0f);

		void execute(spk::IRenderContext& p_renderContext) override;
	};
}

#endif
