#include "tools/pages/encounter_editor_page.hpp"

#include "encounters/encounter_table.hpp"
#include "structures/game_engine/spk_texture_mesh_renderer_3d.hpp"
#include "structures/graphics/geometry/spk_primitive_object.hpp"
#include "tools/widgets/id_picker.hpp"

#include <algorithm>
#include <array>
#include <cctype>

namespace
{
	[[nodiscard]] bool validId(const std::string &p_id)
	{
		if (p_id.empty() || p_id.front() == '-' || p_id.back() == '-')
		{
			return false;
		}
		return std::ranges::all_of(p_id, [](unsigned char p_character) {
			return std::islower(p_character) || std::isdigit(p_character) || p_character == '-';
		});
	}
}

namespace pg::tools
{
	EncounterEditorPage::EncounterEditorPage(
		const std::string &p_name, spk::Widget *p_parent, ToolServices &p_services) :
		IEditorPage(p_name, p_parent),
		_services(p_services),
		_background(p_name + "/Background", this),
		_leftPane(p_name + "/Left", this),
		_newButton(p_name + "/New", "New Table", &_leftPane),
		_saveButton(p_name + "/Save", "Save", &_leftPane),
		_tierButton(p_name + "/Tier", "Tier", &_leftPane),
		_addTeamButton(p_name + "/AddTeam", "Add Team", &_leftPane),
		_deleteTeamButton(p_name + "/DeleteTeam", "Delete Team", &_leftPane),
		_preview(p_name + "/Preview", this),
		_properties(p_name + "/Properties", this),
		_atlas(std::make_shared<spk::SpriteSheet>())
	{
		_atlas->loadFromFile(std::filesystem::path(PG_RESOURCE_DIR) / "textures" / "voxels.png", {8u, 8u});
		for (spk::Widget *widget : std::array<spk::Widget *, 10>{
				 &_background, &_leftPane, &_newButton, &_saveButton, &_tierButton, &_addTeamButton, &_deleteTeamButton, &_preview, &_properties, this})
		{
			if (widget != this)
			{
				widget->activate();
			}
		}
		_newButton.subscribeToClick([this]() {
					  _newTable();
				  })
			.relinquish();
		_saveButton.subscribeToClick([this]() {
					   save();
				   })
			.relinquish();
		_tierButton.subscribeToClick([this]() {
					   _cycleTier();
				   })
			.relinquish();
		_addTeamButton.subscribeToClick([this]() {
						  _addTeam();
					  })
			.relinquish();
		_deleteTeamButton.subscribeToClick([this]() {
							 _deleteTeam();
						 })
			.relinquish();
		_mainLayout.setElementPadding({6, 6});
		_mainLayout.addWidget(&_leftPane, spk::Layout::SizePolicy::Fixed)->setSize({250, 0});
		_mainLayout.addWidget(&_preview);
		_mainLayout.addWidget(&_properties, spk::Layout::SizePolicy::Fixed)->setSize({430, 0});
		_leftLayout.setElementPadding({4, 4});
		reload();
	}

	void EncounterEditorPage::_onGeometryChange()
	{
		_background.setGeometry(geometry().atOrigin());
		_mainLayout.setGeometry(geometry().atOrigin().shrink({6, 6}));
		_leftLayout.setGeometry(_leftPane.geometry().atOrigin());
	}

	std::string EncounterEditorPage::_tierName() const
	{
		return std::string(encounterTierName(static_cast<EncounterTierId>(_tier)));
	}

	nlohmann::json *EncounterEditorPage::_selectedTeam()
	{
		if (_document.id().empty())
		{
			return nullptr;
		}
		auto &teams = _document.data()["tiers"][_tierName()]["weightedTeams"];
		return _team < teams.size() ? &teams[_team] : nullptr;
	}

	void EncounterEditorPage::_rebuildList()
	{
		_leftLayout.clear();
		_listButtons.clear();
		_leftLayout.addWidget(&_newButton, spk::Layout::SizePolicy::Minimum);
		_leftLayout.addWidget(&_saveButton, spk::Layout::SizePolicy::Minimum);
		_tierButton.setText("Tier: " + _tierName());
		_leftLayout.addWidget(&_tierButton, spk::Layout::SizePolicy::Minimum);
		_leftLayout.addWidget(&_addTeamButton, spk::Layout::SizePolicy::Minimum);
		_leftLayout.addWidget(&_deleteTeamButton, spk::Layout::SizePolicy::Minimum);
		const auto add = [this](std::string p_label, std::function<void()> p_callback) {
			auto button = std::make_unique<spk::PushButton>(name() + "/List/" + std::to_string(_listButtons.size()), p_label, &_leftPane);
			button->subscribeToClick(std::move(p_callback)).relinquish();
			button->activate();
			_leftLayout.addWidget(button.get(), spk::Layout::SizePolicy::Minimum);
			_listButtons.push_back(std::move(button));
		};
		const auto directory = _services.dataDirectory() / "encounter-tables";
		std::vector<std::filesystem::path> files;
		for (const auto &entry : std::filesystem::directory_iterator(directory))
		{
			if (entry.path().extension() == ".json")
			{
				files.push_back(entry.path());
			}
		}
		std::ranges::sort(files);
		for (const auto &file : files)
		{
			add("Table: " + file.stem().string(), [this, file]() {
				_selectTable(file);
			});
		}
		if (!_document.id().empty())
		{
			const auto &teams = _document.json().at("tiers").at(_tierName()).at("weightedTeams");
			for (std::size_t index = 0; index < teams.size(); ++index)
			{
				add("Team: " + teams[index].value("displayName", "Team"), [this, index]() {
					_team = index;
					_rebuildProperties();
					_rebuildPreview();
				});
			}
		}
		_onGeometryChange();
	}

	void EncounterEditorPage::_selectTable(const std::filesystem::path &p_file)
	{
		if (_document.dirty())
		{
			_services.report("Save or reload before switching tables");
			return;
		}
		_document.load(p_file, ContentDocument::Domain::Encounter);
		_tier = _team = 0;
		_state.markSaved();
		_rebuildList();
		_rebuildProperties();
		_rebuildPreview();
	}

	void EncounterEditorPage::_newTable()
	{
		if (_document.dirty())
		{
			_services.report("Save or reload before creating a table");
			return;
		}
		std::string id = "new-encounter";
		const auto directory = _services.dataDirectory() / "encounter-tables";
		for (int suffix = 2; std::filesystem::exists(directory / (id + ".json")); ++suffix)
		{
			id = "new-encounter-" + std::to_string(suffix);
		}
		_document.create(_services.dataDirectory(), id, ContentDocument::Domain::Encounter, _services.registries());
		_tier = _team = 0;
		_state.markChanged();
		_addTeam();
	}

	void EncounterEditorPage::_cycleTier()
	{
		_tier = (_tier + 1) % static_cast<std::size_t>(EncounterTierId::Count);
		_team = 0;
		_rebuildList();
		_rebuildProperties();
		_rebuildPreview();
	}

	void EncounterEditorPage::_addTeam()
	{
		if (_document.id().empty())
		{
			return;
		}
		const auto species = _services.registries().creatures().ids();
		const auto ai = _services.registries().ai().ids();
		nlohmann::json slots = nlohmann::json::array({nullptr, nullptr, nullptr, nullptr, nullptr, nullptr});
		if (!species.empty() && !ai.empty())
		{
			slots[0] = {{"species", species.front()}, {"ai", ai.front()}, {"completedNodes", nlohmann::json::array()}};
		}
		auto &teams = _document.editJson()["tiers"][_tierName()]["weightedTeams"];
		teams.push_back({{"displayName", "New Team"}, {"weight", 1}, {"team", slots}});
		_team = teams.size() - 1;
		_changed(true);
		_rebuildList();
	}

	void EncounterEditorPage::_deleteTeam()
	{
		if (nlohmann::json *team = _selectedTeam(); team != nullptr)
		{
			(void)team;
			auto &teams = _document.editJson()["tiers"][_tierName()]["weightedTeams"];
			teams.erase(teams.begin() + static_cast<std::ptrdiff_t>(_team));
			_team = teams.empty() ? 0 : std::min(_team, teams.size() - 1);
			_changed(true);
			_rebuildList();
		}
	}

	void EncounterEditorPage::_changed(bool p_properties)
	{
		_state.markChanged();
		if (p_properties)
		{
			_rebuildProperties();
		}
		_rebuildPreview();
	}

	void EncounterEditorPage::_rebuildProperties()
	{
		_properties.clear();
		if (_document.id().empty())
		{
			return;
		}
		_properties.addString("Table Id", _document.id(), [this](const std::string &p_id) {
			if (validId(p_id))
			{
				_document.rename(p_id);
				_changed();
			}
		});
		nlohmann::json *team = _selectedTeam();
		if (team == nullptr)
		{
			return;
		}
		_properties.addString("Team Name", team->value("displayName", ""), [this, team](const std::string &p_value) {
			(*team)["displayName"] = p_value;
			_changed();
		});
		_properties.addInt("Weight", team->value("weight", 1), 1, 100000, [this, team](int p_value) {
			(*team)["weight"] = p_value;
			_changed();
		});
		const nlohmann::json boardSize = team->value("boardSize", nlohmann::json::array({11, 11}));
		_properties.addInt("Board X", boardSize[0].get<int>(), 1, 64, [this, team](int p_value) {
			if (!team->contains("boardSize"))
			{
				(*team)["boardSize"] = {11, 11};
			}
			(*team)["boardSize"][0] = p_value;
			_changed();
		});
		_properties.addInt("Board Y", boardSize[1].get<int>(), 1, 64, [this, team](int p_value) {
			if (!team->contains("boardSize"))
			{
				(*team)["boardSize"] = {11, 11};
			}
			(*team)["boardSize"][1] = p_value;
			_changed();
		});
		for (std::size_t slotIndex = 0; slotIndex < 6; ++slotIndex)
		{
			nlohmann::json &slot = (*team)["team"][slotIndex];
			const bool active = !slot.is_null();
			_properties.addBool("Slot " + std::to_string(slotIndex + 1), active, [this, team, slotIndex](bool p_active) {
				auto &target = (*team)["team"][slotIndex];
				if (!p_active)
				{
					target = nullptr;
				}
				else if (target.is_null())
				{
					const auto species = _services.registries().creatures().ids();
					const auto ai = _services.registries().ai().ids();
					if (!species.empty() && !ai.empty())
					{
						target = {{"species", species.front()}, {"ai", ai.front()}, {"completedNodes", nlohmann::json::array()}};
					}
				}
				_changed(true);
			});
			if (!active)
			{
				continue;
			}
			auto speciesPicker = std::make_unique<IdPicker>(name() + "/Species/" + std::to_string(slotIndex), &_properties);
			speciesPicker->setIds(_services.registries().creatures().ids());
			speciesPicker->select(slot.value("species", ""));
			speciesPicker->subscribeToSelection([this, team, slotIndex](const std::string &p_id) {
				(*team)["team"][slotIndex]["species"] = p_id;
				(*team)["team"][slotIndex]["completedNodes"] = nlohmann::json::array();
				_changed(true);
			});
			_properties.addCustom("Species", std::move(speciesPicker));
			auto aiPicker = std::make_unique<IdPicker>(name() + "/AI/" + std::to_string(slotIndex), &_properties);
			aiPicker->setIds(_services.registries().ai().ids());
			aiPicker->select(slot.value("ai", ""));
			aiPicker->subscribeToSelection([this, team, slotIndex](const std::string &p_id) {
				(*team)["team"][slotIndex]["ai"] = p_id;
				_changed();
			});
			_properties.addCustom("AI", std::move(aiPicker));
			const CreatureSpecies *species = _services.registries().creatures().tryGet(slot.value("species", ""));
			if (species != nullptr && species->featBoard != nullptr)
			{
				for (const FeatNode &node : species->featBoard->nodes)
				{
					const std::string uuid = node.uuid.toString();
					const bool checked = std::ranges::any_of(slot["completedNodes"], [&uuid](const auto &p_value) {
						return p_value == uuid;
					});
					_properties.addBool("Feat: " + node.displayName, checked, [this, team, slotIndex, uuid](bool p_checked) {
						auto &values = (*team)["team"][slotIndex]["completedNodes"];
						if (p_checked)
						{
							if (!std::ranges::any_of(values, [&uuid](const auto &p_value) {
									return p_value == uuid;
								}))
							{
								values.push_back(uuid);
							}
						}
						else
						{
							for (auto it = values.begin(); it != values.end();)
							{
								if (*it == uuid)
								{
									it = values.erase(it);
								}
								else
								{
									++it;
								}
							}
						}
						_changed(true);
					});
				}
			}
			_properties.addString(
						   "Derived", ContentDocument::completedNodesSummary(slot.value("species", ""), slot["completedNodes"], _services.registries()), [](const std::string &) {
						   })
				.disableEdit();
		}
	}

	void EncounterEditorPage::_rebuildPreview()
	{
		_preview.clearScene();
		nlohmann::json *team = _selectedTeam();
		if (team == nullptr)
		{
			return;
		}
		auto mesh = std::make_shared<spk::TextureMesh3D>(spk::PrimitiveObject::CreateCube(0.8f));
		for (std::size_t index = 0; index < 6; ++index)
		{
			if ((*team)["team"][index].is_null())
			{
				continue;
			}
			spk::Entity3D &entity = _preview.createEntity();
			entity.transform().setPosition({static_cast<float>(index % 3) * 1.4f, 0.5f, static_cast<float>(index / 3) * 1.4f});
			auto &renderer = entity.addComponent<spk::TextureMeshRenderer3D>();
			renderer.setMesh(mesh);
			renderer.setTexture(_atlas.get());
			renderer.setTint({0.35f + 0.1f * static_cast<float>(index), 0.55f, 0.9f, 1.0f});
		}
		_preview.frame({1.4f, 0.5f, 0.7f}, 6.0f);
	}

	std::string EncounterEditorPage::title() const
	{
		return "Encounter / Team Editor";
	}
	bool EncounterEditorPage::hasUnsavedChanges() const
	{
		return _document.dirty();
	}

	void EncounterEditorPage::save()
	{
		const auto errors = _document.validate(_services.registries());
		if (!errors.empty())
		{
			_services.report("Cannot save encounter: " + errors.front());
			return;
		}
		_document.save(_services.writer());
		const auto file = _document.file();
		std::string warning;
		try
		{
			_services.load();
		} catch (const std::exception &e)
		{
			warning = e.what();
		}
		_document.load(file, ContentDocument::Domain::Encounter);
		_state.markSaved();
		_rebuildList();
		_rebuildProperties();
		_rebuildPreview();
		_services.report(warning.empty() ? "Saved encounter table" : "Saved; registry reload pending: " + warning);
	}

	void EncounterEditorPage::reload()
	{
		auto file = _document.file();
		if (file.empty() || !std::filesystem::exists(file))
		{
			file = _services.dataDirectory() / "encounter-tables" / "forest-basic.json";
		}
		_document.load(file, ContentDocument::Domain::Encounter);
		_tier = _team = 0;
		_state.markSaved();
		_rebuildList();
		_rebuildProperties();
		_rebuildPreview();
	}
}
