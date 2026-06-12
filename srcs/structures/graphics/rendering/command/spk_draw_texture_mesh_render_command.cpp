#include "structures/graphics/rendering/command/spk_draw_texture_mesh_render_command.hpp"

#include <span>
#include <stdexcept>
#include <utility>
#include <vector>

#include "structures/graphics/spk_gpu_data_buffer_center.hpp"
#include "structures/graphics/spk_layout_buffer_object.hpp"
#include "structures/graphics/spk_program.hpp"
#include "structures/graphics/spk_uniform_buffer_object.hpp"
#include "structures/graphics/rendering/context/spk_render_context.hpp"
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
		_sampler("uTexture", spk::SamplerObject::Type::Texture2D, 0, _sharedProgram()),
		_layoutBufferDirty(true)
	{
		_sampler.bind(_texture);

		_layoutBuffer.addAttribute(0, spk::LayoutBufferObject::Attribute::Type::Vector3);
		_layoutBuffer.addAttribute(1, spk::LayoutBufferObject::Attribute::Type::Vector2);
	}

	spk::Program& DrawTextureMeshRenderCommand::_sharedProgram()
	{
		static spk::Program program(
			SPARKLE_GET_RESOURCE_AS_STRING("resources/shaders/texture_mesh/draw_texture_mesh.vert"),
			SPARKLE_GET_RESOURCE_AS_STRING("resources/shaders/texture_mesh/draw_texture_mesh.frag"));
		return program;
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
		_uploadMesh();

		if (_layoutBuffer.vertexCount() == 0)
		{
			return;
		}

		spk::OpenGL::Program& program = _sharedProgram().gpu(p_renderContext);

		_layoutBuffer.activate(p_renderContext);
		program.activate();
		viewportUniformBuffer().activate(p_renderContext);

		_sampler.activate(p_renderContext);

		if (_layoutBuffer.isIndexed() == true)
		{
			program.render(spk::Primitive::Triangles, 0, _layoutBuffer.indexCount());
		}
		else
		{
			program.renderRaw(spk::Primitive::Triangles, 0, _layoutBuffer.vertexCount());
		}

		_sampler.deactivate();
		_layoutBuffer.deactivate();
		program.deactivate();
	}
}
