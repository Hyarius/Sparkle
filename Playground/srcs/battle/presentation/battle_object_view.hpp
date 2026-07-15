#pragma once

#include "battle/battle_ids.hpp"
#include "board/board_cell.hpp"
#include "structures/game_engine/spk_entity_3d.hpp"

#include <memory>
#include <optional>

namespace spk
{
	class TextureMeshRenderer3D;
}

namespace pg
{
	struct ObjectCosmeticCue
	{
		float secondsRemaining = 0.0f;
		bool removeWhenComplete = false;
	};

	struct BattleObjectView
	{
		BattleObjectId id;
		std::unique_ptr<spk::Entity3D> entity;
		spk::Transform3D *transform = nullptr;
		spk::TextureMeshRenderer3D *renderer = nullptr;
		BoardCell authoritativeCell;
		std::optional<BattleSide> creatorSide;
		std::optional<ObjectCosmeticCue> cosmetic;
	};
}
