#pragma once

#include "abilities/ability_definition.hpp"
#include "battle/battle_context.hpp"
#include "battle/query/battle_plans.hpp"

#include <vector>

namespace pg
{
	// Internal only: the session owns the command transaction and supplies its staged event list.
	class EffectResolver
	{
	private:
		static void _applyDamage(
			BattleContext &, std::vector<StagedEvent> &, const CastPlan &, const EffectApplication &,
			BattleUnitId, const DamageEffectSpec &);
		static void _applyHealing(
			BattleContext &, std::vector<StagedEvent> &, const CastPlan &, const EffectApplication &,
			BattleUnitId, const HealEffectSpec &);
		static void _applyResourceChange(
			BattleContext &, std::vector<StagedEvent> &, const CastPlan &, const EffectApplication &,
			BattleUnitId, const ChangeResourceEffectSpec &);
		static void _applyNextActivationPenalty(
			BattleContext &, std::vector<StagedEvent> &, const CastPlan &, const EffectApplication &,
			BattleUnitId, const ApplyNextActivationPenaltyEffectSpec &);
		static void _applyTurnBarAdjustment(
			BattleContext &, const CastPlan &, BattleUnitId, const AdjustTurnBarEffectSpec &);

	public:
		[[nodiscard]] static bool supports(const EffectPayload &p_payload) noexcept;
		[[nodiscard]] static bool supportsAll(const AbilityDefinition &p_ability) noexcept;
		static void resolveAbility(BattleContext &, std::vector<StagedEvent> &, const CastPlan &, const AbilityDefinition &);
	};
}
