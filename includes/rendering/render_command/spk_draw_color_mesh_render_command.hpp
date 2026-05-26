#pragma once

#include <memory>

#include "rendering/spk_color.hpp"
#include "math/spk_matrix.hpp"
#include "rendering/spk_color_mesh_2d.hpp"
#include "rendering/spk_render_command.hpp"
#include "opengl/spk_opengl_layout_buffer_object.hpp"
#include "opengl/spk_opengl_program.hpp"

namespace spk
{
	class DrawColorMeshRenderCommand : public spk::RenderCommand
	{
	private:
		spk::ColorMesh2D _mesh;
		spk::LayoutBufferObject _layoutBuffer;
		std::shared_ptr<spk::Program> _program;

		void _ensureProgram();
		void _uploadMesh();

	public:
		DrawColorMeshRenderCommand(spk::ColorMesh2D p_mesh);

		void execute(spk::RenderContext& p_renderContext) override;
	};
}
