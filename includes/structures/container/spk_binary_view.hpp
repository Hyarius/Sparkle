#pragma once

#include <cstddef>
#include <cstring>
#include <memory>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

namespace spk
{
	class BinaryView
	{
	private:
		const BinaryLayout *_layout = nullptr;
		const BinaryLayout::Node *_node = nullptr;
		const std::byte *_data = nullptr;
		std::byte *_writableData = nullptr;
		std::size_t _bufferSize = 0;
		std::size_t _offset = 0;
		std::size_t _size = 0;
		BinaryView(const BinaryLayout &p_layout, const BinaryLayout::Node &p_node, const std::byte *p_data, std::byte *p_writableData, std::size_t p_bufferSize, std::size_t p_offset, std::size_t p_size);
		[[nodiscard]] const BinaryLayout::Node &_requireNode() const;
		[[nodiscard]] const BinaryLayout::Node &_findChild(std::string_view p_name) const;

	public:
		BinaryView(std::span<std::byte> p_data, const BinaryLayout &p_layout);
		BinaryView(std::span<const std::byte> p_data, const BinaryLayout &p_layout);
		[[nodiscard]] std::string_view name() const;
		[[nodiscard]] std::size_t size() const noexcept
		{
			return _size;
		}
		[[nodiscard]] std::span<std::byte> bytes();
		[[nodiscard]] std::span<const std::byte> bytes() const;
		[[nodiscard]] BinaryView operator[](std::string_view p_name) const;
		[[nodiscard]] BinaryView operator[](std::size_t p_index) const;
		template <typename TValue>
		[[nodiscard]] TValue read() const
		{
			static_assert(std::is_trivially_copyable_v<TValue>);
			if (sizeof(TValue) != _size)
			{
				throw std::runtime_error("spk::BinaryView: type size does not match selected field");
			}
			TValue result;
			std::memcpy(&result, _data + _offset, sizeof(TValue));
			return result;
		}
		template <typename TValue>
		void write(const TValue &p_value)
		{
			static_assert(std::is_trivially_copyable_v<TValue>);
			if (_writableData == nullptr)
			{
				throw std::logic_error("spk::BinaryView: view is read-only");
			}
			if (sizeof(TValue) != _size)
			{
				throw std::runtime_error("spk::BinaryView: type size does not match selected field");
			}
			std::memcpy(_writableData + _offset, &p_value, sizeof(TValue));
		}
	};
}
