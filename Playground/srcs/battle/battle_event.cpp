#include "battle/battle_event.hpp"

#include <array>

namespace pg
{
	BattleEventType battleEventType(const BattleEvent &p_event) noexcept
	{
		return std::visit([](const auto &p_typed) {
			using T = std::remove_cvref_t<decltype(p_typed)>;
			if constexpr (std::same_as<T, DamageEvent>) return BattleEventType::Damage;
			else if constexpr (std::same_as<T, HealEvent>) return BattleEventType::Heal;
			else if constexpr (std::same_as<T, AbilityCastEvent>) return BattleEventType::AbilityCast;
			else if constexpr (std::same_as<T, ShieldAppliedEvent>) return BattleEventType::ShieldApplied;
			else if constexpr (std::same_as<T, DamageAbsorbedEvent>) return BattleEventType::DamageAbsorbed;
			else if constexpr (std::same_as<T, ShieldBrokenEvent>) return BattleEventType::ShieldBroken;
			else if constexpr (std::same_as<T, StatusAppliedEvent>) return BattleEventType::StatusApplied;
			else if constexpr (std::same_as<T, StatusRemovedEvent>) return BattleEventType::StatusRemoved;
			else if constexpr (std::same_as<T, UnitDefeatedEvent>) return BattleEventType::UnitDefeated;
			else if constexpr (std::same_as<T, BattleWonEvent>) return BattleEventType::BattleWon;
			else if constexpr (std::same_as<T, ResourceConsumedEvent>) return BattleEventType::ResourceConsumed;
			else if constexpr (std::same_as<T, ResourceChangedEvent>) return BattleEventType::ResourceChanged;
			else if constexpr (std::same_as<T, ResourceStolenEvent>) return BattleEventType::ResourceStolen;
			else if constexpr (std::same_as<T, DistanceTravelledEvent>) return BattleEventType::DistanceTravelled;
			else if constexpr (std::same_as<T, TurnStartedEvent>) return BattleEventType::TurnStarted;
			else if constexpr (std::same_as<T, TurnEndedEvent>) return BattleEventType::TurnEnded;
			else if constexpr (std::same_as<T, HitSurvivedEvent>) return BattleEventType::HitSurvived;
			else if constexpr (std::same_as<T, TeleportedEvent>) return BattleEventType::Teleported;
			else if constexpr (std::same_as<T, DisplacementEvent>) return BattleEventType::Displacement;
			else if constexpr (std::same_as<T, SwapPositionEvent>) return BattleEventType::SwapPosition;
			else if constexpr (std::same_as<T, TurnBarTimeAdjustedEvent>) return BattleEventType::TurnBarTimeAdjusted;
			else if constexpr (std::same_as<T, TurnBarDurationAdjustedEvent>) return BattleEventType::TurnBarDurationAdjusted;
			else return BattleEventType::Revive;
		}, p_event.data);
	}

	const BattleEventContext &battleEventContext(const BattleEvent &p_event) noexcept
	{
		return std::visit([](const auto &p_typed) -> const BattleEventContext & { return p_typed.context; }, p_event.data);
	}

	std::string_view battleEventName(BattleEventType p_type) noexcept
	{
		static constexpr std::array names = {"Damage", "Heal", "AbilityCast", "ShieldApplied", "DamageAbsorbed", "ShieldBroken", "StatusApplied", "StatusRemoved", "UnitDefeated", "BattleWon", "ResourceConsumed", "ResourceChanged", "ResourceStolen", "DistanceTravelled", "TurnStarted", "TurnEnded", "HitSurvived", "Teleported", "Displacement", "SwapPosition", "TurnBarTimeAdjusted", "TurnBarDurationAdjusted", "Revive"};
		const std::size_t index = static_cast<std::size_t>(p_type);
		return index < names.size() ? names[index] : std::string_view{};
	}

	std::string_view battleEventName(const BattleEvent &p_event) noexcept
	{
		return battleEventName(battleEventType(p_event));
	}
}
