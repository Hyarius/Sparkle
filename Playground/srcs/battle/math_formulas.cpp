#include "battle/math_formulas.hpp"
#include "battle/battle_attributes.hpp"

#include <algorithm>
#include <cmath>

namespace pg
{
	float MathFormulas::mitigationRatio(int p_defense, float p_scaling)
	{
		if (p_defense <= 0 || p_scaling <= 0.0f)
		{
			return 0.0f;
		}
		return std::clamp(static_cast<float>(p_defense) / (static_cast<float>(p_defense) + p_scaling), 0.0f, 1.0f);
	}
	int MathFormulas::computeDamage(const BattleAttributes &p_caster, const BattleAttributes &p_target, const Ability &p_ability, float p_mitigationScaling)
	{
		const float raw = std::max(0.0f, static_cast<float>(p_ability.baseDamage) + static_cast<int>(p_caster.attack) * p_ability.attackRatio + static_cast<int>(p_caster.magic) * p_ability.magicRatio);
		const int defense = p_ability.damageKind == DamageKind::Physical
								? std::max(0, static_cast<int>(p_target.armor) - static_cast<int>(p_caster.armorPenetration))
								: std::max(0, static_cast<int>(p_target.resistance) - static_cast<int>(p_caster.resistancePenetration));
		return std::max(0, static_cast<int>(std::lround(raw * (1.0f - mitigationRatio(defense, p_mitigationScaling)))));
	}
}
