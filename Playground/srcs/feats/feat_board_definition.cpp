#include "feats/feat_board_definition.hpp"

#include "core/content_id.hpp"

#include <algorithm>
#include <deque>
#include <variant>

namespace pg
{
	namespace
	{
		[[nodiscard]] std::optional<std::string> optionalContentId(
			const JsonReader &p_reader,
			const std::string &p_key,
			std::string_view p_kind)
		{
			// Omission-only: an explicit null is not "absent", it is a string field holding the
			// wrong type, and gets the same path-aware error everywhere.
			if (!p_reader.contains(p_key))
			{
				return std::nullopt;
			}

			std::string value = p_reader.require<std::string>(p_key);
			requireContentId(value, p_reader.file(), p_reader.pathFor(p_key), p_kind);
			return value;
		}

		[[nodiscard]] std::array<int, 2> requirePosition(const JsonReader &p_reader)
		{
			const std::array<int, 2> position = p_reader.require<std::array<int, 2>>("position");
			for (const int coordinate : position)
			{
				if (coordinate < -MaximumFeatNodeCoordinate || coordinate > MaximumFeatNodeCoordinate)
				{
					throw JsonError(
						p_reader.file(),
						p_reader.pathFor("position"),
						"board coordinates are integers in [-" + std::to_string(MaximumFeatNodeCoordinate) + ", " +
							std::to_string(MaximumFeatNodeCoordinate) + "]");
				}
			}
			return position;
		}

		[[nodiscard]] std::vector<std::string> requireNeighbours(const JsonReader &p_reader, const std::string &p_id)
		{
			const std::vector<std::string> authored = p_reader.require<std::vector<std::string>>("neighbours");

			std::vector<std::string> result;
			result.reserve(authored.size());
			for (const std::string &neighbour : authored)
			{
				requireContentId(neighbour, p_reader.file(), p_reader.pathFor("neighbours"), "feat node id");
				if (neighbour == p_id)
				{
					throw JsonError(
						p_reader.file(),
						p_reader.pathFor("neighbours"),
						"node '" + p_id + "' names itself as a neighbour");
				}
				if (std::ranges::find(result, neighbour) != result.end())
				{
					throw JsonError(
						p_reader.file(),
						p_reader.pathFor("neighbours"),
						"duplicate neighbour '" + neighbour + "' on node '" + p_id + "'");
				}
				result.push_back(neighbour);
			}
			return result;
		}

		[[nodiscard]] FeatNodeDefinition parseFeatNode(
			JsonReader &p_reader,
			ConditionParseState &p_conditions,
			std::set<std::string> &p_rewardIds)
		{
			// A stale "repeatLimit", "repeatCount" or "repeatable" fails here: nodes are one-time
			// completions and there is no repeat model to reinterpret them into.
			p_reader.forbidUnknown(
				{"id",
				 "displayNameKey",
				 "descriptionKey",
				 "position",
				 "kind",
				 "neighbours",
				 "exclusiveGroup",
				 "minimumCompletedNodes",
				 "fromForm",
				 "requirements",
				 "rewards"});

			FeatNodeDefinition result;
			result.id = p_reader.require<std::string>("id");
			requireContentId(result.id, p_reader.file(), p_reader.pathFor("id"), "feat node id");
			result.displayNameKey = requireDisplayNameKey(p_reader);
			result.descriptionKey = requireDescriptionKey(p_reader);
			result.position = requirePosition(p_reader);
			result.kind = p_reader.requireEnum<FeatNodeKind>("kind", featNodeKindNames());
			result.neighbours = requireNeighbours(p_reader, result.id);
			result.exclusiveGroup = optionalContentId(p_reader, "exclusiveGroup", "exclusive group id");
			result.minimumCompletedNodes = static_cast<std::uint32_t>(
				requireIntegerInRange(p_reader, "minimumCompletedNodes", 0, MaximumConditionAmount));
			result.fromForm = optionalContentId(p_reader, "fromForm", "form id");
			result.source = DefinitionSource{p_reader.file(), p_reader.path()};

			// The root is the only node allowed no requirement: every other node has to be earned.
			const bool isRoot = result.kind == FeatNodeKind::Root;
			result.requirements = parseConditions(p_reader, "requirements", p_conditions, isRoot);
			if (!isRoot && result.requirements.empty())
			{
				throw JsonError(
					p_reader.file(),
					p_reader.pathFor("requirements"),
					"only the root node may have no requirement");
			}

			result.rewards = parseFeatRewards(p_reader, "rewards");
			for (const FeatRewardSpec &reward : result.rewards)
			{
				if (!p_rewardIds.insert(reward.id).second)
				{
					throw JsonError(
						reward.source.file,
						reward.source.jsonPath,
						"duplicate reward id '" + reward.id + "' in this board");
				}
			}
			return result;
		}

		[[noreturn]] void failNode(const FeatNodeDefinition &p_node, const std::string &p_message)
		{
			throw JsonError(p_node.source.file, p_node.source.jsonPath, "node '" + p_node.id + "' " + p_message);
		}

		[[nodiscard]] std::size_t countRewards(
			const FeatNodeDefinition &p_node,
			const std::function<bool(const FeatRewardPayload &)> &p_predicate)
		{
			return static_cast<std::size_t>(std::ranges::count_if(p_node.rewards, [&p_predicate](const FeatRewardSpec &p_reward) {
				return p_predicate(p_reward.payload);
			}));
		}

		template <typename TPayload>
		[[nodiscard]] std::size_t countRewardsOf(const FeatNodeDefinition &p_node)
		{
			return countRewards(p_node, [](const FeatRewardPayload &p_payload) {
				return std::holds_alternative<TPayload>(p_payload);
			});
		}

		// A node's kind is a promise about what it grants, so the two cannot disagree. Extra
		// compatible rewards stay legal: a meaningful node may grant more than one result.
		void validateNodeKind(const FeatNodeDefinition &p_node)
		{
			const std::size_t changeForms = countRewardsOf<ChangeFormRewardSpec>(p_node);
			if (p_node.kind != FeatNodeKind::Evolution && changeForms != 0)
			{
				failNode(p_node, "changes form, so its kind must be 'evolution'");
			}

			switch (p_node.kind)
			{
			case FeatNodeKind::Root:
				// Root rewards replay on fresh construction and cannot depend on events, since the
				// root has no requirement. A deterministic baseline grant is therefore fine.
				return;
			case FeatNodeKind::Stats:
				if (countRewardsOf<BonusStatRewardSpec>(p_node) == 0)
				{
					failNode(p_node, "is a stats node, so it grants at least one 'bonusStat' reward");
				}
				return;
			case FeatNodeKind::Ability:
				if (countRewards(p_node, [](const FeatRewardPayload &p_payload) {
						return std::holds_alternative<UnlockAbilityRewardSpec>(p_payload) ||
							   std::holds_alternative<RemoveAbilityRewardSpec>(p_payload);
					}) == 0)
				{
					failNode(p_node, "is an ability node, so it unlocks or removes at least one ability");
				}
				return;
			case FeatNodeKind::Passive:
				if (countRewardsOf<UnlockPassiveRewardSpec>(p_node) == 0)
				{
					failNode(p_node, "is a passive node, so it grants at least one 'unlockPassive' reward");
				}
				return;
			case FeatNodeKind::Evolution:
				if (changeForms != 1)
				{
					failNode(p_node, "is an evolution node, so it carries exactly one 'changeForm' reward");
				}
				return;
			}
		}

		[[nodiscard]] const ChangeFormRewardSpec &changeFormOf(const FeatNodeDefinition &p_node)
		{
			for (const FeatRewardSpec &reward : p_node.rewards)
			{
				if (const auto *changeForm = std::get_if<ChangeFormRewardSpec>(&reward.payload))
				{
					return *changeForm;
				}
			}
			failNode(p_node, "is an evolution node with no 'changeForm' reward");
		}

		void validateRoot(const FeatBoardDefinition &p_board)
		{
			const FeatNodeDefinition *root = nullptr;
			for (const FeatNodeDefinition &node : p_board.nodes)
			{
				if (node.kind != FeatNodeKind::Root)
				{
					continue;
				}
				if (root != nullptr)
				{
					failNode(node, "is a second root; a board has exactly one");
				}
				root = &node;
			}

			if (root == nullptr || root->id != p_board.rootNodeId)
			{
				throw JsonError(
					p_board.source.file,
					p_board.source.jsonPath + ".rootNode",
					"rootNode must name the one node of kind 'root'");
			}
			if (root->minimumCompletedNodes != 0)
			{
				failNode(*root, "starts completed, so it cannot gate on other completed nodes");
			}
			if (root->fromForm.has_value())
			{
				failNode(*root, "starts completed before any evolution, so it cannot gate on a form");
			}
			if (root->exclusiveGroup.has_value())
			{
				failNode(*root, "starts completed, so it can never be an exclusive choice");
			}
		}

		void validateAdjacency(const FeatBoardDefinition &p_board)
		{
			std::map<std::string, const FeatNodeDefinition *> byId;
			for (const FeatNodeDefinition &node : p_board.nodes)
			{
				if (!byId.emplace(node.id, &node).second)
				{
					failNode(node, "has a duplicate id");
				}
			}

			for (const FeatNodeDefinition &node : p_board.nodes)
			{
				if (node.minimumCompletedNodes >= p_board.nodes.size())
				{
					failNode(
						node,
						"gates on more completed nodes than the board has; minimumCompletedNodes is at most " +
							std::to_string(p_board.nodes.size() - 1));
				}

				for (const std::string &neighbourId : node.neighbours)
				{
					const auto entry = byId.find(neighbourId);
					if (entry == byId.end())
					{
						failNode(node, "names an unknown neighbour '" + neighbourId + "'");
					}
					// The graph is undirected, so an edge authored on one side only would make
					// activation depend on which node the traversal happened to reach first.
					if (std::ranges::find(entry->second->neighbours, node.id) == entry->second->neighbours.end())
					{
						failNode(node, "names '" + neighbourId + "', which does not name it back");
					}
				}
			}

			std::set<std::string> reached;
			std::deque<std::string> pending{p_board.rootNodeId};
			reached.insert(p_board.rootNodeId);
			while (!pending.empty())
			{
				const FeatNodeDefinition &node = *byId.at(pending.front());
				pending.pop_front();
				for (const std::string &neighbourId : node.neighbours)
				{
					if (reached.insert(neighbourId).second)
					{
						pending.push_back(neighbourId);
					}
				}
			}

			// A node no path reaches from the root can never activate, whatever its requirements.
			for (const FeatNodeDefinition &node : p_board.nodes)
			{
				if (!reached.contains(node.id))
				{
					failNode(node, "is not reachable from the root node");
				}
			}
		}

		void validateExclusiveGroups(const FeatBoardDefinition &p_board)
		{
			std::map<std::string, std::vector<const FeatNodeDefinition *>> groups;
			for (const FeatNodeDefinition &node : p_board.nodes)
			{
				if (node.exclusiveGroup.has_value())
				{
					groups[*node.exclusiveGroup].push_back(&node);
				}
			}

			for (const auto &[groupId, members] : groups)
			{
				if (members.size() < 2)
				{
					failNode(
						*members.front(),
						"is the only member of exclusive group '" + groupId + "', which therefore excludes nothing");
				}

				std::set<std::string> forms;
				for (const FeatNodeDefinition *member : members)
				{
					if (member->kind != FeatNodeKind::Evolution)
					{
						failNode(*member, "is in exclusive group '" + groupId + "', so its kind must be 'evolution'");
					}
					// Two members leading to the same form would make the choice meaningless while
					// still blocking the other branch forever.
					if (!forms.insert(changeFormOf(*member).form).second)
					{
						failNode(
							*member,
							"changes into the same form as another member of exclusive group '" + groupId + "'");
					}
				}
			}
		}
	}

	const std::map<std::string, FeatNodeKind> &featNodeKindNames()
	{
		static const std::map<std::string, FeatNodeKind> names{
			{"ability", FeatNodeKind::Ability},
			{"evolution", FeatNodeKind::Evolution},
			{"passive", FeatNodeKind::Passive},
			{"root", FeatNodeKind::Root},
			{"stats", FeatNodeKind::Stats}};
		return names;
	}

	FeatBoardDefinition parseFeatBoardDefinition(JsonReader &p_reader, const ConditionLimits &p_limits)
	{
		p_reader.forbidUnknown({"version", "displayNameKey", "rootNode", "nodes"});
		requireVersion(p_reader, FeatBoardSchemaVersion);

		FeatBoardDefinition result;
		result.displayNameKey = requireDisplayNameKey(p_reader);
		result.rootNodeId = p_reader.require<std::string>("rootNode");
		requireContentId(result.rootNodeId, p_reader.file(), p_reader.pathFor("rootNode"), "feat node id");
		result.source = DefinitionSource{p_reader.file(), p_reader.path()};

		std::vector<JsonReader> entries = p_reader.childArray("nodes");
		if (entries.empty())
		{
			throw JsonError(p_reader.file(), p_reader.pathFor("nodes"), "a board has at least its root node");
		}

		// Condition and reward ids are unique across the whole board, not merely within a node:
		// persisted progress keys off them once the arrays have been reordered.
		ConditionParseState conditions;
		conditions.limits = p_limits;
		std::set<std::string> rewardIds;

		result.nodes.reserve(entries.size());
		for (JsonReader &entry : entries)
		{
			result.nodes.push_back(parseFeatNode(entry, conditions, rewardIds));
		}

		validateRoot(result);
		validateAdjacency(result);
		for (const FeatNodeDefinition &node : result.nodes)
		{
			validateNodeKind(node);
		}
		validateExclusiveGroups(result);
		return result;
	}

	void validateFeatBoardFormReferences(
		const FeatBoardDefinition &p_board,
		const std::set<std::string, std::less<>> &p_formIds,
		std::string_view p_speciesId)
	{
		const auto require = [&p_formIds, &p_speciesId](
								 const DefinitionSource &p_source,
								 const std::string &p_form,
								 const std::string &p_what) {
			if (!p_formIds.contains(p_form))
			{
				throw JsonError(
					p_source.file,
					p_source.jsonPath,
					p_what + " references form '" + p_form + "', which species '" + std::string(p_speciesId) +
						"' does not define");
			}
		};

		for (const FeatNodeDefinition &node : p_board.nodes)
		{
			if (node.fromForm.has_value())
			{
				require(node.source, *node.fromForm, "node '" + node.id + "' fromForm");
			}
			for (const FeatRewardSpec &reward : node.rewards)
			{
				if (const auto *changeForm = std::get_if<ChangeFormRewardSpec>(&reward.payload))
				{
					require(reward.source, changeForm->form, "reward '" + reward.id + "'");
				}
			}
		}
	}

	std::set<std::string> featBoardFormReferences(const FeatBoardDefinition &p_board)
	{
		std::set<std::string> result;
		for (const FeatNodeDefinition &node : p_board.nodes)
		{
			if (node.fromForm.has_value())
			{
				result.insert(*node.fromForm);
			}
			for (const FeatRewardSpec &reward : node.rewards)
			{
				if (const auto *changeForm = std::get_if<ChangeFormRewardSpec>(&reward.payload))
				{
					result.insert(changeForm->form);
				}
			}
		}
		return result;
	}
}
