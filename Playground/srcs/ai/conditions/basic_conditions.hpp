#pragma once

#include "ai/ai_condition.hpp"

#include <string>

namespace pg
{
	struct Ability;

	class EnemyWithinRangeCondition final : public AICondition
	{
	public:
		int range = 0;
		explicit EnemyWithinRangeCondition(int p_range) : range(p_range) {}
		[[nodiscard]] bool isMet(const BattleUnit &p_unit, const BattleContext &p_context) const override;
	};

	class AllyHpBelowCondition final : public AICondition
	{
	public:
		float percent = 0.0f;
		explicit AllyHpBelowCondition(float p_percent) : percent(p_percent) {}
		[[nodiscard]] bool isMet(const BattleUnit &p_unit, const BattleContext &p_context) const override;
	};

	class SelfHpBelowCondition final : public AICondition
	{
	public:
		float percent = 0.0f;
		explicit SelfHpBelowCondition(float p_percent) : percent(p_percent) {}
		[[nodiscard]] bool isMet(const BattleUnit &p_unit, const BattleContext &p_context) const override;
	};

	enum class AIStatusSubject
	{
		Self,
		Enemy
	};

	class HasStatusCondition final : public AICondition
	{
	public:
		std::string statusId;
		AIStatusSubject subject = AIStatusSubject::Self;
		HasStatusCondition(std::string p_statusId, AIStatusSubject p_subject) :
			statusId(std::move(p_statusId)), subject(p_subject) {}
		[[nodiscard]] bool isMet(const BattleUnit &p_unit, const BattleContext &p_context) const override;
	};

	class CanCastCondition final : public AICondition
	{
	public:
		const Ability *ability = nullptr;
		explicit CanCastCondition(const Ability &p_ability) : ability(&p_ability) {}
		[[nodiscard]] bool isMet(const BattleUnit &p_unit, const BattleContext &p_context) const override;
	};
}
