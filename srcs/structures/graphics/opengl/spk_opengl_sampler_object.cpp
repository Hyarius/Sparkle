#include "structures/graphics/opengl/spk_opengl_sampler_object.hpp"

#include <stdexcept>

#include "structures/graphics/opengl/spk_opengl_texture.hpp"
#include "structures/graphics/rendering/context/spk_render_context.hpp"
#include "structures/graphics/spk_program.hpp"
#include "structures/graphics/spk_sampler_object.hpp"

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

	void SamplerObject::activate(const spk::RenderContext &p_context, const spk::SamplerObject &p_sampler) const
	{
		glActiveTexture(GL_TEXTURE0 + p_sampler.bindingPoint());

		const spk::Texture *texture = p_sampler.texture();
		if (texture == nullptr)
		{
			glBindTexture(samplerType(p_sampler.type()), 0);
			return;
		}

		const spk::Program &program = p_sampler.program();

		spk::OpenGL::UniformLocation &loc = _locationCache.resolve(
			p_context,
			program.version(),
			[&]() -> std::unique_ptr<spk::OpenGL::UniformLocation> {
				auto obj = std::make_unique<spk::OpenGL::UniformLocation>();
				obj->location = glGetUniformLocation(
					program.gpu(p_context).id(),
					p_sampler.designator().c_str());
				return obj;
			});

		// The binding point never changes and glUniform1i is program state, so it
		// only needs to be sent once per resolved location. The program must be
		// active when the entry is first resolved.
		if (loc.validated == false)
		{
			glUniform1i(loc.location, p_sampler.bindingPoint());
			loc.validated = true;
		}

		const spk::OpenGL::Texture &glTex = texture->gpu(p_context);
		glBindTexture(samplerType(p_sampler.type()), glTex.id());
	}

	void SamplerObject::deactivate(const spk::SamplerObject &p_sampler) const
	{
		glActiveTexture(GL_TEXTURE0 + p_sampler.bindingPoint());
		glBindTexture(samplerType(p_sampler.type()), 0);
	}
}
