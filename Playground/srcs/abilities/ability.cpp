#include "abilities/ability.hpp"

#include "abilities/effect.hpp"
#include "abilities/effects/effect_types.hpp"
#include "core/json.hpp"
#include "core/registry.hpp"
#include "statuses/status.hpp"

#include <map>

namespace pg
{
	Ability parseAbility(JsonReader &p_reader)
	{
		p_reader.forbidUnknown({"version", "displayName", "icon", "cost", "range", "areaOfEffect", "targetProfile", "casterAnimation", "targetAnimation", "travelVfx", "effects"});
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
		static const std::map<std::string, TargetProfile> targets = {
			{"everything", TargetProfile::Everything}, {"ally", TargetProfile::Ally}, {"enemy", TargetProfile::Enemy}, {"empty", TargetProfile::Empty}};

		Ability result;
		result.displayName = p_reader.require<std::string>("displayName");
		result.icon = p_reader.optional<std::array<int, 2>>("icon", {0, 0});
		result.apCost = cost.require<int>("ap");
		result.mpCost = cost.require<int>("mp");
		result.minimumRange = range.require<int>("min");
		result.maximumRange = range.require<int>("max");
		result.rangeShape = range.requireEnum<RangeShape>("shape", rangeShapes);
		result.requiresLineOfSight = range.require<bool>("requiresLineOfSight");
		result.targetProfile = p_reader.requireEnum<TargetProfile>("targetProfile", targets);
		result.casterAnimation = p_reader.optional<std::string>("casterAnimation", "");
		result.targetAnimation = p_reader.optional<std::string>("targetAnimation", "TakeDamage");

		if (p_reader.contains("areaOfEffect"))
		{
			JsonReader area = p_reader.child("areaOfEffect");
			area.forbidUnknown({"shape", "value"});
			result.areaShape = area.requireEnum<AreaShape>("shape", areaShapes);
			result.areaValue = area.require<int>("value");
		}

		if (p_reader.contains("travelVfx"))
		{
			JsonReader travel = p_reader.child("travelVfx");
			const std::string kind = travel.require<std::string>("kind");
			TravelVfx parsed;
			parsed.vfx = travel.require<std::string>("vfx");
			if (kind == "projectile")
			{
				travel.forbidUnknown({"kind", "vfx", "speed"});
				parsed.kind = TravelVfxKind::Projectile;
				parsed.speed = travel.require<float>("speed");
				if (parsed.speed <= 0.0f)
				{
					throw JsonError(travel.file(), travel.pathFor("speed"), "speed must be positive");
				}
			}
			else if (kind == "beam")
			{
				travel.forbidUnknown({"kind", "vfx", "duration"});
				parsed.kind = TravelVfxKind::Beam;
				parsed.duration = travel.require<float>("duration");
				if (parsed.duration <= 0.0f)
				{
					throw JsonError(travel.file(), travel.pathFor("duration"), "duration must be positive");
				}
			}
			else
			{
				throw JsonError(travel.file(), travel.pathFor("kind"), "unknown travel VFX kind '" + kind + "'");
			}
			if (parsed.vfx.empty())
			{
				throw JsonError(travel.file(), travel.pathFor("vfx"), "VFX id must not be empty");
			}
			result.travelVfx = std::move(parsed);
		}

		for (JsonReader effectReader : p_reader.childArray("effects"))
		{
			result.effects.emplace_back(parseEffect(effectReader));
		}

		if (result.displayName.empty() || result.apCost < 0 || result.mpCost < 0 ||
			result.minimumRange < 0 || result.maximumRange < result.minimumRange || result.areaValue < 0)
		{
			throw JsonError(p_reader.file(), p_reader.path(), "ability contains invalid values");
		}
		if (result.targetAnimation.empty())
		{
			throw JsonError(p_reader.file(), p_reader.pathFor("targetAnimation"), "animation id must not be empty");
		}
		if (p_reader.contains("casterAnimation") && result.casterAnimation.empty())
		{
			throw JsonError(p_reader.file(), p_reader.pathFor("casterAnimation"), "animation id must not be empty");
		}
		if (result.icon[0] < 0 || result.icon[1] < 0)
		{
			throw JsonError(p_reader.file(), p_reader.pathFor("icon"), "icon coordinates must be non-negative");
		}
		if (result.effects.empty())
		{
			throw JsonError(p_reader.file(), p_reader.pathFor("effects"), "ability must contain at least one effect");
		}
		return result;
	}

	Ability parseAbility(JsonReader &p_reader, const Registry<Status> &p_statuses)
	{
		Ability result = parseAbility(p_reader);
		for (std::size_t index = 0; index < result.effects.size(); ++index)
		{
			const auto *apply = dynamic_cast<const ApplyStatusEffect *>(result.effects[index].get());
			if (apply == nullptr)
			{
				continue;
			}
			apply->statusDefinition = p_statuses.tryGet(apply->status);
			if (apply->statusDefinition == nullptr)
			{
				throw JsonError(
					p_reader.file(),
					p_reader.pathFor("effects") + "[" + std::to_string(index) + "].status",
					"unknown status id '" + apply->status + "'");
			}
		}
		return result;
	}
}
