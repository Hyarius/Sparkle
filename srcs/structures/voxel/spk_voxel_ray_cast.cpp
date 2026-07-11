#include "structures/voxel/spk_voxel_ray_cast.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>

namespace
{
	constexpr double BoundaryEpsilon = 1.0e-9;

	[[nodiscard]] bool finite(const spk::Vector3 &p_value) noexcept
	{
		return std::isfinite(p_value.x) && std::isfinite(p_value.y) && std::isfinite(p_value.z);
	}

	[[nodiscard]] bool representableCell(double p_coordinate) noexcept
	{
		const double floored = std::floor(p_coordinate);
		return floored >= static_cast<double>(std::numeric_limits<int>::lowest()) &&
			   floored <= static_cast<double>(std::numeric_limits<int>::max());
	}

	[[nodiscard]] int stepOf(float p_direction) noexcept
	{
		if (p_direction > 0.0f)
		{
			return 1;
		}
		if (p_direction < 0.0f)
		{
			return -1;
		}
		return 0;
	}

	[[nodiscard]] double firstBoundary(double p_origin, int p_cell, int p_step, double p_direction) noexcept
	{
		if (p_step == 0)
		{
			return std::numeric_limits<double>::infinity();
		}
		const double boundary = static_cast<double>(p_cell) + (p_step > 0 ? 1.0 : 0.0);
		return (boundary - p_origin) / p_direction;
	}

	[[nodiscard]] double axisDelta(int p_step, double p_direction) noexcept
	{
		return p_step == 0 ? std::numeric_limits<double>::infinity() : std::abs(1.0 / p_direction);
	}

	[[nodiscard]] spk::Vector3 normalizedDirection(const spk::Ray3D &p_ray, float p_maxDistance)
	{
		if (!std::isfinite(p_maxDistance) || !finite(p_ray.origin) || !finite(p_ray.direction))
		{
			throw std::invalid_argument("VoxelRayCast values must be finite");
		}
		const double length = std::sqrt(
			static_cast<double>(p_ray.direction.x) * p_ray.direction.x +
			static_cast<double>(p_ray.direction.y) * p_ray.direction.y +
			static_cast<double>(p_ray.direction.z) * p_ray.direction.z);
		if (length == 0.0)
		{
			throw std::invalid_argument("VoxelRayCast direction cannot be zero");
		}
		return {
			static_cast<float>(p_ray.direction.x / length),
			static_cast<float>(p_ray.direction.y / length),
			static_cast<float>(p_ray.direction.z / length)};
	}

	void validateCoordinateRange(const spk::Vector3 &p_origin, const spk::Vector3 &p_end)
	{
		if (!representableCell(p_origin.x) || !representableCell(p_origin.y) || !representableCell(p_origin.z) ||
			!representableCell(p_end.x) || !representableCell(p_end.y) || !representableCell(p_end.z))
		{
			throw std::out_of_range("VoxelRayCast traverses coordinates outside the voxel cell range");
		}
	}

	[[nodiscard]] std::optional<spk::VoxelRayCast::Hit> inspect(
		const spk::IVoxelCellLookup &p_cells,
		const spk::VoxelRayCast::CellPredicate &p_selectable,
		const spk::Vector3Int &p_cell,
		double p_distance,
		const spk::Vector3Int &p_normal)
	{
		const spk::VoxelCell *value = p_cells.tryCell(p_cell);
		if (value == nullptr || !p_selectable(p_cell, *value))
		{
			return std::nullopt;
		}
		return spk::VoxelRayCast::Hit{
			.cell = p_cell,
			.distance = static_cast<float>(p_distance),
			.entryNormal = p_normal};
	}

	struct Double3
	{
		double x = 0.0;
		double y = 0.0;
		double z = 0.0;
	};

	struct DDAState
	{
		spk::Vector3Int cell{};
		spk::Vector3Int step{};
		Double3 tMaximum{};
		Double3 tDelta{};

		DDAState(const spk::Vector3 &p_origin, const spk::Vector3 &p_direction, const spk::Vector3Int &p_cell) :
			cell(p_cell),
			step(stepOf(p_direction.x), stepOf(p_direction.y), stepOf(p_direction.z)),
			tMaximum(
				firstBoundary(p_origin.x, cell.x, step.x, p_direction.x),
				firstBoundary(p_origin.y, cell.y, step.y, p_direction.y),
				firstBoundary(p_origin.z, cell.z, step.z, p_direction.z)),
			tDelta(
				axisDelta(step.x, p_direction.x),
				axisDelta(step.y, p_direction.y),
				axisDelta(step.z, p_direction.z))
		{
		}

		[[nodiscard]] double nextDistance() const noexcept
		{
			return std::min({tMaximum.x, tMaximum.y, tMaximum.z});
		}

		[[nodiscard]] spk::Vector3Int advance(double p_distance) noexcept
		{
			spk::Vector3Int normal{};
			if (std::abs(tMaximum.x - p_distance) <= BoundaryEpsilon)
			{
				cell.x += step.x;
				tMaximum.x += tDelta.x;
				normal.x = -step.x;
			}
			if (std::abs(tMaximum.y - p_distance) <= BoundaryEpsilon)
			{
				cell.y += step.y;
				tMaximum.y += tDelta.y;
				normal.y = -step.y;
			}
			if (std::abs(tMaximum.z - p_distance) <= BoundaryEpsilon)
			{
				cell.z += step.z;
				tMaximum.z += tDelta.z;
				normal.z = -step.z;
			}
			return normal;
		}
	};
}

namespace spk
{
	std::optional<VoxelRayCast::Hit> VoxelRayCast::cast(
		const spk::IVoxelCellLookup &p_cells,
		const spk::Ray3D &p_ray,
		float p_maxDistance,
		CellPredicate p_selectable)
	{
		if (p_maxDistance < 0.0f)
		{
			return std::nullopt;
		}

		const spk::Vector3 direction = normalizedDirection(p_ray, p_maxDistance);
		validateCoordinateRange(p_ray.origin, p_ray.origin + direction * p_maxDistance);
		if (p_selectable == nullptr)
		{
			p_selectable = [](const spk::Vector3Int &, const spk::VoxelCell &p_cell) {
				return !p_cell.isEmpty();
			};
		}

		const spk::Vector3Int originCell{
			static_cast<int>(std::floor(p_ray.origin.x)),
			static_cast<int>(std::floor(p_ray.origin.y)),
			static_cast<int>(std::floor(p_ray.origin.z))};
		if (const std::optional<Hit> initial = inspect(p_cells, p_selectable, originCell, 0.0, {}); initial.has_value())
		{
			return initial;
		}

		DDAState state(p_ray.origin, direction, originCell);
		while (true)
		{
			const double distance = state.nextDistance();
			if (distance > static_cast<double>(p_maxDistance) + BoundaryEpsilon)
			{
				return std::nullopt;
			}
			const spk::Vector3Int normal = state.advance(distance);
			if (const std::optional<Hit> hit = inspect(p_cells, p_selectable, state.cell, distance, normal); hit.has_value())
			{
				return hit;
			}
		}
	}
}
