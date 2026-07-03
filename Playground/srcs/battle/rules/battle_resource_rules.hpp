#pragma once

#include "battle/battle_event.hpp"

namespace pg
{
	class BattleUnit;
	struct BattleResourceChangeResult
	{
		int gained = 0;
		int lost = 0;
	};
	class BattleResourceRules
	{
	public:
		[[nodiscard]] static BattleResourceChangeResult change(
			BattleUnit &p_unit, BattleResourceKind p_resource, int p_delta);
	};
}
