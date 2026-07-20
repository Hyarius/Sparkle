#pragma once

#include "structures/math/spk_vector2.hpp"

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
	class Polygon2D
	{
	public:
		struct Vertex
		{
			spk::Vector2 position{};
			TData data{};
		};

		class Builder
		{
		private:
			std::vector<Vertex> _vertices;

		public:
			Builder() = default;

			Builder &reserve(std::size_t p_capacity)
			{
				_vertices.reserve(p_capacity);
				return *this;
			}

			Builder &addVertex(const Vertex &p_vertex)
			{
				_vertices.push_back(p_vertex);
				return *this;
			}

			Builder &addVertex(Vertex &&p_vertex)
			{
				_vertices.push_back(std::move(p_vertex));
				return *this;
			}

			[[nodiscard]] Polygon2D bake()
			{
				if (_vertices.size() < 3)
				{
					throw std::logic_error("Polygon2D requires at least three vertices");
				}

				const float signedDoubledArea = Polygon2D::_signedDoubledArea(_vertices);
				if (spk::ApproxValue(signedDoubledArea) == 0.0f)
				{
					throw std::logic_error("Polygon2D cannot be built from a degenerate contour");
				}

				return Polygon2D(std::move(_vertices), signedDoubledArea > 0.0f ? 1.0f : -1.0f);
			}
		};

	private:
		std::vector<Vertex> _vertices;
		float _winding;

		[[nodiscard]] static bool _samePosition(
			const spk::Vector2 &p_left,
			const spk::Vector2 &p_right) noexcept
		{
			return p_left == p_right;
		}

		[[nodiscard]] static float _turn(
			const spk::Vector2 &p_previous,
			const spk::Vector2 &p_current,
			const spk::Vector2 &p_next) noexcept
		{
			return (p_current - p_previous).cross(p_next - p_current);
		}

		[[nodiscard]] static float _signedDoubledArea(std::span<const Vertex> p_vertices) noexcept
		{
			float result = 0.0f;
			for (std::size_t index = 0; index < p_vertices.size(); ++index)
			{
				result += p_vertices[index].position.cross(
					p_vertices[(index + 1) % p_vertices.size()].position);
			}
			return result;
		}

		[[nodiscard]] static float _area(std::span<const Vertex> p_vertices) noexcept
		{
			return std::abs(_signedDoubledArea(p_vertices)) * 0.5f;
		}

		static void _appendDistinct(
			std::vector<Vertex> &p_vertices,
			Vertex p_vertex)
		{
			if (p_vertices.empty() ||
				!_samePosition(p_vertices.back().position, p_vertex.position))
			{
				p_vertices.push_back(std::move(p_vertex));
			}
		}

		[[nodiscard]] static std::vector<Vertex> _cleanVertices(
			std::vector<Vertex> p_vertices)
		{
			if (p_vertices.size() > 1 &&
				_samePosition(p_vertices.front().position, p_vertices.back().position))
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

					if (_samePosition(previous.position, current.position) ||
						spk::ApproxValue(_turn(previous.position, current.position, next.position)) == 0.0f)
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
			const spk::Vector2 &p_edgeStart,
			const spk::Vector2 &p_edgeEnd,
			float p_clipWinding,
			bool p_keepInside,
			TInterpolator &p_interpolateData)
		{
			std::vector<Vertex> result;
			if (p_vertices.empty())
			{
				return result;
			}

			const auto distance = [&](const Vertex &p_vertex) {
				return (p_edgeEnd - p_edgeStart).cross(p_vertex.position - p_edgeStart) * p_clipWinding;
			};
			const auto accepted = [p_keepInside](float p_distance) {
				const bool liesOnEdge = spk::ApproxValue(p_distance) == 0.0f;
				return p_keepInside
						   ? p_distance > 0.0f || liesOnEdge
						   : p_distance < 0.0f || liesOnEdge;
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
					if (spk::ApproxValue(denominator) != 0.0f)
					{
						const float interpolation = previousDistance / denominator;
						_appendDistinct(
							result,
							{.position = previous.position + (current.position - previous.position) * interpolation,
							 .data = std::invoke(p_interpolateData, previous.data, current.data, interpolation)});
					}
				}

				if (currentAccepted)
				{
					_appendDistinct(result, current);
				}

				previous = current;
				previousDistance = currentDistance;
				previousAccepted = currentAccepted;
			}

			return _cleanVertices(std::move(result));
		}

		[[nodiscard]] static std::optional<Polygon2D> _fromClippedVertices(
			std::vector<Vertex> p_vertices)
		{
			p_vertices = _cleanVertices(std::move(p_vertices));
			if (p_vertices.size() < 3 || spk::ApproxValue(_area(p_vertices)) == 0.0f)
			{
				return std::nullopt;
			}

			const float signedDoubledArea = _signedDoubledArea(p_vertices);
			return Polygon2D(
				std::move(p_vertices),
				signedDoubledArea > 0.0f ? 1.0f : -1.0f);
		}

		explicit Polygon2D(std::vector<Vertex> p_vertices, float p_winding) :
			_vertices(std::move(p_vertices)),
			_winding(p_winding)
		{
		}

	public:
		Polygon2D(const Polygon2D &) = default;
		Polygon2D &operator=(const Polygon2D &) = default;
		Polygon2D(Polygon2D &&) noexcept = default;
		Polygon2D &operator=(Polygon2D &&) noexcept = default;

		[[nodiscard]] std::span<const Vertex> vertices() const noexcept
		{
			return _vertices;
		}

		[[nodiscard]] float signedArea() const noexcept
		{
			return _signedDoubledArea(_vertices) * 0.5f;
		}

		[[nodiscard]] float area() const noexcept
		{
			return _area(_vertices);
		}

		[[nodiscard]] bool isClockwise() const noexcept
		{
			return _winding < 0.0f;
		}

		[[nodiscard]] bool isConvex() const noexcept
		{
			if (_vertices.size() < 3 || spk::ApproxValue(area()) == 0.0f)
			{
				return false;
			}

			float turn = 0.0f;
			for (std::size_t index = 0; index < _vertices.size(); ++index)
			{
				const float current = _turn(
					_vertices[index].position,
					_vertices[(index + 1) % _vertices.size()].position,
					_vertices[(index + 2) % _vertices.size()].position);

				if (spk::ApproxValue(current) == 0.0f)
				{
					continue;
				}
				if (turn != 0.0f && (current * turn) < 0.0f)
				{
					return false;
				}
				turn = current;
			}

			return turn != 0.0f;
		}

		template <typename TInterpolator>
		[[nodiscard]] std::vector<Polygon2D> subtractConvex(
			const Polygon2D &p_clip,
			TInterpolator p_interpolateData) const
		{
			if (!isConvex() || !p_clip.isConvex())
			{
				throw std::invalid_argument("Polygon2D convex subtraction requires two convex polygons");
			}

			std::vector<Polygon2D> result;
			std::vector<Vertex> remaining = _vertices;

			for (std::size_t edge = 0; edge < p_clip._vertices.size(); ++edge)
			{
				const spk::Vector2 &start = p_clip._vertices[edge].position;
				const spk::Vector2 &end = p_clip._vertices[(edge + 1) % p_clip._vertices.size()].position;

				if (std::optional<Polygon2D> outside = _fromClippedVertices(
						_clipHalfPlane(
							remaining,
							start,
							end,
							p_clip._winding,
							false,
							p_interpolateData)))
				{
					result.push_back(std::move(*outside));
				}

				remaining = _clipHalfPlane(
					remaining,
					start,
					end,
					p_clip._winding,
					true,
					p_interpolateData);

				if (remaining.size() < 3 || spk::ApproxValue(_area(remaining)) == 0.0f)
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