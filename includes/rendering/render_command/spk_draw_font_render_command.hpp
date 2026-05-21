#pragma once

#if defined(SPARKLE_GPU_BACKEND_OPENGL)

#include <memory>
#include <string>

#include "opengl/spk_opengl_layout_buffer_object.hpp"
#include "opengl/spk_opengl_program.hpp"
#include "rendering/spk_color.hpp"
#include "rendering/spk_font.hpp"
#include "rendering/spk_render_command.hpp"
#include "rendering/spk_texture_mesh_2d.hpp"

namespace spk
{
	class DrawFontRenderCommand : public spk::RenderCommand
	{
	private:
		std::shared_ptr<spk::Font> _font;
		std::wstring _text;
		spk::Vector2Int _baselinePosition;
		spk::Font::Size _size;
		spk::Color _color;
		float _depth = 0.0f;

		mutable std::shared_ptr<spk::Program> _program;
		mutable spk::OpenGL::LayoutBufferObject _layoutBuffer;
		mutable spk::Vector2UInt _cachedViewportSize{0, 0};
		mutable int _textureUniformLocation = -1;
		mutable int _colorUniformLocation = -1;

		void _ensureProgram() const;
		void _uploadMesh(const spk::Vector2UInt& p_viewportSize, spk::Font::Atlas& p_atlas) const;

	public:
		DrawFontRenderCommand(
			std::shared_ptr<spk::Font> p_font,
			std::wstring p_text,
			spk::Vector2Int p_baselinePosition,
			spk::Font::Size p_size,
			spk::Color p_color = spk::Color(1.0f, 1.0f, 1.0f, 1.0f),
			float p_depth = 0.0f);

		void execute(spk::IRenderContext& p_renderContext) override;
	};
}

#endif
