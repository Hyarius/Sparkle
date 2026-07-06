#pragma once

#include <array>
#include <cstdint>
#include <limits>
#include <span>
#include <stdexcept>
#include <type_traits>
#include <utility>
#include <vector>

#include "structures/container/spk_cached_data.hpp"
#include "structures/graphics/spk_buffer_object.hpp"
#include "structures/graphics/spk_layout_buffer_object.hpp"

namespace spk
{
	template <spk::LayoutBufferObject::Attribute... TAttributes>
	struct MeshLayout
	{
		inline static constexpr std::array Attributes = {TAttributes...};
	};

	template <typename TLayout>
	concept mesh_layout =
		requires {
			{ std::span<const spk::LayoutBufferObject::Attribute>(TLayout::Attributes) } -> std::same_as<std::span<const spk::LayoutBufferObject::Attribute>>;
			requires(std::span<const spk::LayoutBufferObject::Attribute>(TLayout::Attributes).empty() == false);
		};

	template <typename TVertex, typename TLayout>
		requires std::is_trivially_copyable_v<TVertex> && spk::mesh_layout<TLayout>
	class GenericMesh
	{
	public:
		using Vertex = TVertex;
		using Index = std::uint32_t;
		using Shape = std::vector<Index>;

		class Builder;

	private:
		class HashKey
		{
		private:
			mutable const spk::BufferObject *_vertexSource = nullptr;
			mutable const spk::BufferObject *_indexSource = nullptr;
			mutable spk::CachedData<std::uint32_t> _cache;

			void _configure()
			{
				_cache.configure([this]() {
					return _compute();
				});
			}

			[[nodiscard]] std::uint32_t _compute() const
			{
				std::uint32_t hash = 2166136261u; // FNV-1a (32-bit)

				const auto mix = [&hash](std::span<const std::uint8_t> p_bytes) {
					for (const std::uint8_t byte : p_bytes)
					{
						hash ^= static_cast<std::uint32_t>(byte);
						hash *= 16777619u;
					}
				};

				if (_vertexSource != nullptr)
				{
					mix(_vertexSource->bytes());
				}
				if (_indexSource != nullptr)
				{
					mix(_indexSource->bytes());
				}
				return hash;
			}

		public:
			HashKey()
			{
				_configure();
			}

			~HashKey() = default;

			HashKey(const HashKey &)
			{
				_configure();
			}

			HashKey &operator=(const HashKey &)
			{
				_vertexSource = nullptr;
				_indexSource = nullptr;
				_cache.release();
				return *this;
			}

			HashKey(HashKey &&) noexcept
			{
				_configure();
			}

			HashKey &operator=(HashKey &&) noexcept
			{
				_vertexSource = nullptr;
				_indexSource = nullptr;
				_cache.release();
				return *this;
			}

			[[nodiscard]] bool isBound() const
			{
				return _vertexSource != nullptr || _indexSource != nullptr;
			}

			void bind(const spk::BufferObject *p_vertexSource, const spk::BufferObject *p_indexSource) const
			{
				_vertexSource = p_vertexSource;
				_indexSource = p_indexSource;
				_cache.release();
			}

			void invalidate() const
			{
				_cache.release();
			}

			[[nodiscard]] std::uint32_t value() const
			{
				return _cache.get();
			}
		};

	protected:
		spk::LayoutBufferObject _layoutBuffer;

	private:
		std::size_t _shapeCount = 0;
		HashKey _hashKey;

		void _ensureCanAppendVertices(std::size_t p_vertexCount) const
		{
			if (p_vertexCount == 0)
			{
				return;
			}

			const std::size_t maxIndex = static_cast<std::size_t>(std::numeric_limits<Index>::max());
			const std::size_t currentVertexCount = _layoutBuffer.vertexCount();

			if (currentVertexCount > maxIndex || p_vertexCount - 1 > maxIndex - currentVertexCount)
			{
				throw std::overflow_error("spk::GenericMesh: too many vertices for uint32_t indexes");
			}
		}

		void _addTriangleIndexes(Index p_a, Index p_b, Index p_c)
		{
			const std::array<Index, 3> indexes = {p_a, p_b, p_c};
			_layoutBuffer.appendIndexes(indexes);
		}

	private:
		void clear()
		{
			_shapeCount = 0;
			_layoutBuffer.setVertices(std::span<const Vertex>{});
			_layoutBuffer.setIndexes(std::span<const Index>{});
			_hashKey.invalidate();
		}

		void reserve(std::size_t p_vertexCount, std::size_t p_indexCount)
		{
			_layoutBuffer.vertices().reserve(p_vertexCount * sizeof(TVertex));
			_layoutBuffer.indexes().reserve(p_indexCount * sizeof(Index));
		}

		void resize(std::size_t p_vertexCount, std::size_t p_indexCount, std::size_t p_shapeCount)
		{
			if (p_vertexCount > std::numeric_limits<std::size_t>::max() / sizeof(Vertex) ||
				p_indexCount > std::numeric_limits<std::size_t>::max() / sizeof(Index))
			{
				throw std::length_error("spk::GenericMesh: requested buffer size is too large");
			}
			if (p_vertexCount != 0 && p_vertexCount - 1 > static_cast<std::size_t>(std::numeric_limits<Index>::max()))
			{
				throw std::overflow_error("spk::GenericMesh: too many vertices for uint32_t indexes");
			}
			_layoutBuffer.vertices().resize(p_vertexCount * sizeof(Vertex));
			_layoutBuffer.indexes().resize(p_indexCount * sizeof(Index));
			_shapeCount = p_shapeCount;
			_hashKey.invalidate();
		}

		void validate() const
		{
			_layoutBuffer.vertices().markContentModified();
			_layoutBuffer.indexes().markContentModified();
			_hashKey.invalidate();
		}

		void addShape(const Vertex &p_a, const Vertex &p_b, const Vertex &p_c)
		{
			const std::array<Vertex, 3> vertices = {p_a, p_b, p_c};

			addShape(vertices);
		}

		void addShape(const Vertex &p_a, const Vertex &p_b, const Vertex &p_c, const Vertex &p_d)
		{
			const std::array<Vertex, 4> vertices = {p_a, p_b, p_c, p_d};

			addShape(vertices);
		}

		void addShape(std::span<const Vertex> p_vertices)
		{
			if (p_vertices.size() < 3)
			{
				throw std::runtime_error("Can't add a shape with less than 3 vertices in a mesh");
			}

			_ensureCanAppendVertices(p_vertices.size());
			const Index firstIndex = static_cast<Index>(_layoutBuffer.vertexCount());
			_layoutBuffer.appendVertices(p_vertices);

			for (std::size_t i = 1; i + 1 < p_vertices.size(); ++i)
			{
				_addTriangleIndexes(
					firstIndex,
					firstIndex + static_cast<Index>(i),
					firstIndex + static_cast<Index>(i + 1));
			}

			++_shapeCount;
			_hashKey.invalidate();
		}

		void addShape(const std::vector<Vertex> &p_vertices)
		{
			addShape(std::span<const Vertex>(p_vertices.data(), p_vertices.size()));
		}

	public:
		GenericMesh() :
			_layoutBuffer(TLayout::Attributes)
		{
		}

		GenericMesh(const GenericMesh &) = default;
		GenericMesh &operator=(const GenericMesh &) = default;
		GenericMesh(GenericMesh &&) noexcept = default;
		GenericMesh &operator=(GenericMesh &&) noexcept = default;
		~GenericMesh() = default;

		[[nodiscard]] std::size_t nbShape() const
		{
			return _shapeCount;
		}

		[[nodiscard]] std::span<const Vertex> vertices() const
		{
			return std::span<const Vertex>(reinterpret_cast<const Vertex *>(_layoutBuffer.vertices().data()), _layoutBuffer.vertices().size() / sizeof(Vertex));
		}

		[[nodiscard]] std::span<const Index> indexes() const
		{
			return std::span<const Index>(reinterpret_cast<const Index *>(_layoutBuffer.indexes().data()), _layoutBuffer.indexes().size() / sizeof(Index));
		}

		[[nodiscard]] const spk::LayoutBufferObject &layoutBuffer() const
		{
			return _layoutBuffer;
		}

		[[nodiscard]] std::uint32_t hashKey() const
		{
			if (_hashKey.isBound() == false)
			{
				_hashKey.bind(&_layoutBuffer.vertices(), &_layoutBuffer.indexes());
			}
			return _hashKey.value();
		}
	};

	template <typename TVertex, typename TLayout>
		requires std::is_trivially_copyable_v<TVertex> && spk::mesh_layout<TLayout>
	class GenericMesh<TVertex, TLayout>::Builder
	{
	private:
		GenericMesh _mesh;

	public:
		Builder() = default;

		Builder(const Builder &) = delete;
		Builder &operator=(const Builder &) = delete;
		Builder(Builder &&) noexcept = default;
		Builder &operator=(Builder &&) noexcept = default;
		~Builder() = default;

		void clear()
		{
			_mesh.clear();
		}

		void reserve(std::size_t p_vertexCount, std::size_t p_indexCount)
		{
			_mesh.reserve(p_vertexCount, p_indexCount);
		}

		void resize(std::size_t p_vertexCount, std::size_t p_indexCount, std::size_t p_shapeCount = 0)
		{
			_mesh.resize(p_vertexCount, p_indexCount, p_shapeCount);
		}

		[[nodiscard]] spk::VertexBufferObject &vertices() noexcept
		{
			return _mesh._layoutBuffer.vertices();
		}

		[[nodiscard]] spk::IndexBufferObject &indexes() noexcept
		{
			return _mesh._layoutBuffer.indexes();
		}

		void addShape(const Vertex &p_a, const Vertex &p_b, const Vertex &p_c)
		{
			_mesh.addShape(p_a, p_b, p_c);
		}

		void addShape(const Vertex &p_a, const Vertex &p_b, const Vertex &p_c, const Vertex &p_d)
		{
			_mesh.addShape(p_a, p_b, p_c, p_d);
		}

		void addShape(std::span<const Vertex> p_vertices)
		{
			_mesh.addShape(p_vertices);
		}

		void addShape(const std::vector<Vertex> &p_vertices)
		{
			_mesh.addShape(p_vertices);
		}

		[[nodiscard]] GenericMesh bake() const &
		{
			_mesh.validate();
			return _mesh;
		}

		[[nodiscard]] GenericMesh bake() &&
		{
			_mesh.validate();
			return std::move(_mesh);
		}
	};
}
