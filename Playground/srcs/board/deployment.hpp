#pragma once

#include "structures/math/spk_vector3.hpp"
#include "voxel/voxel_enums.hpp"

#include <vector>

namespace pg
{
	class BoardData;

	struct DeploymentZones
	{
		std::vector<spk::Vector3Int> player;
		std::vector<spk::Vector3Int> enemy;
	};

	class Deployment
	{
	public:
		[[nodiscard]] static DeploymentZones compute(
			const BoardData &p_board,
			int p_stripDepth,
			VoxelOrientation p_playerFacing = VoxelOrientation::PositiveZ);
	};
}
