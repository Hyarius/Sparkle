#pragma once

#include "battle/battle_side.hpp"

#include <concepts>
#include <string_view>
#include <type_traits>
#include <utility>
#include <variant>

namespace pg
{
	struct Ability;
	struct Status;
	enum class ShieldKind;
	enum class DamageKind;
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
		MP,
		Range,
		TurnBarTime
	};

	enum class DisplacementOrientation
	{
		TowardCaster,
		AwayFromCaster
	};

	struct BattleEventContext
	{
		int turnIndex = 0;
		const Ability *sourceAbility = nullptr;
		BattleUnit *caster = nullptr;
		BattleUnit *target = nullptr;
	};

	struct DamageEvent { BattleEventContext context; int amount = 0; DamageKind kind{}; };
	struct HealEvent { BattleEventContext context; int amount = 0; };
	struct AbilityCastEvent { BattleEventContext context; int distance = 0; };
	struct ShieldAppliedEvent { BattleEventContext context; int amount = 0; ShieldKind kind{}; };
	struct DamageAbsorbedEvent { BattleEventContext context; int amount = 0; DamageKind kind{}; };
	struct ShieldBrokenEvent { BattleEventContext context; ShieldKind kind{}; };
	struct StatusAppliedEvent { BattleEventContext context; const Status *status = nullptr; int count = 0; };
	struct StatusRemovedEvent { BattleEventContext context; const Status *status = nullptr; int count = 0; };
	struct UnitDefeatedEvent { BattleEventContext context; };
	struct BattleWonEvent { BattleEventContext context; BattleSide side = BattleSide::Neutral; bool unitSurvived = true; };
	struct ResourceConsumedEvent { BattleEventContext context; BattleResourceKind resource = BattleResourceKind::None; int amount = 0; };
	struct ResourceChangedEvent { BattleEventContext context; BattleResourceKind resource = BattleResourceKind::None; int amount = 0; float delta = 0.0f; };
	struct ResourceStolenEvent { BattleEventContext context; BattleResourceKind resource = BattleResourceKind::None; int amount = 0; };
	struct DistanceTravelledEvent { BattleEventContext context; int distance = 0; };
	struct TurnStartedEvent { BattleEventContext context; int nearestUnitDistance = 0; };
	struct TurnEndedEvent { BattleEventContext context; int nearestUnitDistance = 0; };
	struct HitSurvivedEvent { BattleEventContext context; int remainingHealth = 0; };
	struct TeleportedEvent { BattleEventContext context; int distance = 0; };
	struct DisplacementEvent { BattleEventContext context; int distance = 0; DisplacementOrientation orientation = DisplacementOrientation::AwayFromCaster; };
	struct SwapPositionEvent { BattleEventContext context; };
	struct TurnBarTimeAdjustedEvent { BattleEventContext context; float delta = 0.0f; };
	struct TurnBarDurationAdjustedEvent { BattleEventContext context; float delta = 0.0f; };
	struct ReviveEvent { BattleEventContext context; int amount = 0; };

	struct BattleEvent
	{
		using Variant = std::variant<
			DamageEvent, HealEvent, AbilityCastEvent, ShieldAppliedEvent, DamageAbsorbedEvent,
			ShieldBrokenEvent, StatusAppliedEvent, StatusRemovedEvent, UnitDefeatedEvent,
			BattleWonEvent, ResourceConsumedEvent, ResourceChangedEvent, ResourceStolenEvent,
			DistanceTravelledEvent, TurnStartedEvent, TurnEndedEvent, HitSurvivedEvent,
			TeleportedEvent, DisplacementEvent, SwapPositionEvent, TurnBarTimeAdjustedEvent,
			TurnBarDurationAdjustedEvent, ReviveEvent>;

		Variant data;

		template <typename TEvent>
			requires(!std::same_as<std::remove_cvref_t<TEvent>, BattleEvent> &&
				 std::constructible_from<Variant, TEvent>)
		BattleEvent(TEvent &&p_event) : data(std::forward<TEvent>(p_event)) {}

		template <typename TEvent>
		[[nodiscard]] const TEvent *getIf() const noexcept { return std::get_if<TEvent>(&data); }
		template <typename TEvent>
		[[nodiscard]] TEvent *getIf() noexcept { return std::get_if<TEvent>(&data); }
	};

	[[nodiscard]] BattleEventType battleEventType(const BattleEvent &p_event) noexcept;
	[[nodiscard]] const BattleEventContext &battleEventContext(const BattleEvent &p_event) noexcept;
	[[nodiscard]] std::string_view battleEventName(BattleEventType p_type) noexcept;
	[[nodiscard]] std::string_view battleEventName(const BattleEvent &p_event) noexcept;
}
