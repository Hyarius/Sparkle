#pragma once

#if defined(SPARKLE_GPU_BACKEND_OPENGL)

#include <optional>

#include <GL/glew.h>

#include "opengl/spk_opengl_buffer_object.hpp"

namespace spk::OpenGL
{
	class UniformBufferObject : public BufferObject
	{
	private:
		std::optional<GLuint> _bindingPoint;

	public:
		explicit UniformBufferObject(
			Usage p_usage = Usage::DynamicDraw,
			std::size_t p_size = 0);
		explicit UniformBufferObject(
			GLuint p_bindingPoint,
			Usage p_usage = Usage::DynamicDraw,
			std::size_t p_size = 0);

		void setBindingPoint(GLuint p_bindingPoint);
		void clearBindingPoint();
		[[nodiscard]] std::optional<GLuint> bindingPoint() const noexcept;

		void activate() override;
	};
}

#endif
