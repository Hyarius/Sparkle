#pragma once

#include "battle/battle_unit.hpp"
#include "taming/taming_progress.hpp"

namespace pg
{
	struct TamingProfile;

	class WildBattleUnit final : public BattleUnit
	{
	public:
		TamingProgress tamingProgress;

		WildBattleUnit(CreatureUnit *p_source, const TamingProfile &p_profile) :
			BattleUnit(p_source, BattleSide::Enemy),
			tamingProgress(p_profile)
		{
		}
	};
}
