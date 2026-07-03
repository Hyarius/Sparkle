#pragma once

namespace pg
{
	class BattleContext;
	class BattleUnit;
	class BattleTurnRules
	{
	public:
		static void advanceTurnBars(BattleContext &p_context, float p_seconds);
		[[nodiscard]] static float timeUntilNextReady(const BattleContext &p_context);
		static void advanceToNextReady(BattleContext &p_context);
		[[nodiscard]] static BattleUnit *selectReady(BattleContext &p_context);
		static void beginTurn(BattleContext &p_context, BattleUnit &p_unit);
		static void endTurn(BattleContext &p_context);
		[[nodiscard]] static bool canContinueTurn(const BattleContext &p_context);
	};
}
