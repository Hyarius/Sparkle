#include "structures/graphics/spk_uniform.hpp"

#include <stdexcept>

#include "structures/graphics/opengl/spk_opengl_render_context.hpp"

namespace spk
{
	UniformBase::UniformBase(std::string p_name, spk::Program& p_program) :
		_name(std::move(p_name)),
		_program(p_program)
	{
		requestSynchronization();
	}

	spk::OpenGL::UniformLocation& UniformBase::_resolveAndValidate(
		GLenum p_expectedType,
		const char* p_expectedTypeName,
		std::size_t p_expectedCount) const
	{
		spk::RenderContext* raw = spk::RenderContext::current();
		if (raw == nullptr)
			throw std::runtime_error("spk::UniformBase: no current render context");

		spk::RenderContext& ctx = *raw;

		spk::OpenGL::UniformLocation& loc = _locationCache.resolve(
			ctx,
			_program.version(),
			[&]() -> std::unique_ptr<spk::OpenGL::UniformLocation>
			{
				const GLuint programId = _program.gpu(ctx).id();
				auto obj = std::make_unique<spk::OpenGL::UniformLocation>();
				obj->location = spk::OpenGL::Uniform::findLocation(programId, _name);
				if (obj->location == -1)
					throw std::runtime_error(
						"spk::UniformBase: uniform [" + _name + "] not found in program");
				return obj;
			});

		if (loc.validated == false)
		{
			const GLuint programId = _program.gpu(ctx).id();
			spk::OpenGL::Uniform::validateDeclaration(
				programId, _name, p_expectedType, p_expectedTypeName, p_expectedCount);
			loc.validated = true;
		}

		return loc;
	}
}
