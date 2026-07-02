#include "rendering/mesh_render_command.hpp"

#include <filesystem>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string>

#include "structures/graphics/spk_buffer_object.hpp"
#include "structures/graphics/spk_primitive.hpp"

#ifndef PG_RESOURCE_DIR
#	define PG_RESOURCE_DIR "."
#endif

namespace
{
	[[nodiscard]] std::string readFile(const std::filesystem::path &p_path)
	{
		std::ifstream file(p_path, std::ios::binary);
		if (file.is_open() == false)
		{
			throw std::runtime_error("pg::MeshRenderCommand: cannot open shader file " + p_path.string());
		}

		std::ostringstream stream;
		stream << file.rdbuf();
		return stream.str();
	}
}

namespace pg
{
	spk::Program &MeshRenderCommand::_program()
	{
		static const std::filesystem::path shaderDirectory =
			std::filesystem::path(PG_RESOURCE_DIR) / "shaders" / "mesh";
		static spk::Program program(
			readFile(shaderDirectory / "mesh.vert"),
			readFile(shaderDirectory / "mesh.frag"));
		return program;
	}

	MeshRenderCommand::MeshRenderCommand(
		const spk::TextureMesh2D &p_mesh,
		const spk::Texture &p_texture,
		const spk::Matrix4x4 &p_modelTransform) :
		_mesh(p_mesh),
		_modelTransform(p_modelTransform),
		_modelUBO(ModelBinding, spk::BufferObject::Usage::DynamicDraw, sizeof(spk::Matrix4x4)),
		_sampler("uTexture", spk::SamplerObject::Type::Texture2D, 0, _program())
	{
		_sampler.bind(p_texture);
	}

	void MeshRenderCommand::execute(spk::RenderContext &p_renderContext)
	{
		const std::size_t indexCount = _mesh.layoutBuffer().indexCount();
		if (indexCount == 0)
		{
			return;
		}

		glEnable(GL_CULL_FACE);
		glCullFace(GL_BACK);
		glFrontFace(GL_CCW);

		spk::Program &program = _program();
		program.activate(p_renderContext);

		_modelUBO.edit(&_modelTransform, sizeof(_modelTransform));
		_modelUBO.activate(p_renderContext);

		_sampler.activate(p_renderContext);
		_mesh.layoutBuffer().activate(p_renderContext);

		program.render(p_renderContext, spk::Primitive::Triangles, 0, indexCount);

		glDisable(GL_CULL_FACE);
	}
}
