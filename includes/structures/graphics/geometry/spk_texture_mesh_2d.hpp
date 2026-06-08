#pragma once

#include <cstdint>
#include <functional>
#include <span>
#include <utility>
#include <vector>

#include "structures/math/spk_vector2.hpp"
#include "structures/math/spk_vector3.hpp"

namespace spk
{
	struct TextureVertex2D
	{
		spk::Vector3 position{};
		spk::Vector2 uv{};

		[[nodiscard]] bool operator==(const TextureVertex2D& p_other) const noexcept
		{
			return position == p_other.position && uv == p_other.uv;
		}
	};
}

namespace std
{
	template <>
	struct hash<spk::TextureVertex2D>
	{
		std::size_t operator()(const spk::TextureVertex2D& p_value) const noexcept
		{
			std::size_t h = std::hash<float>{}(p_value.position.x);
			h ^= std::hash<float>{}(p_value.position.y) + 0x9e3779b9u + (h << 6) + (h >> 2);
			h ^= std::hash<float>{}(p_value.position.z) + 0x9e3779b9u + (h << 6) + (h >> 2);
			h ^= std::hash<spk::Vector2>{}(p_value.uv) + 0x9e3779b9u + (h << 6) + (h >> 2);
			return h;
		}
	};
}

namespace spk
{
	class TextureMesh2D
	{
	public:
		using Vertex = spk::TextureVertex2D;
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
		std::vector<Shape> _shapes;

		[[nodiscard]] Index _appendVertex(const Vertex& p_vertex)
		{
			const Index result = static_cast<Index>(_buffer.vertices.size());
			_buffer.vertices.emplace_back(p_vertex);
			return result;
		}

	public:
		TextureMesh2D() = default;

		void clear()
		{
			_buffer.clear();
			_shapes.clear();
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

			Shape shape;
			shape.reserve(p_vertices.size());
			for (const Vertex& vertex : p_vertices)
			{
				shape.emplace_back(_appendVertex(vertex));
			}

			const Index firstIndex = shape[0];
			for (std::size_t i = 1; i + 1 < shape.size(); ++i)
			{
				_buffer.indexes.emplace_back(firstIndex);
				_buffer.indexes.emplace_back(shape[i]);
				_buffer.indexes.emplace_back(shape[i + 1]);
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
