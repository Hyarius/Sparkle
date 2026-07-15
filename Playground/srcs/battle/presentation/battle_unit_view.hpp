#pragma once

#include "battle/battle_ids.hpp"
#include "battle/battle_sequence_ids.hpp"
#include "board/board_cell.hpp"
#include "structures/game_engine/spk_entity_3d.hpp"

#include <cstddef>
#include <memory>
#include <optional>
#include <queue>
#include <vector>

namespace spk
{
	class TextureMeshRenderer3D;
}

namespace pg
{
	struct UnitCosmeticTrack
	{
		BattleActionId action;
		std::vector<BoardCell> path;
		std::size_t segment = 0;
		float segmentProgress = 0.0f;
		float worldUnitsPerSecond = 5.0f;
	};

	struct BattleUnitView
	{
		BattleUnitId id;
		std::unique_ptr<spk::Entity3D> entity;
		spk::Transform3D *transform = nullptr;
		spk::TextureMeshRenderer3D *renderer = nullptr;
		BoardCell authoritativeCell;
		float renderedHeight = 0.65f;
		std::optional<UnitCosmeticTrack> cosmetic;
		std::queue<UnitCosmeticTrack> queuedCosmetics;
	};
}
