#include "tools/core/feat_board_document.hpp"

#include "core/json.hpp"
#include "feats/feat_board.hpp"
#include "feats/uuid.hpp"

#include <algorithm>
#include <cctype>
#include <queue>
#include <set>
#include <stdexcept>
#include <limits>
#include <unordered_map>
#include <unordered_set>
#include <utility>

namespace
{
	[[nodiscard]] bool validId(const std::string &p_id)
	{
		if (p_id.empty() || p_id.front() == '-' || p_id.back() == '-') return false;
		bool dash = false;
		for (const unsigned char character : p_id)
		{
			const bool currentDash = character == '-';
			if ((!std::islower(character) && !std::isdigit(character) && !currentDash) || (dash && currentDash)) return false;
			dash = currentDash;
		}
		return true;
	}

	[[nodiscard]] std::string formId(const nlohmann::json &p_node)
	{
		if (!p_node.contains("rewards") || !p_node.at("rewards").is_array()) return {};
		for (const auto &reward : p_node.at("rewards"))
			if (reward.value("type", "") == "changeForm") return reward.value("form", "");
		return {};
	}

	[[nodiscard]] const pg::CreatureSpecies *linkedSpecies(
		const std::string &p_boardId,
		const pg::Registries &p_registries)
	{
		for (const std::string &id : p_registries.creatures().ids())
		{
			const pg::CreatureSpecies &species = p_registries.creatures().get(id);
			if (species.featBoardId == p_boardId) return &species;
		}
		return nullptr;
	}

	[[nodiscard]] bool jsonContainsString(const nlohmann::json &p_array, const std::string &p_value)
	{
		return std::ranges::any_of(p_array, [&p_value](const nlohmann::json &p_entry) {
			return p_entry.is_string() && p_entry.get<std::string>() == p_value;
		});
	}

	void eraseJsonString(nlohmann::json &p_array, const std::string &p_value)
	{
		for (auto iterator = p_array.begin(); iterator != p_array.end();)
		{
			if (iterator->is_string() && iterator->get<std::string>() == p_value) iterator = p_array.erase(iterator);
			else ++iterator;
		}
	}
}

namespace pg::tools
{
	void FeatBoardDocument::load(const std::filesystem::path &p_file)
	{
		_file = p_file;
		_sourceFile = p_file;
		_id = p_file.stem().string();
		_json = JsonLoader::parseFile(p_file);
		_selectedNode = _json.contains("rootNode") ? _json.at("rootNode").get<std::string>() : std::string{};
		_dirty = false;
	}

	void FeatBoardDocument::create(const std::filesystem::path &p_directory, std::string p_id)
	{
		if (!validId(p_id)) throw std::invalid_argument("Invalid feat-board id");
		const std::string root = spk::UUID::generate().toString();
		_id = std::move(p_id);
		_file = p_directory / (_id + ".json");
		_sourceFile.clear();
		_json = {
			{"version", 1},
			{"rootNode", root},
			{"nodes", nlohmann::json::array({{
				{"uuid", root}, {"displayName", "Root"}, {"position", {0.0f, 0.0f}},
				{"kind", "statsBonus"}, {"neighbours", nlohmann::json::array()},
				{"repeatLimit", 1}, {"requirements", nlohmann::json::array()}, {"rewards", nlohmann::json::array()}}})}};
		_selectedNode = root;
		_dirty = true;
	}

	void FeatBoardDocument::save(const JsonWriter &p_writer)
	{
		p_writer.write(_file, _json);
		if (!_sourceFile.empty() && _sourceFile != _file)
		{
			std::error_code error;
			std::filesystem::remove(_sourceFile, error);
			if (error) throw std::runtime_error("Unable to remove renamed feat board: " + error.message());
		}
		_sourceFile = _file;
		_dirty = false;
	}

	const std::string &FeatBoardDocument::id() const noexcept { return _id; }
	const std::filesystem::path &FeatBoardDocument::file() const noexcept { return _file; }
	const nlohmann::json &FeatBoardDocument::json() const noexcept { return _json; }
	nlohmann::json &FeatBoardDocument::editJson() noexcept { _dirty = true; return _json; }
	bool FeatBoardDocument::dirty() const noexcept { return _dirty; }
	void FeatBoardDocument::markChanged() noexcept { _dirty = true; }

	void FeatBoardDocument::rename(const std::string &p_id)
	{
		if (p_id == _id) return;
		if (!validId(p_id)) throw std::invalid_argument("Ids use lower-case letters, digits, and single hyphens");
		_id = p_id;
		_file = _file.parent_path() / (_id + ".json");
		_dirty = true;
	}

	nlohmann::json *FeatBoardDocument::_node(const std::string &p_uuid)
	{
		if (!_json.contains("nodes") || !_json["nodes"].is_array()) return nullptr;
		auto iterator = std::ranges::find_if(_json["nodes"], [&p_uuid](const nlohmann::json &p_node) {
			return p_node.value("uuid", "") == p_uuid;
		});
		return iterator == _json["nodes"].end() ? nullptr : &*iterator;
	}

	const nlohmann::json *FeatBoardDocument::_node(const std::string &p_uuid) const
	{
		if (!_json.contains("nodes") || !_json.at("nodes").is_array()) return nullptr;
		const auto &nodes = _json.at("nodes");
		auto iterator = std::ranges::find_if(nodes, [&p_uuid](const nlohmann::json &p_node) {
			return p_node.value("uuid", "") == p_uuid;
		});
		return iterator == nodes.end() ? nullptr : &*iterator;
	}

	const std::string &FeatBoardDocument::selectedNode() const noexcept { return _selectedNode; }
	void FeatBoardDocument::selectNode(const std::string &p_uuid) { if (_node(p_uuid) != nullptr) _selectedNode = p_uuid; }
	nlohmann::json *FeatBoardDocument::selectedNodeJson() { return _node(_selectedNode); }
	const nlohmann::json *FeatBoardDocument::selectedNodeJson() const { return _node(_selectedNode); }

	std::string FeatBoardDocument::addNode(const spk::Vector2 &p_position)
	{
		const std::string uuid = spk::UUID::generate().toString();
		_json["nodes"].push_back({
			{"uuid", uuid}, {"displayName", "New Feat"}, {"position", {p_position.x, p_position.y}},
			{"kind", "statsBonus"}, {"neighbours", nlohmann::json::array()},
			{"repeatLimit", 1}, {"requirements", nlohmann::json::array()}, {"rewards", nlohmann::json::array()}});
		_selectedNode = uuid;
		_dirty = true;
		return uuid;
	}

	bool FeatBoardDocument::deleteNode(const std::string &p_uuid)
	{
		if (_json.value("rootNode", "") == p_uuid || !_json.contains("nodes")) return false;
		auto &nodes = _json["nodes"];
		const auto oldSize = nodes.size();
		for (auto iterator = nodes.begin(); iterator != nodes.end();)
		{
			if (iterator->value("uuid", "") == p_uuid) iterator = nodes.erase(iterator);
			else ++iterator;
		}
		if (nodes.size() == oldSize) return false;
		for (auto &node : nodes)
		{
			if (node.contains("neighbours"))
				eraseJsonString(node["neighbours"], p_uuid);
		}
		_selectedNode = _json.value("rootNode", "");
		_dirty = true;
		return true;
	}

	bool FeatBoardDocument::moveNode(const std::string &p_uuid, const spk::Vector2 &p_position)
	{
		nlohmann::json *node = _node(p_uuid);
		if (node == nullptr) return false;
		(*node)["position"] = {p_position.x, p_position.y};
		_dirty = true;
		return true;
	}

	bool FeatBoardDocument::toggleLink(const std::string &p_first, const std::string &p_second)
	{
		if (p_first == p_second) return false;
		nlohmann::json *first = _node(p_first);
		nlohmann::json *second = _node(p_second);
		if (first == nullptr || second == nullptr) return false;
		auto &firstLinks = (*first)["neighbours"];
		auto &secondLinks = (*second)["neighbours"];
		const bool linked = jsonContainsString(firstLinks, p_second);
		if (linked)
		{
			eraseJsonString(firstLinks, p_second);
			eraseJsonString(secondLinks, p_first);
		}
		else
		{
			firstLinks.push_back(p_second);
			if (!jsonContainsString(secondLinks, p_first)) secondLinks.push_back(p_first);
		}
		_dirty = true;
		return true;
	}

	std::vector<std::string> FeatBoardDocument::requirementTypes()
	{
		return {"dealDamage", "takeDamage", "surviveHit", "maxDamageAbsorbedInOneHit", "healHealth", "healTarget",
			"applyShield", "applyShieldCount", "shieldBroken", "absorbDamageWithShield", "applyStatusCount",
			"removeStatusCount", "killCount", "lastHit", "winBattleCount", "consumeResources",
			"totalDistanceTravelled", "teleportDistance", "teleportCount", "displacementDealt",
			"displacementReceived", "turnStartPosition", "turnEndPosition", "castAbilityCount"};
	}

	std::vector<std::string> FeatBoardDocument::rewardTypes()
	{
		return {"bonusStats", "ability", "removeAbility", "passive", "changeForm"};
	}

	nlohmann::json FeatBoardDocument::requirementDefaults(const std::string &p_type)
	{
		nlohmann::json result = {{"type", p_type}};
		const auto scoped = [&result]() { result["scope"] = "fight"; };
		if (p_type == "dealDamage" || p_type == "takeDamage") { scoped(); result["requiredAmount"] = 1; result["damageKind"] = "any"; result["sourceAbilities"] = nlohmann::json::array(); }
		else if (p_type == "surviveHit" || p_type == "maxDamageAbsorbedInOneHit") result["requiredAmount"] = 1;
		else if (p_type == "healHealth" || p_type == "healTarget") { scoped(); result["requiredAmount"] = 1; if (p_type == "healTarget") result["target"] = "any"; }
		else if (p_type == "applyShield" || p_type == "applyShieldCount" || p_type == "shieldBroken") { scoped(); result[p_type == "applyShield" ? "requiredAmount" : "requiredCount"] = 1; result["kind"] = "any"; }
		else if (p_type == "absorbDamageWithShield") { scoped(); result["requiredAmount"] = 1; }
		else if (p_type == "applyStatusCount" || p_type == "removeStatusCount") { scoped(); result["requiredCount"] = 1; result["status"] = ""; result["sourceAbilities"] = nlohmann::json::array(); }
		else if (p_type == "killCount" || p_type == "lastHit") { scoped(); result["requiredCount"] = 1; result["sourceAbilities"] = nlohmann::json::array(); }
		else if (p_type == "winBattleCount") result["requireUnitSurvival"] = false;
		else if (p_type == "consumeResources") { scoped(); result["resource"] = "ap"; result["requiredAmount"] = 1; }
		else if (p_type == "totalDistanceTravelled" || p_type == "teleportDistance") { scoped(); result["requiredDistance"] = 1; }
		else if (p_type == "teleportCount") { scoped(); result["requiredCount"] = 1; }
		else if (p_type == "displacementDealt") { scoped(); result["requiredDistance"] = 1; }
		else if (p_type == "displacementReceived") { scoped(); result["requiredCount"] = 1; }
		else if (p_type == "turnStartPosition" || p_type == "turnEndPosition") { result["target"] = "enemy"; result["condition"] = "within"; result["distance"] = 1; result["maximumDistance"] = 1; }
		else if (p_type == "castAbilityCount") { scoped(); result["abilities"] = nlohmann::json::array(); result["requiredCount"] = 1; result["targetRangeCondition"] = "either"; result["range"] = 0; }
		else throw std::invalid_argument("Unknown feat requirement type '" + p_type + "'");
		return result;
	}

	nlohmann::json FeatBoardDocument::rewardDefaults(const std::string &p_type)
	{
		if (p_type == "bonusStats") return {{"type", p_type}, {"attribute", "health"}, {"value", 1}};
		if (p_type == "ability" || p_type == "removeAbility") return {{"type", p_type}, {"ability", ""}};
		if (p_type == "passive") return {{"type", p_type}, {"status", ""}};
		if (p_type == "changeForm") return {{"type", p_type}, {"form", ""}};
		throw std::invalid_argument("Unknown feat reward type '" + p_type + "'");
	}

	std::string FeatBoardDocument::addRequirement(const std::string &p_type)
	{
		nlohmann::json *node = selectedNodeJson();
		if (node == nullptr) return {};
		const std::string uuid = spk::UUID::generate().toString();
		nlohmann::json requirement = requirementDefaults(p_type);
		requirement["uuid"] = uuid;
		(*node)["requirements"].push_back(std::move(requirement));
		_dirty = true;
		return uuid;
	}

	void FeatBoardDocument::removeRequirement(std::size_t p_index)
	{
		if (nlohmann::json *node = selectedNodeJson(); node != nullptr && p_index < (*node)["requirements"].size())
		{
			(*node)["requirements"].erase((*node)["requirements"].begin() + static_cast<std::ptrdiff_t>(p_index));
			_dirty = true;
		}
	}

	void FeatBoardDocument::setRequirementType(std::size_t p_index, const std::string &p_type)
	{
		nlohmann::json *node = selectedNodeJson();
		if (node == nullptr || p_index >= (*node)["requirements"].size()) return;
		const std::string uuid = (*node)["requirements"][p_index].value("uuid", spk::UUID::generate().toString());
		(*node)["requirements"][p_index] = requirementDefaults(p_type);
		(*node)["requirements"][p_index]["uuid"] = uuid;
		_dirty = true;
	}

	void FeatBoardDocument::addReward(const std::string &p_type)
	{
		if (nlohmann::json *node = selectedNodeJson(); node != nullptr)
		{
			(*node)["rewards"].push_back(rewardDefaults(p_type));
			_dirty = true;
		}
	}

	void FeatBoardDocument::removeReward(std::size_t p_index)
	{
		if (nlohmann::json *node = selectedNodeJson(); node != nullptr && p_index < (*node)["rewards"].size())
		{
			(*node)["rewards"].erase((*node)["rewards"].begin() + static_cast<std::ptrdiff_t>(p_index));
			_dirty = true;
		}
	}

	void FeatBoardDocument::setRewardType(std::size_t p_index, const std::string &p_type)
	{
		nlohmann::json *node = selectedNodeJson();
		if (node == nullptr || p_index >= (*node)["rewards"].size()) return;
		(*node)["rewards"][p_index] = rewardDefaults(p_type);
		_dirty = true;
	}

	std::size_t FeatBoardDocument::fixNeighbourSymmetry(nlohmann::json &p_json)
	{
		if (!p_json.contains("nodes") || !p_json["nodes"].is_array()) return 0;
		std::unordered_map<std::string, nlohmann::json *> nodes;
		for (auto &node : p_json["nodes"]) nodes[node.value("uuid", "")] = &node;
		std::size_t fixes = 0;
		for (auto &node : p_json["nodes"])
		{
			const std::string id = node.value("uuid", "");
			for (const auto &linked : node.value("neighbours", nlohmann::json::array()))
			{
				if (!linked.is_string()) continue;
				auto found = nodes.find(linked.get<std::string>());
				if (found != nodes.end())
				{
					auto &reverse = (*found->second)["neighbours"];
					if (!jsonContainsString(reverse, id)) { reverse.push_back(id); ++fixes; }
				}
			}
		}
		return fixes;
	}

	bool FeatBoardDocument::reorderFormNodes(
		nlohmann::json &p_json,
		const std::string &p_boardId,
		const Registries &p_registries)
	{
		const CreatureSpecies *species = ::linkedSpecies(p_boardId, p_registries);
		if (species == nullptr || !p_json.contains("nodes")) return false;
		std::vector<std::size_t> slots;
		std::vector<nlohmann::json> forms;
		for (std::size_t index = 0; index < p_json["nodes"].size(); ++index)
		{
			if (p_json["nodes"][index].value("kind", "") == "form")
			{
				slots.push_back(index);
				forms.push_back(p_json["nodes"][index]);
			}
		}
		auto tier = [species](const nlohmann::json &p_node) {
			const std::string form = formId(p_node);
			const auto found = species->forms.find(form);
			return found == species->forms.end() ? std::numeric_limits<int>::max() : found->second.tier;
		};
		const auto original = forms;
		std::ranges::stable_sort(forms, {}, tier);
		if (forms == original) return false;
		for (std::size_t index = 0; index < slots.size(); ++index) p_json["nodes"][slots[index]] = forms[index];
		return true;
	}

	std::vector<FeatBoardValidationIssue> FeatBoardDocument::validate(
		const nlohmann::json &p_json,
		const std::string &p_boardId,
		const Registries &p_registries)
	{
		using Code = FeatBoardValidationIssue::Code;
		std::vector<FeatBoardValidationIssue> issues;
		if (!p_json.contains("nodes") || !p_json.at("nodes").is_array() || p_json.at("nodes").empty())
		{
			issues.push_back({Code::EmptyBoard, "The board has no nodes", false});
			return issues;
		}
		std::unordered_map<std::string, const nlohmann::json *> nodes;
		std::unordered_set<std::string> requirementIds;
		for (const auto &node : p_json.at("nodes"))
		{
			const std::string id = node.value("uuid", "");
			if (!uuidFromString(id).has_value()) issues.push_back({Code::InvalidUuid, "Invalid node UUID '" + id + "'", false});
			else if (!nodes.emplace(id, &node).second) issues.push_back({Code::DuplicateUuid, "Duplicate node UUID '" + id + "'", false});
			for (const auto &requirement : node.value("requirements", nlohmann::json::array()))
			{
				const std::string requirementId = requirement.value("uuid", "");
				if (!uuidFromString(requirementId).has_value()) issues.push_back({Code::InvalidUuid, "Invalid requirement UUID '" + requirementId + "'", false});
				else if (!requirementIds.insert(requirementId).second) issues.push_back({Code::DuplicateUuid, "Duplicate requirement UUID '" + requirementId + "'", false});
			}
		}
		const std::string root = p_json.value("rootNode", "");
		if (!nodes.contains(root)) issues.push_back({Code::MissingRoot, "The root node does not exist", false});
		for (const auto &[id, node] : nodes)
		{
			std::unordered_set<std::string> links;
			for (const auto &value : node->value("neighbours", nlohmann::json::array()))
			{
				const std::string linked = value.is_string() ? value.get<std::string>() : std::string{};
				if (!nodes.contains(linked)) issues.push_back({Code::DanglingNeighbour, "Node '" + id + "' links to missing node '" + linked + "'", false});
				else if (linked != id)
				{
					links.insert(linked);
					const auto reverse = nodes.at(linked)->value("neighbours", nlohmann::json::array());
					if (!jsonContainsString(reverse, id)) issues.push_back({Code::AsymmetricNeighbour, "Link '" + id + "' -> '" + linked + "' is not symmetric", true});
				}
			}
		}
		if (nodes.contains(root))
		{
			std::unordered_set<std::string> visited{root};
			std::queue<std::string> pending; pending.push(root);
			while (!pending.empty())
			{
				const std::string current = pending.front(); pending.pop();
				for (const auto &linked : nodes.at(current)->value("neighbours", nlohmann::json::array()))
					if (linked.is_string() && nodes.contains(linked.get<std::string>()) && visited.insert(linked.get<std::string>()).second) pending.push(linked.get<std::string>());
			}
			if (visited.size() != nodes.size()) issues.push_back({Code::DisconnectedGraph, "The board graph is not connected", false});
		}

		const auto validateReferences = [&](const nlohmann::json &p_value, const auto &self) -> void {
			if (p_value.is_object())
			{
				for (const auto &[key, value] : p_value.items())
				{
					if (key == "ability" && value.is_string() && p_registries.abilities().tryGet(value.get<std::string>()) == nullptr)
						issues.push_back({Code::DanglingAbility, "Unknown ability '" + value.get<std::string>() + "'", false});
					else if (key == "status" && value.is_string() && !value.get<std::string>().empty() && p_registries.statuses().tryGet(value.get<std::string>()) == nullptr)
						issues.push_back({Code::DanglingStatus, "Unknown status '" + value.get<std::string>() + "'", false});
					else if ((key == "abilities" || key == "sourceAbilities") && value.is_array())
						for (const auto &id : value) if (id.is_string() && p_registries.abilities().tryGet(id.get<std::string>()) == nullptr)
							issues.push_back({Code::DanglingAbility, "Unknown ability '" + id.get<std::string>() + "'", false});
					self(value, self);
				}
			}
			else if (p_value.is_array()) for (const auto &entry : p_value) self(entry, self);
		};
		validateReferences(p_json, validateReferences);

		const CreatureSpecies *species = ::linkedSpecies(p_boardId, p_registries);
		if (species != nullptr)
		{
			int previousTier = -1;
			for (const auto &node : p_json.at("nodes"))
			{
				if (node.value("kind", "") != "form") continue;
				const std::string form = formId(node);
				const auto found = species->forms.find(form);
				if (found != species->forms.end())
				{
					if (found->second.tier < previousTier) issues.push_back({Code::FormTierOrder, "Form nodes are not ordered by ascending tier", true});
					previousTier = found->second.tier;
				}
			}
		}

		try
		{
			nlohmann::json copy = p_json;
			JsonReader reader(copy, p_boardId + ".json");
			(void)parseFeatBoard(reader, p_registries.abilities(), p_registries.statuses());
		}
		catch (const std::exception &exception)
		{
			issues.push_back({Code::ParseError, exception.what(), false});
		}
		return issues;
	}

	std::vector<std::string> FeatBoardDocument::linkedSpecies(const Registries &p_registries) const
	{
		std::vector<std::string> result;
		for (const std::string &id : p_registries.creatures().ids())
			if (p_registries.creatures().get(id).featBoardId == _id) result.push_back(id);
		return result;
	}
}
