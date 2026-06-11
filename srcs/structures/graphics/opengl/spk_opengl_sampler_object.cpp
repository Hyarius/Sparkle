#include "structures/graphics/opengl/spk_opengl_sampler_object.hpp"

#include <stdexcept>

#include "structures/graphics/spk_sampler_object.hpp"
#include "structures/graphics/opengl/spk_opengl_texture.hpp"
#include "structures/graphics/rendering/context/spk_render_context.hpp"

namespace spk::OpenGL
{
	GLenum samplerType(spk::SamplerObject::Type p_type)
	{
		switch (p_type)
		{
		case spk::SamplerObject::Type::Texture1D:
			return GL_TEXTURE_1D;
		case spk::SamplerObject::Type::Texture2D:
			return GL_TEXTURE_2D;
		case spk::SamplerObject::Type::Texture3D:
			return GL_TEXTURE_3D;
		case spk::SamplerObject::Type::TextureCubeMap:
			return GL_TEXTURE_CUBE_MAP;
		}

		throw std::runtime_error("Unsupported sampler type");
	}

	void SamplerObject::invalidateUniformLocation() noexcept
	{
		_uniformDestination = -1;
		_uniformContextId = 0;
		_uniformProgramId = 0;
	}

	void SamplerObject::activate(const spk::RenderContext& p_context, const spk::SamplerObject& p_sampler)
	{
		glActiveTexture(GL_TEXTURE0 + p_sampler.bindingPoint());

		if (p_sampler.texture() == nullptr)
		{
			glBindTexture(samplerType(p_sampler.type()), 0);
			return;
		}

		GLint prog = 0;
		glGetIntegerv(GL_CURRENT_PROGRAM, &prog);
		if (_uniformDestination == -1 ||
			_uniformContextId != p_context.id() ||
			_uniformProgramId != static_cast<GLuint>(prog))
		{
			_uniformDestination = glGetUniformLocation(prog, p_sampler.designator().c_str());
			_uniformContextId = p_context.id();
			_uniformProgramId = static_cast<GLuint>(prog);
		}
		glUniform1i(_uniformDestination, p_sampler.bindingPoint());

		const spk::OpenGL::Texture& glTex = p_sampler.texture()->gpu(p_context);
		glBindTexture(samplerType(p_sampler.type()), glTex.id());
	}

	void SamplerObject::deactivate(const spk::SamplerObject& p_sampler)
	{
		glActiveTexture(GL_TEXTURE0 + p_sampler.bindingPoint());
		glBindTexture(samplerType(p_sampler.type()), 0);
	}
}
