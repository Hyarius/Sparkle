#pragma once
namespace pg
{
	class BattleAction;
	class BattleContext;
	class BattleActionResolver
	{
	public:
		[[nodiscard]] static bool resolve(BattleContext &p_context, const BattleAction &p_action, float p_mitigationScaling = 10.0f);
	};
}
