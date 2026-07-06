#pragma once

#include "battle/battle_attributes.hpp"
#include "battle/battle_object.hpp"
#include "battle/battle_statuses.hpp"
#include "structures/math/spk_vector3.hpp"

#include <optional>
#include <string>

namespace pg
{
	struct AIBehaviour;
	class CreatureUnit;

	class BattleUnit : public BattleObject
	{
	private:
		CreatureUnit *_source = nullptr;

	public:
		BattleAttributes attributes;
		BattleStatuses statuses;
		std::optional<spk::Vector3Int> boardPosition;
		std::optional<spk::Vector3Int> lastBoardPosition;
		bool hasLeftBattle = false;
		const AIBehaviour *aiBehaviour = nullptr;
		std::vector<std::string> statusTags;

		BattleUnit(CreatureUnit *p_source, BattleSide p_side);
		BattleUnit(CreatureUnit &p_source, BattleSide p_side);
		~BattleUnit();
		[[nodiscard]] CreatureUnit *source() const noexcept;
		[[nodiscard]] const std::string &displayName() const noexcept;
		[[nodiscard]] bool isDefeated() const noexcept;
		[[nodiscard]] bool isActiveInBattle() const noexcept;
		[[nodiscard]] bool isTurnReady() const noexcept;
		[[nodiscard]] bool hasStatusTag(std::string_view p_tag) const;
	};
}
