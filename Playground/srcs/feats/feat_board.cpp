#include "feats/feat_board.hpp"

#include "abilities/ability.hpp"
#include "core/json.hpp"
#include "core/registry.hpp"
#include "creatures/creature_species.hpp"
#include "creatures/creature_unit.hpp"
#include "feats/uuid.hpp"
#include "statuses/status.hpp"

#include <map>
#include <queue>
#include <unordered_set>

namespace
{
	[[nodiscard]] spk::UUID parseUuid(const std::string &p_value, const pg::JsonReader &p_reader, const std::string &p_path)
	{
		const std::optional<spk::UUID> result = pg::uuidFromString(p_value);
		if (!result.has_value() || result->isNull())
		{
			throw pg::JsonError(p_reader.file(), p_path, "expected a non-null UUID");
		}
		return *result;
	}

	[[nodiscard]] spk::Vector2 parsePosition(pg::JsonReader &p_reader)
	{
		const nlohmann::json value = p_reader.require<nlohmann::json>("position");
		if (!value.is_array() || value.size() != 2 || !value[0].is_number() || !value[1].is_number())
			throw pg::JsonError(p_reader.file(), p_reader.pathFor("position"), "expected an array of two numbers");
		return {value[0].get<float>(), value[1].get<float>()};
	}
}

namespace pg
{
	const FeatNode *FeatBoard::tryNode(const spk::UUID &p_uuid) const noexcept
	{
		for (const FeatNode &entry : nodes) if (entry.uuid == p_uuid) return &entry;
		return nullptr;
	}

	const FeatNode &FeatBoard::node(const spk::UUID &p_uuid) const
	{
		const FeatNode *result = tryNode(p_uuid);
		if (result == nullptr) throw std::out_of_range("unknown feat node UUID '" + p_uuid.toString() + "'");
		return *result;
	}

	std::vector<const FeatNode *> FeatBoard::neighbours(const FeatNode &p_node) const
	{
		std::vector<const FeatNode *> result;
		result.reserve(p_node.neighbourUuids.size());
		for (const spk::UUID &uuid : p_node.neighbourUuids) result.push_back(&node(uuid));
		return result;
	}

	bool FeatBoard::isRoot(const FeatNode &p_node) const noexcept
	{
		return p_node.uuid == rootNodeUuid;
	}

	bool FeatBoard::isReachable(const FeatNode &p_node, const CreatureUnit &p_unit) const
	{
		const FeatNodeProgress *ownProgress = p_unit.featBoardProgress.findProgress(p_node.uuid);
		if (ownProgress != nullptr && ownProgress->isExhausted(p_node)) return false;
		if (isRoot(p_node)) return ownProgress == nullptr || ownProgress->completionCount == 0;
		bool adjacentToCompleted = false;
		for (const FeatNode *neighbour : neighbours(p_node))
		{
			const FeatNodeProgress *progress = p_unit.featBoardProgress.findProgress(neighbour->uuid);
			if (progress != nullptr && progress->completionCount > 0) { adjacentToCompleted = true; break; }
		}
		if (!adjacentToCompleted) return false;
		if (p_node.kind != FeatNodeKind::Form) return true;
		if (p_unit.species == nullptr) return false;
		for (const auto &reward : p_node.rewards)
		{
			if (const auto *changeForm = dynamic_cast<const ChangeFormReward *>(reward.get()); changeForm != nullptr)
				return p_unit.species->form(changeForm->form).tier == p_unit.species->form(p_unit.currentFormId).tier + 1;
		}
		return false;
	}

	FeatBoard parseFeatBoard(JsonReader &p_reader, const Registry<Ability> &p_abilities, const Registry<Status> &p_statuses)
	{
		p_reader.forbidUnknown({"version", "rootNode", "nodes"});
		if (p_reader.require<int>("version") != 1) throw JsonError(p_reader.file(), p_reader.pathFor("version"), "unsupported feat board version");
		FeatBoard result;
		result.rootNodeUuid = parseUuid(p_reader.require<std::string>("rootNode"), p_reader, p_reader.pathFor("rootNode"));
		std::unordered_set<spk::UUID> nodeUuids;
		std::unordered_set<spk::UUID> requirementUuids;
		static const std::map<std::string, FeatNodeKind> kinds = {{"statsBonus", FeatNodeKind::StatsBonus}, {"ability", FeatNodeKind::Ability}, {"passive", FeatNodeKind::Passive}, {"form", FeatNodeKind::Form}};
		for (JsonReader nodeReader : p_reader.childArray("nodes"))
		{
			nodeReader.forbidUnknown({"uuid", "displayName", "position", "kind", "neighbours", "repeatLimit", "requirements", "rewards"});
			FeatNode node;
			node.uuid = parseUuid(nodeReader.require<std::string>("uuid"), nodeReader, nodeReader.pathFor("uuid"));
			if (!nodeUuids.insert(node.uuid).second) throw JsonError(nodeReader.file(), nodeReader.pathFor("uuid"), "duplicate node UUID");
			node.displayName = nodeReader.require<std::string>("displayName");
			if (node.displayName.empty()) throw JsonError(nodeReader.file(), nodeReader.pathFor("displayName"), "value must not be empty");
			node.position = parsePosition(nodeReader);
			node.kind = nodeReader.requireEnum<FeatNodeKind>("kind", kinds);
			node.repeatLimit = nodeReader.optional<int>("repeatLimit", 1);
			if (node.repeatLimit < 0) throw JsonError(nodeReader.file(), nodeReader.pathFor("repeatLimit"), "value must be non-negative");
			const nlohmann::json neighbours = nodeReader.require<nlohmann::json>("neighbours");
			if (!neighbours.is_array()) throw JsonError(nodeReader.file(), nodeReader.pathFor("neighbours"), "expected an array");
			for (std::size_t index = 0; index < neighbours.size(); ++index)
			{
				if (!neighbours[index].is_string()) throw JsonError(nodeReader.file(), nodeReader.pathFor("neighbours") + "[" + std::to_string(index) + "]", "expected a UUID string");
				const spk::UUID neighbour = parseUuid(neighbours[index].get<std::string>(), nodeReader, nodeReader.pathFor("neighbours") + "[" + std::to_string(index) + "]");
				if (neighbour == node.uuid) throw JsonError(nodeReader.file(), nodeReader.pathFor("neighbours") + "[" + std::to_string(index) + "]", "a node cannot neighbour itself");
				if (std::ranges::find(node.neighbourUuids, neighbour) != node.neighbourUuids.end()) throw JsonError(nodeReader.file(), nodeReader.pathFor("neighbours") + "[" + std::to_string(index) + "]", "duplicate neighbour UUID");
				node.neighbourUuids.push_back(neighbour);
			}
			for (JsonReader requirementReader : nodeReader.childArray("requirements"))
			{
				const spk::UUID uuid = parseUuid(requirementReader.require<std::string>("uuid"), requirementReader, requirementReader.pathFor("uuid"));
				if (!requirementUuids.insert(uuid).second) throw JsonError(requirementReader.file(), requirementReader.pathFor("uuid"), "duplicate requirement UUID");
				nlohmann::json conditionJson = requirementReader.value(); conditionJson.erase("uuid");
				JsonReader conditionReader(conditionJson, requirementReader.file(), requirementReader.path());
				node.requirements.push_back({uuid, parseFeatRequirement(conditionReader)});
			}
			for (JsonReader rewardReader : nodeReader.childArray("rewards")) node.rewards.push_back(parseFeatReward(rewardReader, p_abilities, p_statuses));
			result.nodes.push_back(std::move(node));
		}
		if (result.nodes.empty()) throw JsonError(p_reader.file(), p_reader.pathFor("nodes"), "expected at least one node");
		if (result.tryNode(result.rootNodeUuid) == nullptr) throw JsonError(p_reader.file(), p_reader.pathFor("rootNode"), "root node UUID does not exist");
		for (const FeatNode &node : result.nodes)
		{
			for (const spk::UUID &neighbour : node.neighbourUuids)
				if (result.tryNode(neighbour) == nullptr) throw JsonError(p_reader.file(), "$.nodes", "node '" + node.uuid.toString() + "' has dangling neighbour '" + neighbour.toString() + "'");
		}
		std::unordered_set<spk::UUID> visited; std::queue<const FeatNode *> pending;
		pending.push(&result.node(result.rootNodeUuid)); visited.insert(result.rootNodeUuid);
		while (!pending.empty())
		{
			const FeatNode *current = pending.front(); pending.pop();
			for (const FeatNode *neighbour : result.neighbours(*current)) if (visited.insert(neighbour->uuid).second) pending.push(neighbour);
			for (const FeatNode &candidate : result.nodes)
				if (std::ranges::find(candidate.neighbourUuids, current->uuid) != candidate.neighbourUuids.end() && visited.insert(candidate.uuid).second) pending.push(&candidate);
		}
		if (visited.size() != result.nodes.size()) throw JsonError(p_reader.file(), p_reader.pathFor("nodes"), "feat board graph is not connected");
		return result;
	}
}
