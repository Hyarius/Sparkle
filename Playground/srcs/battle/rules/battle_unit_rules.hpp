#pragma once

namespace pg
{
	struct Ability;
	class BattleContext;
	class BattleUnit;

	class BattleUnitRules
	{
	public:
		static void resolvePendingDefeats(
			BattleContext &p_context, BattleUnit *p_caster = nullptr, const Ability *p_ability = nullptr);
	};
}
