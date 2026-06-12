#pragma once

#include <memory>

#include "structures/graphics/geometry/spk_color.hpp"
#include "structures/graphics/geometry/spk_texture_mesh_2d.hpp"
#include "structures/graphics/texture/spk_font.hpp"
#include "structures/graphics/rendering/command/spk_render_command.hpp"

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
		std::shared_ptr<const spk::TextureMesh2D> _mesh;
		spk::LayoutBufferObject _layoutBuffer;
		const spk::UniformBufferObject& _viewportBuffer;
		spk::SamplerObject _atlasSampler;
		spk::Vector4Uniform _colorUniform;
		spk::Vector4Uniform _outlineColorUniform;
		spk::FloatUniform _outlineThicknessUniform;

		[[nodiscard]] static spk::Program& _sharedProgram();
		void _uploadMesh();

	public:
		DrawFontRenderCommand(
			spk::Font::Atlas& p_atlas,
			std::shared_ptr<const spk::TextureMesh2D> p_mesh,
			spk::Font::Size p_size,
			spk::Color p_color = spk::Color(1.0f, 1.0f, 1.0f, 1.0f),
			spk::Color p_outlineColor = spk::Color(0.0f, 0.0f, 0.0f, 0.0f));

		void execute(spk::RenderContext& p_renderContext) override;
	};
}
