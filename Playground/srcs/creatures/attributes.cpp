#include "creatures/attributes.hpp"

#include "core/json.hpp"

namespace pg
{
	Attributes parseAttributes(JsonReader &p_reader)
	{
		p_reader.forbidUnknown({"health", "ap", "mp", "attack", "armor", "armorPenetration", "magic", "resistance", "resistancePenetration", "bonusRange", "stamina", "staminaRate", "bonusHealing", "lifeSteal", "omnivampirism", "timeEffectResistance"});
		Attributes result{
			.health = p_reader.require<int>("health"), .ap = p_reader.require<int>("ap"), .mp = p_reader.require<int>("mp"), .attack = p_reader.require<int>("attack"), .armor = p_reader.require<int>("armor"), .armorPenetration = p_reader.require<int>("armorPenetration"), .magic = p_reader.require<int>("magic"), .resistance = p_reader.require<int>("resistance"), .resistancePenetration = p_reader.require<int>("resistancePenetration"), .bonusRange = p_reader.require<int>("bonusRange"), .stamina = p_reader.require<float>("stamina"), .staminaRate = p_reader.require<float>("staminaRate"), .bonusHealing = p_reader.require<float>("bonusHealing"), .lifeSteal = p_reader.require<float>("lifeSteal"), .omnivampirism = p_reader.require<float>("omnivampirism"), .timeEffectResistance = p_reader.require<float>("timeEffectResistance")};
		if (result.health <= 0 || result.ap < 0 || result.mp < 0 || result.stamina <= 0.0f || result.staminaRate < 0.0f)
		{
			throw JsonError(p_reader.file(), p_reader.path(), "attributes contain invalid resource or stamina values");
		}
		return result;
	}
}
