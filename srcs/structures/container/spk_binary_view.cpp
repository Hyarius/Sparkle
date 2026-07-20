#include "structures/container/spk_binary_field.hpp"

#include <limits>

namespace spk
{
	BinaryView::BinaryView(const BinaryLayout &p_layout, const BinaryLayout::Node &p_node, const std::byte *p_data, std::byte *p_writableData, std::size_t p_bufferSize, std::size_t p_offset, std::size_t p_size) :
		_layout(&p_layout),
		_node(&p_node),
		_data(p_data),
		_writableData(p_writableData),
		_bufferSize(p_bufferSize),
		_offset(p_offset),
		_size(p_size)
	{
	}
	BinaryView::BinaryView(std::span<std::byte> p_data, const BinaryLayout &p_layout) :
		BinaryView(p_layout, p_layout.root(), p_data.data(), p_data.data(), p_data.size(), 0, p_layout.size())
	{
		if (p_data.size() < p_layout.size())
		{
			throw std::out_of_range("spk::BinaryView: buffer is smaller than layout");
		}
	}
	BinaryView::BinaryView(std::span<const std::byte> p_data, const BinaryLayout &p_layout) :
		BinaryView(p_layout, p_layout.root(), p_data.data(), nullptr, p_data.size(), 0, p_layout.size())
	{
		if (p_data.size() < p_layout.size())
		{
			throw std::out_of_range("spk::BinaryView: buffer is smaller than layout");
		}
	}
	const BinaryLayout::Node &BinaryView::_requireNode() const
	{
		if (!_node)
		{
			throw std::logic_error("spk::BinaryView: invalid view");
		}
		return *_node;
	}
	const BinaryLayout::Node &BinaryView::_findChild(std::string_view p_name) const
	{
		const auto &node = _requireNode();
		if (node._kind != BinaryLayout::Kind::Object)
		{
			throw std::logic_error("spk::BinaryView: named lookup requires an object");
		}
		for (const auto &child : node._children)
		{
			if (child->_name == p_name)
			{
				return *child;
			}
		}
		throw std::out_of_range("spk::BinaryView: child not found");
	}
	std::string_view BinaryView::name() const
	{
		return _requireNode()._name;
	}
	std::span<std::byte> BinaryView::bytes()
	{
		if (!_writableData)
		{
			throw std::logic_error("spk::BinaryView: view is read-only");
		}
		return {_writableData + _offset, _size};
	}
	std::span<const std::byte> BinaryView::bytes() const
	{
		return {_data + _offset, _size};
	}
	BinaryView BinaryView::operator[](std::string_view p_name) const
	{
		const auto &child = _findChild(p_name);
		return BinaryView(*_layout, child, _data, _writableData, _bufferSize, _offset + child._offset, child._size);
	}
	BinaryView BinaryView::operator[](std::size_t p_index) const
	{
		const auto &node = _requireNode();
		if (node._kind != BinaryLayout::Kind::Array)
		{
			throw std::logic_error("spk::BinaryView: indexed lookup requires an array");
		}
		if (p_index >= node._count)
		{
			throw std::out_of_range("spk::BinaryView: array index out of range");
		}
		return BinaryView(*_layout, node, _data, _writableData, _bufferSize, _offset + p_index * node._elementSize, node._elementSize);
	}
}
