#include "tools/pages/featboard_editor_page.hpp"

#include "tools/widgets/id_picker.hpp"

#include <algorithm>
#include <cctype>
#include <set>
#include <sstream>
#include <stdexcept>
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

	[[nodiscard]] std::vector<std::string> split(const std::string &p_text)
	{
		std::vector<std::string> result;
		std::stringstream stream(p_text);
		std::string value;
		while (std::getline(stream, value, ','))
		{
			const std::size_t first = value.find_first_not_of(" \t\r\n");
			const std::size_t last = value.find_last_not_of(" \t\r\n");
			if (first != std::string::npos) result.push_back(value.substr(first, last - first + 1));
		}
		return result;
	}

	[[nodiscard]] std::string join(const nlohmann::json &p_values)
	{
		std::string result;
		for (const auto &value : p_values)
		{
			if (!value.is_string()) continue;
			if (!result.empty()) result += ", ";
			result += value.get<std::string>();
		}
		return result;
	}

	[[nodiscard]] spk::Color nodeColor(const std::string &p_kind)
	{
		if (p_kind == "ability") return {0.2f, 0.48f, 0.9f, 1.0f};
		if (p_kind == "passive") return {0.58f, 0.3f, 0.75f, 1.0f};
		if (p_kind == "form") return {0.9f, 0.5f, 0.18f, 1.0f};
		return {0.2f, 0.65f, 0.4f, 1.0f};
	}
}

namespace pg::tools
{
	FeatBoardEditorPage::FeatBoardEditorPage(
		const std::string &p_name,
		spk::Widget *p_parent,
		ToolServices &p_services) :
		IEditorPage(p_name, p_parent),
		_services(p_services),
		_background(p_name + "/Background", this),
		_leftPane(p_name + "/Boards", this),
		_newButton(p_name + "/New", "New Board", &_leftPane),
		_saveButton(p_name + "/Save", "Save", &_leftPane),
		_frameButton(p_name + "/Frame", "Frame Graph", &_leftPane),
		_canvas(p_name + "/Canvas", this),
		_properties(p_name + "/Properties", this)
	{
		_background.activate();
		_leftPane.activate();
		_newButton.activate();
		_saveButton.activate();
		_frameButton.activate();
		_canvas.activate();
		_properties.activate();
		_newButton.subscribeToClick([this]() { _newBoard(); }).relinquish();
		_saveButton.subscribeToClick([this]() { save(); }).relinquish();
		_frameButton.subscribeToClick([this]() { _canvas.frameAll(); }).relinquish();

		_canvas.onSelection([this](const std::string &p_uuid) {
			_document.selectNode(p_uuid);
			_rebuildProperties();
		});
		_canvas.onNodeMoved([this](const std::string &p_uuid, const spk::Vector2 &p_position) {
			_document.moveNode(p_uuid, p_position);
			_state.markChanged();
		});
		_canvas.onLinkToggled([this](const std::string &p_first, const std::string &p_second) {
			_document.toggleLink(p_first, p_second);
			_changed(true, false);
		});
		_canvas.onNodeAdded([this](const spk::Vector2 &p_position) {
			(void)_document.addNode(p_position);
			_changed(true, true);
		});
		_canvas.onNodeDeleted([this](const std::string &p_uuid) {
			if (!_document.deleteNode(p_uuid))
				_services.report("The root node cannot be deleted");
			else
				_changed(true, true);
		});

		_mainLayout.setElementPadding({6, 6});
		_mainLayout.addWidget(&_leftPane, spk::Layout::SizePolicy::Fixed)->setSize({220, 0});
		_mainLayout.addWidget(&_canvas);
		_mainLayout.addWidget(&_properties, spk::Layout::SizePolicy::Fixed)->setSize({380, 0});
		_leftLayout.setElementPadding({4, 4});
		reload();
	}

	void FeatBoardEditorPage::_onGeometryChange()
	{
		_background.setGeometry(geometry().atOrigin());
		_mainLayout.setGeometry(geometry().atOrigin().shrink({6, 6}));
		_leftLayout.setGeometry(_leftPane.geometry().atOrigin());
	}

	void FeatBoardEditorPage::_rebuildBoardList()
	{
		_leftLayout.clear();
		_boardButtons.clear();
		_leftLayout.addWidget(&_newButton, spk::Layout::SizePolicy::Minimum);
		_leftLayout.addWidget(&_saveButton, spk::Layout::SizePolicy::Minimum);
		_leftLayout.addWidget(&_frameButton, spk::Layout::SizePolicy::Minimum);
		std::vector<std::filesystem::path> files;
		const std::filesystem::path directory = _services.dataDirectory() / "featboards";
		for (const auto &entry : std::filesystem::directory_iterator(directory))
			if (entry.is_regular_file() && entry.path().extension() == ".json") files.push_back(entry.path());
		std::ranges::sort(files);
		for (const auto &file : files)
		{
			auto button = std::make_unique<spk::PushButton>(
				name() + "/Board/" + file.stem().string(), file.stem().string(), &_leftPane);
			button->subscribeToClick([this, file]() { _selectBoard(file); }).relinquish();
			button->activate();
			_leftLayout.addWidget(button.get(), spk::Layout::SizePolicy::Minimum);
			_boardButtons.push_back(std::move(button));
		}
		_onGeometryChange();
	}

	void FeatBoardEditorPage::_rebuildGraph(bool p_frame)
	{
		std::vector<GraphCanvas::Node> nodes;
		std::vector<GraphCanvas::Edge> edges;
		std::set<std::pair<std::string, std::string>> seen;
		const std::string root = _document.json().value("rootNode", "");
		for (const auto &node : _document.json().value("nodes", nlohmann::json::array()))
		{
			const std::string id = node.value("uuid", "");
			const auto position = node.value("position", nlohmann::json::array({0.0f, 0.0f}));
			nodes.push_back({
				.id = id,
				.label = node.value("displayName", "Node"),
				.position = {position.at(0).get<float>(), position.at(1).get<float>()},
				.color = nodeColor(node.value("kind", "statsBonus")),
				.root = id == root});
			for (const auto &linkedValue : node.value("neighbours", nlohmann::json::array()))
			{
				if (!linkedValue.is_string()) continue;
				const std::string linked = linkedValue.get<std::string>();
				const auto key = std::minmax(id, linked);
				if (seen.emplace(key.first, key.second).second) edges.push_back({key.first, key.second});
			}
		}
		_canvas.setGraph(std::move(nodes), std::move(edges));
		if (p_frame) _canvas.frameAll();
	}

	void FeatBoardEditorPage::_selectBoard(const std::filesystem::path &p_file)
	{
		if (_document.dirty())
		{
			_services.report("Save or reload the current feat board before switching");
			return;
		}
		_document.load(p_file);
		_state.markSaved();
		_rebuildGraph(true);
		_rebuildProperties();
		_services.report("Loaded " + _document.id());
	}

	void FeatBoardEditorPage::_newBoard()
	{
		if (_document.dirty())
		{
			_services.report("Save or reload the current feat board before creating another");
			return;
		}
		const std::filesystem::path directory = _services.dataDirectory() / "featboards";
		std::string id = "new-featboard";
		for (int suffix = 2; std::filesystem::exists(directory / (id + ".json")); ++suffix)
			id = "new-featboard-" + std::to_string(suffix);
		_document.create(directory, id);
		_state.markChanged();
		_rebuildGraph(true);
		_rebuildProperties();
	}

	void FeatBoardEditorPage::_changed(bool p_graph, bool p_properties)
	{
		_document.markChanged();
		_state.markChanged();
		if (p_graph) _rebuildGraph();
		if (p_properties) _rebuildProperties();
	}

	std::vector<std::string> FeatBoardEditorPage::_formIds() const
	{
		std::vector<std::string> result;
		for (const std::string &speciesId : _document.linkedSpecies(_services.registries()))
		{
			for (const auto &[formId, unused] : _services.registries().creatures().get(speciesId).forms)
			{
				(void)unused;
				if (!std::ranges::contains(result, formId)) result.push_back(formId);
			}
		}
		return result;
	}

	void FeatBoardEditorPage::_addJsonFields(nlohmann::json &p_object, const std::string &p_prefix)
	{
		std::vector<std::string> keys;
		for (const auto &[key, unused] : p_object.items())
		{
			(void)unused;
			if (key != "uuid" && key != "type") keys.push_back(key);
		}
		for (const std::string &key : keys)
		{
			nlohmann::json &value = p_object[key];
			const std::string label = p_prefix + " " + key;
			if (key == "ability" || key == "status" || key == "form")
			{
				auto picker = std::make_unique<IdPicker>(name() + "/" + p_prefix + "/" + key, &_properties);
				if (key == "ability") picker->setIds(_services.registries().abilities().ids());
				else if (key == "status") picker->setIds(_services.registries().statuses().ids());
				else picker->setIds(_formIds());
				picker->select(value.get<std::string>());
				picker->subscribeToSelection([this, &value](const std::string &p_id) { value = p_id; _changed(false, false); });
				_properties.addCustom(label, std::move(picker));
			}
			else if (value.is_boolean())
			{
				_properties.addBool(label, value.get<bool>(), [this, &value](bool p_new) { value = p_new; _changed(false, false); });
			}
			else if (value.is_number_integer())
			{
				_properties.addInt(label, value.get<int>(), 0, 100000, [this, &value](int p_new) { value = p_new; _changed(false, false); });
			}
			else if (value.is_number_float())
			{
				_properties.addFloat(label, value.get<float>(), -10000.0f, 10000.0f, [this, &value](float p_new) { value = p_new; _changed(false, false); });
			}
			else if (value.is_array() && std::ranges::all_of(value, [](const auto &p_entry) { return p_entry.is_string(); }))
			{
				_properties.addString(label, join(value), [this, &value](const std::string &p_text) { value = split(p_text); _changed(false, false); });
			}
			else if (value.is_string())
			{
				std::vector<std::string> options;
				if (key == "scope") options = {"ability", "turn", "fight", "game"};
				else if (key == "damageKind" || key == "kind") options = {"any", "physical", "magical"};
				else if (key == "resource") options = {"ap", "mp"};
				else if (key == "target") options = {"any", "self", "ally", "enemy"};
				else if (key == "condition") options = {"within", "atLeast", "between"};
				else if (key == "targetRangeCondition") options = {"either", "within", "atLeast"};
				else if (key == "orientation") options = {"towardCaster", "awayFromCaster"};
				else if (key == "attribute") options = {"health", "ap", "mp", "attack", "armor", "armorPenetration", "magic", "resistance", "resistancePenetration", "bonusRange", "stamina", "staminaRate", "bonusHealing", "lifeSteal", "omnivampirism", "timeEffectResistance"};
				if (!options.empty())
					_properties.addEnum(label, std::move(options), value.get<std::string>(), [this, &value](const std::string &p_new) { value = p_new; _changed(false, false); });
				else
					_properties.addString(label, value.get<std::string>(), [this, &value](const std::string &p_new) { value = p_new; _changed(false, false); });
			}
		}
	}

	void FeatBoardEditorPage::_rebuildProperties()
	{
		_properties.clear();
		if (_document.id().empty()) return;
		spk::TextEdit &id = _properties.addString("Board Id", _document.id(), [this](const std::string &p_value) {
			if (validId(p_value)) { _document.rename(p_value); _changed(false, false); }
		});
		id.setValidationCallback([id = &id](const spk::Font::Text &) {
			return validId(id->textAsUTF8()) ? spk::TextEdit::ValidationState::Valid : spk::TextEdit::ValidationState::Invalid;
		});
		const std::vector<std::string> species = _document.linkedSpecies(_services.registries());
		_properties.addString("Species", species.empty() ? "(none)" : join(nlohmann::json(species)), [](const std::string &) {}).disableEdit();

		nlohmann::json *node = _document.selectedNodeJson();
		if (node == nullptr) return;
		_properties.addString("Node UUID", node->value("uuid", ""), [](const std::string &) {}).disableEdit();
		_properties.addBool("Root", node->value("uuid", "") == _document.json().value("rootNode", ""), [this, node](bool p_root) {
			if (p_root) { _document.editJson()["rootNode"] = node->value("uuid", ""); _changed(true, false); }
		});
		_properties.addString("Display Name", node->value("displayName", ""), [this, node](const std::string &p_value) {
			(*node)["displayName"] = p_value; _changed(true, false);
		});
		_properties.addEnum("Kind", {"statsBonus", "ability", "passive", "form"}, node->value("kind", "statsBonus"), [this, node](const std::string &p_value) {
			(*node)["kind"] = p_value; _changed(true, false);
		});
		_properties.addInt("Repeat Limit", node->value("repeatLimit", 1), 0, 1000, [this, node](int p_value) {
			(*node)["repeatLimit"] = p_value; _changed(false, false);
		});

		auto addRequirement = std::make_unique<spk::PushButton>(name() + "/AddRequirement", "Add Requirement", &_properties);
		addRequirement->subscribeToClick([this]() { _document.addRequirement(); _changed(false, true); }).relinquish();
		_properties.addCustom("Requirements", std::move(addRequirement));
		for (std::size_t index = 0; index < (*node)["requirements"].size(); ++index)
		{
			nlohmann::json &requirement = (*node)["requirements"][index];
			_properties.addString("Req UUID", requirement.value("uuid", ""), [](const std::string &) {}).disableEdit();
			_properties.addEnum(
				"Req Type", FeatBoardDocument::requirementTypes(), requirement.value("type", "dealDamage"),
				[this, index](const std::string &p_type) { _document.setRequirementType(index, p_type); _changed(false, true); });
			_addJsonFields(requirement, "Req");
			auto remove = std::make_unique<spk::PushButton>(name() + "/RemoveRequirement/" + std::to_string(index), "Remove", &_properties);
			remove->subscribeToClick([this, index]() { _document.removeRequirement(index); _changed(false, true); }).relinquish();
			_properties.addCustom("Requirement", std::move(remove));
		}

		auto addReward = std::make_unique<spk::PushButton>(name() + "/AddReward", "Add Reward", &_properties);
		addReward->subscribeToClick([this]() { _document.addReward(); _changed(false, true); }).relinquish();
		_properties.addCustom("Rewards", std::move(addReward));
		for (std::size_t index = 0; index < (*node)["rewards"].size(); ++index)
		{
			nlohmann::json &reward = (*node)["rewards"][index];
			_properties.addEnum(
				"Reward Type", FeatBoardDocument::rewardTypes(), reward.value("type", "bonusStats"),
				[this, index](const std::string &p_type) { _document.setRewardType(index, p_type); _changed(false, true); });
			_addJsonFields(reward, "Reward");
			auto remove = std::make_unique<spk::PushButton>(name() + "/RemoveReward/" + std::to_string(index), "Remove", &_properties);
			remove->subscribeToClick([this, index]() { _document.removeReward(index); _changed(false, true); }).relinquish();
			_properties.addCustom("Reward", std::move(remove));
		}
	}

	std::string FeatBoardEditorPage::title() const { return "Feat Board Editor"; }
	bool FeatBoardEditorPage::hasUnsavedChanges() const { return _document.dirty(); }

	void FeatBoardEditorPage::save()
	{
		nlohmann::json &json = _document.editJson();
		const std::size_t symmetryFixes = FeatBoardDocument::fixNeighbourSymmetry(json);
		const bool reordered = FeatBoardDocument::reorderFormNodes(json, _document.id(), _services.registries());
		const auto issues = FeatBoardDocument::validate(json, _document.id(), _services.registries());
		if (!issues.empty())
		{
			_services.report("Cannot save feat board: " + issues.front().message);
			return;
		}
		_document.save(_services.writer());
		const std::filesystem::path file = _document.file();
		std::string warning;
		try { _services.load(); }
		catch (const std::exception &exception) { warning = exception.what(); }
		_document.load(file);
		_state.markSaved();
		_rebuildBoardList();
		_rebuildGraph(true);
		_rebuildProperties();
		if (!warning.empty()) _services.report("Saved, but registry reload is waiting on linked content: " + warning);
		else _services.report("Saved feat board" + std::string(symmetryFixes || reordered ? " with graph auto-fixes" : ""));
	}

	void FeatBoardEditorPage::reload()
	{
		std::filesystem::path file = _document.file();
		if (file.empty() || !std::filesystem::exists(file)) file = _services.dataDirectory() / "featboards" / "sprout-board.json";
		_document.load(file);
		_state.markSaved();
		_rebuildBoardList();
		_rebuildGraph(true);
		_rebuildProperties();
		_services.report("Feat Board editor loaded " + _document.id());
	}
}
