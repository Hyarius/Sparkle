#pragma once

#include <cstddef>
#include <cstdint>
#include <initializer_list>
#include <memory>
#include <span>
#include <vector>

#include <GL/glew.h>

#include "structures/graphics/spk_index_buffer_object.hpp"
#include "structures/graphics/spk_vertex_array_object.hpp"
#include "structures/graphics/spk_vertex_buffer_object.hpp"

namespace spk
{
	class RenderContext;

	class LayoutBufferObject
	{
	public:
		struct Attribute
		{
			using Index = GLuint;

			enum class Type
			{
				Float,
				Int,
				UInt,
				Vector2,
				Vector3,
				Vector4,
				Vector2Int,
				Vector3Int,
				Vector4Int,
				Vector2UInt,
				Vector3UInt,
				Vector4UInt
			};

			Index index = 0;
			Type type = Type::Float;
			bool normalized = false;

			[[nodiscard]] static std::size_t typeSize(Type p_type);
			[[nodiscard]] static GLint componentCount(Type p_type);
			[[nodiscard]] static GLenum componentType(Type p_type);
		};

	private:
		std::shared_ptr<spk::VertexBufferObject> _vertexBuffer;
		std::shared_ptr<spk::IndexBufferObject> _indexBuffer;
		std::shared_ptr<spk::VertexArrayObject> _vertexArray;
		std::vector<Attribute> _attributes;
		std::size_t _vertexSize = 0;
		std::size_t _vertexCount = 0;

		void _rebuildVertexArray();

	public:
		LayoutBufferObject();
		LayoutBufferObject(const LayoutBufferObject &p_other);
		LayoutBufferObject &operator=(const LayoutBufferObject &p_other);
		LayoutBufferObject(LayoutBufferObject &&) noexcept = default;
		LayoutBufferObject &operator=(LayoutBufferObject &&) noexcept = default;
		explicit LayoutBufferObject(std::span<const Attribute> p_attributes);
		LayoutBufferObject(std::initializer_list<Attribute> p_attributes);

		void clearAttributes();
		void addAttribute(const Attribute &p_attribute);
		void addAttribute(Attribute::Index p_index, Attribute::Type p_type, bool p_normalized = false);

		[[nodiscard]] bool hasAttribute(Attribute::Index p_index) const;
		[[nodiscard]] std::size_t vertexSize() const noexcept;
		[[nodiscard]] std::size_t vertexCount() const noexcept;
		[[nodiscard]] std::size_t indexCount() const noexcept;
		[[nodiscard]] bool isIndexed() const noexcept;

		void setVertexBytes(const void *p_data, std::size_t p_size);
		void appendVertexBytes(const void *p_data, std::size_t p_size);

		template <typename TVertex>
		void setVertices(std::span<const TVertex> p_vertices)
		{
			if (p_vertices.empty() == true)
			{
				setVertexBytes(nullptr, 0);
				return;
			}
			setVertexBytes(p_vertices.data(), p_vertices.size_bytes());
		}

		template <typename TVertex>
		void appendVertices(std::span<const TVertex> p_vertices)
		{
			if (p_vertices.empty() == true)
			{
				return;
			}
			appendVertexBytes(p_vertices.data(), p_vertices.size_bytes());
		}

		template <typename TVertex>
		void appendVertex(const TVertex &p_vertex)
		{
			appendVertices(std::span<const TVertex>(&p_vertex, 1));
		}

		template <typename TVertex>
		[[nodiscard]] std::span<const TVertex> vertices() const
		{
			const std::span<const std::uint8_t> bytes = _vertexBuffer->bytes();
			if (bytes.empty() == true)
			{
				return {};
			}
			return std::span<const TVertex>(
				reinterpret_cast<const TVertex *>(bytes.data()),
				bytes.size() / sizeof(TVertex));
		}

		void setIndexes(std::span<const std::uint32_t> p_indexes);
		void appendIndexes(std::span<const std::uint32_t> p_indexes);

		[[nodiscard]] spk::VertexBufferObject &vertices() const;
		[[nodiscard]] spk::IndexBufferObject &indexes() const;
		[[nodiscard]] std::span<const std::uint32_t> indexesData() const;

		void activate(const spk::RenderContext &p_context) const;
		void deactivate() const;
	};
}
