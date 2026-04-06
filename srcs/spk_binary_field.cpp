#include "spk_binary_field.hpp"
#include "spk_binary_object.hpp"

#include <cstring>
#include <memory>
#include <stdexcept>

namespace spk
{
	BinaryField::BinaryField(BinaryObject* p_owner, _binary_detail::Node* p_node) :
		_owner(p_owner),
		_node(p_node)
	{
	}

	void BinaryField::_throwIfInvalid() const
	{
		if (_owner == nullptr || _node == nullptr)
		{
			throw std::runtime_error("Invalid BinaryField handle.");
		}
	}

	bool BinaryField::isValid() const
	{
		return (_owner != nullptr && _node != nullptr);
	}

	BinaryFieldKind BinaryField::kind() const
	{
		_throwIfInvalid();
		return _node->kind;
	}

	bool BinaryField::isValue() const
	{
		return kind() == BinaryFieldKind::Value;
	}

	bool BinaryField::isObject() const
	{
		return kind() == BinaryFieldKind::Object;
	}

	bool BinaryField::isArray() const
	{
		return kind() == BinaryFieldKind::Array;
	}

	std::string_view BinaryField::name() const
	{
		_throwIfInvalid();
		return _node->name;
	}

	std::size_t BinaryField::size() const
	{
		_throwIfInvalid();
		_owner->synchronize();
		return _node->size;
	}

	std::size_t BinaryField::offset() const
	{
		_throwIfInvalid();
		_owner->synchronize();
		return _node->offset;
	}

	std::size_t BinaryField::alignment() const
	{
		_throwIfInvalid();
		_owner->synchronize();
		return _node->alignment;
	}

	std::size_t BinaryField::count() const
	{
		_throwIfInvalid();
		_owner->synchronize();
		return _node->count;
	}

	std::size_t BinaryField::stride() const
	{
		_throwIfInvalid();
		_owner->synchronize();
		return _node->stride;
	}

	BinaryField BinaryField::operator[](std::string_view p_name)
	{
		_throwIfInvalid();

		if (_node->kind != BinaryFieldKind::Object)
		{
			throw std::runtime_error("BinaryField::operator[](name) can only be used on object fields.");
		}

		_binary_detail::Node* child = BinaryObject::_findChildByName(_node, p_name);
		if (child == nullptr)
		{
			throw std::runtime_error("BinaryField::operator[](name) could not find the requested child field.");
		}

		return BinaryField(_owner, child);
	}

	BinaryField BinaryField::operator[](std::size_t p_index)
	{
		_throwIfInvalid();

		if (_node->kind != BinaryFieldKind::Array)
		{
			throw std::runtime_error("BinaryField::operator[](index) can only be used on array fields.");
		}

		if (p_index >= _node->children.size())
		{
			throw std::runtime_error("BinaryField::operator[](index) received an out of range index.");
		}

		return BinaryField(_owner, _node->children[p_index].get());
	}

	BinaryField BinaryField::addValue(std::string_view p_name, std::size_t p_size)
	{
		_throwIfInvalid();

		if (_node->kind != BinaryFieldKind::Object)
		{
			throw std::runtime_error("BinaryField::addValue can only be used on object fields.");
		}

		if (p_size == 0)
		{
			throw std::runtime_error("BinaryField::addValue received a zero size.");
		}

		_binary_detail::Node* node = _owner->_appendChild(_node, p_name, BinaryFieldKind::Value);
		node->valueSize = p_size;
		return BinaryField(_owner, node);
	}

	BinaryField BinaryField::addArray(std::string_view p_name, std::size_t p_count, std::size_t p_elementSize)
	{
		_throwIfInvalid();

		if (_node->kind != BinaryFieldKind::Object)
		{
			throw std::runtime_error("BinaryField::addArray can only be used on object fields.");
		}

		if (p_elementSize == 0)
		{
			throw std::runtime_error("BinaryField::addArray received a zero element size.");
		}

		_binary_detail::Node* node = _owner->_appendChild(_node, p_name, BinaryFieldKind::Array);
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

		_owner->requestSynchronization();
		return BinaryField(_owner, node);
	}

	BinaryField BinaryField::addObject(std::string_view p_name)
	{
		_throwIfInvalid();

		if (_node->kind != BinaryFieldKind::Object)
		{
			throw std::runtime_error("BinaryField::addObject can only be used on object fields.");
		}

		_binary_detail::Node* node = _owner->_appendChild(_node, p_name, BinaryFieldKind::Object);
		return BinaryField(_owner, node);
	}

	BinaryField& BinaryField::resize(std::size_t p_count)
	{
		_throwIfInvalid();

		if (_node->kind != BinaryFieldKind::Array)
		{
			throw std::runtime_error("BinaryField::resize can only be used on array fields.");
		}

		const std::size_t currentCount = _node->children.size();

		if (p_count > currentCount)
		{
			for (std::size_t index = currentCount; index < p_count; ++index)
			{
				std::unique_ptr<_binary_detail::Node> child = std::make_unique<_binary_detail::Node>();
				child->name = std::to_string(index);
				child->kind = BinaryFieldKind::Value;
				child->valueSize = _node->elementSize;
				child->parent = _node;
				_node->children.push_back(std::move(child));
			}
		}
		else if (p_count < currentCount)
		{
			_node->children.resize(p_count);
		}

		_owner->requestSynchronization();
		return (*this);
	}

	std::span<std::uint8_t> BinaryField::bytes()
	{
		_throwIfInvalid();
		_owner->synchronize();
		return std::span<std::uint8_t>(_owner->_buffer.data() + _node->offset, _node->size);
	}

	std::span<const std::uint8_t> BinaryField::bytes() const
	{
		_throwIfInvalid();
		_owner->synchronize();
		return std::span<const std::uint8_t>(_owner->_buffer.data() + _node->offset, _node->size);
	}
}