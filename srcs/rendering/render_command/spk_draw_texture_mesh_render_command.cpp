#include "rendering/render_command/spk_draw_texture_mesh_render_command.hpp"

#include <span>
#include <stdexcept>
#include <utility>
#include <vector>

#include <GL/glew.h>

#include "opengl/spk_opengl_gpu_data_buffer_center.hpp"
#include "opengl/spk_opengl_layout_buffer_object.hpp"
#include "opengl/spk_opengl_program.hpp"
#include "opengl/spk_opengl_texture.hpp"
#include "opengl/spk_opengl_uniform_buffer_object.hpp"
#include "spk_generated_resources.hpp"

namespace
{
	spk::UniformBufferObject& viewportUniformBuffer()
	{
		return spk::GPUDataBufferCenter::getUBO(spk::GPUDataBufferCenter::ViewportBlockName);
	}
}

namespace spk
{
	DrawTextureMeshRenderCommand::DrawTextureMeshRenderCommand(const spk::Texture& p_texture, spk::TextureMesh2D p_mesh) :
		_texture(p_texture),
		_mesh(std::move(p_mesh)),
		_gpuTexture(),
		_texturePixels(),
		_textureSize{},
		_textureFormat(spk::Texture::Format::Error),
		_textureFiltering(spk::Texture::Filtering::Nearest),
		_textureWrap(spk::Texture::Wrap::ClampToEdge),
		_textureMipmap(spk::Texture::Mipmap::Enable),
		_layoutBufferDirty(true)
	{
		_layoutBuffer.addAttribute(0, spk::LayoutBufferObject::Attribute::Type::Vector3);
		_layoutBuffer.addAttribute(1, spk::LayoutBufferObject::Attribute::Type::Vector2);
	}

	void DrawTextureMeshRenderCommand::_ensureProgram()
	{
		if (_program == nullptr)
		{
			_program = std::make_shared<spk::Program>(
				SPARKLE_GET_RESOURCE_AS_STRING("resources/shaders/texture_mesh/draw_texture_mesh.vert"),
				SPARKLE_GET_RESOURCE_AS_STRING("resources/shaders/texture_mesh/draw_texture_mesh.frag"));
			_textureUniformLocation = glGetUniformLocation(_program->id(), "uTexture");
		}
	}

	void DrawTextureMeshRenderCommand::_uploadMesh()
	{
		if (_layoutBufferDirty == false)
		{
			return;
		}
		_layoutBufferDirty = false;

		if (_mesh.buffer().vertices.empty() == true)
		{
			return;
		}

		std::vector<float> vertices;
		const auto appendVertex = [&vertices](const spk::TextureVertex2D& vertex)
		{
			vertices.push_back(vertex.position.x);
			vertices.push_back(vertex.position.y);
			vertices.push_back(vertex.position.z);
			vertices.push_back(vertex.uv.x);
			vertices.push_back(vertex.uv.y);
		};

		if (_mesh.buffer().indexes.empty() == false)
		{
			vertices.reserve(_mesh.buffer().indexes.size() * 5);
			for (const std::uint32_t index : _mesh.buffer().indexes)
			{
				appendVertex(_mesh.buffer().vertices[index]);
			}
		}
		else
		{
			vertices.reserve(_mesh.buffer().vertices.size() * 5);
			for (const spk::TextureVertex2D& vertex : _mesh.buffer().vertices)
			{
				appendVertex(vertex);
			}
		}

		_layoutBuffer.setVertexBytes(vertices.data(), vertices.size() * sizeof(float));
		_layoutBuffer.setIndexes(std::span<const std::uint32_t>());
	}

	void DrawTextureMeshRenderCommand::execute(spk::RenderContext& p_renderContext)
	{
		(void)p_renderContext;

		_ensureProgram();
		_uploadMesh();

		if (_layoutBuffer.vertexCount() == 0)
		{
			return;
		}

		// Synchronize texture
		if (_texturePixels != _texture.pixels() ||
			_textureSize != _texture.size() ||
			_textureFormat != _texture.format() ||
			_textureFiltering != _texture.filtering() ||
			_textureWrap != _texture.wrap() ||
			_textureMipmap != _texture.mipmap())
		{
			_texturePixels = _texture.pixels();
			_textureSize = _texture.size();
			_textureFormat = _texture.format();
			_textureFiltering = _texture.filtering();
			_textureWrap = _texture.wrap();
			_textureMipmap = _texture.mipmap();
			_gpuTexture.setPixels(_texturePixels, _textureSize, _textureFormat, _textureFiltering, _textureWrap, _textureMipmap);
		}

		_gpuTexture.synchronize();
		if (_gpuTexture.glId() == spk::GPUTexture::InvalidGLId)
		{
			throw std::runtime_error("DrawTextureMeshRenderCommand received a texture without GPU storage");
		}

		_layoutBuffer.activate();
		_program->activate();
		viewportUniformBuffer().activate();

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, _gpuTexture.glId());
		if (_textureUniformLocation >= 0)
		{
			glUniform1i(_textureUniformLocation, 0);
		}

		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		if (_layoutBuffer.isIndexed() == true)
		{
			_program->render(spk::Primitive::Triangles, 0, _layoutBuffer.indexCount());
		}
		else
		{
			_program->renderRaw(spk::Primitive::Triangles, 0, _layoutBuffer.vertexCount());
		}

		glBindTexture(GL_TEXTURE_2D, 0);
		_layoutBuffer.deactivate();
		_program->deactivate();
	}
}
