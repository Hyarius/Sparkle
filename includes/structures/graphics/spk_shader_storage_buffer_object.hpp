#pragma once

#include <cstddef>
#include <cstdint>
#include <new>
#include <optional>
#include <span>
#include <type_traits>
#include <utility>

#include <GL/glew.h>

#include "structures/graphics/spk_buffer_object.hpp"

namespace spk
{
	class ShaderStorageBufferObject : public BufferObject
	{
	public:
		template <typename TFixed, typename TElement>
		struct CastedType
		{
			TFixed &fixed;
			std::span<TElement> elements;
		};

	private:
		std::optional<GLuint> _bindingPoint;
		std::size_t _elementSize = 0;
		std::size_t _arrayOffset = 0;

		void _requireLayout(std::size_t p_totalSize) const;
		void _validateFixedType(std::size_t p_typeSize) const;
		void _validateElementType(std::size_t p_typeSize) const;

	protected:
		void _validateNewSize(std::size_t p_newSize) const override;

	public:
		explicit ShaderStorageBufferObject(
			Usage p_usage = Usage::DynamicDraw,
			std::size_t p_size = 0,
			std::size_t p_elementSize = 0,
			std::size_t p_arrayOffset = 0);
		explicit ShaderStorageBufferObject(
			GLuint p_bindingPoint,
			Usage p_usage = Usage::DynamicDraw,
			std::size_t p_size = 0,
			std::size_t p_elementSize = 0,
			std::size_t p_arrayOffset = 0);

		[[nodiscard]] std::size_t elementSize() const noexcept;
		[[nodiscard]] std::size_t arrayOffset() const noexcept;
		[[nodiscard]] std::size_t count() const noexcept;

		template <typename TType, typename... TArgs>
		TType &emplaceBack(TArgs &&...p_args)
		{
			static_assert(
				std::is_trivially_copyable_v<TType>,
				"spk::ShaderStorageBufferObject can only store trivially copyable types.");
			static_assert(
				alignof(TType) <= alignof(std::max_align_t),
				"spk::ShaderStorageBufferObject cannot satisfy over-aligned types.");

			const std::size_t offset = _reserveBack(sizeof(TType), alignof(TType));
			void *storage = static_cast<void *>(_addressAt(offset));
			return *(::new (storage) TType(std::forward<TArgs>(p_args)...));
		}

		template <typename TType>
		TType &pushBack(const TType &p_value)
		{
			return emplaceBack<TType>(p_value);
		}

		template <typename TElement>
		[[nodiscard]] std::span<TElement> elements()
		{
			static_assert(
				std::is_trivially_copyable_v<TElement>,
				"spk::ShaderStorageBufferObject::elements requires a trivially copyable type.");
			static_assert(
				alignof(TElement) <= alignof(std::max_align_t),
				"spk::ShaderStorageBufferObject::elements cannot satisfy over-aligned types.");

			_validateElementType(sizeof(TElement));
			return std::span<TElement>(reinterpret_cast<TElement *>(data() + _arrayOffset), count());
		}

		template <typename TElement>
		[[nodiscard]] std::span<const TElement> elements() const
		{
			static_assert(
				std::is_trivially_copyable_v<TElement>,
				"spk::ShaderStorageBufferObject::elements requires a trivially copyable type.");
			static_assert(
				alignof(TElement) <= alignof(std::max_align_t),
				"spk::ShaderStorageBufferObject::elements cannot satisfy over-aligned types.");

			_validateElementType(sizeof(TElement));
			return std::span<const TElement>(reinterpret_cast<const TElement *>(data() + _arrayOffset), count());
		}

		template <typename TFixed, typename TElement>
		[[nodiscard]] CastedType<TFixed, TElement> cast()
		{
			static_assert(
				std::is_trivially_copyable_v<TFixed> && std::is_trivially_copyable_v<TElement>,
				"spk::ShaderStorageBufferObject::cast requires trivially copyable types.");
			static_assert(
				alignof(TFixed) <= alignof(std::max_align_t) && alignof(TElement) <= alignof(std::max_align_t),
				"spk::ShaderStorageBufferObject::cast cannot satisfy over-aligned types.");

			_validateFixedType(sizeof(TFixed));
			_validateElementType(sizeof(TElement));

			std::uint8_t *basePtr = data();
			return CastedType<TFixed, TElement>{
				*reinterpret_cast<TFixed *>(basePtr),
				std::span<TElement>(reinterpret_cast<TElement *>(basePtr + _arrayOffset), count())};
		}

		template <typename TFixed, typename TElement>
		[[nodiscard]] CastedType<const TFixed, const TElement> cast() const
		{
			static_assert(
				std::is_trivially_copyable_v<TFixed> && std::is_trivially_copyable_v<TElement>,
				"spk::ShaderStorageBufferObject::cast requires trivially copyable types.");
			static_assert(
				alignof(TFixed) <= alignof(std::max_align_t) && alignof(TElement) <= alignof(std::max_align_t),
				"spk::ShaderStorageBufferObject::cast cannot satisfy over-aligned types.");

			_validateFixedType(sizeof(TFixed));
			_validateElementType(sizeof(TElement));

			const std::uint8_t *basePtr = data();
			return CastedType<const TFixed, const TElement>{
				*reinterpret_cast<const TFixed *>(basePtr),
				std::span<const TElement>(reinterpret_cast<const TElement *>(basePtr + _arrayOffset), count())};
		}

		void setBindingPoint(GLuint p_bindingPoint);
		void clearBindingPoint();
		[[nodiscard]] std::optional<GLuint> bindingPoint() const noexcept;

		void clear() override;
		void activate(const spk::RenderContext &p_context) const override;
	};
}
