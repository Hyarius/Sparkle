#pragma once

#include "ai/ai_decision.hpp"

namespace pg
{
	struct Ability;

	enum class AITarget
	{
		NearestEnemy,
		LowestHpEnemy,
		Self,
		LowestHpAlly
	};

	class CastAbilityDecision final : public AIDecision
	{
	public:
		const Ability *ability = nullptr;
		AITarget target = AITarget::NearestEnemy;
		CastAbilityDecision(const Ability &p_ability, AITarget p_target) : ability(&p_ability), target(p_target) {}
		[[nodiscard]] std::unique_ptr<BattleAction> produce(
			BattleUnit &p_unit, BattleContext &p_context) const override;
	};

	class MoveTowardNearestEnemyDecision final : public AIDecision
	{
	public:
		[[nodiscard]] std::unique_ptr<BattleAction> produce(
			BattleUnit &p_unit, BattleContext &p_context) const override;
	};

	class MoveAwayFromEnemiesDecision final : public AIDecision
	{
	public:
		[[nodiscard]] std::unique_ptr<BattleAction> produce(
			BattleUnit &p_unit, BattleContext &p_context) const override;
	};

	class EndTurnDecision final : public AIDecision
	{
	public:
		[[nodiscard]] std::unique_ptr<BattleAction> produce(
			BattleUnit &p_unit, BattleContext &p_context) const override;
	};
}
