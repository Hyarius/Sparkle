#pragma once

#include <cstddef>
#include <span>
#include <type_traits>

#include "structures/graphics/spk_buffer_object.hpp"

namespace spk
{
	class VertexBufferObject : public BufferObject
	{
	private:
		void _validateCast(std::size_t p_typeSize) const;

	public:
		explicit VertexBufferObject(
			Usage p_usage = Usage::DynamicDraw,
			std::size_t p_size = 0);

		template <typename TVertex>
		[[nodiscard]] std::span<TVertex> cast()
		{
			static_assert(
				std::is_trivially_copyable_v<TVertex>,
				"spk::VertexBufferObject::cast requires a trivially copyable type.");
			static_assert(
				alignof(TVertex) <= alignof(std::max_align_t),
				"spk::VertexBufferObject::cast cannot satisfy over-aligned types.");

			_validateCast(sizeof(TVertex));
			return std::span<TVertex>(reinterpret_cast<TVertex *>(data()), size() / sizeof(TVertex));
		}

		template <typename TVertex>
		[[nodiscard]] std::span<const TVertex> cast() const
		{
			static_assert(
				std::is_trivially_copyable_v<TVertex>,
				"spk::VertexBufferObject::cast requires a trivially copyable type.");
			static_assert(
				alignof(TVertex) <= alignof(std::max_align_t),
				"spk::VertexBufferObject::cast cannot satisfy over-aligned types.");

			_validateCast(sizeof(TVertex));
			return std::span<const TVertex>(reinterpret_cast<const TVertex *>(data()), size() / sizeof(TVertex));
		}
	};
}
