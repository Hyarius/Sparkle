#pragma once

#include <memory>

#include "structures/graphics/geometry/spk_color.hpp"
#include "structures/math/spk_matrix.hpp"
#include "structures/graphics/geometry/spk_color_mesh_2d.hpp"
#include "structures/graphics/rendering/command/spk_render_command.hpp"
#include "structures/graphics/opengl/spk_opengl_layout_buffer_object.hpp"
#include "structures/graphics/opengl/spk_opengl_program.hpp"

namespace spk
{
	class DrawColorMeshRenderCommand : public spk::RenderCommand
	{
	private:
		spk::ColorMesh2D _mesh;
		spk::LayoutBufferObject _layoutBuffer;
		static inline std::shared_ptr<spk::Program> _program;

		void _ensureProgram();
		void _uploadMesh();

	public:
		DrawColorMeshRenderCommand(spk::ColorMesh2D p_mesh);

		void execute(spk::RenderContext& p_renderContext) override;
	};
}
