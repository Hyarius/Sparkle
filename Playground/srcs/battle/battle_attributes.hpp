#pragma once

#include "abilities/ability.hpp"
#include "core/observable_resource.hpp"
#include "creatures/attributes.hpp"
#include "structures/container/spk_observable_value.hpp"

#include <vector>

namespace pg
{
	enum class ShieldKind
	{
		Physical,
		Magical
	};
	struct BattleShield
	{
		ShieldKind kind;
		int amount = 0;
		int remainingTurns = -1;
	};
	struct AbsorptionResult
	{
		int absorbed = 0;
		int remaining = 0;
		std::vector<ShieldKind> broken;
	};

	class BattleAttributes
	{
	public:
		ObservableResource hp, ap, mp;
		ObservableFloatResource turnBar;
		spk::ObservableValue<float> staminaRate;
		spk::ObservableValue<int> attack, armor, armorPenetration, magic, resistance, resistancePenetration, bonusRange;
		spk::ObservableValue<float> bonusHealing, lifeSteal, omnivampirism, timeEffectResistance;
		ObservableList<BattleShield> shields;

		explicit BattleAttributes(const Attributes &p_attributes = {});
		void setup(const Attributes &p_attributes);
		void addShield(ShieldKind p_kind, int p_amount, int p_remainingTurns);
		[[nodiscard]] AbsorptionResult absorbDamage(DamageKind p_kind, int p_incoming);
		void advanceShieldDurations();
		void clearShields();
	};
}
