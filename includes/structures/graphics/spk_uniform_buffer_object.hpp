#pragma once

#include <GL/glew.h>

#include "structures/graphics/spk_buffer_object.hpp"

namespace spk
{
	class Program;

	class UniformBufferObject : public BufferObject
	{
	private:
		const GLuint _bindingPoint;

	public:
		explicit UniformBufferObject(
			GLuint p_bindingPoint,
			Usage p_usage = Usage::DynamicDraw,
			std::size_t p_size = 0);

		[[nodiscard]] GLuint bindingPoint() const noexcept;

		void validateFor(spk::Program &p_program) const;
		void activate(const spk::RenderContext &p_context) const override;
	};
}
