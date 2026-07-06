#pragma once

#include "feats/feat_node.hpp"

#include <string>
#include <vector>

namespace pg
{
	class JsonReader;
	class CreatureUnit;
	struct Ability;
	struct Status;
	template <typename TDefinition> class Registry;

	class FeatBoard
	{
	public:
		std::string id;
		spk::UUID rootNodeUuid;
		std::vector<FeatNode> nodes;

		[[nodiscard]] const FeatNode *tryNode(const spk::UUID &p_uuid) const noexcept;
		[[nodiscard]] const FeatNode &node(const spk::UUID &p_uuid) const;
		[[nodiscard]] std::vector<const FeatNode *> neighbours(const FeatNode &p_node) const;
		[[nodiscard]] bool isRoot(const FeatNode &p_node) const noexcept;
		[[nodiscard]] bool isReachable(const FeatNode &p_node, const CreatureUnit &p_unit) const;
	};

	[[nodiscard]] FeatBoard parseFeatBoard(
		JsonReader &p_reader,
		const Registry<Ability> &p_abilities,
		const Registry<Status> &p_statuses);
}
