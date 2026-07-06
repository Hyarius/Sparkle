#include "tools/pages/species_editor_page.hpp"

#include "animation/model_definition.hpp"
#include "structures/game_engine/spk_texture_mesh_renderer_3d.hpp"
#include "tools/core/feat_board_document.hpp"
#include "tools/widgets/id_picker.hpp"

#include <algorithm>
#include <cctype>
#include <sstream>

namespace
{
	[[nodiscard]] bool validId(const std::string &p_id)
	{
		return !p_id.empty() && std::ranges::all_of(p_id, [](unsigned char c) {
			return std::islower(c) || std::isdigit(c) || c == '-';
		});
	}
	[[nodiscard]] std::string join(const nlohmann::json &p_values)
	{
		std::string r;
		for (const auto &v : p_values)
		{
			if (!r.empty())
			{
				r += ", ";
			}
			r += v.get<std::string>();
		}
		return r;
	}
	[[nodiscard]] std::vector<std::string> split(const std::string &p_text)
	{
		std::vector<std::string> r;
		std::stringstream s(p_text);
		std::string v;
		while (std::getline(s, v, ','))
		{
			auto f = v.find_first_not_of(" \t");
			auto l = v.find_last_not_of(" \t");
			if (f != std::string::npos)
			{
				r.push_back(v.substr(f, l - f + 1));
			}
		}
		return r;
	}
}

namespace pg::tools
{
	SpeciesEditorPage::SpeciesEditorPage(const std::string &p_name, spk::Widget *p_parent, ToolServices &p_services) :
		IEditorPage(p_name, p_parent),
		_services(p_services),
		_background(p_name + "/Background", this),
		_leftPane(p_name + "/Left", this),
		_newButton(p_name + "/New", "New Species", &_leftPane),
		_saveButton(p_name + "/Save", "Save", &_leftPane),
		_preview(p_name + "/Preview", this),
		_properties(p_name + "/Properties", this),
		_atlas(std::make_shared<spk::SpriteSheet>())
	{
		_atlas->loadFromFile(std::filesystem::path(PG_RESOURCE_DIR) / "textures" / "voxels.png", {8u, 8u});
		_background.activate();
		_leftPane.activate();
		_newButton.activate();
		_saveButton.activate();
		_preview.activate();
		_properties.activate();
		_newButton.subscribeToClick([this]() {
					  _newSpecies();
				  })
			.relinquish();
		_saveButton.subscribeToClick([this]() {
					   save();
				   })
			.relinquish();
		_mainLayout.setElementPadding({6, 6});
		_mainLayout.addWidget(&_leftPane, spk::Layout::SizePolicy::Fixed)->setSize({220, 0});
		_mainLayout.addWidget(&_preview);
		_mainLayout.addWidget(&_properties, spk::Layout::SizePolicy::Fixed)->setSize({440, 0});
		_leftLayout.setElementPadding({4, 4});
		reload();
	}

	void SpeciesEditorPage::_onGeometryChange()
	{
		_background.setGeometry(geometry().atOrigin());
		_mainLayout.setGeometry(geometry().atOrigin().shrink({6, 6}));
		_leftLayout.setGeometry(_leftPane.geometry().atOrigin());
	}

	void SpeciesEditorPage::_rebuildList()
	{
		_leftLayout.clear();
		_speciesButtons.clear();
		_leftLayout.addWidget(&_newButton, spk::Layout::SizePolicy::Minimum);
		_leftLayout.addWidget(&_saveButton, spk::Layout::SizePolicy::Minimum);
		std::vector<std::filesystem::path> files;
		for (const auto &e : std::filesystem::directory_iterator(_services.dataDirectory() / "creatures"))
		{
			if (e.path().extension() == ".json")
			{
				files.push_back(e.path());
			}
		}
		std::ranges::sort(files);
		for (const auto &file : files)
		{
			auto b = std::make_unique<spk::PushButton>(name() + "/Species/" + file.stem().string(), file.stem().string(), &_leftPane);
			b->subscribeToClick([this, file]() {
				 _selectSpecies(file);
			 }).relinquish();
			b->activate();
			_leftLayout.addWidget(b.get(), spk::Layout::SizePolicy::Minimum);
			_speciesButtons.push_back(std::move(b));
		}
		_onGeometryChange();
	}

	void SpeciesEditorPage::_selectSpecies(const std::filesystem::path &p_file)
	{
		if (_document.dirty())
		{
			_services.report("Save or reload before switching species");
			return;
		}
		_document.load(p_file, ContentDocument::Domain::Species);
		_state.markSaved();
		_rebuildProperties();
		_rebuildPreview();
	}
	void SpeciesEditorPage::_newSpecies()
	{
		if (_document.dirty())
		{
			_services.report("Save or reload before creating species");
			return;
		}
		std::string id = "new-species";
		auto d = _services.dataDirectory() / "creatures";
		for (int n = 2; std::filesystem::exists(d / (id + ".json")); ++n)
		{
			id = "new-species-" + std::to_string(n);
		}
		_document.create(_services.dataDirectory(), id, ContentDocument::Domain::Species, _services.registries());
		_state.markChanged();
		_rebuildProperties();
		_rebuildPreview();
	}
	void SpeciesEditorPage::_changed(bool p_properties, bool p_preview)
	{
		(void)_document.editJson();
		_state.markChanged();
		if (p_properties)
		{
			_rebuildProperties();
		}
		if (p_preview)
		{
			_rebuildPreview();
		}
	}

	void SpeciesEditorPage::_addConditionFields(nlohmann::json &p_condition, const std::string &p_prefix)
	{
		std::vector<std::string> keys;
		for (const auto &[k, u] : p_condition.items())
		{
			(void)u;
			if (k != "type")
			{
				keys.push_back(k);
			}
		}
		for (const auto &key : keys)
		{
			auto &v = p_condition[key];
			std::string label = p_prefix + " " + key;
			if (v.is_boolean())
			{
				_properties.addBool(label, v.get<bool>(), [this, &v](bool x) {
					v = x;
					_changed();
				});
			}
			else if (v.is_number_integer())
			{
				_properties.addInt(label, v.get<int>(), 0, 100000, [this, &v](int x) {
					v = x;
					_changed();
				});
			}
			else if (v.is_number_float())
			{
				_properties.addFloat(label, v.get<float>(), 0, 10000, [this, &v](float x) {
					v = x;
					_changed();
				});
			}
			else if (v.is_array() && std::ranges::all_of(v, [](const auto &e) {
						 return e.is_string();
					 }))
			{
				_properties.addString(label, join(v), [this, &v](const std::string &x) {
					v = split(x);
					_changed();
				});
			}
			else if (v.is_string())
			{
				_properties.addString(label, v.get<std::string>(), [this, &v](const std::string &x) {
					v = x;
					_changed();
				});
			}
		}
	}

	void SpeciesEditorPage::_rebuildProperties()
	{
		_properties.clear();
		if (_document.id().empty())
		{
			return;
		}
		auto &root = _document.data();
		_properties.addString("Id", _document.id(), [this](const std::string &p) {
			if (validId(p))
			{
				_document.rename(p);
				_changed();
			}
		});
		_properties.addString("Display Name", root.value("displayName", ""), [this, &root](const std::string &p) {
			root["displayName"] = p;
			_changed();
		});
		for (auto &[key, value] : root["attributes"].items())
		{
			if (value.is_number_integer())
			{
				_properties.addInt("Attribute " + key, value.get<int>(), 0, 100000, [this, &value](int p) {
					value = p;
					_changed();
				});
			}
			else
			{
				_properties.addFloat("Attribute " + key, value.get<float>(), 0, 10000, [this, &value](float p) {
					value = p;
					_changed();
				});
			}
		}
		for (const std::string &abilityId : _services.registries().abilities().ids())
		{
			const bool selected = std::ranges::any_of(root["defaultAbilities"], [&abilityId](const auto &v) {
				return v == abilityId;
			});
			_properties.addBool("Ability: " + abilityId, selected, [this, &root, abilityId](bool enabled) {
				auto &values = root["defaultAbilities"];
				if (enabled)
				{
					if (!std::ranges::any_of(values, [&abilityId](const auto &v) {
							return v == abilityId;
						}))
					{
						values.push_back(abilityId);
					}
				}
				else
				{
					for (auto it = values.begin(); it != values.end();)
					{
						if (*it == abilityId)
						{
							it = values.erase(it);
						}
						else
						{
							++it;
						}
					}
				}
				_changed();
			});
		}
		auto board = std::make_unique<IdPicker>(name() + "/FeatBoard", &_properties);
		board->setIds(_services.registries().featBoards().ids());
		board->select(root.value("featBoard", ""));
		board->subscribeToSelection([this, &root](const std::string &p) {
			root["featBoard"] = p;
			_changed();
		});
		_properties.addCustom("Feat Board", std::move(board));
		std::vector<std::string> formIds;
		for (const auto &[id, u] : root["forms"].items())
		{
			(void)u;
			formIds.push_back(id);
		}
		_properties.addEnum("Default Form", formIds, root.value("defaultFormId", "base"), [this, &root](const std::string &p) {
			root["defaultFormId"] = p;
			_changed(false, true);
		});
		auto addForm = std::make_unique<spk::PushButton>(name() + "/AddForm", "Add Form", &_properties);
		addForm->subscribeToClick([this, &root]() {
				   std::string id = "form" + std::to_string(root["forms"].size() + 1);
				   root["forms"][id] = {{"displayName", "New Form"}, {"tier", static_cast<int>(root["forms"].size())}, {"model", "placeholder-cube"}, {"animationSet", "basic-creature"}, {"avatar", {0, 0}}};
				   _changed(true, true);
			   })
			.relinquish();
		_properties.addCustom("Forms", std::move(addForm));
		for (auto &[formId, form] : root["forms"].items())
		{
			_properties.addString("Form Id", formId, [](const std::string &) {
					   })
				.disableEdit();
			_properties.addString("Form Name", form.value("displayName", ""), [this, &form](const std::string &p) {
				form["displayName"] = p;
				_changed();
			});
			_properties.addInt("Form Tier", form.value("tier", 0), 0, 100, [this, &form](int p) {
				form["tier"] = p;
				_changed();
			});
			auto model = std::make_unique<IdPicker>(name() + "/Model/" + formId, &_properties);
			model->setIds(_services.registries().models().ids());
			model->select(form.value("model", ""));
			model->subscribeToSelection([this, &form](const std::string &p) {
				form["model"] = p;
				_changed(false, true);
			});
			_properties.addCustom("Model", std::move(model));
			_properties.addString("Animation Set", form.value("animationSet", ""), [this, &form](const std::string &p) {
				form["animationSet"] = p;
				_changed();
			});
			_properties.addInt("Avatar X", form["avatar"][0].get<int>(), 0, 255, [this, &form](int p) {
				form["avatar"][0] = p;
				_changed();
			});
			_properties.addInt("Avatar Y", form["avatar"][1].get<int>(), 0, 255, [this, &form](int p) {
				form["avatar"][1] = p;
				_changed();
			});
		}
		auto addCondition = std::make_unique<spk::PushButton>(name() + "/AddCondition", "Add Taming Condition", &_properties);
		addCondition->subscribeToClick([this, &root]() {
						root["tamingProfile"]["conditions"].push_back(FeatBoardDocument::requirementDefaults("dealDamage"));
						_changed(true, false);
					})
			.relinquish();
		_properties.addCustom("Taming", std::move(addCondition));
		auto types = FeatBoardDocument::requirementTypes();
		std::erase(types, "winBattleCount");
		for (std::size_t i = 0; i < root["tamingProfile"]["conditions"].size(); ++i)
		{
			auto &condition = root["tamingProfile"]["conditions"][i];
			_properties.addEnum("Condition Type", types, condition.value("type", "dealDamage"), [this, &root, i](const std::string &p) {
				root["tamingProfile"]["conditions"][i] = FeatBoardDocument::requirementDefaults(p);
				_changed(true, false);
			});
			_addConditionFields(condition, "Condition");
			auto remove = std::make_unique<spk::PushButton>(name() + "/RemoveCondition/" + std::to_string(i), "Remove", &_properties);
			remove->subscribeToClick([this, &root, i]() {
					  root["tamingProfile"]["conditions"].erase(root["tamingProfile"]["conditions"].begin() + static_cast<std::ptrdiff_t>(i));
					  _changed(true, false);
				  })
				.relinquish();
			_properties.addCustom("Condition", std::move(remove));
		}
	}

	void SpeciesEditorPage::_rebuildPreview()
	{
		_preview.clearScene();
		if (_document.id().empty())
		{
			return;
		}
		const auto &root = _document.json();
		const std::string formId = root.value("defaultFormId", "base");
		if (!root["forms"].contains(formId))
		{
			return;
		}
		const std::string modelId = root["forms"][formId].value("model", "");
		const ModelDefinition *model = _services.registries().models().tryGet(modelId);
		if (model == nullptr)
		{
			return;
		}
		for (const ModelPart &part : model->parts)
		{
			spk::Entity3D &e = _preview.createEntity();
			e.transform().setPosition({-part.origin.x, -part.origin.y, -part.origin.z});
			auto &r = e.addComponent<spk::TextureMeshRenderer3D>();
			r.setMesh(part.mesh);
			r.setTexture(_atlas.get());
			r.setTint({0.3f, 0.75f, 0.45f, 1});
		}
		_preview.frame({0, 0, 0}, 4);
	}

	std::string SpeciesEditorPage::title() const
	{
		return "Species Editor";
	}
	bool SpeciesEditorPage::hasUnsavedChanges() const
	{
		return _document.dirty();
	}
	void SpeciesEditorPage::save()
	{
		auto errors = _document.validate(_services.registries());
		if (!errors.empty())
		{
			_services.report("Cannot save species: " + errors.front());
			return;
		}
		_document.save(_services.writer());
		auto file = _document.file();
		std::string warning;
		try
		{
			_services.load();
		} catch (const std::exception &e)
		{
			warning = e.what();
		}
		_document.load(file, ContentDocument::Domain::Species);
		_state.markSaved();
		_rebuildList();
		_rebuildProperties();
		_rebuildPreview();
		_services.report(warning.empty() ? "Saved species" : "Saved; registry reload pending: " + warning);
	}
	void SpeciesEditorPage::reload()
	{
		auto file = _document.file();
		if (file.empty() || !std::filesystem::exists(file))
		{
			file = _services.dataDirectory() / "creatures" / "sprout.json";
		}
		_document.load(file, ContentDocument::Domain::Species);
		_state.markSaved();
		_rebuildList();
		_rebuildProperties();
		_rebuildPreview();
	}
}
