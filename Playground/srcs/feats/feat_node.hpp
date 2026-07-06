#pragma once

#include "feats/feat_requirement.hpp"
#include "feats/feat_reward.hpp"
#include "structures/container/spk_uuid.hpp"
#include "structures/math/spk_vector2.hpp"

#include <memory>
#include <string>
#include <vector>

namespace pg
{
	enum class FeatNodeKind
	{
		StatsBonus,
		Ability,
		Passive,
		Form
	};

	struct FeatRequirementEntry
	{
		spk::UUID uuid;
		std::unique_ptr<FeatRequirement> condition;
	};

	struct FeatNode
	{
		spk::UUID uuid;
		std::string displayName;
		spk::Vector2 position;
		FeatNodeKind kind = FeatNodeKind::StatsBonus;
		std::vector<spk::UUID> neighbourUuids;
		int repeatLimit = 1;
		std::vector<FeatRequirementEntry> requirements;
		std::vector<std::unique_ptr<FeatReward>> rewards;
	};
}
