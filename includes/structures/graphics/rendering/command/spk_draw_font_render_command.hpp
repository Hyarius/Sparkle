#pragma once

#include <string_view>

#include "structures/graphics/geometry/spk_color.hpp"
#include "structures/graphics/texture/spk_font.hpp"
#include "structures/graphics/rendering/command/spk_render_command.hpp"
#include "structures/graphics/geometry/spk_texture_mesh_2d.hpp"

#include "structures/graphics/spk_layout_buffer_object.hpp"
#include "structures/graphics/spk_program.hpp"
#include "structures/graphics/spk_sampler_object.hpp"
#include "structures/graphics/spk_uniform.hpp"
#include "structures/graphics/spk_uniform_buffer_object.hpp"

namespace spk
{
	class DrawFontRenderCommand : public spk::RenderCommand
	{
	private:
		spk::Font::Atlas& _atlas;

		const spk::Color _color;
		const spk::Color _outlineColor;
		const float _outlineThickness;

		spk::TextureMesh2D _mesh;
		spk::LayoutBufferObject _layoutBuffer;
		const spk::UniformBufferObject& _viewportBuffer;

		[[nodiscard]] static spk::Program& _sharedProgram();
		[[nodiscard]] static spk::Vector4Uniform& _colorUniform();
		[[nodiscard]] static spk::Vector4Uniform& _outlineColorUniform();
		[[nodiscard]] static spk::FloatUniform& _outlineThicknessUniform();
		[[nodiscard]] static spk::SamplerObject& _atlasSampler();
		void _uploadMesh();

	public:
		DrawFontRenderCommand(
			spk::Font& p_font,
			spk::Font::Text p_text,
			spk::Vector2Int p_baselinePosition,
			spk::Font::Size p_size,
			spk::Color p_color = spk::Color(1.0f, 1.0f, 1.0f, 1.0f),
			spk::Color p_outlineColor = spk::Color(0.0f, 0.0f, 0.0f, 0.0f),
			float p_depth = 0.0f);

		DrawFontRenderCommand(
			spk::Font& p_font,
			std::string_view p_text,
			spk::Vector2Int p_baselinePosition,
			spk::Font::Size p_size,
			spk::Color p_color = spk::Color(1.0f, 1.0f, 1.0f, 1.0f),
			spk::Color p_outlineColor = spk::Color(0.0f, 0.0f, 0.0f, 0.0f),
			float p_depth = 0.0f);

		void execute(spk::RenderContext& p_renderContext) override;
	};
}
