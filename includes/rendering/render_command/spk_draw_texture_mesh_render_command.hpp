#pragma once

#include <memory>
#include <vector>

#include "rendering/spk_render_command.hpp"
#include "rendering/spk_texture.hpp"
#include "rendering/spk_texture_mesh_2d.hpp"
#include "opengl/spk_opengl_layout_buffer_object.hpp"
#include "opengl/spk_opengl_program.hpp"
#include "opengl/spk_opengl_texture.hpp"

namespace spk
{
	class DrawTextureMeshRenderCommand : public spk::RenderCommand
	{
	private:
		const spk::Texture& _texture;
		spk::TextureMesh2D _mesh;
		spk::GPUTexture _gpuTexture;
		std::vector<uint8_t> _texturePixels;
		spk::Vector2UInt _textureSize;
		spk::Texture::Format _textureFormat;
		spk::Texture::Filtering _textureFiltering;
		spk::Texture::Wrap _textureWrap;
		spk::Texture::Mipmap _textureMipmap;
		spk::LayoutBufferObject _layoutBuffer;
		bool _layoutBufferDirty;
		std::shared_ptr<spk::Program> _program;
		int _textureUniformLocation = -1;

		void _ensureProgram();
		void _uploadMesh();

	public:
		DrawTextureMeshRenderCommand(const spk::Texture& p_texture, spk::TextureMesh2D p_mesh);

		void execute(spk::RenderContext& p_renderContext) override;
	};
}
