#pragma once

namespace pg
{
	class BattleUnit;
	struct TurnContext
	{
		BattleUnit *activeUnit = nullptr;
		int turnIndex = 0;
		bool moved = false;
		bool castAbility = false;
	};
}
