#include "battle/battle_attributes.hpp"

#include <algorithm>

namespace pg
{
	BattleAttributes::BattleAttributes(const Attributes &p_attributes)
	{
		setup(p_attributes);
	}

	void BattleAttributes::setup(const Attributes &p_attributes)
	{
		hp.set(p_attributes.health, p_attributes.health);
		ap.set(p_attributes.ap, p_attributes.ap);
		mp.set(p_attributes.mp, p_attributes.mp);
		turnBar.set(0.0f, std::max(0.1f, p_attributes.stamina));
		staminaRate = p_attributes.staminaRate;
		attack = p_attributes.attack;
		armor = p_attributes.armor;
		armorPenetration = p_attributes.armorPenetration;
		magic = p_attributes.magic;
		resistance = p_attributes.resistance;
		resistancePenetration = p_attributes.resistancePenetration;
		bonusRange = p_attributes.bonusRange;
		bonusHealing = p_attributes.bonusHealing;
		lifeSteal = p_attributes.lifeSteal;
		omnivampirism = p_attributes.omnivampirism;
		timeEffectResistance = p_attributes.timeEffectResistance;
		shields.clear();
	}

	void BattleAttributes::addShield(ShieldKind p_kind, int p_amount, int p_remainingTurns)
	{
		if (p_amount > 0 && (p_remainingTurns > 0 || p_remainingTurns == -1))
		{
			shields.add({p_kind, p_amount, p_remainingTurns});
		}
	}

	AbsorptionResult BattleAttributes::absorbDamage(DamageKind p_kind, int p_incoming)
	{
		AbsorptionResult result{.remaining = std::max(0, p_incoming)};
		const ShieldKind expected = p_kind == DamageKind::Physical ? ShieldKind::Physical : ShieldKind::Magical;
		for (std::size_t index = shields.size(); index > 0 && result.remaining > 0;)
		{
			--index;
			BattleShield &shield = shields[index];
			if (shield.kind != expected)
			{
				continue;
			}
			const int absorbed = std::min(shield.amount, result.remaining);
			shield.amount -= absorbed;
			result.remaining -= absorbed;
			result.absorbed += absorbed;
			if (shield.amount == 0)
			{
				result.broken.push_back(shield.kind);
				shields.removeAt(index);
			}
			else
			{
				shields.notifyItemChangedAt(index);
			}
		}
		return result;
	}

	void BattleAttributes::advanceShieldDurations()
	{
		for (std::size_t index = shields.size(); index > 0;)
		{
			--index;
			BattleShield &shield = shields[index];
			if (shield.remainingTurns < 0)
			{
				continue;
			}
			if (--shield.remainingTurns <= 0)
			{
				shields.removeAt(index);
			}
			else
			{
				shields.notifyItemChangedAt(index);
			}
		}
	}

	void BattleAttributes::clearShields()
	{
		shields.clear();
	}
}
