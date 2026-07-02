#include "rendering/mesh_render_command.hpp"

#include <filesystem>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>

#include "structures/graphics/spk_buffer_object.hpp"
#include "structures/graphics/spk_primitive.hpp"
#include "structures/graphics/spk_uniform.hpp"

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

	void setEnabled(GLenum p_capability, bool p_enabled)
	{
		if (p_enabled)
		{
			glEnable(p_capability);
		}
		else
		{
			glDisable(p_capability);
		}
	}

	class OpenGLDrawStateGuard
	{
	private:
		const bool _cullEnabled = glIsEnabled(GL_CULL_FACE) == GL_TRUE;
		const bool _depthEnabled = glIsEnabled(GL_DEPTH_TEST) == GL_TRUE;
		const bool _blendEnabled = glIsEnabled(GL_BLEND) == GL_TRUE;

		GLint _cullFaceMode = GL_BACK;
		GLint _frontFace = GL_CCW;
		GLint _depthFunction = GL_LESS;
		GLboolean _depthWriteMask = GL_TRUE;
		GLint _blendSourceRGB = GL_ONE;
		GLint _blendDestinationRGB = GL_ZERO;
		GLint _blendSourceAlpha = GL_ONE;
		GLint _blendDestinationAlpha = GL_ZERO;

	public:
		OpenGLDrawStateGuard()
		{
			glGetIntegerv(GL_CULL_FACE_MODE, &_cullFaceMode);
			glGetIntegerv(GL_FRONT_FACE, &_frontFace);
			glGetIntegerv(GL_DEPTH_FUNC, &_depthFunction);
			glGetBooleanv(GL_DEPTH_WRITEMASK, &_depthWriteMask);
			glGetIntegerv(GL_BLEND_SRC_RGB, &_blendSourceRGB);
			glGetIntegerv(GL_BLEND_DST_RGB, &_blendDestinationRGB);
			glGetIntegerv(GL_BLEND_SRC_ALPHA, &_blendSourceAlpha);
			glGetIntegerv(GL_BLEND_DST_ALPHA, &_blendDestinationAlpha);
		}

		~OpenGLDrawStateGuard()
		{
			glCullFace(static_cast<GLenum>(_cullFaceMode));
			glFrontFace(static_cast<GLenum>(_frontFace));
			glDepthFunc(static_cast<GLenum>(_depthFunction));
			glDepthMask(_depthWriteMask);
			glBlendFuncSeparate(
				static_cast<GLenum>(_blendSourceRGB),
				static_cast<GLenum>(_blendDestinationRGB),
				static_cast<GLenum>(_blendSourceAlpha),
				static_cast<GLenum>(_blendDestinationAlpha));

			setEnabled(GL_CULL_FACE, _cullEnabled);
			setEnabled(GL_DEPTH_TEST, _depthEnabled);
			setEnabled(GL_BLEND, _blendEnabled);
		}

		OpenGLDrawStateGuard(const OpenGLDrawStateGuard &) = delete;
		OpenGLDrawStateGuard &operator=(const OpenGLDrawStateGuard &) = delete;
	};
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

	std::shared_ptr<spk::UniformBufferObject> MeshRenderCommand::makeModelUBO(
		const spk::Matrix4x4 &p_model,
		const spk::Color &p_tint)
	{
		auto modelUBO = std::make_shared<spk::UniformBufferObject>(
			ModelBinding, spk::BufferObject::Usage::DynamicDraw, sizeof(ModelData));
		updateModelUBO(*modelUBO, p_model, p_tint);
		return modelUBO;
	}

	void MeshRenderCommand::updateModelUBO(
		spk::UniformBufferObject &p_modelUBO,
		const spk::Matrix4x4 &p_model,
		const spk::Color &p_tint)
	{
		const ModelData data{.model = p_model, .tint = p_tint};
		p_modelUBO.edit(&data, sizeof(data));
	}

	std::shared_ptr<spk::SamplerObject> MeshRenderCommand::makeSampler(const spk::Texture &p_texture)
	{
		auto sampler = std::make_shared<spk::SamplerObject>(
			"uTexture", spk::SamplerObject::Type::Texture2D, 0, _program());
		sampler->bind(p_texture);
		return sampler;
	}

	MeshRenderCommand::MeshRenderCommand(
		std::shared_ptr<const Mesh3D> p_mesh,
		std::shared_ptr<spk::UniformBufferObject> p_modelUBO,
		std::shared_ptr<spk::SamplerObject> p_sampler,
		bool p_translucent) :
		_mesh(std::move(p_mesh)),
		_modelUBO(std::move(p_modelUBO)),
		_sampler(std::move(p_sampler)),
		_translucent(p_translucent)
	{
	}

	void MeshRenderCommand::execute(spk::RenderContext &p_renderContext)
	{
		if (_mesh == nullptr || _modelUBO == nullptr || _sampler == nullptr)
		{
			return;
		}

		const std::size_t indexCount = _mesh->layoutBuffer().indexCount();
		if (indexCount == 0)
		{
			return;
		}

		OpenGLDrawStateGuard stateGuard;

		glEnable(GL_CULL_FACE);
		glCullFace(GL_BACK);
		glFrontFace(GL_CCW);

		glEnable(GL_DEPTH_TEST);
		glDepthFunc(GL_LESS);
		glDepthMask(_translucent ? GL_FALSE : GL_TRUE);

		if (_translucent)
		{
			glEnable(GL_BLEND);
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		}
		else
		{
			glDisable(GL_BLEND);
		}

		spk::Program &program = _program();
		program.activate(p_renderContext);

		_modelUBO->activate(p_renderContext);

		// One shared uniform for the whole program: set per draw, no per-command allocation.
		static spk::BoolUniform translucentUniform("uTranslucent", program);
		translucentUniform.set(_translucent);
		translucentUniform.activate();

		_sampler->activate(p_renderContext);
		_mesh->layoutBuffer().activate(p_renderContext);

		program.render(p_renderContext, spk::Primitive::Triangles, 0, indexCount);
	}
}
