#pragma once

#if defined(SPARKLE_GPU_BACKEND_OPENGL)

#include <vector>

#include "opengl/spk_opengl_layout_buffer_object.hpp"
#include "opengl/spk_opengl_program.hpp"
#include "opengl/spk_opengl_sampler_object.hpp"
#include "opengl/spk_opengl_uniform.hpp"
#include "rendering/spk_color.hpp"
#include "rendering/spk_font.hpp"
#include "rendering/spk_render_command.hpp"
#include "rendering/spk_texture_mesh_2d.hpp"

namespace spk
{
	class DrawFontRenderCommand : public spk::RenderCommand
	{
	private:
		spk::Font::Atlas& _atlas;
		const spk::Color _color;
		const spk::Color _outlineColor;
		const float _outlineThickness;

		spk::Program _program;
		spk::OpenGL::LayoutBufferObject _layoutBuffer;

		spk::OpenGL::SamplerObject _sampler;
		spk::OpenGL::Vector4Uniform _colorUniform;
		spk::OpenGL::Vector4Uniform _outlineColorUniform;
		spk::OpenGL::FloatUniform _outlineThicknessUniform;

		std::vector<float> _vertexData;
		bool _layoutBufferDirty = true;

		void _uploadMesh();

	public:
		DrawFontRenderCommand(
			const spk::Font& p_font,
			std::wstring p_text,
			spk::Vector2Int p_baselinePosition,
			spk::Font::Size p_size,
			spk::Color p_color = spk::Color(1.0f, 1.0f, 1.0f, 1.0f),
			float p_depth = 0.0f,
			spk::Color p_outlineColor = spk::Color(0.0f, 0.0f, 0.0f, 0.0f),
			float p_outlineThickness = 0.0f);

		void execute(spk::IRenderContext& p_renderContext) override;
	};
}

#endif
