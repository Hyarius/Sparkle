#include "structures/graphics/opengl/spk_opengl_uniform_buffer_object.hpp"

#include <stdexcept>
#include <string>

#include "structures/graphics/opengl/spk_opengl_program.hpp"

namespace spk
{
	UniformBufferObject::UniformBufferObject(GLuint p_bindingPoint, Usage p_usage, std::size_t p_size) :
		BufferObject(Target::Uniform, p_usage, p_size),
		_bindingPoint(p_bindingPoint)
	{
	}

	GLuint UniformBufferObject::bindingPoint() const noexcept
	{
		return _bindingPoint;
	}

	void UniformBufferObject::validateFor(spk::Program& p_program) const
	{
		const GLuint programID = p_program.id();
		GLint activeBlocks = 0;
		glGetProgramiv(programID, GL_ACTIVE_UNIFORM_BLOCKS, &activeBlocks);
		for (GLint blockIndex = 0; blockIndex < activeBlocks; ++blockIndex)
		{
			GLint blockBindingPoint = -1;
			glGetActiveUniformBlockiv(
				programID,
				static_cast<GLuint>(blockIndex),
				GL_UNIFORM_BLOCK_BINDING,
				&blockBindingPoint);
			if (blockBindingPoint == static_cast<GLint>(_bindingPoint))
			{
				return;
			}
		}

		throw std::runtime_error(
			"spk::UniformBufferObject::validateFor program has no uniform block at binding point [" +
			std::to_string(_bindingPoint) + "]");
	}

	void UniformBufferObject::activate()
	{
		BufferObject::activate();
		glBindBufferBase(GL_UNIFORM_BUFFER, _bindingPoint, id());
	}
}
