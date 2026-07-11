#include "world/world_raycaster.hpp"

#include "world/voxel_world.hpp"

#include "structures/voxel/spk_voxel_shape.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <optional>
#include <stdexcept>

namespace
{
	constexpr double IntersectionEpsilon = 1.0e-6;

	struct Point3D
	{
		double x = 0;
		double y = 0;
		double z = 0;
	};

	struct SurfaceHit
	{
		double distance = 0;
		spk::Vector3 normal{};
		pg::VoxelAxisPlane face = pg::VoxelAxisPlane::Count;
	};

	[[nodiscard]] bool isFinite(const spk::Vector3 &p_value) noexcept
	{
		return std::isfinite(p_value.x) && std::isfinite(p_value.y) && std::isfinite(p_value.z);
	}

	[[nodiscard]] pg::VoxelAxisPlane axisPlane(const spk::Vector3 &p_normal) noexcept
	{
		if (std::abs(p_normal.y) <= IntersectionEpsilon && std::abs(p_normal.z) <= IntersectionEpsilon)
		{
			return p_normal.x > 0 ? pg::VoxelAxisPlane::PositiveX : pg::VoxelAxisPlane::NegativeX;
		}
		if (std::abs(p_normal.x) <= IntersectionEpsilon && std::abs(p_normal.z) <= IntersectionEpsilon)
		{
			return p_normal.y > 0 ? pg::VoxelAxisPlane::PositiveY : pg::VoxelAxisPlane::NegativeY;
		}
		if (std::abs(p_normal.x) <= IntersectionEpsilon && std::abs(p_normal.y) <= IntersectionEpsilon)
		{
			return p_normal.z > 0 ? pg::VoxelAxisPlane::PositiveZ : pg::VoxelAxisPlane::NegativeZ;
		}
		return pg::VoxelAxisPlane::Count;
	}

	[[nodiscard]] std::pair<double, double> project(
		const Point3D &p_point,
		int p_droppedAxis) noexcept
	{
		switch (p_droppedAxis)
		{
		case 0:
			return {p_point.y, p_point.z};
		case 1:
			return {p_point.x, p_point.z};
		default:
			return {p_point.x, p_point.y};
		}
	}

	[[nodiscard]] std::pair<double, double> project(
		const spk::Vector3 &p_point,
		int p_droppedAxis) noexcept
	{
		return project(Point3D{p_point.x, p_point.y, p_point.z}, p_droppedAxis);
	}

	[[nodiscard]] bool pointOnSegment(
		double p_x,
		double p_y,
		double p_ax,
		double p_ay,
		double p_bx,
		double p_by) noexcept
	{
		const double cross = (p_x - p_ax) * (p_by - p_ay) - (p_y - p_ay) * (p_bx - p_ax);
		if (std::abs(cross) > IntersectionEpsilon)
		{
			return false;
		}
		return p_x >= std::min(p_ax, p_bx) - IntersectionEpsilon &&
			   p_x <= std::max(p_ax, p_bx) + IntersectionEpsilon &&
			   p_y >= std::min(p_ay, p_by) - IntersectionEpsilon &&
			   p_y <= std::max(p_ay, p_by) + IntersectionEpsilon;
	}

	// Even/odd containment after dropping the polygon normal's dominant axis. This handles
	// every simple convex or concave face and treats its boundary as part of the surface.
	[[nodiscard]] bool polygonContains(
		const spk::VoxelShapePolygon &p_polygon,
		const Point3D &p_point) noexcept
	{
		const spk::Vector3 &normal = p_polygon.normal();
		const double ax = std::abs(normal.x);
		const double ay = std::abs(normal.y);
		const double az = std::abs(normal.z);
		const int droppedAxis = ax >= ay && ax >= az ? 0 : (ay >= az ? 1 : 2);
		const auto [x, y] = project(p_point, droppedAxis);

		bool inside = false;
		for (std::size_t index = 0; index < p_polygon.size(); ++index)
		{
			const auto [ax2, ay2] = project(p_polygon[index].position, droppedAxis);
			const auto [bx2, by2] = project(p_polygon[(index + 1) % p_polygon.size()].position, droppedAxis);
			if (pointOnSegment(x, y, ax2, ay2, bx2, by2))
			{
				return true;
			}
			if ((ay2 > y) != (by2 > y))
			{
				const double crossingX = ax2 + (y - ay2) * (bx2 - ax2) / (by2 - ay2);
				if (x < crossingX)
				{
					inside = !inside;
				}
			}
		}
		return inside;
	}

	[[nodiscard]] std::optional<SurfaceHit> intersectPolygon(
		const spk::VoxelShapePolygon &p_polygon,
		const Point3D &p_localOrigin,
		const Point3D &p_direction,
		double p_minimumDistance,
		double p_maximumDistance)
	{
		const spk::Vector3 &normal = p_polygon.normal();
		const double denominator = p_direction.x * normal.x + p_direction.y * normal.y + p_direction.z * normal.z;
		if (std::abs(denominator) <= IntersectionEpsilon)
		{
			return std::nullopt; // parallel rays, including coplanar grazing, do not cross a face
		}

		const spk::Vector3 &vertex = p_polygon[0].position;
		const double distance =
			((vertex.x - p_localOrigin.x) * normal.x +
			 (vertex.y - p_localOrigin.y) * normal.y +
			 (vertex.z - p_localOrigin.z) * normal.z) /
			denominator;
		if (distance < p_minimumDistance - IntersectionEpsilon ||
			distance > p_maximumDistance + IntersectionEpsilon)
		{
			return std::nullopt;
		}

		const double clampedDistance = std::max(0.0, distance);
		const Point3D point{
			p_localOrigin.x + p_direction.x * clampedDistance,
			p_localOrigin.y + p_direction.y * clampedDistance,
			p_localOrigin.z + p_direction.z * clampedDistance};
		if (!polygonContains(p_polygon, point))
		{
			return std::nullopt;
		}
		return SurfaceHit{clampedDistance, normal, axisPlane(normal)};
	}

	[[nodiscard]] std::optional<pg::VoxelHit> intersectShape(
		const pg::VoxelWorld &p_world,
		const spk::Vector3Int &p_cellPosition,
		const spk::Vector3 &p_origin,
		const Point3D &p_direction,
		double p_entryDistance,
		double p_exitDistance)
	{
		const spk::VoxelCell &cell = p_world.cell(p_cellPosition);
		const pg::VoxelDefinition *definition = p_world.tryDefinition(cell);
		if (definition == nullptr || definition->data.traversal != pg::VoxelTraversal::Solid)
		{
			return std::nullopt;
		}

		const spk::VoxelShape *shape = p_world.registry().renderRegistry().tryShape(cell.id);
		if (shape == nullptr)
		{
			return std::nullopt;
		}

		const Point3D localOrigin{
			static_cast<double>(p_origin.x) - static_cast<double>(p_cellPosition.x),
			static_cast<double>(p_origin.y) - static_cast<double>(p_cellPosition.y),
			static_cast<double>(p_origin.z) - static_cast<double>(p_cellPosition.z)};
		std::optional<SurfaceHit> nearest;
		const auto inspectFace = [&](const spk::VoxelShapeFace &p_face) {
			for (const spk::VoxelShapePolygon &polygon : p_face.polygons())
			{
				const std::optional<SurfaceHit> hit = intersectPolygon(
					polygon,
					localOrigin,
					p_direction,
					p_entryDistance,
					p_exitDistance);
				if (hit.has_value() && (!nearest.has_value() || hit->distance < nearest->distance - IntersectionEpsilon))
				{
					nearest = hit;
				}
			}
		};

		const spk::VoxelShapeFaceSet &faces = shape->renderFaces();
		for (std::size_t index = 0; index < faces.innerFaces.size(); ++index)
		{
			inspectFace(shape->transformedInnerFace(index, cell.orientation, cell.flip));
		}
		for (std::size_t index = 0; index < static_cast<std::size_t>(spk::VoxelAxisPlane::Count); ++index)
		{
			const auto plane = static_cast<spk::VoxelAxisPlane>(index);
			if (faces.outer(plane).has_value())
			{
				inspectFace(shape->transformedOuterFace(plane, cell.orientation, cell.flip));
			}
		}

		if (!nearest.has_value())
		{
			return std::nullopt;
		}
		return pg::VoxelHit{
			.cell = p_cellPosition,
			.enterFace = nearest->face,
			.distance = static_cast<float>(nearest->distance),
			.normal = nearest->normal};
	}

	[[nodiscard]] double firstBoundary(double p_origin, int p_cell, int p_step, double p_direction)
	{
		if (p_step == 0)
		{
			return std::numeric_limits<double>::infinity();
		}
		const double boundary = static_cast<double>(p_cell) + (p_step > 0 ? 1.0 : 0.0);
		return (boundary - p_origin) / p_direction;
	}

	[[nodiscard]] bool isRepresentableCell(double p_coordinate) noexcept
	{
		const double floored = std::floor(p_coordinate);
		return floored >= static_cast<double>(std::numeric_limits<int>::lowest()) &&
			   floored <= static_cast<double>(std::numeric_limits<int>::max());
	}
}

namespace pg
{
	std::optional<VoxelHit> WorldRaycaster::raycast(
		const VoxelWorld &p_world,
		const WorldRay &p_ray,
		float p_maxDistance)
	{
		if (p_maxDistance < 0)
		{
			return std::nullopt;
		}
		if (!std::isfinite(p_maxDistance) || !isFinite(p_ray.origin) || !isFinite(p_ray.direction))
		{
			throw std::invalid_argument("World ray values must be finite");
		}

		const double length = std::sqrt(
			static_cast<double>(p_ray.direction.x) * p_ray.direction.x +
			static_cast<double>(p_ray.direction.y) * p_ray.direction.y +
			static_cast<double>(p_ray.direction.z) * p_ray.direction.z);
		if (length == 0.0)
		{
			throw std::invalid_argument("World ray direction cannot be zero");
		}
		const Point3D direction{
			static_cast<double>(p_ray.direction.x) / length,
			static_cast<double>(p_ray.direction.y) / length,
			static_cast<double>(p_ray.direction.z) / length};

		const Point3D end{
			static_cast<double>(p_ray.origin.x) + direction.x * p_maxDistance,
			static_cast<double>(p_ray.origin.y) + direction.y * p_maxDistance,
			static_cast<double>(p_ray.origin.z) + direction.z * p_maxDistance};
		if (!isRepresentableCell(p_ray.origin.x) || !isRepresentableCell(p_ray.origin.y) ||
			!isRepresentableCell(p_ray.origin.z) || !isRepresentableCell(end.x) ||
			!isRepresentableCell(end.y) || !isRepresentableCell(end.z))
		{
			throw std::out_of_range("World ray traverses coordinates outside the voxel cell range");
		}

		spk::Vector3Int cell{
			static_cast<int>(std::floor(p_ray.origin.x)),
			static_cast<int>(std::floor(p_ray.origin.y)),
			static_cast<int>(std::floor(p_ray.origin.z))};

		const int stepX = direction.x > 0 ? 1 : (direction.x < 0 ? -1 : 0);
		const int stepY = direction.y > 0 ? 1 : (direction.y < 0 ? -1 : 0);
		const int stepZ = direction.z > 0 ? 1 : (direction.z < 0 ? -1 : 0);
		double tMaxX = firstBoundary(p_ray.origin.x, cell.x, stepX, direction.x);
		double tMaxY = firstBoundary(p_ray.origin.y, cell.y, stepY, direction.y);
		double tMaxZ = firstBoundary(p_ray.origin.z, cell.z, stepZ, direction.z);
		const double tDeltaX = stepX == 0 ? std::numeric_limits<double>::infinity() : std::abs(1.0 / direction.x);
		const double tDeltaY = stepY == 0 ? std::numeric_limits<double>::infinity() : std::abs(1.0 / direction.y);
		const double tDeltaZ = stepZ == 0 ? std::numeric_limits<double>::infinity() : std::abs(1.0 / direction.z);

		double entryDistance = 0;
		while (entryDistance <= static_cast<double>(p_maxDistance) + IntersectionEpsilon)
		{
			const double exitDistance = std::min({tMaxX, tMaxY, tMaxZ, static_cast<double>(p_maxDistance)});
			if (const auto hit = intersectShape(p_world, cell, p_ray.origin, direction, entryDistance, exitDistance); hit.has_value())
			{
				return hit;
			}

			const double nextDistance = std::min({tMaxX, tMaxY, tMaxZ});
			if (nextDistance > static_cast<double>(p_maxDistance) + IntersectionEpsilon)
			{
				break;
			}

			// Supercover tie rule: advance every axis whose boundary is reached at this
			// distance. Cells touched only along a zero-area edge/corner are never tested.
			const bool crossX = std::abs(tMaxX - nextDistance) <= IntersectionEpsilon;
			const bool crossY = std::abs(tMaxY - nextDistance) <= IntersectionEpsilon;
			const bool crossZ = std::abs(tMaxZ - nextDistance) <= IntersectionEpsilon;
			if (crossX)
			{
				cell.x += stepX;
				tMaxX += tDeltaX;
			}
			if (crossY)
			{
				cell.y += stepY;
				tMaxY += tDeltaY;
			}
			if (crossZ)
			{
				cell.z += stepZ;
				tMaxZ += tDeltaZ;
			}
			entryDistance = nextDistance;
		}
		return std::nullopt;
	}
}
