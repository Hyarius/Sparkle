#pragma once

#include <cstring>
#include <span>
#include <stdexcept>
#include <string_view>
#include <type_traits>

#include "spk_binary_common.hpp"
#include "spk_binary_object.hpp"

namespace spk
{
	class BinaryField
	{
	private:
		BinaryObject* _owner = nullptr;
		_binary_detail::Node* _node = nullptr;

		BinaryField(BinaryObject* p_owner, _binary_detail::Node* p_node);

		void _throwIfInvalid() const;

	public:
		BinaryField() = default;

		bool isValid() const;

		BinaryFieldKind kind() const;
		bool isValue() const;
		bool isObject() const;
		bool isArray() const;

		std::string_view name() const;

		std::size_t size() const;
		std::size_t offset() const;
		std::size_t alignment() const;
		std::size_t count() const;
		std::size_t stride() const;

		BinaryField operator[](std::string_view p_name);
		BinaryField operator[](std::size_t p_index);

		BinaryField addValue(std::string_view p_name, std::size_t p_size);
		BinaryField addArray(std::string_view p_name, std::size_t p_count, std::size_t p_elementSize);
		BinaryField addObject(std::string_view p_name);

		BinaryField& resize(std::size_t p_count);

		template <typename TValueType>
		BinaryField& set(const TValueType& p_value)
		{
			static_assert(std::is_trivially_copyable_v<TValueType>);

			_throwIfInvalid();

			if (_node->kind != BinaryFieldKind::Value)
				throw std::runtime_error("set on non-value");

			_owner->synchronize();

			if (sizeof(TValueType) != _node->size)
				throw std::runtime_error("invalid size");

			std::memcpy(_owner->_buffer.data() + _node->offset, &p_value, sizeof(TValueType));
			return (*this);
		}

		template <typename TValueType>
		TValueType as() const
		{
			static_assert(std::is_trivially_copyable_v<TValueType>);

			_throwIfInvalid();

			if (_node->kind != BinaryFieldKind::Value)
				throw std::runtime_error("as on non-value");

			_owner->synchronize();

			if (sizeof(TValueType) != _node->size)
				throw std::runtime_error("invalid size");

			TValueType result;
			std::memcpy(&result, _owner->_buffer.data() + _node->offset, sizeof(TValueType));
			return result;
		}

		std::span<std::uint8_t> bytes();
		std::span<const std::uint8_t> bytes() const;

		friend class BinaryObject;
	};
}