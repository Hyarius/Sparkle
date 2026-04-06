#include "spk_binary_object.hpp"
#include "spk_binary_field.hpp"

#include <algorithm>
#include <memory>
#include <stdexcept>

namespace spk
{
	_binary_detail::Node* BinaryObject::_findChildByName(_binary_detail::Node* p_parent, std::string_view p_name)
	{
		if (p_parent == nullptr)
		{
			return nullptr;
		}

		for (const std::unique_ptr<_binary_detail::Node>& child : p_parent->children)
		{
			if (child != nullptr && child->name == p_name)
			{
				return child.get();
			}
		}

		return nullptr;
	}

	_binary_detail::Node* BinaryObject::_appendChild(
		_binary_detail::Node* p_parent,
		std::string_view p_name,
		BinaryFieldKind p_kind)
	{
		if (p_parent == nullptr)
		{
			throw std::runtime_error("Invalid parent node.");
		}

		if (p_parent->kind != BinaryFieldKind::Object)
		{
			throw std::runtime_error("Children can only be added to object fields.");
		}

		if (p_name.empty())
		{
			throw std::runtime_error("Field name cannot be empty.");
		}

		if (_findChildByName(p_parent, p_name) != nullptr)
		{
			throw std::runtime_error("A field with the same name already exists.");
		}

		std::unique_ptr<_binary_detail::Node> newNode = std::make_unique<_binary_detail::Node>();
		newNode->name = std::string(p_name);
		newNode->kind = p_kind;
		newNode->parent = p_parent;

		_binary_detail::Node* result = newNode.get();
		p_parent->children.push_back(std::move(newNode));

		requestSynchronization();
		return result;
	}

	std::size_t BinaryObject::_computeValueAlignment(std::size_t p_size) const
	{
		switch (_layoutMode)
		{
		case BinaryLayoutMode::Packed:
			return 1;

		case BinaryLayoutMode::ScalarAligned:
			return _binary_detail::computeScalarAlignedAlignment(p_size);

		case BinaryLayoutMode::Std140:
		case BinaryLayoutMode::Std430:
			return _binary_detail::computeStdAlignmentFromSize(p_size);
		}

		return 1;
	}

	std::size_t BinaryObject::_computeArrayAlignment(std::size_t p_elementSize) const
	{
		switch (_layoutMode)
		{
		case BinaryLayoutMode::Packed:
			return 1;

		case BinaryLayoutMode::ScalarAligned:
			return _computeValueAlignment(p_elementSize);

		case BinaryLayoutMode::Std140:
			return 16;

		case BinaryLayoutMode::Std430:
			return _computeValueAlignment(p_elementSize);
		}

		return 1;
	}

	std::size_t BinaryObject::_computeArrayStride(std::size_t p_elementSize) const
	{
		switch (_layoutMode)
		{
		case BinaryLayoutMode::Packed:
			return p_elementSize;

		case BinaryLayoutMode::ScalarAligned:
		case BinaryLayoutMode::Std430:
			return _binary_detail::alignUp(p_elementSize, _computeArrayAlignment(p_elementSize));

		case BinaryLayoutMode::Std140:
			return _binary_detail::alignUp(p_elementSize, 16);
		}

		return p_elementSize;
	}

	void BinaryObject::_layoutNode(_binary_detail::Node* p_node, std::size_t p_baseOffset)
	{
		if (p_node == nullptr)
		{
			return;
		}

		if (p_node->kind == BinaryFieldKind::Value)
		{
			p_node->alignment = _computeValueAlignment(p_node->valueSize);
			p_node->offset = (_layoutMode == BinaryLayoutMode::Packed)
				? p_baseOffset
				: _binary_detail::alignUp(p_baseOffset, p_node->alignment);
			p_node->size = p_node->valueSize;
			p_node->stride = p_node->valueSize;
			p_node->count = 0;
			return;
		}

		if (p_node->kind == BinaryFieldKind::Array)
		{
			p_node->alignment = _computeArrayAlignment(p_node->elementSize);
			p_node->offset = (_layoutMode == BinaryLayoutMode::Packed)
				? p_baseOffset
				: _binary_detail::alignUp(p_baseOffset, p_node->alignment);
			p_node->stride = _computeArrayStride(p_node->elementSize);
			p_node->count = p_node->children.size();
			p_node->size = p_node->stride * p_node->count;

			for (std::size_t index = 0; index < p_node->children.size(); ++index)
			{
				_binary_detail::Node* child = p_node->children[index].get();
				child->alignment = _computeValueAlignment(p_node->elementSize);
				child->offset = p_node->offset + index * p_node->stride;
				child->size = p_node->elementSize;
				child->stride = p_node->elementSize;
				child->count = 0;
			}

			return;
		}

		std::size_t maximumChildAlignment = 1;
		for (const std::unique_ptr<_binary_detail::Node>& child : p_node->children)
		{
			if (child == nullptr)
			{
				continue;
			}

			switch (child->kind)
			{
			case BinaryFieldKind::Value:
				maximumChildAlignment = std::max(maximumChildAlignment, _computeValueAlignment(child->valueSize));
				break;

			case BinaryFieldKind::Array:
				maximumChildAlignment = std::max(maximumChildAlignment, _computeArrayAlignment(child->elementSize));
				break;

			case BinaryFieldKind::Object:
				break;
			}
		}

		switch (_layoutMode)
		{
		case BinaryLayoutMode::Packed:
			p_node->alignment = 1;
			p_node->offset = p_baseOffset;
			break;

		case BinaryLayoutMode::ScalarAligned:
			p_node->alignment = maximumChildAlignment;
			p_node->offset = _binary_detail::alignUp(p_baseOffset, p_node->alignment);
			break;

		case BinaryLayoutMode::Std140:
			p_node->alignment = 16;
			p_node->offset = _binary_detail::alignUp(p_baseOffset, 16);
			break;

		case BinaryLayoutMode::Std430:
			p_node->alignment = maximumChildAlignment;
			p_node->offset = _binary_detail::alignUp(p_baseOffset, p_node->alignment);
			break;
		}

		std::size_t cursor = p_node->offset;
		for (const std::unique_ptr<_binary_detail::Node>& child : p_node->children)
		{
			if (child == nullptr)
			{
				continue;
			}

			_layoutNode(child.get(), cursor);
			cursor = child->offset + child->size;
		}

		p_node->count = p_node->children.size();
		p_node->stride = 0;
		p_node->size = cursor - p_node->offset;

		if (_layoutMode != BinaryLayoutMode::Packed)
		{
			p_node->size = _binary_detail::alignUp(p_node->size, p_node->alignment);
		}
	}

	void BinaryObject::_synchronize()
	{
		_layoutNode(&_root, 0);
		_buffer.resize(_root.size);
	}

	BinaryObject::BinaryObject(BinaryLayoutMode p_layoutMode) :
		_layoutMode(p_layoutMode)
	{
		_root.name = "<root>";
		_root.kind = BinaryFieldKind::Object;
		_root.parent = nullptr;

		requestSynchronization();
	}

	BinaryField BinaryObject::root()
	{
		return BinaryField(this, &_root);
	}

	BinaryField BinaryObject::operator[](std::string_view p_name)
	{
		_binary_detail::Node* child = _findChildByName(&_root, p_name);
		if (child == nullptr)
		{
			throw std::runtime_error("BinaryObject::operator[] could not find the requested field.");
		}

		return BinaryField(this, child);
	}

	BinaryField BinaryObject::addValue(std::string_view p_name, std::size_t p_size)
	{
		if (p_size == 0)
		{
			throw std::runtime_error("BinaryObject::addValue received a zero size.");
		}

		_binary_detail::Node* node = _appendChild(&_root, p_name, BinaryFieldKind::Value);
		node->valueSize = p_size;
		return BinaryField(this, node);
	}

	BinaryField BinaryObject::addArray(std::string_view p_name, std::size_t p_count, std::size_t p_elementSize)
	{
		if (p_elementSize == 0)
		{
			throw std::runtime_error("BinaryObject::addArray received a zero element size.");
		}

		_binary_detail::Node* node = _appendChild(&_root, p_name, BinaryFieldKind::Array);
		node->elementSize = p_elementSize;

		for (std::size_t index = 0; index < p_count; ++index)
		{
			std::unique_ptr<_binary_detail::Node> child = std::make_unique<_binary_detail::Node>();
			child->name = std::to_string(index);
			child->kind = BinaryFieldKind::Value;
			child->valueSize = p_elementSize;
			child->parent = node;
			node->children.push_back(std::move(child));
		}

		requestSynchronization();
		return BinaryField(this, node);
	}

	BinaryField BinaryObject::addObject(std::string_view p_name)
	{
		_binary_detail::Node* node = _appendChild(&_root, p_name, BinaryFieldKind::Object);
		return BinaryField(this, node);
	}

	std::size_t BinaryObject::size()
	{
		synchronize();
		return _root.size;
	}

	std::span<std::uint8_t> BinaryObject::buffer()
	{
		synchronize();
		return std::span<std::uint8_t>(_buffer.data(), _buffer.size());
	}
}