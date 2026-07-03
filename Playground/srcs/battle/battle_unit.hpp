#pragma once

#include "battle/battle_attributes.hpp"
#include "battle/battle_object.hpp"
#include "structures/math/spk_vector3.hpp"

#include <optional>
#include <string>
#include <vector>

namespace pg
{
	struct Ability;
	struct BattleUnitSource
	{
		std::string displayName;
		Attributes attributes;
		std::vector<const Ability *> abilities;
	};

	class BattleUnit : public BattleObject
	{
	private:
		BattleUnitSource _source;

	public:
		BattleAttributes attributes;
		std::optional<spk::Vector3Int> boardPosition;
		bool hasLeftBattle = false;
		std::vector<std::string> statusTags;

		BattleUnit(BattleUnitSource p_source, BattleSide p_side);
		[[nodiscard]] const BattleUnitSource &source() const noexcept;
		[[nodiscard]] const std::string &displayName() const noexcept;
		[[nodiscard]] bool isDefeated() const noexcept;
		[[nodiscard]] bool isActiveInBattle() const noexcept;
		[[nodiscard]] bool isTurnReady() const noexcept;
		[[nodiscard]] bool hasStatusTag(std::string_view p_tag) const;
	};
}
