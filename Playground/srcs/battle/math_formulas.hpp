#pragma once

#include "abilities/ability.hpp"

namespace pg
{
	class BattleAttributes;
	class MathFormulas
	{
	public:
		[[nodiscard]] static float mitigationRatio(int p_defense, float p_scaling = 10.0f);
		[[nodiscard]] static int computeDamage(
			const BattleAttributes &p_caster,
			const BattleAttributes &p_target,
			const Ability &p_ability,
			float p_mitigationScaling = 10.0f);
	};
}
