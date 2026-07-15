#pragma once

#include "creatures/creature_attributes.hpp"

namespace pg
{
	class BattleUnit;
	class Registries;

	// The only effective-stat formula.  The evaluator is intentionally pure: callers reconcile
	// current pools/events around its result, while targeting, damage and the scheduler read the
	// same already-computed attributes from BattleUnit.
	class EffectiveStatsEvaluator
	{
	public:
		[[nodiscard]] static CreatureAttributes evaluate(const BattleUnit &p_unit, const Registries &p_registries);
	};
}
