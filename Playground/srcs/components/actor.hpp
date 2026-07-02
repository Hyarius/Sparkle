#pragma once

#include "structures/game_engine/spk_component.hpp"
#include "structures/math/spk_vector3.hpp"
#include "voxel/voxel_enums.hpp"

#include <cstddef>
#include <vector>

namespace pg
{
	class Actor : public spk::Component
	{
	public:
		spk::Vector3Int cell{};
		VoxelOrientation facing = VoxelOrientation::PositiveZ;
		float speed = 4.0f;
		bool player = false;
		std::vector<spk::Vector3Int> path;
		std::size_t segment = 0;
		float segmentProgress = 0.0f;
	};
}
