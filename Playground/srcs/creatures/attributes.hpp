#pragma once

namespace pg
{
	class JsonReader;

	struct Attributes
	{
		int health = 0, ap = 0, mp = 0;
		int attack = 0, armor = 0, armorPenetration = 0;
		int magic = 0, resistance = 0, resistancePenetration = 0, bonusRange = 0;
		float stamina = 1.0f, staminaRate = 1.0f;
		float bonusHealing = 0.0f, lifeSteal = 0.0f, omnivampirism = 0.0f;
		float timeEffectResistance = 0.0f;
	};

	[[nodiscard]] Attributes parseAttributes(JsonReader &p_reader);
}
