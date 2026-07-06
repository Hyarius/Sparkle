#pragma once

#include <memory>

namespace pg
{
	struct AIBehaviour;
	class BattleAction;
	class BattleContext;
	class BattleUnit;

	class AIEvaluator
	{
	public:
		[[nodiscard]] static std::unique_ptr<BattleAction> evaluate(
			const AIBehaviour &p_behaviour, BattleUnit &p_unit, BattleContext &p_context);
	};
}
