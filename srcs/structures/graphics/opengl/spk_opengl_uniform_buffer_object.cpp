#include "structures/graphics/spk_uniform_buffer_object.hpp"

#include <stdexcept>
#include <string>

#include "structures/graphics/rendering/context/spk_render_context.hpp"
#include "structures/graphics/spk_program.hpp"

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

	void UniformBufferObject::_validateCast(std::size_t p_typeSize) const
	{
		if (p_typeSize != size())
		{
			throw std::runtime_error(
				"spk::UniformBufferObject::cast type of size [" + std::to_string(p_typeSize) +
				"] does not match the buffer size [" + std::to_string(size()) + "]");
		}
	}

	void UniformBufferObject::validateFor(spk::Program &p_program) const
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

	void UniformBufferObject::activate(const spk::RenderContext &p_context) const
	{
		if (needsSynchronization() == true)
		{
			synchronize();
		}

		spk::OpenGL::Buffer &buffer = gpu(p_context);
		if (p_context.isUniformBufferBaseActive(_bindingPoint, &buffer) == true)
		{
			return;
		}

		glBindBufferBase(GL_UNIFORM_BUFFER, _bindingPoint, buffer.id());
		p_context.setActiveUniformBufferBase(_bindingPoint, &buffer);
	}
}
