#pragma once

#include "battle/battle_ids.hpp"
#include "battle/battle_types.hpp"
#include "battle/effects/duration_state.hpp"

#include <cstdint>
#include <optional>
#include <string>

namespace pg
{
	struct BattleShield
	{
		BattleShieldId id;
		DamageKind kind = DamageKind::Physical;
		std::int64_t remainingAmount = 0;
		DurationState duration;
		std::optional<BattleUnitId> source;
		std::optional<std::string> sourceAbilityId;
		std::optional<std::string> sourceEffectId;
	};
}
