#include "board/board_line_of_sight.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <limits>
#include <stdexcept>

namespace pg
{
	namespace
	{
		// The three axes, in the canonical order every tie is broken in.
		constexpr std::size_t AxisCount = 3;

		[[nodiscard]] double component(const spk::Vector3 &p_value, std::size_t p_axis) noexcept
		{
			return p_axis == 0 ? static_cast<double>(p_value.x)
							   : (p_axis == 1 ? static_cast<double>(p_value.y) : static_cast<double>(p_value.z));
		}

		[[nodiscard]] int &component(spk::Vector3Int &p_value, std::size_t p_axis) noexcept
		{
			return p_axis == 0 ? p_value.x : (p_axis == 1 ? p_value.y : p_value.z);
		}

		// The voxel a point falls in. Floor, not truncation: -0.5 is in voxel -1, not in voxel 0.
		[[nodiscard]] spk::Vector3Int voxelOf(const spk::Vector3 &p_point) noexcept
		{
			return {
				static_cast<int>(std::floor(p_point.x)),
				static_cast<int>(std::floor(p_point.y)),
				static_cast<int>(std::floor(p_point.z))};
		}

		[[nodiscard]] bool isFinite(const spk::Vector3 &p_point) noexcept
		{
			return std::isfinite(p_point.x) && std::isfinite(p_point.y) && std::isfinite(p_point.z);
		}
	}

	LineOfSightResult BoardLineOfSight::trace(
		const BoardData &p_board,
		BoardCell p_fromSupport,
		BoardCell p_toSupport,
		const IBoardLineOfSightExtraBlockers *p_extraBlockers,
		float p_eyeHeight)
	{
		p_board.requireCurrentTerrain();
		if (!p_board.isStandable(p_fromSupport) || !p_board.isStandable(p_toSupport))
		{
			throw std::invalid_argument(
				"line of sight is traced between two standable support cells, got " + p_fromSupport.toString() +
				" -> " + p_toSupport.toString());
		}
		if (!std::isfinite(p_eyeHeight))
		{
			throw std::invalid_argument("the eye height is a finite value");
		}
		if (p_fromSupport == p_toSupport)
		{
			return {.clear = true, .firstBlockingVoxel = std::nullopt};
		}

		const ICellSource &source = p_board.cells();
		const auto eye = [&source, p_eyeHeight](const BoardCell &p_cell) {
			return spk::Vector3{
				static_cast<float>(p_cell.x) + 0.5f,
				walkHeightAtCenter(source, p_cell) + p_eyeHeight,
				static_cast<float>(p_cell.z) + 0.5f};
		};
		const spk::Vector3 origin = eye(p_fromSupport);
		const spk::Vector3 target = eye(p_toSupport);
		if (!isFinite(origin) || !isFinite(target))
		{
			throw std::runtime_error("a line-of-sight endpoint is not a finite point");
		}

		// A voxel in either endpoint's own column never blocks: the caster's own support, the block it
		// is standing against, and the target's own cover are all its own business.
		const auto blocks = [&](const spk::Vector3Int &p_voxel) {
			if ((p_voxel.x == p_fromSupport.x && p_voxel.z == p_fromSupport.z) ||
				(p_voxel.x == p_toSupport.x && p_voxel.z == p_toSupport.z))
			{
				return false;
			}

			const spk::VoxelCell &cell = source.cell(p_voxel);
			const VoxelDefinition *definition = source.tryDefinition(cell);
			if (definition != nullptr && definition->data.traversal == VoxelTraversal::Solid &&
				!definition->data.hasTag(LineOfSightTransparentTag))
			{
				return true;
			}
			return p_extraBlockers != nullptr && p_extraBlockers->blocksVoxel(p_voxel);
		};

		spk::Vector3Int current = voxelOf(origin);
		const spk::Vector3Int last = voxelOf(target);

		// Amanatides-Woo, parametrised over t in [0, 1] along the segment: tMax[axis] is the t at which
		// the ray crosses into the next voxel along that axis, tDelta[axis] how much t one whole voxel
		// of it costs.
		std::array<int, AxisCount> step{};
		std::array<double, AxisCount> tMax{};
		std::array<double, AxisCount> tDelta{};
		for (std::size_t axis = 0; axis < AxisCount; ++axis)
		{
			const double direction = component(target, axis) - component(origin, axis);
			const double start = component(origin, axis);
			const int voxel = component(current, axis);

			if (direction > 0.0)
			{
				step[axis] = 1;
				tDelta[axis] = 1.0 / direction;
				tMax[axis] = (static_cast<double>(voxel) + 1.0 - start) / direction;
			}
			else if (direction < 0.0)
			{
				step[axis] = -1;
				tDelta[axis] = -1.0 / direction;
				tMax[axis] = (static_cast<double>(voxel) - start) / direction;
			}
			else
			{
				step[axis] = 0;
				tDelta[axis] = std::numeric_limits<double>::infinity();
				tMax[axis] = std::numeric_limits<double>::infinity();
			}
		}

		// One step per crossed voxel boundary, plus slack for the ties that advance several axes at
		// once. A ray between two cells of a bounded board cannot need more.
		const std::size_t maximumSteps = static_cast<std::size_t>(
											 std::abs(last.x - current.x) + std::abs(last.y - current.y) +
											 std::abs(last.z - current.z)) +
										 8;
		constexpr double Epsilon = 1e-9;

		for (std::size_t taken = 0; taken < maximumSteps; ++taken)
		{
			const double nextT = std::min({tMax[0], tMax[1], tMax[2]});
			if (!(nextT <= 1.0 + Epsilon) || !std::isfinite(nextT))
			{
				// Past the target point: the segment ends inside the current voxel.
				break;
			}

			// Every axis whose boundary the ray crosses at this same t. Two of them means an edge,
			// three a corner - and the cells they touch have to be inspected, or a diagonal shot would
			// slip between two walls that meet exactly on the ray.
			std::uint32_t tied = 0;
			for (std::size_t axis = 0; axis < AxisCount; ++axis)
			{
				if (tMax[axis] <= nextT + Epsilon)
				{
					tied |= 1U << axis;
				}
			}

			// The newly touched cells, in canonical order: the single-axis neighbours X, Y, Z first,
			// then the edge cells XY, XZ, YZ, then the corner cell XYZ - which is where the ray lands.
			constexpr std::array<std::uint32_t, 7> CanonicalMasks{1, 2, 4, 3, 5, 6, 7};
			for (const std::uint32_t mask : CanonicalMasks)
			{
				if ((mask & tied) != mask)
				{
					continue;
				}
				spk::Vector3Int touched = current;
				for (std::size_t axis = 0; axis < AxisCount; ++axis)
				{
					if ((mask & (1U << axis)) != 0)
					{
						component(touched, axis) += step[axis];
					}
				}
				if (blocks(touched))
				{
					return {.clear = false, .firstBlockingVoxel = touched};
				}
			}

			for (std::size_t axis = 0; axis < AxisCount; ++axis)
			{
				if ((tied & (1U << axis)) != 0)
				{
					component(current, axis) += step[axis];
					tMax[axis] += tDelta[axis];
				}
			}
		}

		return {.clear = true, .firstBlockingVoxel = std::nullopt};
	}
}
