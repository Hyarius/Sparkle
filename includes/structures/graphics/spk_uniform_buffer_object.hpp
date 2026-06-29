#pragma once

#include <cstddef>
#include <type_traits>

#include <GL/glew.h>

#include "structures/graphics/spk_buffer_object.hpp"

namespace spk
{
	class Program;

	class UniformBufferObject : public BufferObject
	{
	private:
		const GLuint _bindingPoint;

		void _validateCast(std::size_t p_typeSize) const;

	public:
		explicit UniformBufferObject(
			GLuint p_bindingPoint,
			Usage p_usage = Usage::DynamicDraw,
			std::size_t p_size = 0);

		[[nodiscard]] GLuint bindingPoint() const noexcept;

		template <typename TType>
		[[nodiscard]] TType &cast()
		{
			static_assert(
				std::is_trivially_copyable_v<TType>,
				"spk::UniformBufferObject::cast requires a trivially copyable type.");
			static_assert(
				alignof(TType) <= alignof(std::max_align_t),
				"spk::UniformBufferObject::cast cannot satisfy over-aligned types.");

			_validateCast(sizeof(TType));
			return *reinterpret_cast<TType *>(data());
		}

		template <typename TType>
		[[nodiscard]] const TType &cast() const
		{
			static_assert(
				std::is_trivially_copyable_v<TType>,
				"spk::UniformBufferObject::cast requires a trivially copyable type.");
			static_assert(
				alignof(TType) <= alignof(std::max_align_t),
				"spk::UniformBufferObject::cast cannot satisfy over-aligned types.");

			_validateCast(sizeof(TType));
			return *reinterpret_cast<const TType *>(data());
		}

		void validateFor(spk::Program &p_program) const;
		void activate(const spk::RenderContext &p_context) const override;
	};
}
