#include "voxel/voxel_traversal.hpp"

#include <stdexcept>

namespace pg
{
	bool isSolid(const spk::VoxelCell &p_cell, const VoxelRegistry &p_registry)
	{
		return !p_cell.isEmpty() && p_registry.get(p_cell.id).data.traversal == VoxelTraversal::Solid;
	}

	bool isPassableSpace(const spk::VoxelCell &p_cell, const VoxelRegistry &p_registry)
	{
		return p_cell.isEmpty() || p_registry.get(p_cell.id).data.traversal == VoxelTraversal::Passable;
	}

	bool isStandable(
		const spk::VoxelGrid &p_grid,
		const spk::Vector3Int &p_position,
		const VoxelRegistry &p_registry)
	{
		const spk::Vector3Int above = p_position + spk::Vector3Int(0, 1, 0);
		const spk::Vector3Int aboveTwice = p_position + spk::Vector3Int(0, 2, 0);
		if (!p_grid.isWithinBounds(p_position) ||
			!p_grid.isWithinBounds(above) ||
			!p_grid.isWithinBounds(aboveTwice))
		{
			return false;
		}
		return isSolid(p_grid.cell(p_position), p_registry) &&
			   isPassableSpace(p_grid.cell(above), p_registry) &&
			   isPassableSpace(p_grid.cell(aboveTwice), p_registry);
	}

	VoxelOrientation resolveLocalDirection(
		VoxelOrientation p_worldDirection,
		VoxelOrientation p_orientation)
	{
		if (p_orientation == VoxelOrientation::PositiveZ)
		{
			return p_worldDirection;
		}

		switch (p_orientation)
		{
		case VoxelOrientation::PositiveX:
			switch (p_worldDirection)
			{
			case VoxelOrientation::PositiveX:
				return VoxelOrientation::PositiveZ;
			case VoxelOrientation::NegativeX:
				return VoxelOrientation::NegativeZ;
			case VoxelOrientation::PositiveZ:
				return VoxelOrientation::NegativeX;
			case VoxelOrientation::NegativeZ:
				return VoxelOrientation::PositiveX;
			}
			break;
		case VoxelOrientation::NegativeX:
			switch (p_worldDirection)
			{
			case VoxelOrientation::PositiveX:
				return VoxelOrientation::NegativeZ;
			case VoxelOrientation::NegativeX:
				return VoxelOrientation::PositiveZ;
			case VoxelOrientation::PositiveZ:
				return VoxelOrientation::PositiveX;
			case VoxelOrientation::NegativeZ:
				return VoxelOrientation::NegativeX;
			}
			break;
		case VoxelOrientation::NegativeZ:
			switch (p_worldDirection)
			{
			case VoxelOrientation::PositiveX:
				return VoxelOrientation::NegativeX;
			case VoxelOrientation::NegativeX:
				return VoxelOrientation::PositiveX;
			case VoxelOrientation::PositiveZ:
				return VoxelOrientation::NegativeZ;
			case VoxelOrientation::NegativeZ:
				return VoxelOrientation::PositiveZ;
			}
			break;
		case VoxelOrientation::PositiveZ:
			break;
		}

		throw std::invalid_argument("Unknown voxel orientation/direction");
	}

	CardinalHeightSet resolveWorldHeights(
		const CardinalHeightSet &p_localHeights,
		VoxelOrientation p_orientation)
	{
		return {
			.positiveX = p_localHeights.get(resolveLocalDirection(VoxelOrientation::PositiveX, p_orientation)),
			.negativeX = p_localHeights.get(resolveLocalDirection(VoxelOrientation::NegativeX, p_orientation)),
			.positiveZ = p_localHeights.get(resolveLocalDirection(VoxelOrientation::PositiveZ, p_orientation)),
			.negativeZ = p_localHeights.get(resolveLocalDirection(VoxelOrientation::NegativeZ, p_orientation)),
			.stationary = p_localHeights.stationary};
	}
}
