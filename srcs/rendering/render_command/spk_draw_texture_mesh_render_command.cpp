#include "rendering/render_command/spk_draw_texture_mesh_render_command.hpp"

#if defined(SPARKLE_GPU_BACKEND_OPENGL)

#include <stdexcept>
#include <span>
#include <utility>
#include <vector>

#include <GL/glew.h>

#include "opengl/spk_opengl_texture.hpp"

namespace
{
	const char* vertexShaderSource()
	{
		return
			"#version 330 core\n"
			"layout(location = 0) in vec3 inPosition;\n"
			"layout(location = 1) in vec2 inUV;\n"
			"out vec2 vertexUV;\n"
			"void main()\n"
			"{\n"
			"	vertexUV = inUV;\n"
			"	gl_Position = vec4(inPosition, 1.0);\n"
			"}\n";
	}

	const char* fragmentShaderSource()
	{
		return
			"#version 330 core\n"
			"uniform sampler2D uTexture;\n"
			"in vec2 vertexUV;\n"
			"out vec4 outColor;\n"
			"void main()\n"
			"{\n"
			"	outColor = texture(uTexture, vertexUV);\n"
			"}\n";
	}

	void bindTexture(const spk::Texture& p_texture)
	{
		const auto* openGLTexture = dynamic_cast<const spk::OpenGL::Texture*>(&p_texture);
		if (openGLTexture == nullptr)
		{
			throw std::runtime_error("DrawTextureMeshRenderCommand requires an OpenGL texture");
		}

		p_texture.synchronize();
		if (openGLTexture->glId() == spk::OpenGL::Texture::InvalidGLId)
		{
			throw std::runtime_error("DrawTextureMeshRenderCommand received a texture without GPU storage");
		}

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, openGLTexture->glId());
	}
}

namespace spk
{
	DrawTextureMeshRenderCommand::DrawTextureMeshRenderCommand(const spk::Texture& p_texture, spk::TextureMesh2D p_mesh) :
		_texture(p_texture),
		_mesh(std::move(p_mesh))
	{
		_layoutBuffer.addAttribute(0, spk::OpenGL::LayoutBufferObject::Attribute::Type::Vector3);
		_layoutBuffer.addAttribute(1, spk::OpenGL::LayoutBufferObject::Attribute::Type::Vector2);
	}

	void DrawTextureMeshRenderCommand::_ensureProgram() const
	{
		if (_program == nullptr)
		{
			_program = std::make_shared<spk::Program>(vertexShaderSource(), fragmentShaderSource());
			_textureUniformLocation = glGetUniformLocation(_program->id(), "uTexture");
		}
	}

	void DrawTextureMeshRenderCommand::_uploadMesh() const
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

	void DrawTextureMeshRenderCommand::execute(spk::IRenderContext& p_renderContext)
	{
		(void)p_renderContext;

		_ensureProgram();
		_uploadMesh();

		if (_layoutBuffer.vertexCount() == 0)
		{
			return;
		}

		_layoutBuffer.activate();
		_program->activate();
		bindTexture(_texture);

		if (_textureUniformLocation >= 0)
		{
			glUniform1i(_textureUniformLocation, 0);
		}

		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		if (_layoutBuffer.isIndexed() == true)
		{
			_program->render(spk::OpenGL::Primitive::Triangles, 0, _layoutBuffer.indexCount());
		}
		else
		{
			_program->renderRaw(spk::OpenGL::Primitive::Triangles, 0, _layoutBuffer.vertexCount());
		}

		glBindTexture(GL_TEXTURE_2D, 0);
		_layoutBuffer.deactivate();
		_program->deactivate();
	}
}

#endif
