#include "structures/graphics/spk_layout_buffer_object.hpp"

#include <algorithm>
#include <stdexcept>
#include <string>

namespace spk
{
	std::size_t LayoutBufferObject::Attribute::typeSize(Type p_type)
	{
		return static_cast<std::size_t>(componentCount(p_type)) * sizeof(float);
	}

	GLint LayoutBufferObject::Attribute::componentCount(Type p_type)
	{
		switch (p_type)
		{
		case Type::Float:
		case Type::Int:
		case Type::UInt:
			return 1;
		case Type::Vector2:
		case Type::Vector2Int:
		case Type::Vector2UInt:
			return 2;
		case Type::Vector3:
		case Type::Vector3Int:
		case Type::Vector3UInt:
			return 3;
		case Type::Vector4:
		case Type::Vector4Int:
		case Type::Vector4UInt:
			return 4;
		}
		throw std::runtime_error("spk::LayoutBufferObject::Attribute has an unsupported component count");
	}

	GLenum LayoutBufferObject::Attribute::componentType(Type p_type)
	{
		switch (p_type)
		{
		case Type::Float:
		case Type::Vector2:
		case Type::Vector3:
		case Type::Vector4:
			return GL_FLOAT;
		case Type::Int:
		case Type::Vector2Int:
		case Type::Vector3Int:
		case Type::Vector4Int:
			return GL_INT;
		case Type::UInt:
		case Type::Vector2UInt:
		case Type::Vector3UInt:
		case Type::Vector4UInt:
			return GL_UNSIGNED_INT;
		}
		throw std::runtime_error("spk::LayoutBufferObject::Attribute has an unsupported component type");
	}

	LayoutBufferObject::LayoutBufferObject() :
		_vertexBuffer(std::make_shared<spk::VertexBufferObject>()),
		_indexBuffer(std::make_shared<spk::IndexBufferObject>()),
		_vertexArray(std::make_shared<spk::VertexArrayObject>())
	{
		_vertexArray->setIndexBuffer(_indexBuffer);
	}

	LayoutBufferObject::LayoutBufferObject(const LayoutBufferObject &p_other) :
		LayoutBufferObject()
	{
		_attributes = p_other._attributes;
		_vertexSize = p_other._vertexSize;
		setVertexBytes(p_other._vertexBuffer->data(), p_other._vertexBuffer->size());
		_indexBuffer->setElementType(p_other._indexBuffer->elementType());
		appendIndexes(p_other.indexesData());
		_rebuildVertexLayout();
	}

	LayoutBufferObject &LayoutBufferObject::operator=(const LayoutBufferObject &p_other)
	{
		if (this != &p_other)
		{
			_attributes = p_other._attributes;
			_vertexSize = p_other._vertexSize;
			setVertexBytes(p_other._vertexBuffer->data(), p_other._vertexBuffer->size());
			_indexBuffer->setElementType(p_other._indexBuffer->elementType());
			_indexBuffer->clear();
			appendIndexes(p_other.indexesData());
			_rebuildVertexLayout();
		}
		return *this;
	}

	LayoutBufferObject::LayoutBufferObject(std::span<const Attribute> p_attributes) :
		LayoutBufferObject()
	{
		for (const Attribute &attribute : p_attributes)
		{
			_appendAttribute(attribute);
		}
		_rebuildVertexLayout();
	}

	LayoutBufferObject::LayoutBufferObject(std::initializer_list<Attribute> p_attributes) :
		LayoutBufferObject(std::span<const Attribute>(p_attributes.begin(), p_attributes.size()))
	{
	}

	void LayoutBufferObject::_appendAttribute(const Attribute &p_attribute)
	{
		if (hasAttribute(p_attribute.index) == true)
		{
			throw std::runtime_error("spk::LayoutBufferObject already contains attribute index " + std::to_string(p_attribute.index));
		}

		_attributes.push_back(p_attribute);
		_vertexSize += Attribute::typeSize(p_attribute.type);
	}

	void LayoutBufferObject::_rebuildVertexLayout()
	{
		_vertexArray->clearVertexBuffers();

		std::size_t offset = 0;
		for (const Attribute &attribute : _attributes)
		{
			_vertexArray->addVertexBuffer(
				_vertexBuffer,
				spk::VertexArrayObject::Attribute{
					.index = attribute.index,
					.componentCount = Attribute::componentCount(attribute.type),
					.componentType = Attribute::componentType(attribute.type),
					.normalized = attribute.normalized,
					.stride = static_cast<GLsizei>(_vertexSize),
					.offset = offset});
			offset += Attribute::typeSize(attribute.type);
		}
	}

	void LayoutBufferObject::clearAttributes()
	{
		_attributes.clear();
		_vertexSize = 0;
		_rebuildVertexLayout();
	}

	void LayoutBufferObject::addAttribute(const Attribute &p_attribute)
	{
		_appendAttribute(p_attribute);
		_rebuildVertexLayout();
	}

	void LayoutBufferObject::addAttribute(Attribute::Index p_index, Attribute::Type p_type, bool p_normalized)
	{
		addAttribute(Attribute{.index = p_index, .type = p_type, .normalized = p_normalized});
	}

	bool LayoutBufferObject::hasAttribute(Attribute::Index p_index) const
	{
		return std::ranges::any_of(
			_attributes,
			[p_index](const Attribute &p_attribute) {
				return p_attribute.index == p_index;
			});
	}

	std::size_t LayoutBufferObject::vertexSize() const noexcept
	{
		return _vertexSize;
	}

	std::size_t LayoutBufferObject::vertexCount() const noexcept
	{
		return _vertexCount;
	}

	std::size_t LayoutBufferObject::indexCount() const noexcept
	{
		return _indexBuffer->count();
	}

	bool LayoutBufferObject::isIndexed() const noexcept
	{
		return indexCount() != 0;
	}

	void LayoutBufferObject::setVertexBytes(const void *p_data, std::size_t p_size)
	{
		if (_vertexSize == 0 && p_size != 0)
		{
			throw std::runtime_error("spk::LayoutBufferObject requires at least one attribute before vertex upload");
		}
		if (_vertexSize != 0 && p_size % _vertexSize != 0)
		{
			throw std::runtime_error(
				"spk::LayoutBufferObject vertex data size [" + std::to_string(p_size) +
				"] is not aligned with its vertex layout [" + std::to_string(_vertexSize) + "]");
		}

		_vertexBuffer->clear();
		if (p_size != 0)
		{
			_vertexBuffer->append(p_data, p_size);
		}
		_vertexCount = _vertexSize == 0 ? 0 : p_size / _vertexSize;
	}

	void LayoutBufferObject::appendVertexBytes(const void *p_data, std::size_t p_size)
	{
		if (_vertexSize == 0 && p_size != 0)
		{
			throw std::runtime_error("spk::LayoutBufferObject requires at least one attribute before vertex upload");
		}
		if (_vertexSize != 0 && p_size % _vertexSize != 0)
		{
			throw std::runtime_error(
				"spk::LayoutBufferObject vertex data size [" + std::to_string(p_size) +
				"] is not aligned with its vertex layout [" + std::to_string(_vertexSize) + "]");
		}

		if (p_size != 0)
		{
			_vertexBuffer->append(p_data, p_size);
		}
		_vertexCount = _vertexSize == 0 ? 0 : _vertexBuffer->size() / _vertexSize;
	}

	void LayoutBufferObject::setIndexes(std::span<const std::uint32_t> p_indexes)
	{
		_indexBuffer->clear();
		_indexBuffer->setElementType(GL_UNSIGNED_INT);
		if (p_indexes.empty() == false)
		{
			_indexBuffer->append(p_indexes.data(), p_indexes.size_bytes());
		}
	}

	void LayoutBufferObject::appendIndexes(std::span<const std::uint32_t> p_indexes)
	{
		_indexBuffer->setElementType(GL_UNSIGNED_INT);
		if (p_indexes.empty() == false)
		{
			_indexBuffer->append(p_indexes.data(), p_indexes.size_bytes());
		}
	}

	[[nodiscard]] spk::VertexBufferObject &LayoutBufferObject::vertices() const
	{
		return *_vertexBuffer;
	}

	[[nodiscard]] spk::IndexBufferObject &LayoutBufferObject::indexes() const
	{
		return *_indexBuffer;
	}

	std::span<const std::uint32_t> LayoutBufferObject::indexesData() const
	{
		const std::span<const std::uint8_t> bytes = _indexBuffer->bytes();
		if (bytes.empty() == true)
		{
			return {};
		}
		return std::span<const std::uint32_t>(
			reinterpret_cast<const std::uint32_t *>(bytes.data()),
			bytes.size() / sizeof(std::uint32_t));
	}

	void LayoutBufferObject::activate(const spk::RenderContext &p_context) const
	{
		_vertexArray->activate(p_context);
	}

	void LayoutBufferObject::deactivate() const
	{
		_vertexArray->deactivate();
	}
}
