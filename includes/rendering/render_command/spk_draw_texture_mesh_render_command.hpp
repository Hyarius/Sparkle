#pragma once

#if defined(SPARKLE_GPU_BACKEND_OPENGL)

#include "opengl/spk_opengl_layout_buffer_object.hpp"
#include "opengl/spk_opengl_program.hpp"
#include "rendering/spk_render_command.hpp"
#include "rendering/spk_texture.hpp"
#include "rendering/spk_texture_mesh_2d.hpp"

#include <memory>

namespace spk
{
	class DrawTextureMeshRenderCommand : public spk::RenderCommand
	{
	private:
		const spk::Texture& _texture;
		spk::TextureMesh2D _mesh;
		std::shared_ptr<spk::Program> _program;
		spk::OpenGL::LayoutBufferObject _layoutBuffer;
		bool _layoutBufferDirty = true;
		int _textureUniformLocation = -1;

		void _ensureProgram();
		void _uploadMesh();

	public:
		DrawTextureMeshRenderCommand(const spk::Texture& p_texture, spk::TextureMesh2D p_mesh);

		void execute(spk::IRenderContext& p_renderContext) override;
	};
}

#endif
