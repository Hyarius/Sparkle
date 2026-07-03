#include "abilities/ability.hpp"

#include "core/json.hpp"

#include <map>

namespace pg
{
	Ability parseAbility(JsonReader &p_reader)
	{
		p_reader.forbidUnknown({"version", "displayName", "cost", "range", "areaOfEffect", "targetProfile", "damage"});
		if (p_reader.require<int>("version") != 1)
		{
			throw JsonError(p_reader.file(), p_reader.pathFor("version"), "unsupported ability version");
		}
		JsonReader cost = p_reader.child("cost");
		cost.forbidUnknown({"ap", "mp"});
		JsonReader range = p_reader.child("range");
		range.forbidUnknown({"shape", "min", "max", "requiresLineOfSight"});
		static const std::map<std::string, RangeShape> rangeShapes = {
			{"circle", RangeShape::Circle}, {"line", RangeShape::Line}, {"diagonal", RangeShape::Diagonal}, {"self", RangeShape::Self}};
		static const std::map<std::string, AreaShape> areaShapes = {
			{"square", AreaShape::Square}, {"cross", AreaShape::Cross}, {"circle", AreaShape::Circle}, {"line", AreaShape::Line}};
		static const std::map<std::string, TargetProfile> targets = {{"everything", TargetProfile::Everything}, {"ally", TargetProfile::Ally}, {"enemy", TargetProfile::Enemy}, {"empty", TargetProfile::Empty}};
		static const std::map<std::string, DamageKind> kinds = {{"physical", DamageKind::Physical}, {"magical", DamageKind::Magical}};
		JsonReader damage = p_reader.child("damage");
		damage.forbidUnknown({"baseDamage", "damageKind", "attackRatio", "magicRatio"});
		Ability result{.displayName = p_reader.require<std::string>("displayName"), .apCost = cost.require<int>("ap"), .mpCost = cost.require<int>("mp"), .minimumRange = range.require<int>("min"), .maximumRange = range.require<int>("max"), .rangeShape = range.requireEnum<RangeShape>("shape", rangeShapes), .requiresLineOfSight = range.require<bool>("requiresLineOfSight"), .targetProfile = p_reader.requireEnum<TargetProfile>("targetProfile", targets), .baseDamage = damage.require<int>("baseDamage"), .damageKind = damage.requireEnum<DamageKind>("damageKind", kinds), .attackRatio = damage.require<float>("attackRatio"), .magicRatio = damage.require<float>("magicRatio")};
		if (p_reader.contains("areaOfEffect"))
		{
			JsonReader area = p_reader.child("areaOfEffect");
			area.forbidUnknown({"shape", "value"});
			result.areaShape = area.requireEnum<AreaShape>("shape", areaShapes);
			result.areaValue = area.require<int>("value");
		}
		if (result.displayName.empty() || result.apCost < 0 || result.mpCost < 0 || result.minimumRange < 0 ||
			result.maximumRange < result.minimumRange || result.baseDamage < 0 || result.areaValue < 0)
		{
			throw JsonError(p_reader.file(), p_reader.path(), "ability contains invalid values");
		}
		return result;
	}
}
