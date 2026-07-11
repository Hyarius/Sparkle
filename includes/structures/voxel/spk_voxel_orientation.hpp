#pragma once

#include "structures/math/spk_vector3.hpp"
#include "structures/voxel/spk_voxel_enums.hpp"

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
}
