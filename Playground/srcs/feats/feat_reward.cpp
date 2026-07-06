#include "feats/feat_reward.hpp"

#include "abilities/ability.hpp"
#include "core/json.hpp"
#include "core/registry.hpp"
#include "creatures/attributes.hpp"
#include "creatures/creature_species.hpp"
#include "creatures/creature_unit.hpp"
#include "statuses/status.hpp"

#include <algorithm>
#include <cmath>

namespace
{
	template <typename TDefinition>
	[[nodiscard]] const TDefinition *resolve(
		pg::JsonReader &p_reader, const std::string &p_key, const pg::Registry<TDefinition> &p_registry)
	{
		const std::string id = p_reader.require<std::string>(p_key);
		const TDefinition *result = p_registry.tryGet(id);
		if (result == nullptr)
		{
			throw pg::JsonError(p_reader.file(), p_reader.pathFor(p_key), "unknown registry id '" + id + "'");
		}
		return result;
	}
}

namespace pg
{
	void BonusStatsReward::apply(CreatureUnit &p_unit) const
	{
		auto addInt = [this](int &p_target) { p_target += static_cast<int>(std::lround(value)); };
		auto addFloat = [this](float &p_target) { p_target += value; };
		Attributes &a = p_unit.attributes;
		if (attribute == "health") addInt(a.health); else if (attribute == "ap") addInt(a.ap);
		else if (attribute == "mp") addInt(a.mp); else if (attribute == "attack") addInt(a.attack);
		else if (attribute == "armor") addInt(a.armor); else if (attribute == "armorPenetration") addInt(a.armorPenetration);
		else if (attribute == "magic") addInt(a.magic); else if (attribute == "resistance") addInt(a.resistance);
		else if (attribute == "resistancePenetration") addInt(a.resistancePenetration); else if (attribute == "bonusRange") addInt(a.bonusRange);
		else if (attribute == "stamina") addFloat(a.stamina); else if (attribute == "staminaRate") addFloat(a.staminaRate);
		else if (attribute == "bonusHealing") addFloat(a.bonusHealing); else if (attribute == "lifeSteal") addFloat(a.lifeSteal);
		else if (attribute == "omnivampirism") addFloat(a.omnivampirism); else if (attribute == "timeEffectResistance") addFloat(a.timeEffectResistance);
		else throw std::logic_error("unknown bonusStats attribute '" + attribute + "'");
	}

	void AbilityReward::apply(CreatureUnit &p_unit) const
	{
		if (remove)
		{
			std::erase(p_unit.abilities, ability);
		}
		else if (std::ranges::find(p_unit.abilities, ability) == p_unit.abilities.end())
		{
			p_unit.abilities.push_back(ability);
		}
	}

	void PassiveReward::apply(CreatureUnit &p_unit) const
	{
		if (std::ranges::find(p_unit.permanentPassives, status) == p_unit.permanentPassives.end())
		{
			p_unit.permanentPassives.push_back(status);
		}
	}

	void ChangeFormReward::apply(CreatureUnit &p_unit) const
	{
		if (p_unit.species == nullptr) throw std::logic_error("changeForm reward requires a creature species");
		(void)p_unit.species->form(form);
		p_unit.currentFormId = form;
	}

	std::unique_ptr<FeatReward> parseFeatReward(
		JsonReader &p_reader, const Registry<Ability> &p_abilities, const Registry<Status> &p_statuses)
	{
		const std::string type = p_reader.require<std::string>("type");
		if (type == "bonusStats")
		{
			p_reader.forbidUnknown({"type", "attribute", "value"});
			auto result = std::make_unique<BonusStatsReward>();
			result->attribute = p_reader.require<std::string>("attribute");
			result->value = p_reader.require<float>("value");
			static const std::vector<std::string> attributes = {"health", "ap", "mp", "attack", "armor", "armorPenetration", "magic", "resistance", "resistancePenetration", "bonusRange", "stamina", "staminaRate", "bonusHealing", "lifeSteal", "omnivampirism", "timeEffectResistance"};
			if (std::ranges::find(attributes, result->attribute) == attributes.end())
				throw JsonError(p_reader.file(), p_reader.pathFor("attribute"), "unknown creature attribute '" + result->attribute + "'");
			return result;
		}
		if (type == "ability" || type == "removeAbility")
		{
			p_reader.forbidUnknown({"type", "ability"});
			auto result = std::make_unique<AbilityReward>();
			result->ability = resolve(p_reader, "ability", p_abilities); result->remove = type == "removeAbility";
			return result;
		}
		if (type == "passive")
		{
			p_reader.forbidUnknown({"type", "status"});
			auto result = std::make_unique<PassiveReward>(); result->status = resolve(p_reader, "status", p_statuses); return result;
		}
		if (type == "changeForm")
		{
			p_reader.forbidUnknown({"type", "form"});
			auto result = std::make_unique<ChangeFormReward>(); result->form = p_reader.require<std::string>("form");
			if (result->form.empty()) throw JsonError(p_reader.file(), p_reader.pathFor("form"), "value must not be empty");
			return result;
		}
		throw JsonError(p_reader.file(), p_reader.pathFor("type"), "unknown feat reward type '" + type + "'");
	}
}
