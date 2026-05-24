#pragma once

#include <memory>

#include "rendering/spk_color.hpp"
#include "math/spk_matrix.hpp"
#include "rendering/spk_mesh_2d.hpp"
#include "rendering/spk_render_command.hpp"
#include "opengl/spk_opengl_layout_buffer_object.hpp"
#include "opengl/spk_opengl_program.hpp"

namespace spk
{
	class DrawColorMeshRenderCommand : public spk::RenderCommand
	{
	private:
		spk::Mesh2D _mesh;
		spk::Color _color;
		spk::Matrix3x3 _transformation;
		spk::LayoutBufferObject _layoutBuffer;
		bool _layoutBufferDirty;
		std::shared_ptr<spk::Program> _program;
		int _colorUniformLocation = -1;

		void _ensureProgram();
		void _uploadMesh();

	public:
		DrawColorMeshRenderCommand(spk::Mesh2D p_mesh, spk::Color p_color, spk::Matrix3x3 p_transformation = spk::Matrix3x3::identity());

		void execute(spk::RenderContext& p_renderContext) override;
	};
}
