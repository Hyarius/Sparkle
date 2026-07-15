#pragma once

#include "battle/battle_ids.hpp"
#include "battle/effects/duration_state.hpp"

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace pg
{
	// Passives and transient effects deliberately share one runtime representation.  The origin is
	// what prevents a cleanse or an authored removeStatus from changing an innate projection.
	enum class BattleStatusOrigin
	{
		PassiveProjection,
		TransientEffect
	};

	struct BattleStatusInstance
	{
		BattleStatusInstanceId id;
		std::string definitionId;
		BattleStatusOrigin origin = BattleStatusOrigin::TransientEffect;
		std::uint32_t stacks = 1;
		DurationState duration{InfiniteDurationState{}};
		std::optional<BattleUnitId> appliedBy;
		std::optional<std::string> sourceAbilityId;
		std::optional<std::string> sourceEffectId;

		[[nodiscard]] bool operator==(const BattleStatusInstance &) const = default;
	};

	struct BattleStatusSnapshot
	{
		BattleStatusInstanceId id;
		std::string definitionId;
		BattleStatusOrigin origin = BattleStatusOrigin::TransientEffect;
		std::uint32_t stacks = 1;
		DurationSnapshot duration;
		std::optional<BattleUnitId> appliedBy;
		std::optional<std::string> sourceAbilityId;
		std::optional<std::string> sourceEffectId;

		[[nodiscard]] bool operator==(const BattleStatusSnapshot &) const = default;
	};

	[[nodiscard]] BattleStatusSnapshot snapshotStatus(const BattleStatusInstance &p_status);
}
