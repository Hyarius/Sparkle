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
	class BinaryLayout
	{
	public:
		enum class Kind
		{
			Value,
			Object,
			Array
		};
		class Node
		{
			friend class BinaryLayout;
			friend class BinaryView;
			std::string _name;
			Kind _kind = Kind::Value;
			std::size_t _offset = 0;
			std::size_t _size = 0;
			std::size_t _elementSize = 0;
			std::size_t _count = 0;
			std::vector<std::unique_ptr<Node>> _children;
			Node(std::string p_name, Kind p_kind, std::size_t p_offset, std::size_t p_size) :
				_name(std::move(p_name)),
				_kind(p_kind),
				_offset(p_offset),
				_size(p_size)
			{
			}
			Node &_add(std::string_view p_name, std::size_t p_offset, std::size_t p_size, Kind p_kind);

		public:
			[[nodiscard]] std::string_view name() const noexcept
			{
				return _name;
			}
			[[nodiscard]] Kind kind() const noexcept
			{
				return _kind;
			}
			[[nodiscard]] std::size_t offset() const noexcept
			{
				return _offset;
			}
			[[nodiscard]] std::size_t size() const noexcept
			{
				return _size;
			}
			[[nodiscard]] std::size_t count() const noexcept
			{
				return _count;
			}
			[[nodiscard]] std::size_t elementSize() const noexcept
			{
				return _elementSize;
			}
			Node &addValue(std::string_view p_name, std::size_t p_offset, std::size_t p_size)
			{
				return _add(p_name, p_offset, p_size, Kind::Value);
			}
			Node &addObject(std::string_view p_name, std::size_t p_offset, std::size_t p_size)
			{
				return _add(p_name, p_offset, p_size, Kind::Object);
			}
			Node &addArray(std::string_view p_name, std::size_t p_offset, std::size_t p_count, std::size_t p_elementSize);
			template <typename TValue>
			Node &addValue(std::string_view p_name, std::size_t p_offset)
			{
				static_assert(std::is_trivially_copyable_v<TValue>);
				return addValue(p_name, p_offset, sizeof(TValue));
			}
			template <typename TValue>
			Node &addArray(std::string_view p_name, std::size_t p_offset, std::size_t p_count)
			{
				static_assert(std::is_trivially_copyable_v<TValue>);
				return addArray(p_name, p_offset, p_count, sizeof(TValue));
			}
		};

	private:
		std::unique_ptr<Node> _root;

	public:
		explicit BinaryLayout(std::size_t p_size);
		[[nodiscard]] std::size_t size() const noexcept;
		[[nodiscard]] const Node &root() const noexcept;
		Node &addValue(std::string_view p_name, std::size_t p_offset, std::size_t p_size)
		{
			return _root->addValue(p_name, p_offset, p_size);
		}
		Node &addObject(std::string_view p_name, std::size_t p_offset, std::size_t p_size)
		{
			return _root->addObject(p_name, p_offset, p_size);
		}
		Node &addArray(std::string_view p_name, std::size_t p_offset, std::size_t p_count, std::size_t p_elementSize)
		{
			return _root->addArray(p_name, p_offset, p_count, p_elementSize);
		}
		template <typename TValue>
		Node &addValue(std::string_view p_name, std::size_t p_offset)
		{
			return _root->template addValue<TValue>(p_name, p_offset);
		}
		template <typename TValue>
		Node &addArray(std::string_view p_name, std::size_t p_offset, std::size_t p_count)
		{
			return _root->template addArray<TValue>(p_name, p_offset, p_count);
		}
	};
}
