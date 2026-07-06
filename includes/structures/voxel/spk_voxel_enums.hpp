#pragma once

#include <cstddef>

namespace spk
{
	// The value names identify the world direction reached by the shape's local +Z axis.
	enum class VoxelOrientation
	{
		PositiveX,
		PositiveZ,
		NegativeX,
		NegativeZ
	};

	enum class VoxelFlip
	{
		PositiveY,
		NegativeY
	};

	enum class VoxelAxisPlane : std::size_t
	{
		PositiveX = 0,
		NegativeX,
		PositiveY,
		NegativeY,
		PositiveZ,
		NegativeZ,
		Count
	};
}
