#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <type_traits>

#include <GL/glew.h>

#include "structures/graphics/spk_buffer_object.hpp"

namespace spk
{
	class IndexBufferObject : public BufferObject
	{
	private:
		GLenum _elementType = GL_UNSIGNED_INT;

		void _validateCast(std::size_t p_typeSize) const;

	public:
		explicit IndexBufferObject(
			Usage p_usage = Usage::DynamicDraw,
			std::size_t p_size = 0);

		void setElementType(GLenum p_elementType);
		[[nodiscard]] GLenum elementType() const noexcept;

		[[nodiscard]] std::size_t count() const noexcept;

		template <typename TIndex = std::uint32_t>
		[[nodiscard]] std::span<TIndex> cast()
		{
			static_assert(
				std::is_integral_v<TIndex>,
				"spk::IndexBufferObject::cast requires an integral index type.");
			static_assert(
				alignof(TIndex) <= alignof(std::max_align_t),
				"spk::IndexBufferObject::cast cannot satisfy over-aligned types.");

			_validateCast(sizeof(TIndex));
			return std::span<TIndex>(reinterpret_cast<TIndex *>(data()), count());
		}

		template <typename TIndex = std::uint32_t>
		[[nodiscard]] std::span<const TIndex> cast() const
		{
			static_assert(
				std::is_integral_v<TIndex>,
				"spk::IndexBufferObject::cast requires an integral index type.");
			static_assert(
				alignof(TIndex) <= alignof(std::max_align_t),
				"spk::IndexBufferObject::cast cannot satisfy over-aligned types.");

			_validateCast(sizeof(TIndex));
			return std::span<const TIndex>(reinterpret_cast<const TIndex *>(data()), count());
		}
	};
}
