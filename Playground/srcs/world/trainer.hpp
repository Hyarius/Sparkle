#pragma once

#include "structures/game_engine/spk_component.hpp"
#include "structures/math/spk_vector2.hpp"
#include "voxel/voxel_enums.hpp"

#include <string>

namespace pg
{
	class Actor;
	struct EncounterTable;

	class Trainer : public spk::Component
	{
	public:
		std::string id;
		Actor *actor = nullptr;
		const EncounterTable *encounterTable = nullptr;
		VoxelOrientation facing = VoxelOrientation::PositiveZ;
		int sightRange = 6;
		std::string clearedFlag;
		spk::Vector2Int boardSize{11, 11};
		bool encounterPending = false;
	};
}
