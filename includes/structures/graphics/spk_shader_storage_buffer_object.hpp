#pragma once

#include <optional>

#include <GL/glew.h>

#include "structures/graphics/spk_buffer_object.hpp"

namespace spk
{
	class ShaderStorageBufferObject : public BufferObject
	{
	private:
		std::optional<GLuint> _bindingPoint;

	public:
		explicit ShaderStorageBufferObject(
			Usage p_usage = Usage::DynamicDraw,
			std::size_t p_size = 0);
		explicit ShaderStorageBufferObject(
			GLuint p_bindingPoint,
			Usage p_usage = Usage::DynamicDraw,
			std::size_t p_size = 0);

		void setBindingPoint(GLuint p_bindingPoint);
		void clearBindingPoint();
		[[nodiscard]] std::optional<GLuint> bindingPoint() const noexcept;

		void activate(const spk::RenderContext& p_context) override;
	};
}
