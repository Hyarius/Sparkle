#pragma once

#include <cstdint>
#include <limits>
#include <span>
#include <stdexcept>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "type/spk_concepts.hpp"

namespace spk
{
	template <typename TVertex>
		requires spk::comparison_compatible<TVertex> && spk::hashable<TVertex>
	class GenericMesh
	{
	public:
		using Vertex = TVertex;
		using Index = std::uint32_t;
		using Shape = std::vector<Index>;

		struct Buffer
		{
			std::vector<Vertex> vertices;
			std::vector<Index> indexes;

			void clear()
			{
				vertices.clear();
				indexes.clear();
			}

			void reserve(std::size_t p_vertexCount, std::size_t p_indexCount)
			{
				vertices.reserve(p_vertexCount);
				indexes.reserve(p_indexCount);
			}
		};

	private:
		Buffer _buffer;
		std::unordered_map<Vertex, Index> _vertexLookup;
		std::vector<Shape> _shapes;

		void _ensureCanAppendVertices(std::size_t p_vertexCount) const
		{
			if (p_vertexCount == 0)
			{
				return;
			}

			const std::size_t maxIndex = static_cast<std::size_t>(std::numeric_limits<Index>::max());
			const std::size_t currentVertexCount = _buffer.vertices.size();

			if (currentVertexCount > maxIndex || p_vertexCount - 1 > maxIndex - currentVertexCount)
			{
				throw std::overflow_error("spk::GenericMesh: too many vertices for uint32_t indexes");
			}
		}

		[[nodiscard]] Index _indexFromVertexIndex(std::size_t p_vertexIndex) const
		{
			return static_cast<Index>(p_vertexIndex);
		}

		[[nodiscard]] std::size_t _countMissingVertices(std::span<const Vertex> p_vertices) const
		{
			std::unordered_set<Vertex> seenNew;
			std::size_t count = 0;

			for (const Vertex& vertex : p_vertices)
			{
				if (_vertexLookup.count(vertex) == 0 && seenNew.insert(vertex).second)
				{
					++count;
				}
			}

			return count;
		}

		[[nodiscard]] Index _findOrAddVertex(const Vertex& p_vertex)
		{
			auto it = _vertexLookup.find(p_vertex);
			if (it != _vertexLookup.end())
			{
				return it->second;
			}

			const Index index = _indexFromVertexIndex(_buffer.vertices.size());
			_buffer.vertices.emplace_back(p_vertex);
			_vertexLookup.emplace(p_vertex, index);
			return index;
		}

		void _addTriangleIndexes(Index p_a, Index p_b, Index p_c)
		{
			_buffer.indexes.emplace_back(p_a);
			_buffer.indexes.emplace_back(p_b);
			_buffer.indexes.emplace_back(p_c);
		}

	public:
		GenericMesh() = default;
		GenericMesh(const GenericMesh&) = default;
		GenericMesh& operator=(const GenericMesh&) = default;
		GenericMesh(GenericMesh&&) noexcept = default;
		GenericMesh& operator=(GenericMesh&&) noexcept = default;
		~GenericMesh() = default;

		void clear()
		{
			_shapes.clear();
			_buffer.clear();
			_vertexLookup.clear();
		}

		void reserve(std::size_t p_vertexCount, std::size_t p_indexCount)
		{
			_buffer.reserve(p_vertexCount, p_indexCount);
		}

		void addShape(const Vertex& p_a, const Vertex& p_b, const Vertex& p_c)
		{
			const Vertex vertices[] = {p_a, p_b, p_c};

			addShape(std::span<const Vertex>(vertices, 3));
		}

		void addShape(const Vertex& p_a, const Vertex& p_b, const Vertex& p_c, const Vertex& p_d)
		{
			const Vertex vertices[] = {p_a, p_b, p_c, p_d};

			addShape(std::span<const Vertex>(vertices, 4));
		}

		void addShape(std::span<const Vertex> p_vertices)
		{
			if (p_vertices.size() < 3)
			{
				return;
			}

			_ensureCanAppendVertices(_countMissingVertices(p_vertices));

			Shape shape;
			shape.reserve(p_vertices.size());

			for (const Vertex& vertex : p_vertices)
			{
				shape.emplace_back(_findOrAddVertex(vertex));
			}

			const Index firstIndex = shape[0];

			for (std::size_t i = 1; i + 1 < shape.size(); ++i)
			{
				_addTriangleIndexes(firstIndex, shape[i], shape[i + 1]);
			}

			_shapes.emplace_back(std::move(shape));
		}

		void addShape(const std::vector<Vertex>& p_vertices)
		{
			addShape(std::span<const Vertex>(p_vertices.data(), p_vertices.size()));
		}

		[[nodiscard]] std::size_t nbShape() const
		{
			return _shapes.size();
		}

		[[nodiscard]] const std::vector<Shape>& shapes() const
		{
			return _shapes;
		}
		
		[[nodiscard]] const Buffer& buffer() const
		{
			return _buffer;
		}
	};
}