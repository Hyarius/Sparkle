#pragma once

#include <memory>

namespace pg
{
	class BattleAction;
	class BattleContext;
	class BattleUnit;

	class AIDecision
	{
	public:
		virtual ~AIDecision() = default;
		[[nodiscard]] virtual std::unique_ptr<BattleAction> produce(
			BattleUnit &p_unit, BattleContext &p_context) const = 0;
	};
}
