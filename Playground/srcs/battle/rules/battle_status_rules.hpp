#pragma once

namespace pg
{
	class BattleContext;
	class BattleUnit;
	enum class StatusHookPoint;

	class BattleStatusRules
	{
	public:
		static void applyHook(BattleUnit &p_owner, StatusHookPoint p_hookPoint, int p_triggerAmount = 0);
		static void advanceSeconds(BattleContext &p_context, float p_seconds);
		[[nodiscard]] static bool isApplyingHook() noexcept;
	};
}
