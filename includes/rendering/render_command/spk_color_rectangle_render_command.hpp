#pragma once

#if defined(SPARKLE_GPU_BACKEND_OPENGL)

#include <memory>

#include "math/spk_rect_2d.hpp"
#include "math/spk_vector2.hpp"
#include "opengl/spk_opengl_layout_buffer_object.hpp"
#include "opengl/spk_opengl_program.hpp"
#include "rendering/spk_color.hpp"
#include "rendering/spk_render_command.hpp"

namespace spk
{
	class ColorRectangleRenderCommand : public spk::RenderCommand
	{
	private:
		spk::Rect2D _rect;
		spk::Color _color;
		float _depth = 0.0f;

		mutable std::shared_ptr<spk::Program> _program;
		mutable spk::OpenGL::LayoutBufferObject _layoutBuffer;
		mutable spk::Vector2UInt _cachedViewportSize{0, 0};
		mutable int _colorUniformLocation = -1;

		void _ensureProgram() const;
		void _uploadMesh(const spk::Vector2UInt& p_viewportSize) const;

	public:
		ColorRectangleRenderCommand(
			spk::Rect2D p_rect,
			spk::Color p_color,
			float p_depth = 0.0f);

		void execute(spk::IRenderContext& p_renderContext) override;
	};
}

#endif
