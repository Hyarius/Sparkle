#pragma once

#include "abilities/effect.hpp"
#include "battle/battle_attributes.hpp"
#include "battle/battle_event.hpp"

#include <string>
#include <vector>

namespace pg
{
	struct Status;
	enum class EffectResource
	{
		Health,
		AP,
		MP,
		Range,
		TurnBarTime
	};

	class DamageEffect final : public Effect
	{
	public:
		int baseDamage = 0;
		DamageKind damageKind = DamageKind::Physical;
		float attackRatio = 0.0f;
		float magicRatio = 0.0f;
		[[nodiscard]] std::string_view type() const noexcept override
		{
			return "damage";
		}
		void apply(BattleAbilityExecutionContext &p_context) const override;
	};

	class HealEffect final : public Effect
	{
	public:
		int baseHealing = 0;
		float attackRatio = 0.0f;
		float magicRatio = 0.0f;
		[[nodiscard]] std::string_view type() const noexcept override
		{
			return "heal";
		}
		void apply(BattleAbilityExecutionContext &p_context) const override;
	};

	class ApplyStatusEffect final : public Effect
	{
	public:
		std::string status;
		mutable const Status *statusDefinition = nullptr;
		int stackCount = 1;
		Duration duration;
		[[nodiscard]] std::string_view type() const noexcept override
		{
			return "applyStatus";
		}
		void apply(BattleAbilityExecutionContext &) const override;
	};

	class RemoveStatusEffect final : public Effect
	{
	public:
		std::string status;
		int stackCount = 1;
		[[nodiscard]] std::string_view type() const noexcept override
		{
			return "removeStatus";
		}
		void apply(BattleAbilityExecutionContext &) const override;
	};

	class ConsumeStatusEffect final : public Effect
	{
	public:
		std::string status;
		int stackCount = 1;
		[[nodiscard]] std::string_view type() const noexcept override
		{
			return "consumeStatus";
		}
		void apply(BattleAbilityExecutionContext &) const override;
	};

	class CleanseEffect final : public Effect
	{
	public:
		std::vector<std::string> tags;
		[[nodiscard]] std::string_view type() const noexcept override
		{
			return "cleanse";
		}
		void apply(BattleAbilityExecutionContext &) const override;
	};

	class ReviveEffect final : public Effect
	{
	public:
		int healthRestored = 1;
		[[nodiscard]] std::string_view type() const noexcept override
		{
			return "revive";
		}
		void apply(BattleAbilityExecutionContext &p_context) const override;
	};

	class ApplyShieldEffect final : public Effect
	{
	public:
		ShieldKind kind = ShieldKind::Physical;
		int amount = 0;
		int durationTurns = -1;
		[[nodiscard]] std::string_view type() const noexcept override
		{
			return "applyShield";
		}
		void apply(BattleAbilityExecutionContext &p_context) const override;
	};

	class ResourceChangeEffect final : public Effect
	{
	public:
		EffectResource resource = EffectResource::AP;
		int value = 0;
		[[nodiscard]] std::string_view type() const noexcept override
		{
			return "resourceChange";
		}
		void apply(BattleAbilityExecutionContext &p_context) const override;
	};

	class StealResourceEffect final : public Effect
	{
	public:
		EffectResource resource = EffectResource::Health;
		int value = 0;
		[[nodiscard]] std::string_view type() const noexcept override
		{
			return "stealResource";
		}
		void apply(BattleAbilityExecutionContext &p_context) const override;
	};

	class DisplacementEffect final : public Effect
	{
	public:
		DisplacementOrientation orientation = DisplacementOrientation::AwayFromCaster;
		int force = 0;
		[[nodiscard]] std::string_view type() const noexcept override
		{
			return "displacement";
		}
		void apply(BattleAbilityExecutionContext &p_context) const override;
	};

	class SwapPositionEffect final : public Effect
	{
	public:
		[[nodiscard]] std::string_view type() const noexcept override
		{
			return "swapPosition";
		}
		void apply(BattleAbilityExecutionContext &p_context) const override;
	};

	class SwapPositionWithCasterEffect final : public Effect
	{
	public:
		[[nodiscard]] std::string_view type() const noexcept override
		{
			return "swapPositionWithCaster";
		}
		void apply(BattleAbilityExecutionContext &p_context) const override;
	};

	class TeleportSelfEffect final : public Effect
	{
	public:
		[[nodiscard]] std::string_view type() const noexcept override
		{
			return "teleportSelf";
		}
		void apply(BattleAbilityExecutionContext &p_context) const override;
	};

	class AdjustTurnBarTimeEffect final : public Effect
	{
	public:
		float delta = 0.0f;
		[[nodiscard]] std::string_view type() const noexcept override
		{
			return "adjustTurnBarTime";
		}
		void apply(BattleAbilityExecutionContext &p_context) const override;
	};

	class AdjustTurnBarDurationEffect final : public Effect
	{
	public:
		float delta = 0.0f;
		[[nodiscard]] std::string_view type() const noexcept override
		{
			return "adjustTurnBarDuration";
		}
		void apply(BattleAbilityExecutionContext &p_context) const override;
	};

	class PlaceObjectEffect final : public Effect
	{
	public:
		std::string object;
		Duration duration;
		[[nodiscard]] std::string_view type() const noexcept override
		{
			return "placeObject";
		}
		void apply(BattleAbilityExecutionContext &p_context) const override;
	};

	class RemoveObjectEffect final : public Effect
	{
	public:
		std::vector<std::string> tags;
		[[nodiscard]] std::string_view type() const noexcept override
		{
			return "removeObject";
		}
		void apply(BattleAbilityExecutionContext &p_context) const override;
	};
}
