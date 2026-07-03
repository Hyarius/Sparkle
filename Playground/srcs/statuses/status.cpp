#include "statuses/status.hpp"

#include "abilities/effect.hpp"
#include "abilities/effects/effect_types.hpp"
#include "core/json.hpp"
#include "core/registry.hpp"

#include <map>
#include <set>

namespace pg
{
	Status parseStatus(JsonReader &p_reader)
	{
		p_reader.forbidUnknown({"version", "displayName", "icon", "tags", "hookPoint", "effects"});
		if (p_reader.require<int>("version") != 1)
		{
			throw JsonError(p_reader.file(), p_reader.pathFor("version"), "unsupported status version");
		}
		static const std::map<std::string, StatusHookPoint> hooks = {
			{"turnStart", StatusHookPoint::TurnStart},
			{"turnEnd", StatusHookPoint::TurnEnd},
			{"beforeConsumingResources", StatusHookPoint::BeforeConsumingResources},
			{"afterConsumingResources", StatusHookPoint::AfterConsumingResources},
			{"onHPLoss", StatusHookPoint::OnHPLoss},
			{"onAPLoss", StatusHookPoint::OnAPLoss},
			{"onMPLoss", StatusHookPoint::OnMPLoss},
			{"onHPGain", StatusHookPoint::OnHPGain},
			{"onAPGain", StatusHookPoint::OnAPGain},
			{"onMPGain", StatusHookPoint::OnMPGain}};

		Status result;
		result.displayName = p_reader.require<std::string>("displayName");
		result.icon = p_reader.require<std::array<int, 2>>("icon");
		result.tags = p_reader.require<std::vector<std::string>>("tags");
		result.hookPoint = p_reader.requireEnum<StatusHookPoint>("hookPoint", hooks);
		for (JsonReader effectReader : p_reader.childArray("effects"))
		{
			result.effects.emplace_back(parseEffect(effectReader));
		}
		if (result.displayName.empty() || result.icon[0] < 0 || result.icon[1] < 0)
		{
			throw JsonError(p_reader.file(), p_reader.path(), "status contains invalid display or icon values");
		}
		std::set<std::string> uniqueTags;
		for (const std::string &tag : result.tags)
		{
			if (tag.empty() || !uniqueTags.insert(tag).second)
			{
				throw JsonError(p_reader.file(), p_reader.pathFor("tags"), "status tags must be non-empty and unique");
			}
		}
		return result;
	}

	void resolveStatusReferences(
		Status &p_status, const Registry<Status> &p_statuses, const std::filesystem::path &p_file)
	{
		for (std::size_t index = 0; index < p_status.effects.size(); ++index)
		{
			const auto *apply = dynamic_cast<const ApplyStatusEffect *>(p_status.effects[index].get());
			if (apply == nullptr)
			{
				continue;
			}
			apply->statusDefinition = p_statuses.tryGet(apply->status);
			if (apply->statusDefinition == nullptr)
			{
				throw JsonError(
					p_file,
					"$.effects[" + std::to_string(index) + "].status",
					"unknown status id '" + apply->status + "'");
			}
		}
	}
}
