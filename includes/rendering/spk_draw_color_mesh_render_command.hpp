#pragma once

#if defined(SPARKLE_GPU_BACKEND_OPENGL)

#include <memory>

#include "rendering/spk_color.hpp"
#include "math/spk_matrix.hpp"
#include "rendering/spk_mesh_2d.hpp"
#include "opengl/spk_opengl_layout_buffer_object.hpp"
#include "opengl/spk_opengl_program.hpp"
#include "rendering/spk_render_command.hpp"

namespace spk
{
	class DrawColorMeshRenderCommand : public spk::RenderCommand
	{
	private:
		spk::Mesh2D _mesh;
		spk::Color _color;
		spk::Matrix3x3 _transformation;
		mutable std::shared_ptr<spk::Program> _program;
		mutable spk::OpenGL::LayoutBufferObject _layoutBuffer;
		mutable bool _layoutBufferDirty = true;
		mutable int _colorUniformLocation = -1;

		void _ensureProgram() const;
		void _uploadMesh() const;

	public:
		DrawColorMeshRenderCommand(spk::Mesh2D p_mesh, spk::Color p_color, spk::Matrix3x3 p_transformation = spk::Matrix3x3::identity());

		void execute(spk::IRenderContext& p_renderContext) override;
	};
}

#endif
