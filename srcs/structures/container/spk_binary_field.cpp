#include "structures/container/spk_binary_field.hpp"

#include <limits>

namespace spk
{
	BinaryLayout::Node &BinaryLayout::Node::_add(std::string_view p_name, std::size_t p_offset, std::size_t p_size, Kind p_kind)
	{
		if (_kind != Kind::Object)
		{
			throw std::logic_error("spk::BinaryLayout: children can only be added to object nodes");
		}
		if (p_name.empty())
		{
			throw std::invalid_argument("spk::BinaryLayout: child name cannot be empty");
		}
		if (p_size == 0 || p_offset > _size || p_size > _size - p_offset)
		{
			throw std::out_of_range("spk::BinaryLayout: child exceeds parent bounds");
		}
		for (const auto &child : _children)
		{
			if (child->_name == p_name)
			{
				throw std::invalid_argument("spk::BinaryLayout: duplicate child name");
			}
		}
		_children.emplace_back(std::unique_ptr<Node>(new Node(std::string(p_name), p_kind, p_offset, p_size)));
		return *_children.back();
	}
	BinaryLayout::Node &BinaryLayout::Node::addArray(std::string_view p_name, std::size_t p_offset, std::size_t p_count, std::size_t p_elementSize)
	{
		if (p_count == 0 || p_elementSize == 0 || p_count > std::numeric_limits<std::size_t>::max() / p_elementSize)
		{
			throw std::invalid_argument("spk::BinaryLayout: invalid array extent");
		}
		Node &result = _add(p_name, p_offset, p_count * p_elementSize, Kind::Array);
		result._count = p_count;
		result._elementSize = p_elementSize;
		return result;
	}
	BinaryLayout::BinaryLayout(std::size_t p_size) :
		_root(std::unique_ptr<Node>(new Node("<root>", Kind::Object, 0, p_size)))
	{
	}
	std::size_t BinaryLayout::size() const noexcept
	{
		return _root->_size;
	}
	const BinaryLayout::Node &BinaryLayout::root() const noexcept
	{
		return *_root;
	}
}
