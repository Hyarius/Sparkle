#pragma once

#include "structures/math/spk_vector3.hpp"

#include <cmath>
#include <cstddef>
#include <functional>
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

		[[nodiscard]] static bool _samePosition(
			const spk::Vector3 &p_left,
			const spk::Vector3 &p_right,
			float p_epsilon) noexcept
		{
			const spk::Vector3 difference = p_left - p_right;
			return difference.squaredLength() <= p_epsilon * p_epsilon;
		}

		[[nodiscard]] static float _turn(
			const spk::Vector3 &p_previous,
			const spk::Vector3 &p_current,
			const spk::Vector3 &p_next,
			const spk::Vector3 &p_normal) noexcept
		{
			return (p_current - p_previous).cross(p_next - p_current).dot(p_normal);
		}

		[[nodiscard]] static float _area(
			std::span<const Vertex> p_vertices,
			const spk::Vector3 &p_normal) noexcept
		{
			spk::Vector3 doubledArea{};
			for (std::size_t index = 0; index < p_vertices.size(); ++index)
			{
				doubledArea += p_vertices[index].position.cross(p_vertices[(index + 1) % p_vertices.size()].position);
			}
			return std::abs(doubledArea.dot(p_normal)) * 0.5f;
		}

		static void _appendDistinct(std::vector<Vertex> &p_vertices, Vertex p_vertex, float p_epsilon)
		{
			if (p_vertices.empty() || !_samePosition(p_vertices.back().position, p_vertex.position, p_epsilon))
			{
				p_vertices.push_back(std::move(p_vertex));
			}
		}

		[[nodiscard]] static std::vector<Vertex> _cleanVertices(
			std::vector<Vertex> p_vertices,
			const spk::Vector3 &p_normal,
			float p_epsilon)
		{
			if (p_vertices.size() > 1 && _samePosition(p_vertices.front().position, p_vertices.back().position, p_epsilon))
			{
				p_vertices.pop_back();
			}

			bool changed = true;
			while (changed && p_vertices.size() >= 3)
			{
				changed = false;
				for (std::size_t index = 0; index < p_vertices.size(); ++index)
				{
					const Vertex &previous = p_vertices[(index + p_vertices.size() - 1) % p_vertices.size()];
					const Vertex &current = p_vertices[index];
					const Vertex &next = p_vertices[(index + 1) % p_vertices.size()];
					if (_samePosition(previous.position, current.position, p_epsilon) ||
						std::abs(_turn(previous.position, current.position, next.position, p_normal)) <= p_epsilon)
					{
						p_vertices.erase(p_vertices.begin() + static_cast<std::ptrdiff_t>(index));
						changed = true;
						break;
					}
				}
			}
			return p_vertices;
		}

		template <typename TInterpolator>
		[[nodiscard]] static std::vector<Vertex> _clipHalfPlane(
			std::span<const Vertex> p_vertices,
			const spk::Vector3 &p_edgeStart,
			const spk::Vector3 &p_edgeEnd,
			const spk::Vector3 &p_clipNormal,
			const spk::Vector3 &p_resultNormal,
			bool p_keepInside,
			TInterpolator &p_interpolateData,
			float p_epsilon)
		{
			std::vector<Vertex> result;
			if (p_vertices.empty())
			{
				return result;
			}

			const auto distance = [&](const Vertex &p_vertex) {
				return (p_edgeEnd - p_edgeStart).cross(p_vertex.position - p_edgeStart).dot(p_clipNormal);
			};
			const auto accepted = [p_keepInside, p_epsilon](float p_distance) {
				return p_keepInside ? p_distance >= -p_epsilon : p_distance <= p_epsilon;
			};

			Vertex previous = p_vertices.back();
			float previousDistance = distance(previous);
			bool previousAccepted = accepted(previousDistance);
			for (const Vertex &current : p_vertices)
			{
				const float currentDistance = distance(current);
				const bool currentAccepted = accepted(currentDistance);
				if (currentAccepted != previousAccepted)
				{
					const float denominator = previousDistance - currentDistance;
					if (std::abs(denominator) > p_epsilon)
					{
						const float interpolation = previousDistance / denominator;
						_appendDistinct(
							result,
							{
								.position = previous.position + (current.position - previous.position) * interpolation,
								.data = std::invoke(p_interpolateData, previous.data, current.data, interpolation)},
							p_epsilon);
					}
				}
				if (currentAccepted)
				{
					_appendDistinct(result, current, p_epsilon);
				}
				previous = current;
				previousDistance = currentDistance;
				previousAccepted = currentAccepted;
			}
			return _cleanVertices(std::move(result), p_resultNormal, p_epsilon);
		}

		[[nodiscard]] static std::optional<Polygon3D> _fromClippedVertices(
			std::vector<Vertex> p_vertices,
			const spk::Vector3 &p_normal,
			float p_epsilon)
		{
			p_vertices = _cleanVertices(std::move(p_vertices), p_normal, p_epsilon);
			if (p_vertices.size() < 3 || _area(p_vertices, p_normal) <= p_epsilon)
			{
				return std::nullopt;
			}
			return Polygon3D(std::move(p_vertices), p_normal);
		}

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

		[[nodiscard]] float area() const noexcept
		{
			return _area(_vertices, _normal);
		}

		[[nodiscard]] bool isConvex(float p_epsilon) const noexcept
		{
			if (_vertices.size() < 3 || area() <= p_epsilon)
			{
				return false;
			}

			float turn = 0.0f;
			for (std::size_t index = 0; index < _vertices.size(); ++index)
			{
				const float current = _turn(
					_vertices[index].position,
					_vertices[(index + 1) % _vertices.size()].position,
					_vertices[(index + 2) % _vertices.size()].position,
					_normal);
				if (std::abs(current) <= p_epsilon)
				{
					continue;
				}
				if (turn != 0.0f && current * turn < 0.0f)
				{
					return false;
				}
				turn = current;
			}
			return turn != 0.0f;
		}

		// Subtracts the orthogonal footprint of a convex polygon on a parallel plane
		// from this convex polygon. Intersection vertices interpolate their payload
		// through the caller-provided function.
		template <typename TInterpolator>
		[[nodiscard]] std::vector<Polygon3D> subtractConvex(
			const Polygon3D &p_clip,
			TInterpolator p_interpolateData,
			float p_epsilon) const
		{
			if (!isConvex(p_epsilon) || !p_clip.isConvex(p_epsilon))
			{
				throw std::invalid_argument("Polygon3D convex subtraction requires two convex polygons");
			}
			if (std::abs(std::abs(_normal.dot(p_clip._normal)) - 1.0f) > p_epsilon)
			{
				throw std::invalid_argument("Polygon3D convex subtraction requires parallel polygon planes");
			}

			std::vector<Polygon3D> result;
			std::vector<Vertex> remaining = _vertices;
			for (std::size_t edge = 0; edge < p_clip._vertices.size(); ++edge)
			{
				const spk::Vector3 &start = p_clip._vertices[edge].position;
				const spk::Vector3 &end = p_clip._vertices[(edge + 1) % p_clip._vertices.size()].position;
				if (std::optional<Polygon3D> outside = _fromClippedVertices(
						_clipHalfPlane(remaining, start, end, p_clip._normal, _normal, false, p_interpolateData, p_epsilon),
						_normal,
						p_epsilon))
				{
					result.push_back(std::move(*outside));
				}

				remaining = _clipHalfPlane(remaining, start, end, p_clip._normal, _normal, true, p_interpolateData, p_epsilon);
				if (remaining.size() < 3 || _area(remaining, _normal) <= p_epsilon)
				{
					break;
				}
			}
			return result;
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
