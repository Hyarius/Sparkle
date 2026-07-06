#pragma once

namespace pg
{
	class BattleContext;
	class BattleUnit;

	class AICondition
	{
	public:
		virtual ~AICondition() = default;
		[[nodiscard]] virtual bool isMet(const BattleUnit &p_unit, const BattleContext &p_context) const = 0;
	};
}
