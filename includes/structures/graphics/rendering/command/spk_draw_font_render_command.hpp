#pragma once

#include <memory>
#include <string_view>
#include <vector>

#include "structures/graphics/geometry/spk_color.hpp"
#include "structures/graphics/texture/spk_font.hpp"
#include "structures/graphics/rendering/command/spk_render_command.hpp"
#include "structures/graphics/geometry/spk_texture_mesh_2d.hpp"

#include "structures/graphics/opengl/spk_opengl_layout_buffer_object.hpp"
#include "structures/graphics/opengl/spk_opengl_program.hpp"
#include "structures/graphics/opengl/spk_opengl_sampler_object.hpp"
#include "structures/graphics/opengl/spk_opengl_uniform.hpp"

namespace spk
{
	class DrawFontRenderCommand : public spk::RenderCommand
	{
	private:
		spk::Font::Atlas& _atlas;

		const spk::Color _color;
		const spk::Color _outlineColor;
		const float _outlineThickness;

		std::unique_ptr<spk::Program> _program;
		std::unique_ptr<spk::Vector4Uniform> _colorUniform;
		std::unique_ptr<spk::Vector4Uniform> _outlineColorUniform;
		std::unique_ptr<spk::FloatUniform> _outlineThicknessUniform;

		spk::SamplerObject _sampler;

		std::vector<float> _vertexData;
		spk::LayoutBufferObject _layoutBuffer;
		bool _layoutBufferDirty;

		void _ensureProgram();
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