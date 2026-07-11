#pragma once

#include "structures/math/spk_vector3.hpp"
#include "structures/voxel/spk_voxel_enums.hpp"

#include <stdexcept>

namespace spk
{
	// Quarter-turn arithmetic for the lattice-preserving voxel transforms: rotations
	// around +Y in the order PositiveZ -> PositiveX -> NegativeZ -> NegativeX (the
	// enum value names give the world direction of the shape's local +Z axis), and
	// the vertical mirror carried by VoxelFlip. These are the only transforms that
	// keep every voxel on the grid, which is why prefab stamping and shape variants
	// are expressed with them rather than with free angles.

	[[nodiscard]] constexpr int quarterTurnsOf(spk::VoxelOrientation p_orientation) noexcept
	{
		switch (p_orientation)
		{
		case spk::VoxelOrientation::PositiveX:
			return 1;
		case spk::VoxelOrientation::NegativeZ:
			return 2;
		case spk::VoxelOrientation::NegativeX:
			return 3;
		default:
			return 0;
		}
	}

	// Accepts any turn count, negatives included.
	[[nodiscard]] constexpr spk::VoxelOrientation orientationFromQuarterTurns(int p_turns) noexcept
	{
		switch (((p_turns % 4) + 4) % 4)
		{
		case 1:
			return spk::VoxelOrientation::PositiveX;
		case 2:
			return spk::VoxelOrientation::NegativeZ;
		case 3:
			return spk::VoxelOrientation::NegativeX;
		default:
			return spk::VoxelOrientation::PositiveZ;
		}
	}

	// The orientation reached by applying p_second after p_first (turns add; the
	// group is commutative).
	[[nodiscard]] constexpr spk::VoxelOrientation composeOrientations(
		spk::VoxelOrientation p_first,
		spk::VoxelOrientation p_second) noexcept
	{
		return orientationFromQuarterTurns(quarterTurnsOf(p_first) + quarterTurnsOf(p_second));
	}

	// Two vertical mirrors cancel out.
	[[nodiscard]] constexpr spk::VoxelFlip composeFlips(spk::VoxelFlip p_first, spk::VoxelFlip p_second) noexcept
	{
		return p_first == p_second ? spk::VoxelFlip::PositiveY : spk::VoxelFlip::NegativeY;
	}

	// Rotates a pivot-relative position by quarter turns around the +Y axis.
	// Accepts any turn count, negatives included.
	[[nodiscard]] constexpr spk::Vector3Int rotateQuarterTurns(const spk::Vector3Int &p_local, int p_turns) noexcept
	{
		switch (((p_turns % 4) + 4) % 4)
		{
		case 1:
			return {p_local.z, p_local.y, -p_local.x};
		case 2:
			return {-p_local.x, p_local.y, -p_local.z};
		case 3:
			return {-p_local.z, p_local.y, p_local.x};
		default:
			return p_local;
		}
	}

	// Transforms a normalized position inside a voxel cell around the cell center.
	[[nodiscard]] constexpr spk::Vector3 transformVoxelPosition(
		const spk::Vector3 &p_position,
		spk::VoxelOrientation p_orientation,
		spk::VoxelFlip p_flip)
	{
		spk::Vector3 result;
		switch (p_orientation)
		{
		case spk::VoxelOrientation::PositiveZ:
			result = p_position;
			break;
		case spk::VoxelOrientation::PositiveX:
			result = {p_position.z, p_position.y, 1.0f - p_position.x};
			break;
		case spk::VoxelOrientation::NegativeZ:
			result = {1.0f - p_position.x, p_position.y, 1.0f - p_position.z};
			break;
		case spk::VoxelOrientation::NegativeX:
			result = {1.0f - p_position.z, p_position.y, p_position.x};
			break;
		default:
			throw std::invalid_argument("Invalid voxel orientation");
		}

		if (p_flip == spk::VoxelFlip::NegativeY)
		{
			result.y = 1.0f - result.y;
		}
		else if (p_flip != spk::VoxelFlip::PositiveY)
		{
			throw std::invalid_argument("Invalid voxel flip");
		}
		return result;
	}

	// Maps a world-facing cell plane back to the corresponding plane in an
	// oriented and optionally vertically mirrored voxel shape.
	[[nodiscard]] constexpr spk::VoxelAxisPlane mapWorldPlaneToLocal(
		spk::VoxelAxisPlane p_worldPlane,
		spk::VoxelOrientation p_orientation,
		spk::VoxelFlip p_flip)
	{
		spk::Vector3Int direction;
		switch (p_worldPlane)
		{
		case spk::VoxelAxisPlane::PositiveX:
			direction = {1, 0, 0};
			break;
		case spk::VoxelAxisPlane::NegativeX:
			direction = {-1, 0, 0};
			break;
		case spk::VoxelAxisPlane::PositiveY:
			direction = {0, 1, 0};
			break;
		case spk::VoxelAxisPlane::NegativeY:
			direction = {0, -1, 0};
			break;
		case spk::VoxelAxisPlane::PositiveZ:
			direction = {0, 0, 1};
			break;
		case spk::VoxelAxisPlane::NegativeZ:
			direction = {0, 0, -1};
			break;
		case spk::VoxelAxisPlane::Count:
		default:
			throw std::invalid_argument("VoxelAxisPlane::Count is not a geometric plane");
		}

		if (p_flip == spk::VoxelFlip::NegativeY)
		{
			direction.y = -direction.y;
		}
		else if (p_flip != spk::VoxelFlip::PositiveY)
		{
			throw std::invalid_argument("Invalid voxel flip");
		}

		int turns = 0;
		switch (p_orientation)
		{
		case spk::VoxelOrientation::PositiveZ:
			break;
		case spk::VoxelOrientation::PositiveX:
			turns = 1;
			break;
		case spk::VoxelOrientation::NegativeZ:
			turns = 2;
			break;
		case spk::VoxelOrientation::NegativeX:
			turns = 3;
			break;
		default:
			throw std::invalid_argument("Invalid voxel orientation");
		}
		direction = rotateQuarterTurns(direction, -turns);

		if (direction.x == 1)
		{
			return spk::VoxelAxisPlane::PositiveX;
		}
		if (direction.x == -1)
		{
			return spk::VoxelAxisPlane::NegativeX;
		}
		if (direction.y == 1)
		{
			return spk::VoxelAxisPlane::PositiveY;
		}
		if (direction.y == -1)
		{
			return spk::VoxelAxisPlane::NegativeY;
		}
		if (direction.z == 1)
		{
			return spk::VoxelAxisPlane::PositiveZ;
		}
		return spk::VoxelAxisPlane::NegativeZ;
	}
}
