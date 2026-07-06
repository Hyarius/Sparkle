#pragma once

#include "structures/voxel/spk_voxel_enums.hpp"

namespace pg
{
	// Gameplay-only classification. This one is NOT part of the spk voxel render library
	// (which promoted render geometry only), so it stays defined app-side.
	enum class VoxelTraversal
	{
		Solid,
		Passable
	};

	// Render orientation / flip / axis-plane are shared verbatim with the spk voxel library;
	// alias them so downstream Playground code keeps using pg:: names while feeding spk types.
	using VoxelOrientation = spk::VoxelOrientation;
	using VoxelFlip = spk::VoxelFlip;
	using VoxelAxisPlane = spk::VoxelAxisPlane;
}
