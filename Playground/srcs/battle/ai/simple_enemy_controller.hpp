#pragma once

#include <memory>
namespace pg
{
	class BattleAction;
	class BattleContext;
	class BattleUnit;
	class SimpleEnemyController
	{
	public:
		[[nodiscard]] static std::unique_ptr<BattleAction> choose(BattleContext &p_context, BattleUnit &p_unit);
	};
}
