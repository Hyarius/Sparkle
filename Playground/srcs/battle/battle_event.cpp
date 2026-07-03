#include "battle/battle_event.hpp"

#include <array>

namespace pg
{
	std::string_view battleEventName(BattleEventType p_type) noexcept
	{
		static constexpr std::array names = {"Damage", "Heal", "AbilityCast", "ShieldApplied", "DamageAbsorbed", "ShieldBroken", "StatusApplied", "StatusRemoved", "UnitDefeated", "BattleWon", "ResourceConsumed", "ResourceChanged", "ResourceStolen", "DistanceTravelled", "TurnStarted", "TurnEnded", "HitSurvived", "Teleported", "Displacement", "SwapPosition", "TurnBarTimeAdjusted", "TurnBarDurationAdjusted", "Revive"};
		const std::size_t index = static_cast<std::size_t>(p_type);
		return index < names.size() ? names[index] : std::string_view{};
	}
}
