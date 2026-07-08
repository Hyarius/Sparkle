#pragma once

#include "structures/math/spk_vector3.hpp"

#include <cstddef>
#include <optional>
#include <span>
#include <stdexcept>
#include <utility>
#include <vector>

namespace spk
{
	template <typename TData>
	class Polygon3D
	{
	public:
		struct Vertex
		{
			spk::Vector3 position{};
			TData data{};
		};

		class Builder
		{
		private:
			std::vector<Vertex> _vertices;
			std::optional<spk::Vector3> _normal;

			[[nodiscard]] static spk::Vector3 _computeNormal(
				const spk::Vector3 &p_first,
				const spk::Vector3 &p_second,
				const spk::Vector3 &p_third)
			{
				const spk::Vector3 normal =
					(p_second - p_first).cross(p_third - p_first);

				if (spk::ApproxValue(normal.squaredLength()) == 0.0f)
				{
					throw std::logic_error("Polygon3D cannot be built from collinear vertices");
				}

				return normal.normalized();
			}

			void _validateOnPlane(const spk::Vector3 &p_position) const
			{
				if (_normal.has_value() == false)
				{
					return;
				}

				const spk::Vector3 &origin = _vertices[0].position;

				const float signedDistance =
					(p_position - origin).dot(_normal.value());

				if (spk::ApproxValue(signedDistance) != 0.0f)
				{
					throw std::logic_error("Polygon3D vertex is not on the polygon plane");
				}
			}

			void _addVertex(Vertex p_vertex)
			{
				if (_vertices.size() < 2)
				{
					_vertices.push_back(std::move(p_vertex));
					return;
				}

				if (_vertices.size() == 2)
				{
					_normal = _computeNormal(
						_vertices[0].position,
						_vertices[1].position,
						p_vertex.position);

					_vertices.push_back(std::move(p_vertex));
					return;
				}

				_validateOnPlane(p_vertex.position);
				_vertices.push_back(std::move(p_vertex));
			}

		public:
			Builder() = default;

			Builder &reserve(std::size_t p_capacity)
			{
				_vertices.reserve(p_capacity);
				return *this;
			}

			Builder &addVertex(const Vertex &p_vertex)
			{
				_addVertex(p_vertex);
				return *this;
			}

			Builder &addVertex(Vertex &&p_vertex)
			{
				_addVertex(std::move(p_vertex));
				return *this;
			}

			[[nodiscard]] Polygon3D bake()
			{
				if (_vertices.size() < 3)
				{
					throw std::logic_error("Polygon3D requires at least three vertices");
				}

				return Polygon3D(std::move(_vertices), _normal.value());
			}
		};

	private:
		std::vector<Vertex> _vertices;
		spk::Vector3 _normal;

		explicit Polygon3D(
			std::vector<Vertex> p_vertices,
			spk::Vector3 p_normal) :
			_vertices(std::move(p_vertices)),
			_normal(p_normal)
		{
		}

	public:
		Polygon3D(const Polygon3D &) = default;
		Polygon3D &operator=(const Polygon3D &) = default;
		Polygon3D(Polygon3D &&) noexcept = default;
		Polygon3D &operator=(Polygon3D &&) noexcept = default;

		[[nodiscard]] std::span<const Vertex> vertices() const noexcept
		{
			return _vertices;
		}

		[[nodiscard]] const spk::Vector3 &normal() const noexcept
		{
			return _normal;
		}

		[[nodiscard]] std::size_t size() const noexcept
		{
			return _vertices.size();
		}

		[[nodiscard]] const Vertex &operator[](std::size_t p_index) const noexcept
		{
			return _vertices[p_index];
		}

		[[nodiscard]] auto begin() const noexcept
		{
			return _vertices.begin();
		}

		[[nodiscard]] auto end() const noexcept
		{
			return _vertices.end();
		}
	};
}
