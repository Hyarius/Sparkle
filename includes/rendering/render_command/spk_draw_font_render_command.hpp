#pragma once

#include <string_view>
#include <vector>

#include "rendering/spk_color.hpp"
#include "rendering/spk_font.hpp"
#include "rendering/spk_render_command.hpp"
#include "rendering/spk_texture_mesh_2d.hpp"

#include "opengl/spk_opengl_layout_buffer_object.hpp"
#include "opengl/spk_opengl_program.hpp"
#include "opengl/spk_opengl_sampler_object.hpp"
#include "opengl/spk_opengl_uniform.hpp"

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
		spk::SamplerObject _sampler;
		spk::Vector4Uniform _colorUniform;
		spk::Vector4Uniform _outlineColorUniform;
		spk::FloatUniform _outlineThicknessUniform;

		std::vector<float> _vertexData;
		spk::LayoutBufferObject _layoutBuffer;
		bool _layoutBufferDirty;

		void _uploadMesh();

	public:
		DrawFontRenderCommand(
			const spk::Font& p_font,
			spk::Font::Text p_text,
			spk::Vector2Int p_baselinePosition,
			spk::Font::Size p_size,
			spk::Color p_color = spk::Color(1.0f, 1.0f, 1.0f, 1.0f),
			spk::Color p_outlineColor = spk::Color(0.0f, 0.0f, 0.0f, 0.0f),
			float p_depth = 0.0f);

		DrawFontRenderCommand(
			const spk::Font& p_font,
			std::string_view p_text,
			spk::Vector2Int p_baselinePosition,
			spk::Font::Size p_size,
			spk::Color p_color = spk::Color(1.0f, 1.0f, 1.0f, 1.0f),
			spk::Color p_outlineColor = spk::Color(0.0f, 0.0f, 0.0f, 0.0f),
			float p_depth = 0.0f);

		void execute(spk::RenderContext& p_renderContext) override;
	};
}