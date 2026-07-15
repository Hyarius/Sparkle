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
			BattleContext &, std::vector<StagedEvent> &, std::vector<BattleUnitId> &, const CastPlan &, const EffectApplication &, BattleUnitId, const DamageEffectSpec &);
		static void _applyHealing(
			BattleContext &, std::vector<StagedEvent> &, const CastPlan &, const EffectApplication &, BattleUnitId, const HealEffectSpec &);
		static void _applyResourceChange(
			BattleContext &, std::vector<StagedEvent> &, const CastPlan &, const EffectApplication &, BattleUnitId, const ChangeResourceEffectSpec &);
		static void _applyNextActivationPenalty(
			BattleContext &, std::vector<StagedEvent> &, const CastPlan &, const EffectApplication &, BattleUnitId, const ApplyNextActivationPenaltyEffectSpec &);
		static void _applyShield(
			BattleContext &, std::vector<StagedEvent> &, const CastPlan &, const EffectApplication &, BattleUnitId, const ApplyShieldEffectSpec &);
		static void _applyStatus(
			BattleContext &, std::vector<StagedEvent> &, const CastPlan &, const EffectApplication &, BattleUnitId, const ApplyStatusEffectSpec &);
		static void _removeStatus(
			BattleContext &, std::vector<StagedEvent> &, const CastPlan &, const EffectApplication &, BattleUnitId, const RemoveStatusEffectSpec &);
		static void _cleanse(
			BattleContext &, std::vector<StagedEvent> &, const CastPlan &, const EffectApplication &, BattleUnitId, const CleanseEffectSpec &);
		static void _placeObject(
			BattleContext &, std::vector<StagedEvent> &, const CastPlan &, const EffectApplication &, BoardCell, const PlaceObjectEffectSpec &);
		static void _removeObjects(
			BattleContext &, std::vector<StagedEvent> &, const CastPlan &, const EffectApplication &, BoardCell, const RemoveObjectsEffectSpec &);
		static void _applyTurnBarAdjustment(
			BattleContext &, std::vector<StagedEvent> &, const CastPlan &, const EffectApplication &, BattleUnitId, const AdjustTurnBarEffectSpec &);
		static void _applyDisplace(BattleContext &, std::vector<StagedEvent> &, const CastPlan &, const EffectApplication &, BattleUnitId, const DisplaceEffectSpec &);
		static void _applySwap(BattleContext &, std::vector<StagedEvent> &, const CastPlan &, const EffectApplication &);
		static void _applyTeleport(BattleContext &, std::vector<StagedEvent> &, const CastPlan &, const EffectApplication &);

		// Step 10 owns the bodies.  Step 09 calls these explicit seams only while the command is
		// still staging its batch; they deliberately cannot publish or submit a nested command.
		static void _afterDamageDealtSeam(BattleContext &, std::vector<StagedEvent> &, const CastPlan &, const EffectApplication &, BattleUnitId, BattleUnitId);
		static void _afterDamageTakenSeam(BattleContext &, std::vector<StagedEvent> &, const CastPlan &, const EffectApplication &, BattleUnitId, BattleUnitId);
		static void _afterHealingDoneSeam(BattleContext &, std::vector<StagedEvent> &, const CastPlan &, const EffectApplication &, BattleUnitId, BattleUnitId);
		static void _afterHealingReceivedSeam(BattleContext &, std::vector<StagedEvent> &, const CastPlan &, const EffectApplication &, BattleUnitId, BattleUnitId);
		static void _afterAbilityCastReaction(BattleContext &, std::vector<StagedEvent> &, const CastPlan &, const AbilityDefinition &);
		static void _unitLeftCellSeam(BattleContext &, std::vector<StagedEvent> &, BattleUnitId, BoardCell);
		static void _unitEnteredCellSeam(BattleContext &, std::vector<StagedEvent> &, BattleUnitId, BoardCell);
		static void _unitEndedMoveOnCellSeam(BattleContext &, std::vector<StagedEvent> &, BattleUnitId, BoardCell);

	public:
		[[nodiscard]] static bool supports(const EffectPayload &p_payload) noexcept;
		[[nodiscard]] static bool supportsAll(const AbilityDefinition &p_ability) noexcept;
		// The one committed single-unit transition used by voluntary movement, displacement, and
		// teleport.  It writes the step fact then invokes the leave/enter seams; callers add their
		// cause-specific aggregate before calling finishSpatialMove.
		[[nodiscard]] static bool applySpatialStep(
			BattleContext &, std::vector<StagedEvent> &, BattleUnitId, BoardCell, BoardCell, int, SpatialMoveCause, const BattleEventOrigin &);
		static void finishSpatialMove(BattleContext &, std::vector<StagedEvent> &, BattleUnitId, BoardCell);
		static void resolveAbility(BattleContext &, std::vector<StagedEvent> &, const CastPlan &, const AbilityDefinition &);
	};
}
