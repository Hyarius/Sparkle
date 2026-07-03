#pragma once

#include "battle/battle_side.hpp"

#include <string_view>

namespace pg
{
	struct Ability;
	class BattleUnit;

	enum class BattleEventType
	{
		Damage,
		Heal,
		AbilityCast,
		ShieldApplied,
		DamageAbsorbed,
		ShieldBroken,
		StatusApplied,
		StatusRemoved,
		UnitDefeated,
		BattleWon,
		ResourceConsumed,
		ResourceChanged,
		ResourceStolen,
		DistanceTravelled,
		TurnStarted,
		TurnEnded,
		HitSurvived,
		Teleported,
		Displacement,
		SwapPosition,
		TurnBarTimeAdjusted,
		TurnBarDurationAdjusted,
		Revive
	};
	enum class BattleResourceKind
	{
		None,
		Health,
		AP,
		MP
	};

	struct BattleEvent
	{
		BattleEventType type = BattleEventType::Damage;
		int turnIndex = 0;
		const Ability *sourceAbility = nullptr;
		BattleUnit *caster = nullptr;
		BattleUnit *target = nullptr;
		int amount = 0;
		int distance = 0;
		BattleSide side = BattleSide::Neutral;
		BattleResourceKind resource = BattleResourceKind::None;
	};

	[[nodiscard]] std::string_view battleEventName(BattleEventType p_type) noexcept;
}
